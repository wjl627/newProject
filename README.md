# First Person Shooter（UE5 多人联机射击 Demo）

基于 **Unreal Engine 5.7** 第一人称模板开发的多人 FPS 小项目，实现完整的对战循环，涵盖角色控制、武器系统、AI 行为树、网络同步等核心模块。

- **网络模型**：服务端权威（Server-Authoritative），监听服务器（Listen Server）
- **游戏逻辑**：全部 C++ 手写（AI、武器、手感、得分、网络同步）
- **对局流程**：主菜单 → 大厅 → 对战 → 结算排名 → 自动重开

> 练手项目，不发布到任何平台。目的是实践 UE5 的多人联机、战斗手感、AI 与完整对局流程，用于技术积累与简历展示。

---

## 功能特性

**战斗**
- 两把武器（步枪 / 手枪 ），即时命中（Hitscan）+ 弹道拖尾
- 全自动 / 半自动射速控制，换弹、切枪（带卸下/装备动画）、武器拾取
- 双后坐力：相机后坐力（俯仰/偏航冲击 + 平滑回位）+ 武器网格 kick
- 命中施加物理冲量，受击/死亡布娃娃

**AI**
- `AIPerception` 视觉感知 + `Behavior Tree`，巡逻 / 追击 / 攻击
- 近距离自动检测、目标丢失后的最后已知位置追击

**成长与得分**
- 击杀得分、死亡减半，队伍分 + 个人分
- 击杀奖励：弹药 + 概率触发的回血 / 伤害增益 / 分数增益

**联机与流程**
- 服务端权威：伤害、弹药、生成判定全部在服务端，客户端只做表现
- 玩家死亡自动重生，比赛倒计时，结算排名（前三名），自动重开
- 主菜单 IP 直连（支持 `127.0.0.1` 本机双实例联机），大厅玩家列表
- 支持移动端触控输入（可选）

---

## 技术架构

### 1. 网络同步模型

所有伤害、弹药、生成、计分在**服务端**执行，客户端只做输入上报与表现播放。核心选型：

| 场景 | 机制 | 说明 |
|------|------|------|
| 血量 / 当前武器 | `ReplicatedUsing` 属性复制 | 持续性状态，服务端改，客户端响应 `OnRep_` |
| 开火 / 换弹 / 切枪 | `Server` RPC | 客户端的一次性请求，服务端执行 |
| 开火特效 / 死亡 | `NetMulticast` RPC | 一次性表现事件，广播给所有客户端 |
| 分数 / 比赛时间 / 增益提示 | `Client` RPC | 只推给特定客户端 |
| 全自动连射瞄准 | `Server` RPC（**Unreliable**） | 高频更新，允许丢包 |

```
客户端输入 ──(Server RPC)──> 服务端权威逻辑 ──(Spawn / 扣血 / 计分)
                                 │
                ┌────────────────┼────────────────┐
           (Multicast RPC)  (属性复制+OnRep)   (Client RPC)
                │                │                │
          所有客户端特效    客户端状态更新     特定客户端 UI
```

权限判断（贯穿所有逻辑）：

```cpp
HasAuthority()        // 服务端逻辑
IsLocallyControlled() // 本地玩家表现
```

### 2. 武器系统

- **命中判定**：服务端 `LineTrace`（Hitscan），从枪口/相机发射射线，命中即 `ApplyDamage`；本地玩家用相机射线保证准星准确，NPC 从枪口开火
- **射速**：`RefireRate` 控制全自动（计时器连发）与半自动（冷却结束回调）两种模式
- **换弹**：弹匣 + 备用弹匣双计数，换弹时长取动画长度（无动画则回退到配置值）
- **切枪**：卸下动画 → 切换动画蓝图 → 装备动画 → 完成，期间用 `bIsSwitchingWeapon` 锁定输入
- **拾取**：`FDataTableRowHandle` 从数据表读取武器类型与外观，拾取后进入拥有列表，可重复刷新
- **第一/第三人称双网格**：第一人称网格仅本机可见，第三人称网格本机隐藏，正确呈现其他玩家视角

### 3. 战斗手感（后坐力）

两套独立的后坐力叠加：

| 类型 | 位置 | 实现 |
|------|------|------|
| 相机后坐力 | `AShooterCharacter` | 开火累计俯仰/偏航冲击（带随机散布），停止后按恢复速度平滑回位，期间不恢复 |
| 武器 kick | `AShooterWeapon` | 武器网格向后/向上位移，按冲击时长衰减回静止变换，`Tick` 每帧插值 |

### 4. AI 系统

放弃官方模板的 StateTree，改用 **`AIPerception` + `Behavior Tree`**：

- `AIPerceptionComponent` 配置 Sight Sense（运行时按攻击范围调整视野半径/角度/丢失超时），回调过滤带 `Player` 标签的角色
- `UBTService_UpdatePerception` 每帧更新黑板（目标 Actor、是否在攻击范围、是否有视线）
- `UBTTask_FindPatrolPoint`（随机巡逻）、`UBTTask_FireWeapon`（持续开火）、`UEnvQueryContext_Target`（提供目标上下文）
- 视线检测用 `ECC_Pawn` 通道打射线，避免武器/子弹子 Actor 挡射线；近距离自动检测半径解决"太近无法攻击"

### 5. 增益系统

击杀触发 `GrantKillRewards`（服务端）：

- 固定奖励：所有武器 +1 备用弹匣
- 概率触发：回血（30%）、伤害增益 ×2（30%，10s）、分数增益 ×2（20%，15s）
- 增益提示通过 `Client_ShowBuffMessage` RPC 推送给击杀者，`BuffMessageUI` 显示

### 6. 对局流程与 UI

```
MainMenuUI（IP 直连）──> LobbyUI（玩家列表 + 房主开始）──> 对战
      │                                                        │
      │                                      倒计时结束 / EndMatch
      ▼                                                        ▼
   ExitGame                                    MatchResultOverlay（排名）
                                                   │
                                              15s 后自动重开
```

- `EMatchState`（Lobby / InProgress / Ended）作为 GameState 复制属性驱动各客户端 UI
- 玩家死亡 → `RespawnTime` 计时 → 销毁角色 → PlayerController 检测 Pawn 销毁后重生

---

## 代码结构

```
Source/newProject/Variant_Shooter/
├── AI/
│   ├── ShooterAIController.h/.cpp     # AI 控制器（视觉感知、视线检测）
│   ├── ShooterNPC.h/.cpp              # NPC（伤害、死亡、复制、武器持有）
│   ├── ShooterNPCSpawner.h/.cpp       # NPC 生成点
│   ├── BTService_UpdatePerception.*   # 行为树服务：更新感知黑板
│   ├── BTTask_FindPatrolPoint.*       # 行为树任务：随机巡逻
│   ├── BTTask_FireWeapon.*            # 行为树任务：持续开火
│   └── EnvQueryContext_Target.*       # EQS 上下文：目标
├── Weapons/
│   ├── ShooterWeapon.h/.cpp           # 武器基类（hitscan/弹药/后坐力/复制）
│   ├── ShooterBulletTracer.h/.cpp     # 弹道拖尾（视觉）
│   ├── ShooterPickup.h/.cpp           # 武器拾取物（数据表驱动）
│   ├── ShooterProjectile.h/.cpp       # 抛射体类（模板遗留，当前未使用）
│   └── ShooterWeaponHolder.h          # 武器持有者接口
├── UI/
│   ├── MainMenuUI.*                   # 主菜单（IP 直连/主机）
│   ├── LobbyUI.*                      # 大厅（玩家列表 + 房主开始）
│   ├── MatchResultOverlay.*           # 结算排名
│   ├── BuffMessageUI.*                # 增益提示
│   ├── ShooterBulletCounterUI.*       # 血条/子弹数 HUD
│   └── ShooterUI.*                    # 记分板
├── ShooterCharacter.h/.cpp            # 玩家角色（HP/武器复制、开火/换弹/切枪 RPC、后坐力、增益、死亡/重生）
├── ShooterGameMode.h/.cpp             # 游戏模式（计分、敌人生成、对局流程）
├── ShooterGameState.h/.cpp            # 游戏状态（比赛状态/时间复制）
└── ShooterPlayerController.h/.cpp     # 玩家控制器（UI 创建、输入、重生、触控）
```

蓝图资产（`Content/Variant_Shooter/`）：`BP_ShooterCharacter`、`BP_ShooterGameMode`、`BP_ShooterNPC`、`BP_ShooterWeapon_Rifle/Pistol/GrenadeLauncher`、行为树、动画蓝图、UI Widget。

---

## 关键技术难点与解决

| 难点 | 根因 | 解决 |
|------|------|------|
| 客户端开不了枪 | 客户端无权限 `SpawnActor` 生成子弹 | `HasAuthority()` 分支：服务端直接开火，客户端发 `Server_StartFiring` RPC |
| 客户端看不到武器/子弹 | 武器与 `CurrentWeapon` 未复制 | 武器 `bReplicates=true`；`CurrentWeapon` 用 `ReplicatedUsing=OnRep_CurrentWeapon`，客户端收到后切换动画蓝图 |
| 客户端死亡不播动画/布娃娃 | 死亡逻辑只在服务端执行 | 死亡用 `Multicast_OnDeath` 广播；NPC `bIsDead` 加 `OnRep_` 开启布娃娃物理 |
| 客户端切换武器无效 | 武器列表不复制，客户端误判无第二把枪 | 客户端发 `Server_SwitchWeapon` RPC，切枪逻辑只在服务端跑 |
| 客户端记分板不显示 | `ShooterUI` 在 GameMode 创建，GameMode 仅服务端存在 | 记分板移到 `PlayerController` 创建，分数用 RPC 推送 |
| 全自动连射打不准 | 客户端只发一次开火请求，瞄准点不更新 | 全自动期间用 Unreliable `Server_UpdateAimTarget` 定时流式上传瞄准点 |
| AI 太近无法攻击 / 被自己子 Actor 挡视线 | 感知锥盲区 + 射线命中自身武器 | 近距离自动检测半径 + `ECC_Pawn` 通道做视线检测 |

---

## 构建与运行

1. 用 **UE 5.7** 打开 `newProject.uproject`（首次会编译 C++ 并加载蓝图）
2. 默认地图为 `Variant_Shooter/Lvl_Shooter`，默认 GameMode 为 `BP_ShooterGameMode`

**多人联机（监听服务器 / IP 直连）**：

- 主机：主菜单输入本机局域网 IP（或留空），点击连接即可作为主机
- 另一名玩家：输入主机 IP 加入
- 本机双实例调试：两个实例都输入 `127.0.0.1`

---



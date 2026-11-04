# Sn · 第三人称动作 RPG（UE 5.8 / C++ / GAS）

一个用 **Unreal Engine 5.8 + C++** 实现的第三人称动作 RPG。战斗、技能、背包、掉落、存档全部在 C++ 模块 `Sn` 里实现，蓝图只做资产配置与界面拼装。

| | |
|---|---|
| 引擎 | Unreal Engine 5.8 |
| 语言 | C++（178 个源文件，约 2.4 万行；蓝图仅做配置） |
| 模块 | 单模块 `Sn`（`Source/` 下 `Public` / `Private` 按领域对称分目录） |
| 依赖插件 | GameplayAbilities、CommonUI、OnlineSubsystemNull、FlatNodes |
| 玩法类型 | 第三人称动作 RPG：刷怪生存 + 装备构筑（自由网格 + 石板 + 羁绊） |

> **这个仓库只包含 C++ 源码。** 美术资产、蓝图、DataTable 资产不在仓库里，克隆后把 `Source/` 放回工程根目录即可编译。

---

## 系统一览

| 系统 | 做了什么 |
|---|---|
| **技能（GAS）** | 四类 C++ 能力家族：伤害形状（球形瞬伤 / 弹道）、生命周期（计时释放 / 连射）、召唤窗口、事件计数。家族内新增技能是纯数据资产，零 C++ |
| **属性与伤害** | 30 余项属性；`ExecCalc` 统一结算：闪避 → 元素抗性 → 暴击 → 韧性 → 防御（护甲穿透抵消）→ 护盾吸收 → 生命 |
| **事件通道** | 统一的 `Event.*` GameplayTag 事件体系（命中、受伤、资源获得、击杀），带递归防护与语义约定 |
| **背包 / 装备网格** | 6×6 自由网格，物品可旋转、稀疏存储；石板提升覆盖格等级；装备等级 = 行等级 + 格子等级 |
| **羁绊** | 按网格上装备的 tag 计数，取满足的最高档生效；属性走通用 GE，被动走授予的能力 |
| **物品与经济** | DataTable 驱动；右键动作表；合成（全有或全无）；金币账本（服务器授权、客户端只读） |
| **交互系统** | 五层：感知 / 数据 / 配置 / 逻辑 / 调度。定时器扫描而非 Tick，候选列表稳定排序，无状态动作对象 |
| **对话** | 行表 + 推进控制器，窗口系统承载 |
| **敌人与刷怪** | DataTable 配置敌人；敌人对象池；生存刷怪（波次、难度表） |
| **掉落与经验** | 代码弹道（不依赖刚体）；经验粒子按碎片拆分、双半径磁吸；掉落物按类分池；全队经验 |
| **UI** | UMG 窗口管理器（常驻实例、Z 序、输入模式、焦点）+ WidgetController 自上而下注入 |
| **存档** | 带版本号的存档结构、字段迁移、只持久化无法推导的数据 |
| **联机** | LAN 会话（OnlineSubsystemNull），服务器权威 + 属性复制，客户端预测与纠正 |

## 架构要点

**1. 服务器权威，客户端只读。**
所有属性、掉落、经验、击杀判定在服务器计算，客户端只播放表现。客户端 UI 初始化不依赖复制到达顺序——PlayerState / ASC 未就绪时静默等待，由就绪链通知，不靠 `OnPossess` 单路径。

**2. GAS 归属按实体类型分开。**
玩家 ASC + AttributeSet 挂在 `PlayerState`（随玩家跨 Pawn 存活）；敌人 ASC + AttributeSet 是 `Character` 的 subobject（随敌人一起池化复用）。

**3. 数据驱动到底。**
敌人、物品、技能、交互、羁绊、配方、品质样式全部是 DataTable 行，C++ 里没有硬编码的数值和 ID。新增内容 = 加一行数据，不改调度代码。

**4. 派生-重放模型。**
装备网格只存布局（哪格放了什么、旋转），属性与被动由布局 + 配置表重算；读档和旅行时全量重算，局内改布局时按覆盖闭包增量重放。好处是配置改动自动生效、存档不出现两份真相。

**5. 事件而不是直接调用。**
伤害事件分攻击侧（减免前）与受击侧（真实数值）两个，因为两半在不同机器上结算；次级伤害不发事件；跨实例递归用反弹标记掐断。

**6. 无状态行为对象。**
交互动作、物品动作都是每次执行 `NewObject` 出来的无状态对象——天然可重入、可单测、重复包不会产生副作用。

## 代码结构

```
Source/
├── Sn.Target.cs
├── SnEditor.Target.cs
└── Sn/
    ├── Sn.Build.cs
    ├── Sn.cpp / Sn.h
    ├── Public/          （与 Private 对称，按领域分目录）
    │   ├── AbilitySystem/   GAS：AttributeSet、GameplayTags、能力家族、ExecCalc
    │   ├── Characters/      玩家 / 敌人 / 伴侣 / AI 控制器
    │   ├── Player/          PlayerController（RPC 与输入）、PlayerState
    │   ├── Game/            GameMode / GameState / HUD / Settings / GameInstance / Save
    │   ├── Interaction/     交互五层
    │   ├── Inventory/       背包 / 装备网格 / 羁绊 / 物品表 / 合成 / 金币
    │   ├── Combat/          伤害管线、弹道、可破坏物、战斗模式组件
    │   ├── Spawn/           敌人池、生存刷怪
    │   ├── Loot/            经验球、物品掉落、弹道组件、掉落池
    │   ├── Dialogue/        对话行表与推进
    │   └── UI/              Widget / WidgetController / 窗口系统
    └── Private/         （同上）
```

规模分布（行数）：AbilitySystem 5.3k、Game 4.3k、Inventory 4.2k、UI 2.7k、Player 2.6k、Characters 2.2k、Loot 1.1k、Combat 1.0k、Spawn 0.6k、Interaction 0.4k。

## 编译

1. 安装 Unreal Engine 5.8 与 Visual Studio 2022（含「使用 C++ 的游戏开发」工作负载）
2. 把本仓库的 `Source/` 放进一个 UE 5.8 工程的根目录（工程文件与资产不在此仓库）
3. 生成工程文件后编译 `SnEditor` 目标

## 演示

视频：<https://www.bilibili.com/video/BV1JgeY65EMJ/>

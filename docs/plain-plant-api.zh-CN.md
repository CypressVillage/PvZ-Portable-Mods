# 无特殊行为植物：Lua API 与原版接入

本文描述首个可游玩的植物增量，不代表整个 Mod API 已完成。

## 本次范围

- 新植物使用独立内容 ID，不占用或冒充原版类型，仍由 C++ `Plant` 承载。
- 支持普通植物的选卡、种植预览、地形及占用判断、费用、冷却、静态外观、被吃与移除。
- 保留原版控制流程，在定义查询、初始化、创建、放置检查与移除位置接入扩展。
- 提供 `plants.define`、`plants.get`、`hooks.on`、`hooks.off`。
- **尚不支持**动画资源、能力、通用实体／世界操作、原版定义修改、实例订阅、图鉴条目、自定义解锁和中途存档。
- 原有 `mod.register('plant', ...)` 仍只注册基础数据；只有 `plants.define` 创建可玩的植物。

## 最小 Mod

```text
plain-plant/
├── mod.json
├── main.lua
└── assets/
    ├── stump.png
    └── stump_card.png
```

`mod.json`：

```json
{
  "id": "plain-plant",
  "name": "Plain Plant Example",
  "version": "1.0.0",
  "api_version": 1,
  "entry": "main.lua"
}
```

`main.lua`：

```lua
local id = plants.define("stump", {
    name = "Stump",
    description = "A plain plant with no special abilities.",
    cost = 50,
    cooldown = 750,
    health = 300,
    appearance = {image = "assets/stump.png"},
    card = {image = "assets/stump_card.png"},
})

hooks.on("plant.spawned", {type = id}, function(ctx)
    mod.log("spawned at " .. ctx.column .. "," .. ctx.row)
end)
```

不需要能力或更新回调。完整示例为 `examples/mods/plain-plant/`，包含两张可直接使用的原创程序生成 PNG。`make_assets.py` 只用于重建图片，玩家不需要 Python。

将包复制到资源根目录的 `mods/`，在**原始玩家数据根目录**的 `userdata/mods.json` 显式启用：

```json
{"enabled": ["plain-plant"]}
```

也可以在主菜单的“选项 → Mods”中查看简介并启用／关闭，再点击 Save 保存配置，详见 [游戏内 Mod 管理](mod-manager.zh-CN.md)。两种方式均为重启生效，无热重载。正常选卡时，点击面板标题右侧的 `1/2 >` 进入 Mod 页，每页最多 40 株，继续点击循环翻页；已选卡片始终保留在卡槽中。冒险模式最初几关没有正常选卡，该流程保持原样，不自动解锁原版植物。

## `plants.define(name, definition) → id`

仅注册期调用，返回 `mod-id:name`。本地名称以小写 ASCII 字母开头，后续允许小写字母、数字、`_`、`-`、`.`，最长 96 字符，不允许 `..` 或尾随 `.`。重复定义或与 `mod.register` 重复占用 ID 阻止启动。

| 字段 | 类型 | 默认值／规则 |
| --- | --- | --- |
| `name` | string | 默认注册名，不允许空；直接显示，不是原版翻译键 |
| `description` | string | 默认空，用于卡片提示 |
| `cost` | integer | 默认 50，范围 0..9990 |
| `cooldown` | integer | 默认 750，范围 1..1000000；游戏逻辑 tick，不是毫秒 |
| `health` | integer | 默认 300，范围 1..100000000，初始与最大生命 |
| `appearance` | `{image=string}` | 必填，包内相对 PNG 路径 |
| `card` | `{image=string}` | 可省略，默认使用 appearance 图片 |

图片均须为 **80×80 单张静态 PNG**。卡片图片是卡内植物插图，不是整张卡背景。主线程启动时加载，损坏图片或尺寸不符会阻止启动。

不接受未知字段、函数、元表；例如 `abilities`、`placement`、`appearance.animation` 暂时报错，而不是被静默忽略。路径拒绝绝对路径、目录穿越与包外符号链接等。

默认按普通、非水生、非飞行、非升级植物种植：水上需要睡莲、屋顶需要花盆，占据普通植物位置。

## `plants.get(id) → definition | nil`

查询通过 `plants.define` 注册的新植物。无 `:` 的名称按当前 Mod 补全。返回带 `id` 和默认字段的独立数据副本；不存在返回 nil。可以查询其他已注册的新植物，但本版不能查询原版定义。

```lua
assert(plants.get("stump").id == "plain-plant:stump")
```

修改返回值不改变注册表。这是定义查询，不是能力实例状态；能力系统尚未实现。

## Hook

```lua
local token = hooks.on(event, callback)
local token = hooks.on(event, {type = id, priority = 0}, callback)
local removed = hooks.off(token)
```

- `on` 仅注册期调用，同事件允许多个回调。
- 可省略 type 筛选；无 `:` 时按当前 Mod 补全。
- priority 范围 -1000000..1000000，小值先执行。相同优先级按依赖／加载顺序，再按本 Mod 注册顺序。
- 后续回调取得前面已生效的修改，同字段后写生效。
- `off` 可在运行期调用，只能取消自己的订阅；返回是否找到 token。可取消自己或本轮尚未执行的回调。
- 上下文是快照，不是实体对象；只有各事件明确说明的字段可写。
- 回调异常或非法字段值记录带 Mod ID、事件名的错误；Lua 执行异常含 traceback。该回调修改不生效，后续回调继续；日志、`mod.set_state` 等其他副作用不回滚。
- 本版提供全局订阅和类型筛选，暂不提供定义内函数或实例级订阅。

### `plant.before_initialize`

原版／自定义初始属性计算后、设置最大生命之前执行，仅场上实体，不在菜单缓存绘制时执行。

上下文：`type`、`column`、`row`、`health`。仅 health 可写，范围同定义，作用于本株初始和最大生命。不支持取消创建。

```lua
hooks.on("plant.before_initialize", {type = "stump"}, function(ctx)
    ctx.health = 600
end)
```

### `plant.spawned`

`Board::NewPlant` 初始化完成后通知。包括正常种植和直接创建，**不等价于已扣费的种植成功**。上下文同上，只观察，修改不影响实体。本版无实体引用。

### `plant.can_place`

坐标范围检查后、原版种植判断之前执行。上下文：`type`、`column`、`row`、`cancel`（默认 false）。仅 cancel 可写。true 拒绝放置，false 继续原版判断，不绕过地形／占用检查。后续 Hook 可将 cancel 重新设为 false。

```lua
hooks.on("plant.can_place", {type = "stump"}, function(ctx)
    ctx.cancel = ctx.column == 0
end)
```

列、行从 **0** 开始。此事件在预览或其他合法性查询中会频繁发生，**不是种植通知，不应在这里扣费、发奖励**。

### `plant.removed`

场上植物首次进入 `Plant::Die`、标记死亡后通知。包括被吃、铲除等调用 Die 的路径，不区分原因，不能取消。上下文为类型、格子与最后生命值，只观察。重复 Die 不重复通知；不是原生析构或整个 Board 销毁通知。

### 原版覆盖

以上 Hook 同样覆盖经过这些入口的原版场上植物，不要求改用能力系统。原版 ID 暂按 `gPlantDefs.mPlantName` 转小写并加 `base:` 生成，例如 `base:peashooter`、`base:wall_nut`、`base:starfruit`。不要假设全部等于枚举名去掉前缀；这是当前映射。

## 档案与存档

存在可玩定义或游戏 Hook 时，在加载玩家档案前切换到：

```text
原始玩家数据根目录/mod-profiles/<配置摘要>/
```

该目录有独立的 `userdata/` 和 `registry.regemu`。摘要根据启用 Mod 的加载顺序、ID、版本及规范化植物定义生成，不是包指纹，不检测版本和定义不变时的图片／脚本更新。启用配置仍从原始根目录读取。

- 不同配置可能获得新空档案，不复制原版进度。
- 运行时类型尚不能可靠跨版本恢复，**禁止中途保存与恢复**，包括直接 LawnSaveGame／LawnLoadGame 路径。
- 隔离档案可以记录关卡进度；未完成关卡退出后需重新开始。
- 无游戏定义／Hook 的纯基础层 Mod 不触发此隔离。

## 实现位置

- `src/Mod/ModRuntime.cpp`：Lua API、验证、分发与错误归属。
- `src/Lawn/ModPlants.h/.cpp`：类型、资源所有权、PlantDefinition 适配、上下文。
- `src/LawnApp.cpp`：主线程发布、档案隔离、HasSeedType。
- `src/Lawn/Plant.cpp`：定义查询、初始生命、静态帧、名称／提示与移除 Hook。
- `src/Lawn/Board.cpp`：创建通知、放置否决 Hook、存档保护。
- `src/Lawn/SeedPacket.cpp`：卡内插图，费用和冷却复用原版流程。
- `src/Lawn/Widget/SeedChooserScreen.h/.cpp`：可扩展选择状态与分页。
- `src/Lawn/System/SaveGame.cpp`：最低层保存／恢复保护。

## 验证命令

对已有 Linux/Ninja Debug 构建运行 Lua 合约测试：

```sh
python3 tools/run-plain-plant-tests.py --build build-vendored-lua
```

指定合法游戏资源目录，增加原生集成测试（需要 Xvfb）：

```sh
python3 tools/run-plain-plant-tests.py --build build-vendored-lua \
  --resources '/path/to/Plants vs. Zombies'
```

脚本重用构建的真实游戏对象和链接参数，只替换测试入口。挂载资源、安装示例、启用配置与存档均位于 `/tmp/opencode/` 新目录，不改真实资源／档案。不提供正式游戏资源。逻辑与绘制调用测试不等于人工观感验收或其他平台验证。

本次 Linux Debug 验证通过：完整游戏构建；Lua 定义校验、Hook 顺序／筛选／取消订阅、异常隔离与清理；42 株植物的分页和选取／取消；正常种植扣费与冷却、静态绘制、原版啃食和铲除；Lua 初始生命修改与放置否决；档案隔离及 Mod 存档保护；无 Mod 的原版植物初始化与新 Board 存档恢复。未进行其他平台构建、sanitizer 或人工界面观感验收。

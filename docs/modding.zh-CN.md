# PvZ-Portable Mod 制作规范（当前实现版）

本文描述 PvZ-Portable 当前已经实现的 Lua Mod 体系。目标是新增植物、僵尸与模式，同时保持跨平台与存档兼容。

实现状态：已于 2026-06-24 对照 C++ 源码核对。当前可用管线是 `mod.json` + Lua 注册 API + `resources/` 资源覆盖；`data/plants.json`、`data/zombies.json` 等 JSON 数据表尚未由引擎加载。

## 1. 设计目标

- 跨平台：桌面、移动、主机、WASM 都可用（不依赖 JIT）。
- 可控：脚本错误不影响主程序稳定性。
- Lua 优先：内容通过 Lua API 注册，资源通过 `resources/` 提供或覆盖。
- 兼容：原版存档不变，Mod 数据独立保存。

## 2. 目录与包结构

```
mods/
  <mod_id>/
    mod.json
    scripts/
      main.lua
      <optional>.lua
    resources/
      images/
      sounds/
      reanim/
      properties/
        default.xml
```

说明：
- `mod_id` 必须唯一，建议仅用 `a-z`、`0-9`、`_`、`-`。
- `resources/` 是资源覆盖层，优先级高于基础资源。
- `properties/default.xml` 可覆盖字符串键值。

## 3. 清单格式 (mod.json)

```json
{
  "id": "my_mod",
  "name": "My Mod",
  "version": "0.1.0",
  "author": "you",
  "description": "Adds a new plant and zombie",
  "priority": 100,
  "dependencies": ["base"],
  "entry": "scripts/main.lua"
}
```

字段说明：
- `priority`：当前会被解析，但实际排序主要由依赖拓扑顺序决定；不要依赖它解决冲突。
- `dependencies`：依赖的 Mod id。当前只会对已存在的依赖建立顺序关系，缺失依赖不会导致加载失败。
- `entry`：Lua 入口脚本。
- `data`：未实现。植物、僵尸、模式、投射物请在 Lua 中用 `Game.Register*` 注册。

## 4. 加载顺序

1) 扫描 `mods/` 并解析 `mod.json`。
2) 对已加载的依赖做拓扑排序。
3) 执行每个 Mod 的 Lua `entry`，并调用 `OnModInit`。
4) 在资源 XML 解析前注册每个 Mod 的 `resources/` 覆盖目录。

冲突与限制：
- `ModLoader` 当前没有严格拒绝重复的 manifest id。
- 植物、僵尸、模式、投射物的重复注册 id 会被 `ModRegistry` 拒绝，不会后加载覆盖先加载。
- 所有 Mod 共享一个 Lua 全局环境。入口加载后会捕获回调列表，但全局变量/函数仍可能互相覆盖；建议尽量使用 `local`。

## 5. Lua 数据注册

当前所有 Mod 内容都通过 Lua API 注册，不读取 JSON 数据表。

### 5.1 植物

```lua
Game.RegisterPlant({
  id = "fire_pea",
  name = "Fire Pea",
  cost = 200,
  cooldown = 750,
  subClass = 1,
  launchRate = 90,
  reanimation = "reanim/FirePea.reanim"
})
```

必填字段：
- `id`：植物唯一标识符

可选字段：
- `name`：显示名称（默认 `"ModPlant"`）
- `cost`：阳光花费（默认 `50`）
- `cooldown`：冷却帧数（默认 `750`）
- `subClass`：`0`=普通，`1`=射手（默认 `0`）
- `launchRate`：发射间隔帧数（默认 `0`）
- `reanimation`：复用的原版动画路径，如 `"reanim/FirePea.reanim"`
- `reanimFile`：外部 reanim XML 路径（相对于 mod 根目录），如 `"resources/reanim/custom.xml"`

动画说明：
- `reanimFile` 优先于 `reanimation`；同时设置时使用外部文件。
- 自定义 reanim XML 格式详见 `docs/custom-plants.md`。

保留范围：
- Mod 植物的 SeedType ID 从 `2000` 起自动分配。

### 5.2 僵尸

```lua
Game.RegisterZombie({
  id = "conehead_fast",
  name = "Conehead Fast",
  bodyHealth = 600,
  headHealth = 100,
  speed = 1.2,
  damage = 100,
  reanimation = "reanim/Zombie.reanim",
  helmType = HelmType.TRAFFIC_CONE,
  helmHealth = 370
})
```

必填字段：`id`

保留范围：
- Mod 僵尸的 ZombieType ID 从 `3000` 起自动分配。

### 5.3 投射物

```lua
Game.RegisterProjectile({
  id = "pea_plus",
  damage = 20,
  speed = 7.5,
  image = "IMAGE_PROJECTILE_PEA_PLUS"
})
```

### 5.4 模式

```lua
Game.RegisterMode({
  id = "rush_mode",
  name = "Rush Mode",
  page = 0,
  row = 0,
  col = 0,
  iconIndex = 0
})
```

必填字段：`id`

### 5.5 字符串

当前字符串覆盖使用 `resources/properties/default.xml`。`strings.json` 尚未实现。

## 6. 资源覆盖规则

- 资源查找顺序：Mod 资源 > 原版资源。
- 同名资源后加载覆盖先加载。
- 缺失资源则回退到原版。

建议路径：
- `resources/images/`
- `resources/sounds/`
- `resources/reanim/`
- `resources/properties/`

## 7. Lua API (最小版)

### 7.1 全局对象

`Game`:
- `Game.Log(text)` — 输出日志
- `Game.RegisterPlant(def)` — 注册自定义植物
- `Game.RegisterZombie(def)` — 注册自定义僵尸
- `Game.RegisterMode(def)` — 注册自定义模式
- `Game.RegisterProjectile(def)` — 注册自定义投射物
- `Game.GetMode()` — 获取当前游戏模式 ID
- `Game.SaveModData(key, value)` — 保存 Mod 数据（字符串键值对）
- `Game.LoadModData(key)` — 读取 Mod 数据，不存在返回 `nil`

`Board`:
- `Board.SpawnZombie(zombie_id, row)` — 在指定行生成僵尸，`zombie_id` 可为字符串（Mod 注册 ID）或整数（运行时类型），返回 Entity 或 nil
- `Board.SpawnPlant(plant_id, row, col)` — 在指定行列种植植物，`plant_id` 可为字符串或整数，返回 Entity 或 nil
- `Board.GetWave()` — 获取当前波次编号

`Entity`（植物或僵尸的包装对象）:
- `entity.id` — Mod 注册 ID 字符串，如 `"peashooter_plus"`；原版实体返回 `""`
- `entity.type` — 实体类型（`SeedType` 或 `ZombieType` 的整数值）
- `entity.hp` — 当前血量（`mPlantHealth` 或 `mBodyHealth`）
- `entity:Damage(amount)` — 对实体造成伤害，血量归零时自动触发死亡
- `entity:IsSun()` — 仅对掉落物（Coin）有效，判断是否为阳光
- `entity:Collect()` — 仅对掉落物（Coin）有效，触发收集（伴随音效）

`UI`（自定义对话框系统）:
- `UI.CreateDialog(opts)` — 创建并显示自定义对话框
- `UI.ShowMessage(title, body)` — 快捷创建单按钮模态提示框

`Dialog`（对话框 userdata，由 `UI.CreateDialog` 或 `UI.ShowMessage` 返回）:
- `dialog:AddButton(text, callback)` — 添加按钮，`callback` 为 Lua 函数闭包，按钮被点击时调用，调用后对话框自动关闭
- `dialog:AddLabel(text, x, y)` — 添加静态文本标签
- `dialog:Close()` — 手动关闭并销毁对话框
- `dialog:SetTitle(text)` — 动态修改对话框标题
- `dialog:SetBody(text)` — 动态修改对话框正文

### 7.2 回调列表

所有回调为可选，未定义时安全跳过。回调参数中的 `plant`/`zombie` 均为 Entity 对象。

```lua
function OnModInit() end
function OnGameStart() end
function OnLevelStart(mode_id) end
function OnWaveStart(wave_index) end
function OnPlantSpawn(plant) end
function OnZombieSpawn(zombie) end
function OnPlantAttack(plant, target) end
function OnZombieDie(zombie) end
function OnLevelEnd(is_win) end
function OnCoinSpawn(coin) end
```

回调参数说明：
- `mode_id`：整数，当前游戏模式
- `wave_index`：整数，波次编号
- `plant` / `zombie` / `target` / `coin`：Entity 对象，支持 `.type`、`.hp` 属性和相关方法
- `is_win`：布尔值，`true` 表示关卡胜利，`false` 表示失败

## 8. 示例脚本

### 8.1 基础示例

```lua
function OnModInit()
  Game.Log("My Mod initialized")
end

function OnLevelStart(mode_id)
  if mode_id == "rush_mode" then
    Game.Log("Rush Mode start")
  end
end
```

### 8.2 自定义对话框示例

```lua
function OnLevelStart(mode_id)
  local dlg = UI.CreateDialog({
    title = "关卡提示",
    body = "这是一个由 Lua Mod 创建的对话框。",
    modal = true
  })

  dlg:AddButton("生成僵尸", function()
    Board.SpawnZombie(0, 2)
  end)

  dlg:AddButton("关闭", function()
    Game.Log("对话框已关闭")
  end)
end

function OnZombieSpawn(zombie)
  Game.Log("僵尸生成，血量: " .. tostring(zombie.hp))
end
```

## 9. 存档策略

- 原版存档保持不变。
- Mod 数据存放：

```
<save_root>/modsave/<mod_id>.json
```

API：
- `Game.SaveModData(key, value)`
- `Game.LoadModData(key)`

## 10. 热重载 (开发模式)

未实现。可作为未来开发调试功能：
- 重载 Lua 脚本与资源。
- 建议只允许在主菜单或重开关卡后生效。
- 避免在关卡中热替换已有实体。

## 11. 错误处理

- 清单缺失或非法：跳过该 Mod 并记录错误。
- Lua 入口或回调异常：记录日志并继续运行其他回调。

## 12. 版本与兼容

- 未实现。当前加载器只把 `version` 当作字符串读取，不进行 schema/version 校验。
- 未来可能支持 `mod.json` 字段：

```json
"schema_version": 1
```

当前不会因为 schema 版本较高而提示警告。

## 13. 后续可扩展项

- ~~自定义 UI 面板~~（已部分实现：`UI.CreateDialog` / `UI.ShowMessage`，支持按钮、文本标签与 Lua 闭包回调；待扩展：输入框等控件）
- JSON 数据表注册植物、僵尸、投射物、模式与字符串
- 每个 Mod 独立 Lua 环境或模块隔离
- 事件优先级与过滤器
- Mod 依赖版本范围
- 关卡编辑器与离线打包工具

## 14. 引擎接入点与现状

本节记录当前源码中的接入点。早期设计中的 JSON 数据表加载尚未实现。

### 14.1 资源与字符串加载接入点

- 资源 XML 入口：在 [src/LawnApp.cpp](src/LawnApp.cpp) 中调用 `mResourceManager->ParseResourcesFile("properties/resources.xml")`。
- 字符串加载入口：在 [src/LawnApp.cpp](src/LawnApp.cpp) 中的加载线程调用 `TodStringListLoad` 与 `LoadProperties`。

当前流程：
1) `LawnApp::Init()` 扫描并加载 Mod 清单，执行 Lua 入口。
2) `ParseResourcesFile` 前注册每个 Mod 的 `resources/` 目录叠加层。
3) `LoadingThreadProc()` 中加载 Mod 的 `resources/properties/default.xml`。
4) `strings.json` 尚未实现。

### 14.2 资源覆盖实现建议

目标：资源查找顺序为 `Mod > Base`，并保持原有 `ResourceManager` 接口不变。

实现策略：
- 在资源路径解析阶段对 `path` 做“多根目录搜索”。
- 维护 `std::vector<std::string> mResourceRoots`，按加载顺序插入。
- 修改 `GetResourcePath` 或 `ResourceManager::ParseCommonResource` 中对 `mDefaultPath` 的组合方式，使其支持多根目录。

涉及文件：
- [src/SexyAppFramework/Common.cpp](src/SexyAppFramework/Common.cpp)
- [src/SexyAppFramework/misc/ResourceManager.cpp](src/SexyAppFramework/misc/ResourceManager.cpp)

### 14.3 保存路径与 Mod 数据

现有保存路径由 `GetAppDataPath` 统一生成，入口在 [src/SexyAppFramework/Common.cpp](src/SexyAppFramework/Common.cpp)。

当前实现：
- `modsave/` 目录作为 Mod 额外数据区。
- 保持 `userdata/` 不变，保证原版存档兼容。

### 14.4 启动流程接入

当前应用初始化时加载 Mod：

1) 在 `SexyAppBase::Init()` 后、`LawnApp::Init()` 前完成 Mod 扫描与清单解析。
2) 在 `LawnApp::Init()` 中资源加载之前注册 Mod 资源根。
3) 在 `LoadingThreadProc()` 中加载 Mod 字符串覆盖。

涉及文件：
- [src/main.cpp](src/main.cpp)
- [src/LawnApp.cpp](src/LawnApp.cpp)
- [src/SexyAppFramework/SexyAppBase.cpp](src/SexyAppFramework/SexyAppBase.cpp)

## 15. Lua 绑定与事件触发策略

### 15.1 绑定层建议

- 使用 Lua 5.4 + 原生 C API。
- 在引擎侧构建轻量包装对象：`Game`、`Board`、`Entity`。
- 绑定函数必须是纯 ASCII 接口，避免 Unicode 名称。

### 15.2 事件触发点建议

在游戏主循环与实体生命周期中插入回调，最低成本触发点：

- `OnGameStart`：当 `LawnApp::Start()` 完成并进入标题画面时触发。
- `OnLevelStart`：`LawnApp::PreNewGame()` 或 `LawnApp::StartPlaying()` 进入关卡时触发。
- `OnWaveStart`：僵尸波次推进时触发，可在波次管理逻辑中插入。
- `OnPlantSpawn`：植物放置成功时触发。
- `OnZombieSpawn`：僵尸实例化成功时触发。
- `OnPlantAttack`：植物发射投射物或攻击时触发。
- `OnZombieDie`：僵尸死亡时触发。
- `OnLevelEnd`：结算时触发。
- `OnCoinSpawn`：掉落物（阳光/金币/其他）生成时触发。
- `OnPlantDie`：植物死亡时触发。
- `OnZombieAttack`：僵尸开始啃食植物时触发。
- `OnProjectileSpawn`：投射物生成时触发。
- `OnProjectileHit`：投射物命中僵尸时触发。
- `OnProjectileMiss`：投射物飞出屏幕时触发。
- `OnCoinCollect`：硬币/阳光被玩家收集时触发。
- `OnZombieReachHouse`：僵尸进入房子时触发。

涉及文件：
- [src/LawnApp.cpp](src/LawnApp.cpp)
- [src/Lawn/Board.cpp](src/Lawn/Board.cpp)
- [src/Lawn/Plant.cpp](src/Lawn/Plant.cpp)
- [src/Lawn/Zombie.cpp](src/Lawn/Zombie.cpp)
- [src/Lawn/Projectile.cpp](src/Lawn/Projectile.cpp)

## 16. 数据注册流程（植物/僵尸/模式）

当前流程：
1) Lua 入口调用 `Game.RegisterPlant` / `Game.RegisterZombie` / `Game.RegisterMode` / `Game.RegisterProjectile`。
2) `ModRegistry` 使用 `id` 作为主键，分配运行时 id。
3) 游戏逻辑通过 `ModRegistry` 查询运行时定义，避免侵入式改枚举值。

已实现结构：
- `ModRegistry`
- 为 Mod 内容分配运行时 id，并维护注册 id 与运行时 id 的反查

注意点：
- 原生枚举保持不变，Mod id 走动态映射。
- 避免直接硬编码数组索引，改为查表访问。

## 17. 资源与字符串覆盖的落地顺序

当前加载顺序：
1) `LawnStrings.txt`（原版）
2) 原版 `properties/default.xml` 与 `properties/Layout.xml`
3) `ModRegistry` 注入植物名称/描述字符串
4) Mod `resources/properties/default.xml`

说明：后加载覆盖前加载。

## 18. Mod 加载器的最小职责

- 扫描 `mods/` 目录并解析 `mod.json`
- 对已存在依赖做拓扑排序
- 记录循环依赖错误
- 资源根注册由 `LawnApp` 根据 manifest 完成
- 初始化 Lua VM 并执行入口脚本

建议日志输出：
- 解析成功/失败、依赖缺失、字段缺失、覆盖冲突

当前限制：缺失依赖、重复 manifest id、priority 冲突策略尚未严格处理。

## 19. 开发模式热重载建议

未实现。未来可考虑：

- 提供命令行参数 `-moddev` 打开热重载
- 仅允许在主菜单或关卡外重载
- 允许重载：数据表、Lua 脚本、资源索引
- 不允许：在关卡中热替换实体

## 20. 最小实现里程碑

1) Mod 扫描与清单解析
2) 资源覆盖与字符串覆盖
3) Lua 注册与注册表查询
4) Lua 入口与 `OnModInit` 回调
5) 事件回调最小集合（Spawn/Die/LevelStart/LevelEnd）
6) `modsave/` 独立存档

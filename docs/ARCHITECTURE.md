# PvZ-Portable Mod 框架架构（当前实现）

本文档已于 2026-06-24 对照源码核对。当前实现以 `mod.json` 清单、Lua 注册 API、`resources/` 覆盖和 `ModRegistry` 动态注册为核心；JSON 数据表加载、热重载、依赖版本约束和每个 Mod 独立 Lua 环境尚未实现。

## 一、Mod 框架组成部分

```
src/Mod/                         # Mod 框架核心（6 个模块）
├── ModLoader.h/.cpp             # Mod 加载器
├── ModLua.h/.cpp                # Lua 虚拟机与 API 绑定
├── ModRegistry.h/.cpp           # 数据注册表（植物、僵尸、模式、投射物）
├── ModSave.h/.cpp               # Mod 独立存档（JSON 格式）
├── ModJson.h/.cpp               # JSON 解析/序列化共享基础模块
└── LuaProxyDialog.h/.cpp        # Lua 创建的对话框代理
```

## 二、各模块功能

### (1) ModLoader — Mod 加载器

- 扫描 `mods/<mod_id>/mod.json` 清单文件
- 解析 `ModManifest`（id, name, version, priority, dependencies, entry）
- **依赖排序**：对已存在的 dependencies 做拓扑排序；循环依赖会记录错误并追加末尾
- **priority 限制**：字段会被解析，但当前排序主要由拓扑顺序决定，不能可靠用于覆盖冲突处理
- **未实现限制**：缺失依赖不会导致加载失败，重复 manifest id 不会被严格拒绝
- **字符串覆盖**：加载所有 Mod 的 `resources/properties/default.xml`，并注入已注册 Mod 植物的名称/描述字符串

### (2) ModLua — Lua 虚拟机与 API

- 集成 **Lua 5.4**，通过 `PVZ_ENABLE_LUA` CMake 选项控制
- 注册 Lua 全局 API 表：

| 全局表 | API | 说明 |
|--------|-----|------|
| `Game` | `Log(text)` | 输出日志 |
| | `SaveModData(key, value)` | 保存 Mod 数据 |
| | `LoadModData(key)` | 加载 Mod 数据 |
| | `SetSpeed(multiplier)` | 设置游戏速度 |
| | `GetSpeed()` | 获取游戏速度 |
| | `GetMode()` | 获取当前游戏模式 |
| | `GetMenuButtonRect()` | 获取菜单按钮位置 |
| | `GetSun()` / `SetSun(amount)` | 读取/设置阳光 |
| | `GetTotalWaves()` | 获取总波数 |
| | `IsNight()` / `HasPool()` / `IsRoof()` / `IsFog()` | 当前场景查询 |
| | `SpawnSun(x, y, value)` / `SpawnCoin(x, y, type)` | 生成掉落物 |
| | `PlayFoley(type)` / `DisplayAdvice(text, style)` | 音效与提示 |
| | `RegisterProjectile(def)` | 注册自定义投射物 |
| | `RegisterPlant(def)` | 注册自定义植物 |
| | `RegisterZombie(def)` | 注册自定义僵尸 |
| | `RegisterMode(def)` | 注册自定义模式 |
| `Board` | `SpawnZombie(id_or_type, row)` | 生成僵尸 |
| | `SpawnPlant(id_or_type, row, col)` | 放置植物 |
| | `GetWave()` | 获取当前波次 |
| | `Pause(bool)` / `IsPaused()` | 暂停控制 |
| | `FindTargetZombie(plant)` | 为植物查找目标 |
| | `GetZombiesInRow(row)` / `GetPlantsInRow(row)` | 行内实体查询 |
| | `GetZombieAt(col,row)` / `GetPlantAt(col,row)` | 格子实体查询 |
| | `GetAllZombies()` / `GetAllPlants()` | 全局实体查询 |
| | `AddProjectile(x,y,row,type)` | 生成投射物 |
| | `AddButton(id, x, y, w, h, label)` | 添加自定义按钮 |
| | `RemoveButton(id)` | 移除按钮 |
| | `SetButtonVisible(id, visible)` | 设置按钮可见性 |
| | `SetButtonLabel(id, label)` | 设置按钮标签 |
| `Entity` | `entity.type` / `entity.hp` / `entity.id` | 属性只读访问（`id` 返回 Mod 注册字符串） |
| | `entity:Damage(amount)` | 造成伤害 |
| | `entity:DistanceTo(other)` | 实体距离 |
| | `entity:IsSun()` | 是否为阳光 |
| | `entity:Collect()` | 收集（硬币） |
| | 植物/僵尸/投射物扩展方法 | 动画、状态、伤害、速度、目标等控制 |
| `UI` | `CreateDialog({title, body, modal})` | 创建自定义对话框 |
| | `ShowMessage(title, body)` | 显示消息框 |
| `Dialog` | `dialog:AddButton(text, callback)` | 添加按钮 |
| | `dialog:AddLabel(text, x, y)` | 添加静态文本标签 |
| | `dialog:Close()` | 关闭对话框 |
| | `dialog:SetTitle(text)` | 设置标题 |
| | `dialog:SetBody(text)` | 设置正文 |

- **事件钩子**：当前源码注册 26 个回调名

| 钩子 | 触发时机 |
|------|---------|
| `OnModInit()` | Mod 脚本首次加载 |
| `OnGameStart()` | 游戏启动，关卡开始前 |
| `OnLevelStart(mode)` | 关卡开始 |
| `OnWaveStart(wave)` | 波次推进 |
| `OnPlantSpawn(plant)` | 植物放置 |
| `OnPlantUpdate(plant)` | 植物每帧更新 |
| `OnZombieSpawn(zombie)` | 僵尸生成 |
| `OnPlantAttack(plant, target)` | 植物攻击 |
| `OnZombieDie(zombie)` | 僵尸死亡 |
| `OnLevelEnd(isWin)` | 关卡结束 |
| `OnCoinSpawn(coin)` | 硬币/阳光生成 |
| `OnBoardButtonClick(btnId)` | 自定义按钮点击 |
| `OnPlantDie(plant)` | 植物死亡 |
| `OnZombieAttack(zombie, plant)` | 僵尸啃食植物 |
| `OnProjectileSpawn(proj)` | 投射物生成 |
| `OnProjectileHit(proj, zombie)` | 投射物命中僵尸 |
| `OnProjectileMiss(proj)` | 投射物飞出屏幕 |
| `OnCoinCollect(coin)` | 硬币/阳光被收集 |
| `OnZombieReachHouse(zombie)` | 僵尸进入房子 |
| `OnPlantEaten(plant, zombie)` | 植物被啃食 |
| `OnPlantProduce(plant)` | 植物产出 |
| `OnPlantUpgrade(plant, oldType)` | 植物升级/变形 |
| `OnZombieFrozen(zombie, isFrozen)` | 僵尸冻结/冰冻 |
| `OnZombieButtered(zombie)` | 僵尸被黄油命中 |
| `OnZombieMindControl(zombie)` | 僵尸被魅惑 |
| `OnCoinExpire(coin)` | 掉落物过期 |
| `OnFlagRaise(waveIndex)` | 旗帜波提示 |
| `OnMowerTriggered(row, mowerType)` | 小推车触发 |
| `OnSunCountChange(oldAmount, newAmount)` | 阳光数量改变 |

- **Lua 环境限制**：所有 Mod 当前共享同一个 Lua VM 和全局环境。入口脚本执行后会捕获全局回调到内部 handler 表，但全局变量和辅助函数仍可能相互覆盖。

### (3) ModRegistry — 数据注册表

- **运行时 ID 映射**：Mod 植物 `seedType >= 2000`，僵尸 `zombieType >= 3000`，模式 `baseMode >= 5000`，投射物 `projectileType >= 4000`（自动分配）
- **注册类型**：
  - `ModPlantDef` — id, seedType, seedCost, refreshTime, subClass, launchRate, projectileType, plantName, reanimationName, imageName
  - `ModZombieDef` — id, zombieType, bodyHealth, headHealth, speed, damage, reanimationName, helm/shield, hasHead/hasArm, zombieName
  - `ModModeDef` — id, baseMode, challengePage/Row/Col/IconIndex/Name
  - `ModProjectileDef` — id, damage, speed, imageName, projectileType
  - `ModModeChallengeDef` — baseMode, page, row, col, iconIndex, name（供 ChallengeScreen 动态渲染）
- **反向查询**：`FindByRuntimeId()` 支持运行时 ID → Mod 定义的反查
- **动态 Reanim**：`RegisterDynamicReanim()` 支持外部 reanim XML 注册
- **图鉴集成**：`GetTotalAlmanacPlants()` / `GetAlmanacPlantAt()` 与图鉴联动
- **冲突策略**：重复注册 id 会失败并记录错误，不会后加载覆盖先加载

### (4) ModJson — JSON 共享模块

- 提供 `ModJsonParser`、`ModJsonSerializeMap`、`ModJsonDeserializeMap`、`ModJsonReadFile`
- 被 `ModLoader`（解析 mod.json 清单）和 `ModSave`（存档读写）统一调用，消除重复代码

### (5) ModSave — Mod 独立存档

- 存储路径：`<appdata>/modsave/<mod_id>.json`
- 格式：标准 JSON `{"key":"value", ...}`
- API：`SaveValue(modId, key, value)` / `LoadValue(modId, key, &outValue)`
- 与游戏存档 `userdata/` 完全隔离

### (6) LuaProxyDialog — Lua 对话框代理

- 继承 `LawnDialog`，支持 Lua 侧创建模态/非模态对话框
- 使用 `luaL_ref` 管理 Lua 回调引用，避免 GC 问题
- 支持 `AddLuaButton()` 动态添加按钮
- 支持 `AddLuaLabel()` 添加静态文本标签
- 对话框 ID 从 `1000` 开始分配
- `ButtonDepress`：Lua 回调返回 `false` 阻止对话框关闭，返回 `nil`/其他值则照常关闭

### (7) 引擎适配集成点

引擎集成点直接内联在各模块中（`Plant.cpp`、`Projectile.cpp`、`SeedChooserScreen` 等），通过 `gModRegistry` 查询运行时数据：
- **AlmanacDialog**：显示 Mod 植物的图鉴条目
- **SeedChooserScreen**：Mod 植物显示在选卡界面、支持分页、随机选取
- **Board**：Mod 植物的放置与行为
- **Plant::GetPlantDefinition()**：将 ModPlantDef 字段映射到引擎 PlantDefinition
- **Projectile::Draw()** / **GetProjectileDef()**：Mod 投射物自定义图片与属性
- **ReanimationType** 解析：将名称字符串映射到运行时 ID
- **PreloadForUser() / CutScene**：预加载 Mod 植物资源

## 三、Mod 包格式

```
mods/<mod_id>/
├── mod.json                           # 清单文件（id, name, version, priority, entry）
├── scripts/
│   └── main.lua                       # Lua 入口脚本（植物/僵尸/模式/投射物注册在此）
├── resources/
│   ├── images/                        # 图片资源
│   ├── sounds/                        # 音频资源
│   ├── reanim/                        # 自定义动画 XML
│   └── properties/
│       └── default.xml                # 属性文件覆盖（字符串本地化）
```

当前不会读取 `data/plants.json`、`data/zombies.json`、`data/modes.json`、`data/projectiles.json` 或 `strings.json`。

## 四、修改了原项目的哪些部分

| 文件 | 修改内容 |
|------|---------|
| `CMakeLists.txt` | 添加 Mod 源文件、Lua 链接选项 `PVZ_ENABLE_LUA` |
| `src/LawnApp.h/.cpp` | Mod 初始化接入（`InitModSystem()`）、游戏启动/停止钩子、存档隔离 |
| `src/Lawn/Board.h/.cpp` | 事件钩子插入、自定义按钮支持（`mLuaButtons` 容器） |
| `src/Lawn/Plant.h/.cpp` | 注册表查询替代硬编码属性、植物生成/攻击/更新钩子、Mod 植物自定义贴图加载 |
| `src/Lawn/Zombie.cpp` | 注册表查询僵尸属性、僵尸生成/死亡钩子 |
| `src/Lawn/Projectile.cpp` | 注册表查询投射物属性、Mod 投射物自定义图片渲染 |
| `src/Lawn/SeedPacket.cpp` | Mod 植物冷却/阳光显示适配 |
| `src/Lawn/Widget/SeedChooserScreen.h/.cpp` | Mod 植物显示、分页支持、随机选取适配 |
| `src/Lawn/Widget/AlmanacDialog.h/.cpp` | 图鉴 Mod 植物条目渲染 |
| `src/Lawn/Challenge.cpp` | 移除硬编码模式数量检查 |
| `src/Lawn/CutScene.cpp` | Mod 启动适配、Mod 植物资源预加载 |
| `src/Lawn/CursorObject.cpp` | Mod 光标支持 |
| `src/Lawn/System/ReanimationLawn.h/.cpp` | Mod reanim 注册 |
| `src/SexyAppFramework/Common.h/.cpp` | 多根目录资源路径、Mod 存档路径 `modsave/` |
| `src/SexyAppFramework/paklib/PakInterface.cpp` | PAK 覆盖支持 |
| `src/Sexy.TodLib/Reanimator.h/.cpp` | 动态 reanim 加载（外部 XML 跟踪元素解析） |
| `src/Mod/ModLua.h/.cpp` | 新增 `OnGameStart`、`OnWaveStart`、`OnPlantUpdate` 钩子、`Dialog:AddLabel` API |
| `docs/pvz-animation.md` | Reanim 动画系统参考文档（以豌豆射手为例，含 XML 格式、轨道命名、Mod 使用指南） |
| `src/Mod/ModRegistry.h/.cpp` | `ModPlantDef` 新增 `imageName` 字段；`ModModeDef` 新增挑战 UI 字段；新增 `ModModeChallengeDef`、`GetModeChallengeDefs()` |
| `src/Mod/ModJson.h/.cpp` | **新增**：JSON 解析/序列化共享模块，供 ModLoader 和 ModSave 统一调用 |
| `src/Mod/ModSave.cpp` | 存档格式从 `key=value` 文本改为标准 JSON，依赖 `ModJson` |
| `src/Mod/ModLoader.cpp` | 移除内联 JSON 解析器，改用 `ModJson` |
| `src/Mod/ModLua.cpp` | `entity.id` 返回 Mod 注册 ID 字符串；`Game.RegisterMode()` 新增挑战 UI 参数读取 |
| `src/Mod/LuaProxyDialog.cpp` | `ButtonDepress` Lua 回调返回 `false` 可阻止对话框关闭 |
| `src/Lawn/Widget/ChallengeScreen.h/.cpp` | 支持 `ModRegistry` 注册的自定义模式动态生成按钮 |

## 五、架构关系图

```
┌─────────────────────────────────────────────────────┐
│                    main.cpp                         │
│                      │                              │
│                   LawnApp                           │
│                   /      \                          │
│          SexyAppFramework    Mod 框架                │
│          (引擎层)            │                      │
│          ┌────┴────┐    ┌───┴──────────────┐        │
│          │         │    │                  │        │
│    Sexy.TodLib  Lawn/  ModLoader  ModLua   │        │
│    (动画/粒子)  (逻辑)  ModRegistry ModSave │        │
│                          LuaProxyDialog    │        │
│                               │            │        │
│                               ▼            │        │
│                     mods/<mod_id>/         │        │
│                     ├─ mod.json            │        │
│                     ├─ scripts/main.lua    │        │
│                     └─ resources/          │        │
│                                          │        │
│    ModJson (解析/序列化) ─── ModLoader   │        │
│                └──────── ModSave         │        │
└─────────────────────────────────────────────────────┘
```

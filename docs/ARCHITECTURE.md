# PvZ-Portable Mod 框架架构

## 一、Mod 框架组成部分

```
src/Mod/                         # Mod 框架核心（5 个模块）
├── ModLoader.h/.cpp             # Mod 加载器
├── ModLua.h/.cpp                # Lua 虚拟机与 API 绑定
├── ModRegistry.h/.cpp           # 数据注册表（植物、僵尸、模式、投射物）
├── ModSave.h/.cpp               # Mod 独立存档
└── LuaProxyDialog.h/.cpp        # Lua 创建的对话框代理
```

## 二、各模块功能

### (1) ModLoader — Mod 加载器

- 扫描 `mods/<mod_id>/mod.json` 清单文件
- 解析 `ModManifest`（id, name, version, priority, dependencies, entry）
- **依赖排序**：按 dependencies 拓扑排序 + priority 稳定排序
- **字符串覆盖**：加载所有 Mod 的 `properties/default.xml`

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
| | `GetMenuButtonRect()` | 获取菜单按钮位置 |
| | `RegisterProjectile(def)` | 注册自定义投射物 |
| | `RegisterPlant(def)` | 注册自定义植物 |
| | `RegisterZombie(def)` | 注册自定义僵尸 |
| | `RegisterMode(def)` | 注册自定义模式 |
| `Board` | `SpawnZombie(id_or_type, row)` | 生成僵尸 |
| | `SpawnPlant(id_or_type, row, col)` | 放置植物 |
| | `GetWave()` | 获取当前波次 |
| | `AddButton(id, x, y, w, h, label)` | 添加自定义按钮 |
| | `RemoveButton(id)` | 移除按钮 |
| | `SetButtonVisible(id, visible)` | 设置按钮可见性 |
| | `SetButtonLabel(id, label)` | 设置按钮标签 |
| `Entity` | `entity.type` / `entity.hp` / `entity.id` | 属性只读访问 |
| | `entity:Damage(amount)` | 造成伤害 |
| | `entity:IsSun()` | 是否为阳光 |
| | `entity:Collect()` | 收集（硬币） |
| `UI` | `CreateDialog({title, body, modal})` | 创建自定义对话框 |
| | `ShowMessage(title, body)` | 显示消息框 |
| `Dialog` | `dialog:AddButton(text, callback)` | 添加按钮 |
| | `dialog:AddLabel(text, x, y)` | 添加静态文本标签 |
| | `dialog:Close()` | 关闭对话框 |
| | `dialog:SetTitle(text)` | 设置标题 |
| | `dialog:SetBody(text)` | 设置正文 |

- **事件钩子**：12 个生命周期回调

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

### (3) ModRegistry — 数据注册表

- **运行时 ID 映射**：Mod 植物 `seedType >= 2000`，僵尸 `zombieType >= 3000`，模式 `baseMode >= 5000`，投射物 `projectileType >= 4000`（自动分配）
- **注册类型**：
  - `ModPlantDef` — id, seedType, seedCost, refreshTime, subClass, launchRate, projectileType, plantName, reanimationName, imageName
  - `ModZombieDef` — id, zombieType
  - `ModModeDef` — id, baseMode
  - `ModProjectileDef` — id, damage, speed, imageName, projectileType
- **反向查询**：`FindByRuntimeId()` 支持运行时 ID → Mod 定义的反查
- **动态 Reanim**：`RegisterDynamicReanim()` 支持外部 reanim XML 注册
- **图鉴集成**：`GetTotalAlmanacPlants()` / `GetAlmanacPlantAt()` 与图鉴联动

### (4) ModSave — Mod 独立存档

- 存储路径：`<appdata>/modsave/<mod_id>.json`
- API：`SaveValue(modId, key, value)` / `LoadValue(modId, key, &outValue)`
- 与游戏存档 `userdata/` 完全隔离

### (5) LuaProxyDialog — Lua 对话框代理

- 继承 `LawnDialog`，支持 Lua 侧创建模态/非模态对话框
- 使用 `luaL_ref` 管理 Lua 回调引用，避免 GC 问题
- 支持 `AddLuaButton()` 动态添加按钮
- 支持 `AddLuaLabel()` 添加静态文本标签
- 对话框 ID 从 `1000` 开始分配

### (6) 引擎适配集成点

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
| `src/Mod/ModRegistry.h/.cpp` | `ModPlantDef` 新增 `imageName` 字段 |

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
└─────────────────────────────────────────────────────┘
```

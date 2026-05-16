# PvZ-Portable 项目架构概览

> 基于提交记录分析，涵盖原项目组成与 Mod 框架架构。

---

## 一、原项目（PvZ-Portable）基本架构

PvZ-Portable 是 Plants vs. Zombies GOTY Edition (v1.2.0.1073) 的跨平台社区重实现版本，使用 **C++20 / SDL2 / OpenGL ES 2.0**，支持 Linux / Windows / macOS / Android / iOS / WASM / Switch 等平台。

### 1.1 顶层目录结构

```
src/
├── main.cpp                    # 程序入口，创建 LawnApp
├── LawnApp.h/.cpp              # 主游戏应用（继承 SexyApp）
├── GameConstants.h              # 游戏常量、枚举（SeedType, ZombieType 等）
├── ConstEnums.h                 # 额外的枚举定义
├── Resources.h/.cpp             # 资源提取
│
├── Lawn/                        # 游戏逻辑（核心）
│   ├── Board.h/.cpp             # 游戏主面板 / 关卡逻辑
│   ├── Plant.h/.cpp             # 植物实体
│   ├── Zombie.h/.cpp            # 僵尸实体
│   ├── Projectile.h/.cpp        # 投射物
│   ├── Coin.h/.cpp              # 硬币/阳光
│   ├── GridItem.h/.cpp          # 网格物品（墓碑、水花等）
│   ├── Challenge.h/.cpp         # 挑战模式
│   ├── CutScene.h/.cpp          # 过场动画
│   ├── LawnMower.h/.cpp         # 割草机
│   ├── SeedPacket.h/.cpp        # 种子包（冷却、阳光消耗）
│   ├── ZenGarden.h/.cpp         # 禅境花园
│   ├── GameObject.h/.cpp        # 游戏对象基类
│   ├── CursorObject.h/.cpp      # 光标对象
│   ├── MessageWidget.h/.cpp     # 消息组件
│   ├── ToolTipWidget.h/.cpp     # 工具提示
│   ├── LawnCommon.h/.cpp        # 通用工具函数
│   ├── System/                  # 系统级工具
│   │   ├── ReanimationLawn.h/.cpp # 动画系统（游戏侧封装）
│   └── Widget/                  # UI 组件
│       ├── SeedChooserScreen.h/.cpp # 选卡界面
│       ├── AlmanacDialog.h/.cpp     # 图鉴对话框
│       ├── LawnDialog.h/.cpp        # 通用对话框
│       ├── GameButton.h/.cpp        # 游戏按钮
│       ├── AwardScreen.h/.cpp       # 过关奖励界面
│       └── ...
│
├── SexyAppFramework/            # 引擎层
│   ├── SexyAppBase.h/.cpp       # 应用基类（窗口、事件循环）
│   ├── SexyApp.h/.cpp           # 应用派生类
│   ├── Common.h/.cpp            # 路径、文件系统工具
│   ├── graphics/                # 图形渲染（OpenGL）
│   ├── sound/                   # 音频（SDL/SDL_mixer）
│   ├── widget/                  # Widget 体系（UI 树、事件分发）
│   ├── paklib/                  # PAK 包文件支持
│   ├── platform/                # 平台相关代码
│   ├── misc/                    # 资源管理器 ResourceManager
│   ├── imagelib/                # 图片解码
│   └── glad/                    # OpenGL 加载器
│
└── Sexy.TodLib/                 # Tod 库（原版引擎工具库）
    ├── Reanimator.h/.cpp        # 骨骼动画系统
    ├── Attachment.h/.cpp        # 附着物系统
    ├── EffectSystem.h/.cpp      # 特效系统
    ├── FilterEffect.h/.cpp      # 滤镜效果
    ├── TodParticle.h/.cpp       # 粒子系统
    ├── TodFoley.h/.cpp          # 音效系统
    ├── TodStringFile.h/.cpp     # 字符串文件
    ├── Trail.h/.cpp             # 轨迹特效
    ├── DataArray.h              # 数据数组
    ├── Definition.h/.cpp        # 定义文件
    └── ...
```

### 1.2 核心数据流

```
main.cpp → LawnApp::Init() → 加载资源、初始化引擎
       → LawnApp::Start() → 进入标题画面
       → LawnApp::StartPlaying() → 创建 Board 开始关卡
       → Board::Update() → 每帧更新植物、僵尸、投射物、硬币等
       → Board::Draw() → 渲染所有游戏对象
       → 过关/失败 → LawnApp 状态切换
```

### 1.3 设计特点

- **SexyAppFramework 风格**：公开成员变量、无访问器方法、最小化注释
- **命名约定**：`m` 前缀成员变量，`the` 前缀参数，`a` 前缀局部变量
- **存档格式**：`v4` TLV（Type-Length-Value）格式，非原始内存转储
- **跨平台**：通过 SDL2 抽象平台差异，CMake 管理构建

---

## 二、Mod 框架架构

Mod 框架在 **4ee3701**（基础结构）~ **fc85b1b**（新植物框架）共 7 个提交中逐步构建完成。

### 2.1 Mod 框架组成部分

```
src/Mod/                         # Mod 框架核心（6 个模块）
├── ModLoader.h/.cpp             # Mod 加载器
├── ModLua.h/.cpp                # Lua 虚拟机与 API 绑定
├── ModRegistry.h/.cpp           # 数据注册表（植物、僵尸、模式、投射物）
├── ModSave.h/.cpp               # Mod 独立存档
├── LuaProxyDialog.h/.cpp        # Lua 创建的对话框代理
└── ModRegistry_patch.cpp        # 注册表集成补丁（引擎接入点）
```

### 2.2 各模块功能

#### (1) ModLoader — Mod 加载器

- 扫描 `mods/<mod_id>/mod.json` 清单文件
- 解析 `ModManifest`（id, name, version, priority, dependencies, entry, dataFiles）
- **依赖排序**：按 dependencies 拓扑排序 + priority 稳定排序
- **字符串覆盖**：加载所有 Mod 的 `strings.json` 和 `properties/default.xml`
- **数据文件加载**：`LoadPlantDefs()` / `LoadProjectileDefs()` 加载 JSON 定义

#### (2) ModLua — Lua 虚拟机与 API

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
| | `dialog:Close()` | 关闭对话框 |
| | `dialog:SetTitle(text)` | 设置标题 |
| | `dialog:SetBody(text)` | 设置正文 |

- **事件钩子**：11 个生命周期回调

| 钩子 | 触发时机 |
|------|---------|
| `OnModInit()` | Mod 脚本首次加载 |
| `OnGameStart()` | 游戏启动，进入标题画面 |
| `OnLevelStart(mode)` | 关卡开始 |
| `OnWaveStart(wave)` | 波次推进 |
| `OnPlantSpawn(plant)` | 植物放置 |
| `OnZombieSpawn(zombie)` | 僵尸生成 |
| `OnPlantAttack(plant, target)` | 植物攻击 |
| `OnZombieDie(zombie)` | 僵尸死亡 |
| `OnLevelEnd(isWin)` | 关卡结束 |
| `OnCoinSpawn(coin)` | 硬币/阳光生成 |
| `OnBoardButtonClick(btnId)` | 自定义按钮点击 |

#### (3) ModRegistry — 数据注册表

- **运行时 ID 映射**：Mod 植物 `seedType >= 2000`，僵尸 `zombieType >= 3000`，投射物 `projectileType >= 4000`
- **注册类型**：
  - `ModPlantDef` — id, seedType, seedCost, refreshTime, subClass, launchRate, projectileType, plantName, reanimationName
  - `ModZombieDef` — id, zombieType
  - `ModModeDef` — id, baseMode
  - `ModProjectileDef` — id, damage, speed, imageName, projectileType
- **反向查询**：`FindByRuntimeId()` 支持运行时 ID → Mod 定义的反查
- **动态 Reanim**：`RegisterDynamicReanim()` 支持外部 reanim XML 注册
- **图鉴集成**：`GetTotalAlmanacPlants()` / `GetAlmanacPlantAt()` 与图鉴联动

#### (4) ModSave — Mod 独立存档

- 存储路径：`<appdata>/modsave/<mod_id>.json`
- API：`SaveValue(modId, key, value)` / `LoadValue(modId, key, &outValue)`
- 与游戏存档 `userdata/` 完全隔离

#### (5) LuaProxyDialog — Lua 对话框代理

- 继承 `LawnDialog`，支持 Lua 侧创建模态/非模态对话框
- 使用 `luaL_ref` 管理 Lua 回调引用，避免 GC 问题
- 支持 `AddLuaButton()` 动态添加按钮
- 对话框 ID 从 `1000` 开始分配

#### (6) ModRegistry_patch — 引擎适配补丁

- 将注册表查询注入到引擎关键路径：
  - **AlmanacDialog**：显示 Mod 植物的图鉴条目
  - **SeedChooserScreen**：Mod 植物显示在选卡界面、支持分页
  - **Board**：Mod 植物的放置与行为
  - **ReanimationType** 解析：将名称字符串映射到运行时 ID

### 2.3 Mod 包格式

```
mods/<mod_id>/
├── mod.json                           # 清单文件（id, name, version, priority, entry, data）
├── scripts/
│   └── main.lua                       # Lua 入口脚本
├── data/
│   ├── plants.json                    # 植物定义
│   ├── zombies.json                   # 僵尸定义
│   ├── modes.json                     # 模式定义
│   ├── projectiles.json               # 投射物定义
│   └── strings.json                   # 字符串覆盖
├── resources/
│   ├── images/                        # 图片资源
│   ├── sounds/                        # 音频资源
│   ├── reanim/                        # 自定义动画 XML
│   └── properties/
│       └── default.xml                # 属性文件覆盖
```

### 2.4 修改了原项目的哪些部分

| 文件 | 修改内容 |
|------|---------|
| `CMakeLists.txt` | 添加 Mod 源文件、Lua 链接选项 `PVZ_ENABLE_LUA` |
| `src/LawnApp.h/.cpp` | Mod 初始化接入（`InitModSystem()`）、游戏启动/停止钩子、存档隔离 |
| `src/Lawn/Board.h/.cpp` | 事件钩子插入（关卡开始/结束、僵尸/植物生成、硬币生成）、自定义按钮支持（`mLuaButtons` 容器） |
| `src/Lawn/Plant.h/.cpp` | 注册表查询替代硬编码属性、植物生成/攻击钩子 |
| `src/Lawn/Zombie.cpp` | 注册表查询僵尸属性、僵尸生成/死亡钩子 |
| `src/Lawn/Projectile.cpp` | 注册表查询投射物属性（伤害、速度、图片） |
| `src/Lawn/SeedPacket.cpp` | Mod 植物冷却/阳光显示适配 |
| `src/Lawn/Widget/SeedChooserScreen.h/.cpp` | Mod 植物显示、分页支持（超过 8 页的滚动分页） |
| `src/Lawn/Widget/AlmanacDialog.h/.cpp` | 图鉴 Mod 植物条目渲染 |
| `src/Lawn/Challenge.cpp` | 移除硬编码模式数量检查 |
| `src/Lawn/CutScene.cpp` | Mod 启动适配 |
| `src/Lawn/CursorObject.cpp` | Mod 光标支持 |
| `src/Lawn/System/ReanimationLawn.h/.cpp` | Mod reanim 注册 |
| `src/SexyAppFramework/Common.h/.cpp` | 多根目录资源路径、Mod 存档路径 `modsave/` |
| `src/SexyAppFramework/paklib/PakInterface.cpp` | PAK 覆盖支持 |
| `src/Sexy.TodLib/Reanimator.h/.cpp` | 动态 reanim 加载（外部 XML 跟踪元素解析） |
| `src/ConstEnums.h` | 新增 Mod 类型范围注释/预留枚举值 |

### 2.5 开发阶段（参见 TODO_ModSystem.md）

| 阶段 | 内容 | 状态 |
|------|------|------|
| **1** | Mod 加载器（扫描、清单解析、依赖排序） | ✅ 完成 |
| **2** | 资源覆盖与字符串本地化 | ✅ 完成 |
| **3** | 数据注册表（植物、僵尸、模式、投射物） | ✅ 完成 |
| **4** | Lua VM 与 API 绑定（Game, Board, Entity, UI, Dialog） | ✅ 完成 |
| **5** | 事件钩子（11 个生命周期回调） | ✅ 完成 |
| **6** | Mod 存档与数据隔离 | ✅ 完成 |
| **7** | 实体扩展与深度 UI 适配 | ⏳ 待完成 |
| **8** | 自定义 UI 对话框（AddLabel 待完成） | ⏳ 待完成 |

### 2.6 现有 Mod 示例

| Mod | 功能 |
|-----|------|
| `example_mod` | 验证管线（字符串覆盖、日志、僵尸生成/死亡钩子） |
| `auto_collect_sun` | 自动收集阳光（`OnCoinSpawn` + `coin:Collect()`） |
| `demo_plant` | 自定义植物（fire_pea, ice_melon, custom_shooter + 外部 reanim） |
| `speed_control` | 速度控制按钮（`Board.AddButton` + `Game.SetSpeed` + 存档持久化） |
| `ui_test_mod` | UI 测试（`UI.CreateDialog` / `UI.ShowMessage` / `Dialog:AddButton`） |
| `super_pea` | 超级豌豆（增强型投射物） |

---

## 三、架构关系图

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
│                          ModRegistry_patch │        │
│                            │               │        │
│                            ▼               │        │
│                     mods/<mod_id>/         │        │
│                     ├─ mod.json            │        │
│                     ├─ scripts/main.lua    │        │
│                     ├─ data/*.json         │        │
│                     └─ resources/          │        │
└─────────────────────────────────────────────────────┘
```

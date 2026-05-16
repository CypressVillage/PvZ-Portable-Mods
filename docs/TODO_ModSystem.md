# PvZ-Portable Mod 体系开发计划与 TODO 列表

根据 `docs/modding.zh-CN.md` 规范，将 PvZ-Portable Mod 系统的开发划分为以下几个阶段。该计划旨在逐步实现跨平台的 Lua Mod 体系，确保数据与行为分离，并保持原版代码和存档的兼容性。

## 阶段一：Mod 加载器与基础框架 (Mod Loader)
**目标**：实现 Mod 的发现、清单解析、依赖分析和启动接入。

- [x] **定义清单结构**：创建 `ModManifest` 数据结构，包含 `id`, `version`, `priority`, `dependencies`, `entry`, `data` 等字段。
- [x] **Mod 扫描**：在 `mods/` 目录下遍历子文件夹，寻找并读取 `mod.json` 文件。
- [x] **清单解析与校验**：解析 JSON 格式，校验必填字段，检查 `id` 是否重复，版本号 (`schema_version`) 是否兼容。
- [x] **依赖排序**：根据 `dependencies` 进行拓扑排序，并结合 `priority`（优先级数值越大越后加载）进行稳定排序。
- [x] **错误与日志**：实现对缺失清单、依赖错误等情况的异常捕获与日志输出，并在错误时安全跳过该 Mod。
- [x] **引擎启动接入**：修改 `src/main.cpp` 或 `src/SexyAppFramework/SexyAppBase.cpp`，在 `SexyAppBase::Init()` 之后、`LawnApp::Init()` 之前完成上述流程。

## 阶段二：资源覆盖与字符串本地化 (Resource & Strings)
**目标**：支持 Mod 替换或增加图片、音效资源以及文本字符串，且回退机制正常。

- [x] **多根目录资源系统**：修改 `src/SexyAppFramework/misc/ResourceManager.cpp`（及相关 `Common.cpp` 逻辑），维护一个资源搜索路径列表 (`mResourceRoots`)。
- [x] **注册资源目录**：在 `LawnApp::Init()` 资源加载前，将每个 Mod 的 `resources/` 目录按优先级顺序注册到资源系统的搜索列表中。
- [x] **修改解析逻辑**：使 `GetResourcePath` 或 `ParseCommonResource` 能够按照 `Mod 资源 -> 原版资源` 的优先级顺序查找文件。
- [x] **字符串覆盖加载**：在 `src/LawnApp.cpp` (加载线程 `LoadingThreadProc`) 调用 `TodStringListLoad` 和 `LoadProperties` 的流程中，追加加载所有 Mod 的 `strings.json` 和 `resources/properties/default.xml`。

## 阶段三：数据表注册与运行时映射 (Data Registry)
**目标**：通过 JSON 数据表动态添加植物、僵尸、模式等，避免硬编码。

- [x] **数据表解析器**：实现针对 `plants.json`, `zombies.json`, `projectiles.json`, `modes.json` 的解析模块。
- [x] **动态注册表 (Registry)**：新增 `PlantDefRegistry`, `ZombieDefRegistry`, `ModeDefRegistry` 等数据结构，用 `id` (字符串) 作为主键进行存储。
- [x] **运行时 ID 映射**：为 Mod 新增的内容分配运行时的枚举类型/整形 ID（例如：植物 `seed_type` >= 2000, 僵尸 `zombie_type` >= 3000），建立 `mod_id <-> runtime_id` 的反查映射。
- [x] **引擎适配器改造**：重构引擎内部直接使用数组索引或硬编码枚举的代码，改为通过查询注册表获取实体属性（生命值、伤害、资源名等）。

## 阶段四：Lua 脚本虚拟机与 API 绑定 (Lua VM & API)
**目标**：为 Mod 提供行为逻辑的扩展能力，建立安全的沙盒边界。

- [x] **集成 Lua 环境**：引入 Lua 5.4 及绑定库（推荐 `sol2` 或纯 C API 构建），完成环境初始化。
- [x] **全局对象绑定 (Game)**：
  - [x] `Game.Log(text)`
  - [x] `Game.RegisterPlant(def)`, `Game.RegisterZombie(def)`, `Game.RegisterMode(def)`, `Game.RegisterProjectile(def)` (结合阶段三的 Registry)
  - [x] `Game.GetMode()`
- [x] **实体与场景绑定 (Board & Entity)**：
  - [x] `Board.SpawnZombie(zombie_id, row)`, `Board.SpawnPlant(plant_id, row, col)`, `Board.GetWave()`
  - [x] `Entity` 属性只读访问 (`id`, `type`, `hp`) 及方法暴露 (`Entity:Damage(amount)`, `Entity:IsSun()`, `Entity:Collect()`)。
- [x] **脚本执行入口**：在 Mod 数据表加载完成后，执行每个 Mod 的 `entry` 脚本，并调用 `OnModInit()` 回调。

## 阶段五：游戏生命周期与事件触发钩子 (Event Hooks)
**目标**：在引擎关键路径抛出事件给 Lua 侧接管。捕获 Lua 异常以防崩溃。

- [x] **OnGameStart**：在 `LawnApp::Start()` 完成，进入标题画面时触发。
- [x] **OnLevelStart(mode_id)**：在 `LawnApp::PreNewNewGame()` 或 `LawnApp::StartPlaying()` 关卡初始化时触发。
- [x] **OnWaveStart(wave_index)**：在 `src/Lawn/Board.cpp` 波次推进逻辑中触发。
- [x] **OnPlantSpawn(plant)**：在 `src/Lawn/Board.cpp` `Board::AddPlant()` 植物成功放置时触发，通过 `PushEntity` 传递 `Plant*` 实体对象。
- [x] **OnZombieSpawn(zombie)**：在 `src/Lawn/Board.cpp` `Board::AddZombieInRow()` 僵尸实例化成功时触发，通过 `PushEntity` 传递 `Zombie*` 实体对象。
- [x] **OnPlantAttack(plant, target)**：在 `src/Lawn/Plant.cpp` `Plant::Fire()` 植物发射投射物时触发，传递攻击者 `Plant*` 与目标 `Zombie*` 实体。
- [x] **OnZombieDie(zombie)**：在 `src/Lawn/Zombie.cpp` `Zombie::DieNoLoot()` 僵尸死亡时触发，通过 `PushEntity` 传递 `Zombie*` 实体对象。
- [x] **OnLevelEnd(result)**：在 `src/LawnApp.cpp` `LawnApp::KillBoard()` 关卡结算时触发，传递 `boolean isWin`（`BOARDRESULT_WON` / `BOARDRESULT_LOST`）。
- [x] **OnCoinSpawn(coin)**：在 `src/Lawn/Board.cpp` `Board::AddCoin()` 掉落物成功实例化时触发，通过 `PushEntity` 传递 `Coin*` 实体对象。

## 阶段六：Mod 独立存档与热重载 (Save & Dev Mode)
**目标**：保证玩家数据的隔离，同时提供便利的开发者工具。

- [x] **Mod 专属存档路径**：在 `SexyAppFramework/Common.cpp` 的 `GetAppDataPath()` 逻辑旁，新增 `modsave/` 路径作为 Mod 数据存储区，与 `userdata/` 完全独立。
- [x] **存档 API 实现**：实现 `Game.SaveModData(key, value)` 将数据持久化写入 `<save_root>/modsave/<mod_id>.json`，以及对应的 `Game.LoadModData(key)`。
- [ ] **开发者热重载模式**：
  - [ ] 支持命令行参数 `-moddev` 启动。
  - [ ] 实现数据表、Lua 脚本、资源索引的重新加载热键或控制台命令。
  - [ ] 添加状态校验（限制在主菜单或非战斗状态进行重载）。

## 阶段七：实体定义与深度适配 (Entity Extension & UI)
**目标**：彻底打通从数据到表现层的链路，使完整定义并表现一个全新植物/僵尸成为可能。

- [ ] **扩展数据表字段映射**：完善 `ModPlantDef` 等结构，解析阳光花费、冷却时间、发射间隔等数值，并对齐映射至引擎内部 Definition 对象。
- [ ] **动态动画与资源注册**：为 Mod 添加的新实体动态分配注册 `ReanimationType` 及贴图指针，确保资源句柄正确挂载以解决隐形或渲染崩溃问题。
- [ ] **UI 界面解绑硬编码**：重构选卡界面 (`SeedChooserScreen`) 与图鉴 (`AlmanacDialog`)，移除对 `NUM_SEED_TYPES` 的遍历限制，支持动态植物数量的翻页或滚动。
- [ ] **深度行为逻辑钩子**：在源码的帧更新、索敌、攻击等环节增加事件抛出（如 `OnPlantUpdate`），使得 Lua 能够彻底接管新植物特有逻辑，避开原版的 `switch (mSeedType)` 硬编码。

## 阶段八：Mod 自定义 UI 对话框系统 (Mod UI & Dialogs)
**目标**：为 Mod 提供 Lua 侧创建自定义对话框、按钮、文本标签等 UI 元素的能力，使 Mod 可以在游戏内展示自定义界面并接收用户交互。

- [x] **LuaProxyDialog 代理对话框类**：新建 `src/Mod/LuaProxyDialog.h/.cpp`，继承 `LawnDialog`，作为 Lua 创建 UI 的 C++ 宿主。
  - [x] 构造函数接收标题、正文、模态标记等参数，使用动态 Dialog ID（`DIALOG_MOD_BASE = 1000` 起自增）。
  - [x] 内部维护 `std::vector<ButtonEntry>` 列表，记录每个按钮的 ID 与 `luaL_ref` 回调引用。
  - [x] 重写 `ButtonDepress(int theId)`：根据 `theId` 查找对应的 `luaL_ref`，通过 `lua_rawgeti` 取出 Lua 闭包并 `lua_pcall` 调用，调用后自动 `KillDialog` 关闭对话框。
  - [x] 重写 `AddedToManager / RemovedFromManager`：注册/注销动态添加的子控件（按钮），支持对话框已显示后动态添加按钮（`mInManager` 标记）。
  - [x] 重写 `Resize`：自动布局所有动态子控件（垂直排列按钮、居中等）。
  - [x] 析构时调用 `ReleaseRefs()` 释放所有 Lua 函数引用。
  - [x] 在 `RemovedFromManager` 中标记 `mDestroyed = true`，防止 Lua 侧悬空引用。
- [x] **动态 Dialog ID 分配**：在 `LuaProxyDialog` 中使用静态自增计数器 `sNextDialogId`（起始 `DIALOG_MOD_BASE = 1000`），替代固定 `Dialogs` 枚举值。确保 `SexyAppBase::AddDialog/KillDialog` 的 `mDialogMap` 查找逻辑兼容 int 键。
- [x] **Lua `UI` 全局表注册**：在 `ModLua.cpp` 的 `Lua_RegisterGameTable()` 中新增 `UI` 表：
  - [x] `UI.CreateDialog(opts)` — 创建并显示对话框，`opts` 为 table（`title`, `body`, `modal`），返回 Dialog userdata。
  - [x] `UI.ShowMessage(title, body)` — 快捷创建单按钮模态提示框。
- [x] **Lua Dialog userdata 绑定**：
  - [x] 定义 `LuaDialogUD` 结构（持有 `LuaProxyDialog*` 指针与 `luaL_ref` 自引用防止 GC）。
  - [x] 注册 `"Game.Dialog"` metatable，`__index` 分发方法调用。
  - [x] `dialog:AddButton(text, onClick)` — 向对话框添加按钮，`onClick` 为 Lua 函数闭包，通过 `luaL_ref` 存储。
  - [ ] `dialog:AddLabel(text)` — 向对话框添加静态文本标签。
  - [x] `dialog:Close()` — 关闭并销毁对话框。
  - [x] `dialog:SetTitle(text)` / `dialog:SetBody(text)` — 动态修改标题/正文。
  - [x] 在 dialog 销毁时（`RemovedFromManager`）将 userdata 中通过 `mDestroyed` 标记防止悬空引用，后续操作安全返回 `nil`。
- [x] **Lua 闭包回调存储**：在 `LuaProxyDialog` 中使用 `luaL_ref(L, LUA_REGISTRYINDEX)` 将 Lua 函数闭包存入注册表，按钮点击时通过 `lua_rawgeti` 取出并调用。对话框析构时统一 `luaL_unref` 释放。
- [x] **集成测试**：编写 `mods/ui_test_mod` 示例 Mod 脚本验证完整链路（创建对话框 → 添加按钮 → 点击回调 → 关闭对话框）。

---

*注：本 TODO 计划旨在实现最小可用版本（MVP），后续可在此基础上迭代"自定义 UI 面板"、"事件过滤器"和"Mod 打包工具"等高级特性。*

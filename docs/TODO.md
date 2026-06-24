# Mod Framework — 待办清单

> 源码核对日期：2026-06-24。当前框架可编译并已实现 Lua 注册、资源覆盖、事件回调、ModRegistry、ModSave、PlantHelper 与 UI Dialog；本文仍保留历史任务记录，完成项不等于 API 稳定承诺。
> 
> **近期目标阶段**：P7+P8 — Game/Board API 扩展 + 次要回调（2026-05-19 达成）
>
> 当前已知限制：JSON 数据表加载、热重载、依赖版本约束、缺失依赖失败处理、重复 manifest id 检测、可靠 priority 冲突排序、每个 Mod 独立 Lua 环境尚未实现。

---

## 🔴 高优先级（基础设施修复 + 常量表 + 自定义植物核心 API）

### 基础设施修复

- [x] **实现 `Game.GetMode()` Lua API 绑定**
  - 位置：`src/Mod/ModLua.cpp:725-730`
  - 新加 `Lua_GameGetMode`，注册为 `Game.GetMode()`，返回 `gLawnApp->mGameMode`

- [x] **扩展 `ModZombieDef` 添加实体属性**
  - 位置：`src/Mod/ModRegistry.h:32-36`，`src/Mod/ModLua.cpp:241-320`，`src/Lawn/Zombie.cpp:100-118`
  - 新增 bodyHealth/headHealth/speed/damage/reanimationName/helm/shield/hasHead/hasArm/zombieName
  - `GetZombieDefinition()` 现从 ModZombieDef 读取 Reanim 类型

- [x] **添加依赖拓扑排序（仍有限制）**
  - 位置：`src/Mod/ModLoader.cpp:401-442`
  - 实现 Kahn 算法，对已存在 dependencies 做拓扑排序
  - 检测循环依赖的 mod 会追加末尾并记录错误
  - 限制：缺失依赖不报错；重复 manifest id 不严格拒绝；`priority` 当前不能可靠作为覆盖/冲突排序依据

### 常量表暴露 — 第一阶段（无 C++ 改动，纯 Lua 注册）

- [x] **Lua 侧注册 `PlantType` 常量表**
  - 位置：`src/Mod/ModLua.cpp:820-872`
  - 作为 Lua 只读全局表注册（`PEASHOOTER=0, SUNFLOWER=1, ..., IMITATER=48, NONE=-1`）

- [x] **Lua 侧注册 `ZombieType` 常量表**
  - 位置：`src/Mod/ModLua.cpp:874-910`
  - `NORMAL=0, FLAG=1, ..., REDEYE_GARGANTUAR=35`

- [x] **Lua 侧注册 `ProjectileType` 常量表**
  - 位置：`src/Mod/ModLua.cpp:912-929`
  - `PEA=0, SNOWPEA=1, ..., ZOMBIE_PEA=13`

- [x] **Lua 侧注册 `CoinType` 常量表**
  - 位置：`src/Mod/ModLua.cpp:931-962`
  - `NONE=0, ..., PRESENT_SURVIVAL_MODE=27`

- [x] **Lua 侧注册 `GameMode` 常量表**
  - 位置：`src/Mod/ModLua.cpp:964-1026`
  - `ADVENTURE=0, SURVIVAL_NORMAL_STAGE_1=1, ..., INTRO=69`

- [x] **Lua 侧注册 `PlantSubClass` / `PlantState` / `ZombiePhase` / `ProjectileMotion` 常量表**
  - 位置：`src/Mod/ModLua.cpp:1028-1157`

- [x] **Lua 侧注册 `DamageFlags` / `BackgroundType` / `HelmType` / `ShieldType` 常量表**
  - 位置：`src/Mod/ModLua.cpp:1159-1210`

- [x] **Lua 侧注册 `GridConstants` 游戏常量表**
  - 位置：`src/Mod/ModLua.cpp:1222-1232`
  - COLS=9, ROWS=6, BOARD_WIDTH=800, BOARD_HEIGHT=600, LAWN_XMIN=40, LAWN_YMIN=80, BOARD_OFFSET=220, SEEDBANK_MAX=10

- [x] **Lua 侧注册 `ReanimLoopType` 常量表**
  - 位置：`src/Mod/ModLua.cpp:1212-1220`
  - `LOOP=0, LOOP_FULL_LAST_FRAME=1, PLAY_ONCE=2, PLAY_ONCE_AND_HOLD=3, ...`

### 自定义植物行为 API（核心三大块）

#### A. 动画控制 API — 暴露 Reanimation 系统到 Lua

- [x] **`plant:PlayBodyReanim(trackName, loopType, blendTime, animRate)`**
  - 映射 C++ `Plant::PlayBodyReanim(theTrackName, theLoopType, theBlendTime, theAnimRate)`
  - 位置：`src/Mod/ModLua.cpp` — `Lua_EntityPlayBodyReanim`
  - `plant:GetBodyReanim()` 返回可链式调用的 Reanim 对象

- [x] **`plant:PlayIdleAnim(animRate)`**
  - 映射 C++ `Plant::PlayIdleAnim(theRate)` → 播放 `"anim_idle"` 轨道
  - 位置：`src/Mod/ModLua.cpp` — `Lua_EntityPlayIdleAnim`

- [x] **`plant:GetBodyReanimProgress()`** → float
  - 返回 `mAnimTime`（0.0~1.0 归一化进度）
  - 位置：`src/Mod/ModLua.cpp` — `Lua_EntityGetBodyReanimProgress`

- [x] **`plant:SetBodyReanimRate(rate)`**
  - 设置 `mAnimRate`，控制动画播放速度
  - 位置：`src/Mod/ModLua.cpp` — `Lua_EntitySetBodyReanimRate`

- [x] **`plant:GetBodyReanimRate()`** → float
  - 位置：`src/Mod/ModLua.cpp` — `Lua_EntityGetBodyReanimRate`

- [x] **`plant:IsAnimPlaying(trackName)`** → bool
  - 映射 C++ `Reanimation::IsAnimPlaying(theTrackName)`
  - 位置：`src/Mod/ModLua.cpp` — `Lua_EntityIsAnimPlaying`

- [x] **`plant:TrackExists(trackName)`** → bool
  - 映射 C++ `Reanimation::TrackExists(theTrackName)`
  - 位置：`src/Mod/ModLua.cpp` — `Lua_EntityTrackExists`

- [x] **`plant:GetBodyReanimLoopType()`** → int
  - 返回当前 `mLoopType`
  - 位置：`src/Mod/ModLua.cpp` — `Lua_EntityGetBodyReanimLoopType`

- [x] **`plant:GetBodyReanimLoopCount()`** → int
  - 返回已循环次数 `mLoopCount`
  - 位置：`src/Mod/ModLua.cpp` — `Lua_EntityGetBodyReanimLoopCount`

#### D. Reanim 对象独立 API — 新增 Reanimation Lua 用户数据类型

- [x] **`plant:GetBodyReanim()`** → reanim_userdata
  - 返回植物主体动画的 Reanim 对象，可链式调用以下方法

- [x] **`reanim:Play(trackName, loopType[, blendTime=0, animRate=0])`**
  - 映射 C++ `Reanimation::PlayReanim(theTrackName, theLoopType, theBlendTime, theAnimRate)`

- [x] **`reanim:GetRate()` / `reanim:SetRate(rate)`** → float
  - 读写 `mAnimRate`

- [x] **`reanim:GetProgress()` / `reanim:SetProgress(time)`** → float
  - 读写 `mAnimTime`（0.0~1.0）

- [x] **`reanim:IsPlaying(trackName)`** → bool
  - 映射 `Reanimation::IsAnimPlaying`

- [x] **`reanim:TrackExists(trackName)`** → bool

- [x] **`reanim:GetLoopType()` / `reanim:GetLoopCount()`** → int
  - 读取 `mLoopType` / `mLoopCount`

- [x] **`reanim:SetPosition(x, y)`**
  - 映射 `Reanimation::SetPosition`

- [x] **`reanim:OverrideScale(sx[, sy])`**
  - 映射 `Reanimation::OverrideScale`，单参数时等比缩放

- [x] **`reanim:ShowOnlyTrack(trackName)`**
  - 映射 `Reanimation::ShowOnlyTrack`

- 位置：`src/Mod/ModLua.cpp` — `Game.Reanim` 元表 + `Lua_Reanim*` 系列函数

#### B. 目标查找 API — 暴露实体查询到 Lua ✅

- [x] **`Board:FindTargetZombie(plant)`** → zombie_entity | nil
  - 映射 C++ `Plant::FindTargetZombie(weaponType)`，为指定植物查找最近目标
  - 实现：`src/Mod/ModLua.cpp` — `Lua_BoardFindTargetZombie`

- [x] **`Board:GetZombiesInRow(row)`** → table of zombie_entity
  - 遍历 `mZombies` 按行筛选，返回 Lua 数组

- [x] **`Board:GetPlantsInRow(row)`** → table of plant_entity

- [x] **`Board:GetZombieAt(col, row)`** → zombie_entity | nil
  - 用 `PixelToGridX` 计算僵尸所在格子，取最接近格心的那个

- [x] **`Board:GetPlantAt(col, row)`** → plant_entity | nil
  - 委托 `GetTopPlantAt(col, row, TOPPLANT_BUNGEE_ORDER)`

- [x] **`Board:GetAllZombies()`** → table of zombie_entity

- [x] **`Board:GetAllPlants()`** → table of plant_entity

- [x] **`entity:DistanceTo(otherEntity)`** → float
  - 像素距离计算（支持 Plant/Zombie/Coin 实体）

#### C. 投射物生成 API — 暴露发射系统到 Lua ✅

- [x] **`Board:AddProjectile(x, y, row, projectileType)`** → proj_entity
  - 映射 C++ `Board::AddProjectile(theX, theY, theRenderOrder, theRow, theProjectileType)`
  - 返回 Projectile 实体供后续操作
  - 位置：`src/Mod/ModLua.cpp` — `Lua_BoardAddProjectile`

- [x] **`proj:SetDamage(amount)`**
  - 设置伤害值（写入 `mDamageOverride`，优先级高于定义伤害）

- [x] **`proj:SetVelocity(vx, vy, vz)`**
  - 设置速度分量

- [x] **`proj:SetDamageFlags(flags)`**
  - 设置伤害标记（冻结、穿透等），参考 `DamageFlags` 常量

- [x] **`proj:SetMotionType(motionType)`**
  - 设置弹道类型（直线、抛物线、跟踪等）

- [x] **`proj:SetTargetZombie(zombie)`**
  - 设置跟踪目标（用于跟踪弹）

### 现有问题修复（移至此处以便集中处理）

- [x] **修复保存格式为 JSON**
  - 位置：`src/Mod/ModJson.h/.cpp`, `src/Mod/ModSave.cpp`
  - 提取 JSON 解析/序列化到 `ModJson` 共享模块，`ModLoader` 和 `ModSave` 统一调用

- [x] **实现实体 `id` 字符串查找**
  - 位置：`src/Mod/ModLua.cpp:386-413`
  - `entity.id` 通过 `ModRegistry` 查询自定义实体注册 ID（plant≥2000, zombie≥3000, projectile≥4000）

- [x] **`LuaProxyDialog::ButtonDepress` 支持不关闭对话框**
  - 位置：`src/Mod/LuaProxyDialog.cpp:99-134`
  - Lua 回调返回 `false` 阻止对话框关闭

- [x] **实现自定义模式挑战 UI 集成**
  - 位置：`src/Mod/ModRegistry.h/.cpp`, `src/Lawn/Widget/ChallengeScreen.h/.cpp`
  - `ModModeDef` 新增挑战 UI 字段，`ChallengeScreen` 动态读取并创建按钮

---

## 🟡 中优先级（实体属性扩展 + 关键回调 + 辅助模块）

### 实体通用属性与方法

- [x] **扩展 Plant 实体属性**
  - `row`, `col`, `x`, `y`, `maxHp`, `state`, `subClass`, `isAsleep`, `isDead`, `launchCounter`, `launchRate`, `age`, `imitaterType`, `recentlyEaten`, `squished`
  - 位置：`src/Mod/ModLua.cpp` — Lua_EntityIndex 元表

- [x] **扩展 Plant 实体方法**
  - `:Die()`, `:Squish()`, `:SetSleeping(bool)`, `:GetCost()`, `:GetName()`
  - `:IsNocturnal()`, `:IsFungus()`, `:IsAquatic()`, `:IsUpgrade()`, `:IsFlying()`

- [x] **扩展 Zombie 实体属性**
  - `row`, `x`, `y`, `velX`, `maxHp`, `phase`, `isEating`, `isDead`, `age`
  - `chilled`, `buttered`, `mindControlled`, `helmType`, `helmHp`, `shieldType`, `shieldHp`
  - `hasHead`, `hasArm`, `inPool`, `onHighGround`, `altitude`

- [x] **扩展 Zombie 实体方法**
  - `:SetRow(n)`, `:ApplyChill(isIce)`, `:ApplyButter()`, `:RemoveButter()`
  - `:StartMindControlled()`, `:DieNoLoot()`, `:DieWithLoot()`
  - `:TakeHelmDamage(n)`, `:TakeShieldDamage(n)`
  - `:IsFlying()`, `:IsOnHighGround()`, `:IsImmobilized()`

- [x] **扩展 Coin 实体属性**
  - `x`, `y`, `velX`, `velY`, `age`, `value`, `isBeingCollected`, `coinMotion`, `isMoney`, `scale`

- [x] **扩展 Coin 实体方法**
  - `:GetValue()`, `:Die()`, `:StartFade()`

- [x] **新增 Projectile 实体类型（全新包装）**
  - 属性：`type`, `x`, `y`, `z`, `velX`, `velY`, `row`, `motionType`, `age`, `isDead`, `damage`, `rotation`
  - 方法：`:SetDamage()`, `:SetVelocity()`, `:SetDamageFlags()`, `:SetMotionType()`, `:SetTargetZombie()`
  - 位置：`src/Mod/ModLua.cpp` — `ENTITY_TYPE_PROJECTILE=3` + `PushEntity` 重载

### 关键新生命周期回调

- [x] **`OnPlantDie(plant)`** — 补丁位置：`Plant::Die()`
- [x] **`OnZombieAttack(zombie, plant)`** — 补丁位置：`Zombie::EatPlant()`
- [x] **`OnProjectileSpawn(proj)`** — 补丁位置：`Projectile::ProjectileInitialize()`
- [x] **`OnProjectileHit(proj, zombie)`** — 补丁位置：`Projectile::DoImpact()`
- [x] **`OnProjectileMiss(proj)`** — 补丁位置：`Projectile::Update()` 飞出检测
- [x] **`OnCoinCollect(coin)`** — 补丁位置：`Coin::Collect()`
- [x] **`OnZombieReachHouse(zombie)`** — 补丁位置：`Zombie::WalkIntoHouse()`

### Lua 辅助模块体系 — Phase 1：基础设施 + plant_helper（当前阶段目标）

**架构决策**：所有辅助模块以独立 `.lua` 文件存放在 `mods/_internal/` 目录下，在 `Init()` 中通过 `Sexy::GetResourcePath()` 读取文件后 `luaL_loadbuffer` 执行，注册为全局表。Mod 作者可直接阅读源码学习，修改无需重新编译。

**C++ 侧改动**（`src/Mod/ModLua.cpp`）：
- 新增 `Lua_LoadHelperModules(lua_State* L)` 函数
- 使用 `Sexy::GetResourcePath("mods/_internal/...lua")` 定位文件
- 通过已有的 `ReadFileText()` 读取 + `luaL_loadbuffer` + `lua_pcall` 执行
- 在 `Init()` 末尾调用 `Lua_LoadHelperModules(L)`
- `ModLua.h` 无需新增公开方法（仅在 `Init()` 内部调用）

**Lua 侧策略**：
- 使用弱表（`__mode = "k"`）管理 plant→state 映射，植物销毁后自动回收
- 每个模块提供 `Update(plant)` 供 mod 在 `OnPlantUpdate` 中调用
- 所有模块通过 `_helpers` 内部表共享计时器基础设施

#### 步骤 1：基础计时器设施（`_timer` 内部模块）

- [x] **实现 C++ 全局计时器池（取代原 Lua 实现）**
  - 路径：`src/Mod/ModTimer.h`、`src/Mod/ModTimer.cpp`
  - 功能：全局统一的帧计数器管理，供所有 helper 模块共用
  - 实现方式：C++ `ModTimer` 类管理定时器池，通过 Lua C 绑定注册为 `_timer` 全局表
  - API 签名（与旧 Lua 实现完全一致，对 Lua 侧透明）：
    ```lua
    -- _timer 模块（内部使用，不暴露给 mod 作者）
    _timer.New(parent, frames, callback) → timer_id  -- 创建计时器
    _timer.Cancel(id)                                  -- 取消计时器
    _timer.TickAll(parent)                             -- 每帧推进某父对象的所有计时器
    _timer.CancelAll(parent)                           -- 取消某父对象的所有计时器
    ```
  - 实现：`_timer.TickAll()` 由各 helper 的 `Update()` 隐式调用
  - 回调管理：Lua 函数通过 `lua_Ref` 存储在 C++ 端，回调时通过 Lua C API 调用
  - 生命周期：`gModTimer.Shutdown()` 在 `ModLua::Shutdown()` 中释放所有 Lua 引用

#### 步骤 2：`plant_helper` 模块 — 封装成独立的 Lua 模块字符串

- [x] **`plant_helper.SimpleAI(plant, state_table)`** — 简易状态机 AI
  - 位置：`src/Mod/ModLua.cpp` 嵌入字符串 `plant_helper.lua`
  - 注册为全局表 `PlantHelper`
  - 设计细节：
    - 内部维护 `_plants` 弱表：`plant_userdata → { current_state, states, anim_playing }`
    - 状态结构：`{ animation?, loopType?, on_update?, on_finish?, on_enter?, on_exit? }`
    - `on_update(p)` → 返回下一个状态名或 nil（保持当前状态）
    - `on_finish(p)` → 动画播放完毕时调用，返回下一个状态名
    - 自动管理动画切换：状态变更时自动调用 `plant:PlayBodyReanim(state.animation, state.loopType)`
    - 通过 `reanim:GetLoopCount()` 变化检测 `on_finish` 触发时机
  - 用法示例：
    ```lua
    function OnPlantUpdate(plant)
        PlantHelper.Update(plant)
        -- 如果植物没有注册 SimpleAI，Update 是空操作
    end

    function OnPlantSpawn(plant)
        if plant.id == "fire_pea" then
            PlantHelper.SimpleAI(plant, {
                idle = {
                    animation = "anim_idle",
                    on_update = function(p)
                        if Board:FindTargetZombie(p) then return "attacking" end
                    end
                },
                attacking = {
                    animation = "anim_shoot",
                    loopType = ReanimLoopType.PLAY_ONCE_AND_HOLD,
                    on_finish = function(p)
                        Board:AddProjectile(p.x, p.y, p.row, ProjectileType.PEA)
                        return "idle"
                    end
                }
            })
        end
    end
    ```
  - 验证：用 `peashooter_plus` 或新建 `demo_plant` mod 做端到端测试

- [x] **`plant_helper.AutoShooter(plant, config)`** — 自动射击封装
  - 设计细节：
    - 基于 `_timer` 实现发射间隔计数器
    - 配置：`{ range, fireRate, projectileType, projectileDamage?, onFire? }`
    - `Update(plant)` 内部：查找目标 → 目标在射程内 → 倒计时递减 → 到 0 发射
    - 发射时调用 `Board:AddProjectile()` + 设置伤害 + 触发 `onFire` 回调
    - 无目标时重置计时器（不浪费帧）
  - 用法示例：
    ```lua
    function OnPlantSpawn(plant)
        PlantHelper.AutoShooter(plant, {
            range = 300,
            fireRate = 90,
            projectileType = ProjectileType.PEA,
            projectileDamage = 20,
        })
    end
    ```

- [x] **`plant_helper.TimedAction(plant, frames, callback)`** — 单次延时
  - 设计细节：
    - 基于 `_timer` 实现，`frames` 帧后调用 `callback(plant)`
    - 返回 `cancel()` 函数用于提前取消
    - 支持链式调用：`TimedAction(p, 30, f):Then(60, g)`
  - 用法示例：
    ```lua
    function OnPlantSpawn(plant)
        PlantHelper.TimedAction(plant, 60, function(p)
            p:PlayBodyReanim("anim_idle", ReanimLoopType.LOOP)
        end)
    end
    ```

- [x] **`plant_helper.RepeatAction(plant, interval, callback)`** — 重复执行
  - 类似 TimedAction 但循环执行，`callback` 返回 false 时停止
  - 适合周期性效果（如每 120 帧产生阳光）

### Lua 辅助模块体系 — Phase 2：扩展模块

以下模块在 Phase 1 基础设施就绪后按需添加，共享 `_timer` 设施。

#### `zombie_helper` — 僵尸行为辅助

- [ ] **`zombie_helper.SimpleAI(zombie, state_table)`**
  - 位置：`src/Mod/ModLua.cpp` 嵌入字符串 `zombie_helper.lua`
  - 注册为全局表 `ZombieHelper`
  - 与 `plant_helper.SimpleAI` 结构相同，但操作 Zombie 实体
  - 僵尸状态机可在 `OnZombieSpawn` 中注册，`OnPlantUpdate` 中不适用（需新增 `OnZombieUpdate` 回调 —— 参见 Phase 3）

#### `projectile_helper` — 弹道辅助

- [ ] **`projectile_helper.Spread(plant, count, angle, config)`** — 扇形散射
  - 一次发射 count 发投射物，呈 angle 度扇形分布
  - 每发单独调用 `Board:AddProjectile()` + `proj:SetVelocity()` 设置方向

- [ ] **`projectile_helper.Burst(plant, count, delay, config)`** — 连射
  - 使用 `_timer` 按 delay 帧间隔依次发射 count 发

- [ ] **`projectile_helper.Homing(projectile, target, turnRate)`** — 跟踪弹
  - 标记投射物为跟踪模式，在 `OnProjectileUpdate`（需新增回调）中逐帧修正方向

#### `wave_helper` — 波次/刷怪辅助

- [ ] **`wave_helper.ScheduleWave(wave_index, spawn_list)`** — 自定义波次
  - 格式：`{ { type, row, delay }, ... }`
  - 在 `OnWaveStart` 中使用 `_timer` 调度僵尸生成

- [ ] **`wave_helper.TimedSpawn(delay, type, row, count)`** — 延时刷怪
  - 任意时刻调用，不依赖波次系统

#### `tween_helper` — 插值/动画辅助

- [ ] **`tween_helper.Tween(entity, props, duration, easing)`** — 属性平滑过渡
  - 支持对 `x`, `y`, `scale`, `rotation` 等属性的线性/缓动插值
  - 基于 `_timer` 驱动，每帧更新实体属性
  - 缓动函数：`linear`, `easeIn`, `easeOut`, `easeInOut`, `bounce`

- [ ] **`tween_helper.Sequence(steps)`** — 动画序列编排
  - `steps` 数组：`{ { action, duration }, ... }`
  - `action` 可以是函数调用或 Tween 配置

#### `ui_helper` — UI 辅助

- [ ] **`ui_helper.ProgressBar(x, y, w, h, max_value)`** — HUD 进度条
  - 基于 `Board.AddButton` 或全新绘制（需 C++ 侧渲染支持）
  - 当前优先使用 `Board.AddButton` 模拟

- [ ] **`ui_helper.FloatingText(text, x, y, duration, color)`** — 浮动文字
  - 使用 `_timer` 驱动显示和淡出

#### `util_helper` — 通用工具

- [ ] **`util_helper.Clamp(v, min, max)`**
- [ ] **`util_helper.Lerp(a, b, t)`**
- [ ] **`util_helper.RandomWeighted({{value, weight}, ...})`**
- [ ] **`util_helper.GridDistance(col1, row1, col2, row2)`**
- [ ] **`util_helper.Shuffle(t)`**
- [ ] **`util_helper.TableContains(t, v)`**

---

### C++ 侧预置条件（辅助模块的前置基础设施）

以下 C++ 改动是辅助模块正常工作所必需的：

- [x] **在 `ModLua::Init()` 中加载 helper 模块**
  - 位置：`src/Mod/ModLua.cpp`（`Init()` 中 `Lua_RegisterGameTable(L)` 后调用）
  - 实现方式：
    - `_timer`：C++ 类 `ModTimer`，通过 `Lua_RegisterTimerTable()` 注册为 `_timer` 全局表
    - `plant_helper`：`Sexy::GetResourcePath()` + `ReadFileText()` + `luaL_loadbuffer` + `lua_pcall`
  - `src/Mod/Scripts/plant_helper.lua` 以独立文件存放；`_timer` 不再从 Lua 文件加载
  - 新增 `entity._ptr` 属性（lightuserdata）作为跨回调稳定标识符

- [ ] **考虑新增 `OnZombieUpdate(zombie)` 回调**（如需僵尸辅助模块）
  - 位置：`src/Mod/ModLua.h:29`，`src/Mod/ModLua.cpp` 新增函数
  - 补丁：`Zombie::Update()` 中插入调用
  - 这是 `zombie_helper.SimpleAI` 能正常工作的前提

- [ ] **考虑新增 `OnProjectileUpdate(projectile)` 回调**（如需跟踪弹辅助）
  - 位置：`src/Mod/ModLua.h`，`src/Mod/ModLua.cpp` 新增函数
  - 补丁：`Projectile::Update()` 中插入调用

### 实施验证（测试）

- [x] **创建 `_test_helpers/` 测试 mod 验证 Phase 1**
  - 测试 `SimpleAI`：注册一个自定义植物，状态机在 idle/attacking 之间切换
  - 测试 `AutoShooter`：注册一个自动射击植物，验证发射间隔和伤害
  - 测试 `TimedAction`：验证延时执行和取消

- [ ] **创建 `demo_plant` 示例 mod**（AGENTS.md 提及但不存在）
  - 演示完整的自定义植物生命周期：注册 → 动画 → AI → 射击

---

## 🟠 中低优先级（Game/Board API 扩展 + 次要回调）

### Game API 扩展

- [x] **`Game.GetSun()` / `Game.SetSun(amount)`** — 阳光数
- [x] **`Game.GetTotalWaves()`** — 总波数
- [x] **`Game.IsNight()` / `Game.HasPool()` / `Game.IsRoof()` / `Game.IsFog()`** — 场地类型
- [x] **`Game.SpawnSun(x, y, value)`** — 生成阳光
- [x] **`Game.SpawnCoin(x, y, coinType)`** — 生成硬币
- [x] **`Game.PlayFoley(foleyType)`** — 播放音效
- [x] **`Game.GetPlayerCoins()` / `Game.AddPlayerCoins(amount)`** — 银币管理
- [x] **`Game.Shake(x, y)`** — 屏幕震动
- [x] **`Game.DisplayAdvice(text, style)`** — 提示文字

### Board API 扩展

- [x] **`Board.Pause(bool)` / `Board.IsPaused()`** — 暂停控制
- [x] **`Board.GetWave()`** — 当前波次（已实现，确认）

### 次要新回调

- [x] **`OnPlantEaten(plant, zombie)`** — `Zombie::EatPlant()` 啃食时
- [x] **`OnPlantProduce(plant)`** — `Plant::UpdateProductionPlant()` 产出时
- [x] **`OnPlantUpgrade(plant, oldType)`** — `Plant::ImitaterMorph()` 升级时
- [x] **`OnZombieFrozen(zombie, isFrozen)`** — `Zombie::ApplyChill()`
- [x] **`OnZombieButtered(zombie)`** — `Zombie::ApplyButter()`
- [x] **`OnZombieMindControl(zombie)`** — `Zombie::StartMindControlled()`
- [x] **`OnCoinExpire(coin)`** — `Coin::Update()` 超时消失
- [x] **`OnFlagRaise(waveIndex)`** — `Board::NextWaveComing()` 旗帜升起
- [x] **`OnMowerTriggered(row, mowerType)`** — `LawnMower::StartMower()`
- [x] **`OnSunCountChange(oldAmount, newAmount)`** — `Board::SetSunMoney()`

---

## 🟢 低优先级（UI 回调 + 质量）

### UI 生命周期回调

- [ ] **`OnPause()` / `OnResume()`** — `LawnApp::DoPauseDialog()`
- [ ] **`OnSeedChooserOpen()`** — `LawnApp::ShowSeedChooserScreen()`
- [ ] **`OnSeedChooserClose(seeds)`** — `LawnApp::KillSeedChooserScreen()`
- [ ] **`OnAlmanacOpen()` / `OnAlmanacClose()`** — 图鉴
- [ ] **`OnStoreOpen()` / `OnStoreClose()`** — 商店

### 质量与测试

- [ ] **添加测试模版/测试套件**
  - `demo_plant`、`ui_test_mod`、`example_mod` 等被 AGENTS.md 提及但不存在
  - 需包含 C++ 单元测试和 Lua 集成测试

- [ ] **实现声音覆盖 / `properties/default.xml` 资源覆盖**
  - 文档提到但代码未实现

---

## 附录：建议实施顺序

| 阶段 | 内容 | 预估工作量 | 状态 |
|------|------|-----------|------|
| **P0** | 基础设施修复（Game.GetMode、ModZombieDef、依赖排序） | ~1天 | ✅ 完成 |
| **P1** | 常量表注册（全部枚举，纯 Lua 侧） | ~半天 | ✅ 完成 |
| **P2** | **自定义植物三大 API**（动画控制 + 目标查找 + 投射物生成） | ~3天 | ✅ 完成 |
| **P3** | 实体属性+方法扩展 + Projectile 新实体类型 | ~2天 | ✅ 完成 |
| **P4** | 关键回调（PlantDie、ZombieAttack、ProjectileSpawn/Hit/Miss、CoinCollect、ZombieReachHouse） | ~2天 | ✅ 完成 |
| **P5a** | **C++ 预置条件**：helper 模块嵌入加载、`entity._ptr` 稳定标识符 | ~0.5天 | ✅ 完成 |
| **P5b** | **plant_helper 模块**：C++ `_timer` + SimpleAI + AutoShooter + TimedAction | ~1天 | ✅ 完成 |
| **P5c** | **验证**：`_test_helpers` 端到端测试 | ~0.5天 | ✅ 完成 |
| **P6** | 辅助模块 Phase 2（zombie_helper + projectile_helper + wave_helper + tween_helper + util_helper + ui_helper） | ~2天 | ❌ 待定 |
| **P7** | Game/Board API 扩展（Sun、Coin、Foley、场地类型等） | ~1天 | ✅ 完成 |
| **P8** | 次要回调 + UI 回调（OnPlantEaten、OnPause、OnSeedChooserOpen 等） | ~2天 | ✅ 中优先级完成（OnPlantEaten/Produce/Upgrade、OnZombieFrozen/Buttered/MindControl、OnCoinExpire、OnFlagRaise、OnMowerTriggered、OnSunCountChange） |
| **P9** | 质量增强：测试套件、声音覆盖、资源覆盖 | ~2天 | ❌ 待定 |

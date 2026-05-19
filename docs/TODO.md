# Mod Framework — 待办清单

> 评估日期：2026-05-19，框架完整性约 75%

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

- [x] **修复依赖排序：添加拓扑排序**
  - 位置：`src/Mod/ModLoader.cpp:401-442`
  - 实现 Kahn 算法，先按 dependencies 拓扑排序，再按 priority 稳定排序
  - 检测循环依赖的 mod 会追加末尾并记录错误

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

### Lua 辅助模块（可选但强烈推荐）

- [ ] **创建 `plant_helper.lua` 内置辅助模块**
  - 路径：`mods/_internal/plant_helper.lua`（或嵌入到 ModLua 中作为 require 模块）
  - 提供以下高阶封装降低模组作者门槛：

- [ ] **`plant_helper.SimpleAI(plant, { ... })`** — 简易状态机 AI
  ```lua
  -- 用法示例
  plant_helper.SimpleAI(plant, {
      idle = {
          animation = "anim_idle",
          on_update = function(p) 
              local t = Board:FindTargetZombie(p)
              if t then return "attacking" end
          end
      },
      attacking = {
          animation = "anim_shoot",
          loopType = REANIM_PLAY_ONCE_AND_HOLD,
          on_finish = function(p)
              Board:AddProjectile(p.x, p.y, p.row, PROJ_PEA)
              return "idle"
          end
      }
  })
  ```

- [ ] **`plant_helper.AutoShooter(plant, { ... })`** — 简化版自动射击封装
  ```lua
  plant_helper.AutoShooter(plant, {
      range = 300,
      fireRate = 90,
      projectileType = PROJ_PEA,
      projectileDamage = 20,
      onFire = function(plant, target) end,  -- 可选回调
  })
  ```

- [ ] **`plant_helper.TimedAction(plant, frames, callback)`** — 计时器辅助
  - 方便在 `OnPlantUpdate` 中实现延迟/间隔逻辑，无需手动管理计数器

---

## 🟠 中低优先级（Game/Board API 扩展 + 次要回调）

### Game API 扩展

- [ ] **`Game.GetSun()` / `Game.SetSun(amount)`** — 阳光数
- [ ] **`Game.GetTotalWaves()`** — 总波数
- [ ] **`Game.IsNight()` / `Game.HasPool()` / `Game.IsRoof()` / `Game.IsFog()`** — 场地类型
- [ ] **`Game.SpawnSun(x, y, value)`** — 生成阳光
- [ ] **`Game.SpawnCoin(x, y, coinType)`** — 生成硬币
- [ ] **`Game.PlayFoley(foleyType)`** — 播放音效
- [ ] **`Game.GetPlayerCoins()` / `Game.AddPlayerCoins(amount)`** — 银币管理
- [ ] **`Game.Shake(x, y)`** — 屏幕震动
- [ ] **`Game.DisplayAdvice(text, style)`** — 提示文字

### Board API 扩展

- [ ] **`Board.Pause(bool)` / `Board.IsPaused()`** — 暂停控制
- [ ] **`Board.GetWave()`** — 当前波次（已实现，确认）

### 次要新回调

- [ ] **`OnPlantEaten(plant, zombie)`** — `Zombie::EatPlant()` 啃食时
- [ ] **`OnPlantProduce(plant)`** — `Plant::UpdateProductionPlant()` 产出时
- [ ] **`OnPlantUpgrade(plant, oldType)`** — `Plant::ImitaterMorph()` 升级时
- [ ] **`OnZombieFrozen(zombie, isFrozen)`** — `Zombie::ApplyChill()`
- [ ] **`OnZombieButtered(zombie)`** — `Zombie::ApplyButter()`
- [ ] **`OnZombieMindControl(zombie)`** — `Zombie::StartMindControlled()`
- [ ] **`OnCoinExpire(coin)`** — `Coin::Update()` 超时消失
- [ ] **`OnFlagRaise(waveIndex)`** — `Board::NextWaveComing()` 旗帜升起
- [ ] **`OnMowerTriggered(row, mowerType)`** — `LawnMower::StartMower()`
- [ ] **`OnSunCountChange(oldAmount, newAmount)`** — `Board::SetSunMoney()`

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

| 阶段 | 内容 | 预估工作量 |
|------|------|-----------|
| **P0** | 基础设施修复（Game.GetMode、ModZombieDef、依赖排序） | ~1天 |
| **P1** | 常量表注册（全部枚举，纯 Lua 侧） | ~半天 |
| **P2** | **自定义植物三大 API**（动画控制 + 目标查找 + 投射物生成） | ~3天 |
| **P3** | 实体属性+方法扩展 + Projectile 新实体类型 | ~2天 |
| **P4** | 关键回调（PlantDie、ZombieAttack、ProjectileSpawn/Hit/Miss、CoinCollect、ZombieReachHouse） | ~2天 |
| **P5** | Lua 辅助模块（plant_helper 状态机/自动射击/计时器） | ~1天 |
| **P6** | Game/Board API 扩展 + 现有问题修复 | ~1天 |
| **P7** | 次要回调 + UI 回调 + 质量增强 | ~2天 |

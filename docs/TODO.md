# Mod Framework — 待办清单

> 评估日期：2026-05-19，框架完整性约 70%

---

## 🔴 高优先级（基础设施修复 + 常量表 + 自定义植物核心 API）

### 基础设施修复

- [ ] **实现 `Game.GetMode()` Lua API 绑定**
  - 位置：`src/Mod/ModLua.cpp` — `Lua_RegisterGameTable()`
  - 文档均有提及但代码缺失，模组需运行时判断模式

- [ ] **扩展 `ModZombieDef` 添加实体属性**
  - 位置：`src/Mod/ModRegistry.h:32-36`
  - 目前只有 `id`/`zombieType`，缺少血量、速度、伤害、动画名等
  - 需同步扩展 `ModModeDef`、`ModPlantDef` 中缺失的字段

- [ ] **修复依赖排序：添加拓扑排序**
  - 位置：`src/Mod/ModLoader.cpp:401-403`
  - `dependencies` 字段已解析但未用于排序
  - 需在 stable_sort 之前先做 DAG 拓扑排序

### 常量表暴露 — 第一阶段（无 C++ 改动，纯 Lua 注册）

- [ ] **Lua 侧注册 `PlantType` 常量表**
  - 当前枚举值可通过 `ConstEnums.h` 获取
  - 作为 Lua 只读全局表注册（`PEASHOOTER=0, SUNFLOWER=1, ...`）

- [ ] **Lua 侧注册 `ZombieType` 常量表**
  - `NORMAL=0, FLAG=1, ..., BOSS=25`

- [ ] **Lua 侧注册 `ProjectileType` 常量表**
  - `PEA=0, SNOWPEA=1, ..., ZOMBIE_PEA=13`

- [ ] **Lua 侧注册 `CoinType` 常量表**
  - `NONE=0, ..., SUN=4, SMALLSUN=5, LARGESUN=6`

- [ ] **Lua 侧注册 `GameMode` 常量表**
  - `ADVENTURE=0, SURVIVAL_NORMAL_STAGE_1=1, ..., CHALLENGE_FINAL_BOSS=39`

- [ ] **Lua 侧注册 `PlantSubClass` / `PlantState` / `ZombiePhase` / `ProjectileMotion` 常量表**
  - 各状态枚举供实体 API 判定使用

- [ ] **Lua 侧注册 `DamageFlags` / `BackgroundType` / `HelmType` / `ShieldType` 常量表**

- [ ] **Lua 侧注册 `GridConstants` 游戏常量表**
  - MAX_COLS=9, MAX_ROWS=6, BOARD_WIDTH=800, BOARD_HEIGHT=600

- [ ] **Lua 侧注册 `ReanimLoopType` 常量表**
  - `LOOP=0, LOOP_FULL_LAST_FRAME=1, PLAY_ONCE=2, PLAY_ONCE_AND_HOLD=3, ...`

### 自定义植物行为 API（核心三大块）

#### A. 动画控制 API — 暴露 Reanimation 系统到 Lua

- [ ] **`plant:PlayBodyReanim(trackName, loopType, blendTime, animRate)`**
  - 映射 C++ `Plant::PlayBodyReanim(theTrackName, theLoopType, theBlendTime, theAnimRate)`
  - 位置：`src/Lawn/Plant.cpp:1148`

- [ ] **`plant:PlayIdleAnim(animRate)`**
  - 映射 C++ `Plant::PlayIdleAnim(theRate)` → 播放 `"anim_idle"` 轨道
  - 位置：`src/Lawn/Plant.cpp:5311`

- [ ] **`plant:GetBodyReanimProgress()`** → float
  - 返回 `mAnimTime`（0.0~1.0 归一化进度）
  - 位置：`src/Sexy.TodLib/Reanimator.h:211`

- [ ] **`plant:SetBodyReanimRate(rate)`**
  - 设置 `mAnimRate`，控制动画播放速度

- [ ] **`plant:GetBodyReanimRate()`** → float

- [ ] **`plant:IsAnimPlaying(trackName)`** → bool
  - 映射 C++ `Reanimation::IsAnimPlaying(theTrackName)`
  - 位置：`src/Sexy.TodLib/Reanimator.h:279`

- [ ] **`plant:TrackExists(trackName)`** → bool
  - 映射 C++ `Reanimation::TrackExists(theTrackName)`
  - 位置：`src/Sexy.TodLib/Reanimator.h:252`

- [ ] **`plant:GetBodyReanimLoopType()`** → int
  - 返回当前 `mLoopType`

- [ ] **`plant:GetBodyReanimLoopCount()`** → int
  - 返回已循环次数 `mLoopCount`

#### B. 目标查找 API — 暴露实体查询到 Lua

- [ ] **`Board:FindTargetZombie(plant)`** → zombie_entity | nil
  - 映射 C++ `Plant::FindTargetZombie(weaponType)`，为指定植物查找最近目标
  - 位置：`src/Lawn/Plant.cpp:4801`

- [ ] **`Board:GetZombiesInRow(row)`** → table of zombie_entity
  - 遍历 `mZombies[]` 按行筛选，返回 Lua 数组

- [ ] **`Board:GetPlantsInRow(row)`** → table of plant_entity

- [ ] **`Board:GetZombieAt(col, row)`** → zombie_entity | nil
  - 获取指定格子上的僵尸

- [ ] **`Board:GetPlantAt(col, row)`** → plant_entity | nil
  - 已在 TODO 中，移至此处

- [ ] **`Board:GetAllZombies()`** → table of zombie_entity

- [ ] **`Board:GetAllPlants()`** → table of plant_entity

- [ ] **`entity:DistanceTo(otherEntity)`** → float
  - 像素距离计算

#### C. 投射物生成 API — 暴露发射系统到 Lua

- [ ] **`Board:AddProjectile(x, y, row, projectileType)`** → proj_entity
  - 映射 C++ `Board::AddProjectile(theX, theY, theRenderOrder, theRow, theProjectileType)`
  - 返回 Projectile 实体供后续操作
  - 位置：`src/Lawn/Board.cpp:2414`

- [ ] **`proj:SetDamage(amount)`**
  - 设置伤害值

- [ ] **`proj:SetVelocity(vx, vy, vz)`**
  - 设置速度分量

- [ ] **`proj:SetDamageFlags(flags)`**
  - 设置伤害标记（冻结、穿透等），参考 `DamageFlags` 常量

- [ ] **`proj:SetMotionType(motionType)`**
  - 设置弹道类型（直线、抛物线、跟踪等）

- [ ] **`proj:SetTargetZombie(zombie)`**
  - 设置跟踪目标（用于跟踪弹）

### 现有问题修复（移至此处以便集中处理）

- [ ] **修复保存格式为 JSON**
  - 位置：`src/Mod/ModSave.cpp:74`

- [ ] **实现实体 `id` 字符串查找**
  - 位置：`src/Mod/ModLua.cpp:313-321`

- [ ] **`LuaProxyDialog::ButtonDepress` 支持不关闭对话框**
  - 位置：`src/Mod/LuaProxyDialog.cpp:128`

- [ ] **实现自定义模式挑战 UI 集成**
  - Challenge.cpp 仍使用硬编码数组

---

## 🟡 中优先级（实体属性扩展 + 关键回调 + 辅助模块）

### 实体通用属性与方法

- [ ] **扩展 Plant 实体属性**
  - `row`, `col`, `x`, `y`, `maxHp`, `state`, `subClass`, `isAsleep`, `isDead`, `launchCounter`, `launchRate`, `age`, `imitaterType`, `recentlyEaten`, `squished`
  - 位置：`src/Mod/ModLua.cpp` — Lua_EntityIndex 元表

- [ ] **扩展 Plant 实体方法**
  - `:Die()`, `:Squish()`, `:SetSleeping(bool)`, `:GetCost()`, `:GetName()`
  - `:IsNocturnal()`, `:IsFungus()`, `:IsAquatic()`, `:IsUpgrade()`, `:IsFlying()`

- [ ] **扩展 Zombie 实体属性**
  - `row`, `x`, `y`, `velX`, `maxHp`, `phase`, `isEating`, `isDead`, `age`
  - `chilled`, `buttered`, `mindControlled`, `helmType`, `helmHp`, `shieldType`, `shieldHp`
  - `hasHead`, `hasArm`, `inPool`, `onHighGround`, `altitude`

- [ ] **扩展 Zombie 实体方法**
  - `:SetRow(n)`, `:ApplyChill(isIce)`, `:ApplyButter()`, `:RemoveButter()`
  - `:StartMindControlled()`, `:DieNoLoot()`, `:DieWithLoot()`
  - `:TakeHelmDamage(n)`, `:TakeShieldDamage(n)`
  - `:IsFlying()`, `:IsOnHighGround()`, `:IsImmobilized()`

- [ ] **扩展 Coin 实体属性**
  - `x`, `y`, `velX`, `velY`, `age`, `value`, `isBeingCollected`, `coinMotion`, `isMoney`, `scale`

- [ ] **扩展 Coin 实体方法**
  - `:GetValue()`, `:Die()`, `:StartFade()`

- [ ] **新增 Projectile 实体类型（全新包装）**
  - 属性：`type`, `x`, `y`, `z`, `velX`, `velY`, `row`, `motionType`, `age`, `isDead`, `damage`, `rotation`
  - 方法：`:Die()`, `:GetDamage()`, `:IsSplash()`
  - 位置：`src/Mod/ModLua.cpp` — 新增 `ENTITY_TYPE_PROJECTILE=3` + PushEntity 重载

### 关键新生命周期回调

- [ ] **`OnPlantDie(plant)`** — 补丁位置：`Plant::Die()`
- [ ] **`OnZombieAttack(zombie, plant)`** — 补丁位置：`Zombie::EatPlant()`
- [ ] **`OnProjectileSpawn(proj)`** — 补丁位置：`Projectile::ProjectileInitialize()`
- [ ] **`OnProjectileHit(proj, zombie)`** — 补丁位置：`Projectile::DoImpact()`
- [ ] **`OnProjectileMiss(proj)`** — 补丁位置：`Projectile::Update()` 飞出检测
- [ ] **`OnCoinCollect(coin)`** — 补丁位置：`Coin::Collect()`
- [ ] **`OnZombieReachHouse(zombie)`** — 补丁位置：`Zombie::WalkIntoHouse()`

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

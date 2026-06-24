# PvZ-Portable Modding Specification (Current Implementation)

This document describes the currently implemented mod system for PvZ-Portable with Lua scripting and resource overrides.
The goal is to enable new plants, new zombies, and new modes while keeping cross-platform compatibility.

Implementation status: verified against the C++ source on 2026-06-24. The implemented content path is `mod.json` + Lua registration APIs + `resources/`; JSON data tables such as `data/plants.json` are not loaded by the current engine.

## 1. Design Goals

- Cross-platform: must work on desktop, mobile, consoles, and WASM without JIT.
- Deterministic: mods should not destabilize core gameplay.
- Lua-first: content is registered through Lua APIs; resources can be supplied through `resources/`.
- Safe fallback: missing resources or script errors should fail gracefully.
- Save compatibility: preserve original saves, store mod data separately.

## 2. Mod Package Layout

All mods live under the game resource root:

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

Notes:
- `mod_id` must be unique and ASCII-safe: `a-z`, `0-9`, `_`, `-`.
- `resources/` is an override layer with higher priority than base assets.
- `properties/default.xml` can override string keys for UI/almanac text.

## 3. Mod Manifest (mod.json)

Minimal schema:

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

Fields:
- `priority`: parsed by the loader, but the current implementation primarily preserves dependency/topological order; do not rely on it for conflict resolution.
- `dependencies`: list of mod ids that should load before this mod when present. Missing dependencies are not currently treated as fatal errors.
- `entry`: Lua entry file executed during mod initialization; use `OnModInit()` to register content.
- `data`: not implemented. Define plants, zombies, modes, and projectiles in Lua with `Game.Register*` APIs.

## 4. Loading Order

1) Scan `mods/` and parse all `mod.json` files.
2) Topologically order loaded mods by dependencies that are also present.
3) Run each Lua `entry` and call its `OnModInit`.
4) Register each mod's `resources/` directory as a resource overlay before resource files are parsed.

Conflicts and limitations:
- Duplicate manifest ids are not currently rejected by `ModLoader`.
- Duplicate plant, zombie, mode, or projectile ids are rejected by `ModRegistry`; later registrations do not override earlier ones.
- All mod scripts share one Lua global state. Callbacks are captured per mod after loading, but global variables/functions can still collide. Prefer `local` variables and uniquely named globals.
- Script errors in entries and callbacks are logged and do not intentionally crash the game.

## 5. Data Registration via Lua

All mod content (plants, zombies, projectiles, modes) is registered via Lua API calls in the entry script's `OnModInit()` function. No JSON data files are used.

### 5.1 Plants

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

Required fields:
- `id`: unique identifier for the plant

Optional fields:
- `name`: display name (default: `"ModPlant"`)
- `cost`: sun cost (default: `50`)
- `cooldown`: cooldown in frames (default: `750`)
- `subClass`: `0` = normal, `1` = shooter (default: `0`)
- `launchRate`: frames between shots (default: `0`)
- `projectileType`: string (projectile ID) or integer (runtime type)
- `reanimation`: vanilla reanim path, e.g. `"reanim/FirePea.reanim"`
- `reanimFile`: external reanim XML path relative to mod root, e.g. `"resources/reanim/custom.xml"`
- `image`: custom static image path for seed packet icon
- `description`: Almanac description text

Reserved ranges:
- Mod plant SeedType IDs are automatically assigned starting from `2000`.

### 5.2 Zombies

```lua
Game.RegisterZombie({
    id = "conehead_fast"
})
```

Required fields:
- `id`: unique identifier

Reserved ranges:
- ZombieType IDs are automatically assigned starting from `3000`.

### 5.3 Projectiles

```lua
Game.RegisterProjectile({
    id = "pea_plus",
    damage = 20,
    speed = 7.5,
    image = "IMAGE_PROJECTILE_PEA_PLUS"
})
```

Required fields:
- `id`: unique identifier

Optional fields:
- `damage`: damage value (default: `0`)
- `speed`: projectile speed (default: `3.0`)
- `image`: image name for rendering

Reserved ranges:
- ProjectileType IDs are automatically assigned starting from `4000`.

### 5.4 Modes

```lua
Game.RegisterMode({
    id = "rush_mode"
})
```

Required fields:
- `id`: unique identifier

Reserved ranges:
- Mode IDs are automatically assigned starting from `5000`.

### 5.5 Strings

String overrides can be provided via `resources/properties/default.xml` in the mod package. See Section 6 for the overlay rules.

## 6. Resource Overlay Rules

- The resource manager should search mod resources before base resources.
- Overlay scope is per mod, but the final lookup order is mod load order.
- If a resource is missing, fall back to base resources.

Recommended paths inside `resources/`:
- `images/` for PNG or atlas textures
- `sounds/` for audio
- `reanim/` for animation atlases
- `properties/` for XML string overrides

## 7. Lua API Reference

### 7.1 Global Objects

#### Game

| API | Returns | Description |
|-----|---------|-------------|
| `Game.Log(text)` | — | Log a message to `log.txt` with `[Lua]` prefix |
| `Game.RegisterPlant(def)` | int | Register a custom plant, returns SeedType (≥2000) |
| `Game.RegisterZombie(def)` | int | Register a custom zombie, returns ZombieType (≥3000) |
| `Game.RegisterMode(def)` | int | Register a custom game mode, returns mode ID (≥5000) |
| `Game.RegisterProjectile(def)` | int | Register a custom projectile, returns type ID (≥4000) |
| `Game.GetMode()` | int | Current game mode ID |
| `Game.GetSpeed()` | int | Current game speed multiplier |
| `Game.SetSpeed(mult)` | — | Set game speed (1, 2, 3, 5, 10) |
| `Game.SaveModData(key, value)` | — | Persist a string KV pair for this mod |
| `Game.LoadModData(key)` | string\|nil | Load a saved value; `nil` if not found |
| `Game.GetMenuButtonRect()` | x, y, w, h | Position of the menu button in the board |
| `Game.GetSun()` | int | Current sun count |
| `Game.SetSun(amount)` | — | Set sun count |
| `Game.GetTotalWaves()` | int | Total number of waves in level |
| `Game.IsNight()` | bool | Level has night background |
| `Game.HasPool()` | bool | Level has pool (includes fog) |
| `Game.IsRoof()` | bool | Level has roof background |
| `Game.IsFog()` | bool | Level has fog background |
| `Game.SpawnSun(x, y [, value])` | Entity\|nil | Spawn a sun at pixel position (default value=50) |
| `Game.SpawnCoin(x, y, coinType)` | Entity\|nil | Spawn a coin at pixel position |
| `Game.PlayFoley(foleyType)` | — | Play a sound effect by FoleyType enum |
| `Game.GetPlayerCoins()` | int | Total coins in player's wallet |
| `Game.AddPlayerCoins(amount)` | — | Add coins to player's wallet |
| `Game.Shake(x, y)` | — | Shake the screen by given intensity |
| `Game.DisplayAdvice(text [, style])` | — | Show advice text (uses MessageStyle enum) |

#### Board

| API | Returns | Description |
|-----|---------|-------------|
| `Board.SpawnZombie(id, row)` | Entity\|nil | Spawn a zombie by string ID or integer type |
| `Board.SpawnPlant(id, row, col)` | Entity\|nil | Plant at grid position by string ID or integer type |
| `Board.GetWave()` | int | Current wave index |
| `Board.Pause(bool)` | — | Pause or unpause the board |
| `Board.IsPaused()` | bool | Whether the board is paused |
| `Board.FindTargetZombie(plant)` | Entity\|nil | Nearest zombie target for a plant |
| `Board.GetZombiesInRow(row)` | table | Array of zombie entities in a row |
| `Board.GetPlantsInRow(row)` | table | Array of plant entities in a row |
| `Board.GetZombieAt(col, row)` | Entity\|nil | Zombie occupying grid cell |
| `Board.GetPlantAt(col, row)` | Entity\|nil | Top plant at grid cell |
| `Board.GetAllZombies()` | table | Array of all living zombie entities |
| `Board.GetAllPlants()` | table | Array of all living plant entities |
| `Board.AddProjectile(x, y, row, projType)` | Entity\|nil | Create a projectile at pixel position |
| `Board.AddButton(id, x, y, w, h, label)` | — | Add a custom board button |
| `Board.RemoveButton(id)` | — | Remove a custom board button |
| `Board.SetButtonVisible(id, visible)` | — | Show/hide a custom button |
| `Board.SetButtonLabel(id, label)` | — | Change a custom button's text |

#### Entity

Entity is a unified wrapper for `Plant`, `Zombie`, `Coin`, and `Projectile` objects.
All callbacks receive Entities. Methods that don't apply to a given type are no-ops.

**Properties (read-only):**

| Property | Plant | Zombie | Coin | Projectile | Description |
|----------|-------|--------|------|------------|-------------|
| `.type` | ✅ SeedType | ✅ ZombieType | ✅ CoinType | ✅ ProjectileType | Runtime type integer |
| `.id` | ✅ | ✅ | ✅ (blank) | ✅ (blank) | Mod registration ID string, `""` for vanilla |
| `.hp` | ✅ mPlantHealth | ✅ mBodyHealth | ✅ (0) | ✅ (1) | Current hit points |
| `.row` | ✅ | ✅ | — | ✅ | Row index (0-5) |
| `.col` | ✅ | — | — | — | Column index (0-8) |
| `.x` / `.y` | ✅ | ✅ | ✅ | ✅ | Pixel position |
| `.z` | — | — | — | ✅ | Vertical position (for lobbed projectiles) |
| `.maxHp` | ✅ | ✅ | — | — | Maximum HP |
| `.state` | ✅ | — | — | — | PlantState enum |
| `.subClass` | ✅ | — | — | — | PlantSubClass enum |
| `.isAsleep` | ✅ | — | — | — | Whether plant is asleep |
| `.isDead` | ✅ | ✅ | — | ✅ | Whether entity is marked dead |
| `.launchCounter` | ✅ | — | — | — | Frames until next shot |
| `.launchRate` | ✅ | — | — | — | Frames between shots |
| `.age` | ✅ (mAnimCounter) | ✅ (mZombieAge) | ✅ (mCoinAge) | ✅ (mProjectileAge) | Age in frames |
| `.imitaterType` | ✅ | — | — | — | Copied plant type if imitater |
| `.recentlyEaten` | ✅ | — | — | — | Eaten countdown |
| `.squished` | ✅ | — | — | — | Whether squished |
| `.velX` / `.velY` | — | ✅ (velX) | ✅ | ✅ | Velocity components |
| `.velZ` | — | — | — | ✅ | Vertical velocity |
| `.phase` | — | ✅ | — | — | ZombiePhase enum |
| `.isEating` | — | ✅ | — | — | Zombie currently eating |
| `.chilled` | — | ✅ | — | — | Chilled status |
| `.buttered` | — | ✅ | — | — | Buttered status |
| `.mindControlled` | — | ✅ | — | — | Mind-controlled status |
| `.helmType` / `.helmHp` | — | ✅ | — | — | Helmet type and health |
| `.shieldType` / `.shieldHp` | — | ✅ | — | — | Shield type and health |
| `.hasHead` / `.hasArm` | — | ✅ | — | — | Head/arm presence |
| `.inPool` | — | ✅ | — | — | Zombie in pool water |
| `.onHighGround` | — | ✅ | — | — | Zombie on high ground |
| `.altitude` | — | ✅ | — | — | Flight altitude |
| `.value` | — | — | ✅ | — | Coin/sun value |
| `.isBeingCollected` | — | — | ✅ | — | Coin being collected |
| `.coinMotion` | — | — | ✅ | — | CoinMotion enum |
| `.isMoney` | — | — | ✅ | — | Whether coin is currency |
| `.scale` | — | — | ✅ | — | Visual scale |
| `.motionType` | — | — | — | ✅ | ProjectileMotion enum |
| `.damage` | — | — | — | ✅ | Current damage value |
| `.rotation` | — | — | — | ✅ | Visual rotation |
| `._ptr` | ✅ | ✅ | ✅ | ✅ | Stable lightuserdata for C++ pointer (use as table key for per-entity state) |

**Methods:**

| Method | Applies to | Description |
|--------|-----------|-------------|
| `:Damage(amount)` | Plant, Zombie | Deal damage; auto-kill at 0 HP |
| `:DistanceTo(other)` | All | Pixel distance to another entity |
| `:IsSun()` | Coin | Returns true if coin is a sun |
| `:Collect()` | Coin | Play collect sound + bank coin |
| `:GetValue()` | Coin | Get coin/sun value |
| `:StartFade()` | Coin | Start fade-out animation |
| `:Die()` | Plant, Coin | Remove from board |
| `:Squish()` | Plant | Squish animation (like squash) |
| `:SetSleeping(bool)` | Plant | Set asleep/wake |
| `:GetCost()` | Plant | Sun cost |
| `:GetName()` | Plant | Display name string |
| `:IsNocturnal()` / `:IsFungus()` / `:IsAquatic()` / `:IsUpgrade()` / `:IsFlying()` | Plant | Plant classification queries |
| `:IsFlying()` | Zombie | Zombie is flying |
| `:SetRow(n)` | Zombie | Move zombie to row |
| `:ApplyChill(isIce)` | Zombie | Apply chill effect |
| `:ApplyButter()` / `:RemoveButter()` | Zombie | Butter control |
| `:StartMindControlled()` | Zombie | Mind control |
| `:DieNoLoot()` / `:DieWithLoot()` | Zombie | Death variants |
| `:TakeHelmDamage(n)` / `:TakeShieldDamage(n)` | Zombie | Damage helmet/shield |
| `:IsOnHighGround()` / `:IsImmobilized()` | Zombie | State queries |
| `:SetDamage(amount)` | Projectile | Override projectile damage |
| `:SetVelocity(vx, vy, vz)` | Projectile | Set velocity components |
| `:SetDamageFlags(flags)` | Projectile | Set DamageFlags bitmask |
| `:SetMotionType(motionType)` | Projectile | Change projectile motion |
| `:SetTargetZombie(zombie)` | Projectile | Set homing target |

**Reanimation Methods (Plant only):**

| Method | Description |
|--------|-------------|
| `:GetBodyReanim()` | Returns Reanim userdata for chain calls |
| `:PlayBodyReanim(track, loopType[, blendTime, rate])` | Play animation track |
| `:PlayIdleAnim([rate])` | Play `"anim_idle"` track |
| `:GetBodyReanimProgress()` | Normalized animation progress 0.0–1.0 |
| `:SetBodyReanimRate(rate)` | Set animation playback speed |
| `:GetBodyReanimRate()` | Get animation playback speed |
| `:IsAnimPlaying(track)` | Check if track is playing |
| `:TrackExists(track)` | Check if track exists in reanim |
| `:GetBodyReanimLoopType()` | Current loop type |
| `:GetBodyReanimLoopCount()` | Times animation has looped |

**Reanim userdata** (returned by `:GetBodyReanim()`):

| Method | Description |
|--------|-------------|
| `reanim:Play(track, loopType[, blendTime=0, rate=0])` | Play a track |
| `reanim:GetRate()` / `:SetRate(rate)` | AnimRate |
| `reanim:GetProgress()` / `:SetProgress(time)` | AnimTime 0.0–1.0 |
| `reanim:IsPlaying(track)` | Track is playing |
| `reanim:TrackExists(track)` | Track exists |
| `reanim:GetLoopType()` / `:GetLoopCount()` | Loop info |
| `reanim:SetPosition(x, y)` | Pixel position |
| `reanim:OverrideScale(sx[, sy])` | Scale (single = uniform) |
| `reanim:ShowOnlyTrack(track)` | Hide all other tracks |

#### UI

| API | Returns | Description |
|-----|---------|-------------|
| `UI.CreateDialog({title, body, modal})` | Dialog | Create a custom dialog |
| `UI.ShowMessage(title, body)` | Dialog | Modal message box with OK button |

#### Dialog

| Method | Description |
|--------|-------------|
| `dialog:AddButton(text, callback)` | Add button; callback runs on click, dialog auto-closes unless callback returns `false` |
| `dialog:AddLabel(text, x, y)` | Static label in `FONT_DWARVENTODCRAFT12` |
| `dialog:Close()` | Close and destroy |
| `dialog:SetTitle(text)` | Change header |
| `dialog:SetBody(text)` | Change body |

### 7.2 Lua Callbacks

All callbacks are optional. Parameters prefixed with `Entity` are entity objects with the full API above.

```lua
-- Lifecycle
function OnModInit()                                                              end  -- Mod script first loaded
function OnGameStart()                                                            end  -- Gameplay begins (after level start)
function OnLevelStart(mode_id)                                                    end  -- Level started, mode_id is integer
function OnLevelEnd(is_win)                                                       end  -- Level ended, is_win is boolean

-- Wave events
function OnWaveStart(wave_index)                                                  end  -- Wave advancing

-- Plant events
function OnPlantSpawn(plant)        -- Entity: Plant                               end
function OnPlantUpdate(plant)       -- Entity: Plant (every frame for mod plants)  end
function OnPlantAttack(plant, target) -- Entity: Plant, Zombie                     end
function OnPlantDie(plant)          -- Entity: Plant                               end

-- Zombie events
function OnZombieSpawn(zombie)      -- Entity: Zombie                              end
function OnZombieDie(zombie)        -- Entity: Zombie                              end
function OnZombieAttack(zombie, plant) -- Entity: Zombie, Plant                    end
function OnZombieReachHouse(zombie) -- Entity: Zombie                              end

-- Projectile events
function OnProjectileSpawn(proj)    -- Entity: Projectile                          end
function OnProjectileHit(proj, zombie) -- Entity: Projectile, Zombie               end
function OnProjectileMiss(proj)     -- Entity: Projectile                          end

-- Coin events
function OnCoinSpawn(coin)          -- Entity: Coin                                end
function OnCoinCollect(coin)        -- Entity: Coin                                end
function OnCoinExpire(coin)         -- Entity: Coin (faded out, about to be removed) end

-- Plant events (extended)
function OnPlantEaten(plant, zombie)    -- Entity: Plant, Zombie                   end
function OnPlantProduce(plant)          -- Entity: Plant (sunflower/marigold output) end
function OnPlantUpgrade(plant, oldType) -- Entity: Plant, int (imitater morph)     end

-- Zombie events (extended)
function OnZombieFrozen(zombie, isFrozen)    -- Entity: Zombie, bool               end
function OnZombieButtered(zombie)             -- Entity: Zombie                     end
function OnZombieMindControl(zombie)          -- Entity: Zombie                     end

-- Wave events (extended)
function OnFlagRaise(waveIndex)               -- int (flag wave raised)             end

-- Mower events
function OnMowerTriggered(row, mowerType)     -- int, int (LawnMowerType)           end

-- Game state events
function OnSunCountChange(oldAmount, newAmount) -- int, int                         end

-- UI events
function OnBoardButtonClick(button_id)                                             end
```

Notes:
- `OnPlantUpdate(plant)` fires every frame **only for mod plants** (seedType ≥ 2000). Always call `PlantHelper.Update(plant)` here if using any PlantHelper features.
- Errors in callbacks are caught and logged without crashing the game.

### 7.3 PlantHelper API — Built-in Helper Module

`PlantHelper` is a built-in module (no `require` needed) that provides reusable plant behavior patterns. It is loaded automatically by the game engine.

**Usage pattern:**

```lua
function OnPlantUpdate(plant)
    PlantHelper.Update(plant)  -- drives all PlantHelper behaviors for this plant
end
```

#### PlantHelper.SimpleAI(plant, state_table)

A lightweight state machine for custom plants. Each state can have animation, enter/update/exit/finish callbacks.

**State structure:**

| Field | Type | Description |
|-------|------|-------------|
| `animation` | string | Optional. Reanim track name to play on state entry |
| `loopType` | int | Optional. `ReanimLoopType` enum (default: `LOOP`) |
| `on_enter` | function(plant) | Called when state becomes active |
| `on_update` | function(plant) → string\|nil | Called every frame. Return a state name to switch, or nil to stay |
| `on_finish` | function(plant) → string\|nil | Called when a `PLAY_ONCE_AND_HOLD` animation finishes. Return next state |
| `on_exit` | function(plant) | Called when leaving this state |

**Example:**

```lua
function OnPlantSpawn(plant)
    PlantHelper.SimpleAI(plant, {
        idle = {
            animation = "anim_idle",
            on_update = function(p)
                if Board:FindTargetZombie(p) then
                    return "attacking"
                end
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
```

#### PlantHelper.AutoShooter(plant, config)

Automated shooting behaviour with configurable range, fire rate, and damage.

**Config fields:**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `range` | number | — | Maximum pixel distance to target |
| `fireRate` | number | — | Frames between shots |
| `projectileType` | int | — | `ProjectileType` enum or mod projectile ID |
| `projectileDamage` | int | nil (use default) | Override projectile damage |
| `onFire` | function(plant, target) | nil | Callback after each shot |

**Example:**

```lua
function OnPlantSpawn(plant)
    PlantHelper.AutoShooter(plant, {
        range = 400,
        fireRate = 60,
        projectileType = ProjectileType.PEA,
        projectileDamage = 30,
        onFire = function(p, target)
            Game.Log("Shot at zombie type " .. target.type)
        end
    })
end
```

#### PlantHelper.TimedAction(plant, frames, callback)

Schedule a one-shot callback to fire after `frames` frames.

```lua
function OnPlantUpdate(plant)
    if plant.age == 1 then
        local t = PlantHelper.TimedAction(plant, 60, function(p)
            Game.Log("60 frames have passed since plant was placed")
            p:Squish()
        end)
        -- Optional: cancel early
        -- t:Cancel()
    end
end
```

#### PlantHelper.RepeatAction(plant, interval, callback)

Schedule a repeating callback every `interval` frames.

```lua
function OnPlantSpawn(plant)
    local r = PlantHelper.RepeatAction(plant, 120, function(p)
        Game.Log("Tick every 120 frames")
    end)
    -- Cancel later: r:Cancel()
end
```

### 7.4 Constants

All game constants are registered as read-only global tables:

| Table | Example Values |
|-------|---------------|
| `PlantType` | `PEASHOOTER=0, SUNFLOWER=1, ..., IMITATER=48, NONE=-1, NUM_TYPES=49` |
| `ZombieType` | `NORMAL=0, FLAG=1, ..., REDEYE_GARGANTUAR=35, NUM_TYPES=36` |
| `ProjectileType` | `PEA=0, SNOWPEA=1, ..., ZOMBIE_PEA=13, NUM_TYPES=14` |
| `CoinType` | `NONE=0, SILVER=1, ..., PRESENT_SURVIVAL_MODE=27` |
| `GameMode` | `ADVENTURE=0, SURVIVAL_NORMAL_STAGE_1=1, ..., INTRO=69, NUM_MODES=70` |
| `PlantSubClass` | `NORMAL=0, SHOOTER=1` |
| `PlantState` | `NOTREADY=0, READY=1, ..., LILYPAD_INVULNERABLE=44` |
| `ZombiePhase` | `ZOMBIE_NORMAL=0, ZOMBIE_DYING=1, ..., YETI_RUNNING=82` |
| `ProjectileMotion` | `STRAIGHT=0, LOBBED=1, ..., HOMING=9` |
| `DamageFlags` | `BYPASSES_SHIELD=1, FREEZE=4, ...` |
| `BackgroundType` | `DAY=0, NIGHT=1, POOL=2, FOG=3, ROOF=4, BOSS=5` |
| `HelmType` | `NONE=0, TRAFFIC_CONE=1, ..., TALLNUT=9` |
| `ShieldType` | `NONE=0, DOOR=1, NEWSPAPER=2, LADDER=3` |
| `ReanimLoopType` | `LOOP=0, PLAY_ONCE=2, PLAY_ONCE_AND_HOLD=3, ...` |
| `GridConstants` | `COLS=9, ROWS=6, BOARD_WIDTH=800, BOARD_HEIGHT=600, LAWN_XMIN=40, LAWN_YMIN=80, BOARD_OFFSET=220, SEEDBANK_MAX=10` |
| `MessageStyle` | `OFF=0, TUTORIAL_LEVEL1=1, ..., HINT_FAST=6, BIG_MIDDLE=12, ...` |
| `FoleyType` | `SUN=0, SPLAT=1, ..., FROZEN=18, EXPLOSION=22, BUTTER=42, ...` |

## 8. Example Mods

### 8.1 `demo_plant` — Complete Custom Plant with SimpleAI

This is the recommended starting point for new mod authors. See `mods/demo_plant/` for the full source.

The plant, **Triple Pea**, fires three peas in a spread pattern when a zombie is in range. It demonstrates:

- `Game.RegisterPlant` / `Game.RegisterProjectile` — Content registration
- `PlantHelper.SimpleAI` — State machine with idle/attacking states
- `OnPlantUpdate` + `PlantHelper.Update` — Per-frame update
- `Board:FindTargetZombie` / `Board:AddProjectile` — Game interaction
- `entity:PlayBodyReanim` — Animation control

```lua
function OnModInit()
    Game.RegisterProjectile({
        id = "triple_pea_bullet",
        damage = 25,
        speed = 3.0,
        image = "IMAGE_REANIM_PEA_PROJECTILE"
    })

    Game.RegisterPlant({
        id = "triple_pea",
        name = "Triple Pea",
        cost = 200,
        cooldown = 750,
        subClass = 0,
        reanimation = "reanim/PeaShooterSingle.reanim",
        description = "Fires three peas in a spread. Demonstrates PlantHelper.SimpleAI."
    })
end

function OnPlantSpawn(plant)
    if plant.id == "triple_pea" then
        PlantHelper.SimpleAI(plant, {
            idle = {
                animation = "anim_idle",
                on_update = function(p)
                    if Board:FindTargetZombie(p) then
                        return "attacking"
                    end
                end
            },
            attacking = {
                animation = "anim_shoot",
                loopType = ReanimLoopType.PLAY_ONCE_AND_HOLD,
                on_enter = function(p)
                    Game.Log("Triple Pea attacking!")
                end,
                on_finish = function(p)
                    -- Fire three peas in a spread
                    local cx, cy, row = p.x, p.y, p.row
                    Board:AddProjectile(cx, cy, row, ProjectileType.PEA)
                    Board:AddProjectile(cx - 10, cy, row, ProjectileType.PEA)
                    Board:AddProjectile(cx + 10, cy, row, ProjectileType.PEA)
                    return "idle"
                end
            }
        })
    end
end

function OnPlantUpdate(plant)
    if plant.id == "triple_pea" then
        PlantHelper.Update(plant)
    end
end
```

### 8.2 Basic Mod Template

```lua
function OnModInit()
    Game.Log("My Mod initialized")
end

function OnLevelStart(mode_id)
    Game.Log("Level started, mode=" .. mode_id)
end

function OnZombieSpawn(zombie)
    Game.Log("Zombie spawned, HP: " .. zombie.hp)
end

function OnLevelEnd(is_win)
    Game.Log("Level ended, win=" .. tostring(is_win))
end
```

### 8.3 Custom Dialog Example

```lua
function OnLevelStart(mode_id)
    local dlg = UI.CreateDialog({
        title = "Level Hint",
        body = "This dialog was created by a Lua mod.",
        modal = true
    })

    dlg:AddButton("Spawn Zombie", function()
        Board.SpawnZombie(ZombieType.NORMAL, 2)
    end)

    dlg:AddButton("Close", function()
        Game.Log("Dialog closed")
    end)
end
```

### 8.4 Auto Collect Sun (Existing Mod)

See `mods/auto_collect_sun/scripts/main.lua`:

```lua
function OnCoinSpawn(coin)
    if coin:IsSun() then
        coin:Collect()
    end
end
```

### 8.5 Speed Control (Existing Mod)

See `mods/speed_control/scripts/main.lua` — demonstrates `Board.AddButton`, `Game.SetSpeed`, and `Game.SaveModData` / `Game.LoadModData`.

## 9. Save Data

- Original saves remain unchanged for compatibility.
- Mod-specific state is stored in:

```
<save_root>/modsave/<mod_id>.json
```

Access via Lua:
- `Game.SaveModData(key, value)`
- `Game.LoadModData(key)`

## 10. Hot Reload (Developer Mode)

Not implemented. Possible future developer-only feature:
- Reload Lua scripts and resource indexes.
- Require returning to title or restarting level.
- Avoid hot patching in-flight entities.

## 11. Error Handling

- Missing files: skip mod and log error.
- Invalid schema: skip record and log field name.
- Lua error in callbacks: caught and logged without crashing the game.

## 12. Versioning

- Not implemented. The current loader ignores schema/version gating beyond parsing the `version` string from `mod.json`.
- A future schema field may look like:

```json
"schema_version": 1
```

The loader does not currently warn if a mod uses a newer schema.

## 13. Future Extensions (Non-Blocking)

- ~~Custom UI panels for mods~~ (implemented: `UI.CreateDialog` / `UI.ShowMessage` with buttons and `Dialog:AddLabel`)
- JSON data tables for plants, zombies, projectiles, modes, and strings
- Per-mod Lua environments or module isolation
- Modular event filters and priorities
- Network-safe mod validation for competitive modes
- Dependency version ranges

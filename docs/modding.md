# PvZ-Portable Modding Specification (Draft v0.1)

This document defines a practical mod system for PvZ-Portable with Lua scripting and data-driven content.
The goal is to enable new plants, new zombies, and new modes while keeping cross-platform compatibility.

## 1. Design Goals

- Cross-platform: must work on desktop, mobile, consoles, and WASM without JIT.
- Deterministic: mods should not destabilize core gameplay.
- Data-first: content should be defined in data tables, scripts add behavior.
- Safe fallback: missing data or script errors should fail gracefully.
- Save compatibility: preserve original saves, store mod data separately.

## 2. Mod Package Layout

All mods live under the game resource root:

```
mods/
  <mod_id>/
    mod.json
    data/
      plants.json
      zombies.json
      modes.json
      projectiles.json
      strings.json
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
  "entry": "scripts/main.lua",
  "data": {
    "plants": ["data/plants.json"],
    "zombies": ["data/zombies.json"],
    "modes": ["data/modes.json"],
    "projectiles": ["data/projectiles.json"],
    "strings": ["data/strings.json"]
  }
}
```

Fields:
- `priority`: higher loads later and overrides earlier content.
- `dependencies`: list of mod ids required before loading this mod.
- `entry`: Lua entry file executed after data/asset registration.
- `data`: optional lists of data tables. Missing sections are allowed.

## 4. Loading Order

1) Scan `mods/` and parse all `mod.json`
2) Validate ids, detect duplicates
3) Topological sort by dependencies; stable sort by `priority`
4) For each mod:
   - Register its resource root (overlay)
   - Load data tables and register new content
   - Run Lua `entry` and call `OnModInit`

Conflicts:
- If two mods define the same `id`, the later mod overrides by default.
- The loader should log overrides with mod ids and file paths.

## 5. Data Tables

All tables are arrays of objects. Unknown fields are ignored.

### 5.1 Plants (data/plants.json)

```json
[
  {
    "id": "pea_shooter_plus",
    "seed_type": 2001,
    "name_key": "PEA_SHOOTER_PLUS_NAME",
    "desc_key": "PEA_SHOOTER_PLUS_DESC",
    "cost": 125,
    "cooldown": 7.5,
    "hp": 300,
    "attack": {
      "type": "projectile",
      "damage": 20,
      "rate": 1.5,
      "projectile_id": "pea_plus"
    },
    "animation": {
      "reanim": "REANIM_PEA_SHOOTER",
      "atlas": "reanim/peashooter_plus.atlas"
    },
    "ui": {
      "seedpacket_image": "IMAGE_SEEDPACKET_PEA_PLUS",
      "icon_image": "IMAGE_ICON_PEA_PLUS"
    }
  }
]
```

Required fields:
- `id`, `seed_type`, `cost`, `cooldown`

Reserved ranges:
- `seed_type` >= 2000 for mods

### 5.2 Zombies (data/zombies.json)

```json
[
  {
    "id": "conehead_fast",
    "zombie_type": 3001,
    "name_key": "CONEHEAD_FAST_NAME",
    "desc_key": "CONEHEAD_FAST_DESC",
    "hp": 600,
    "speed": "fast",
    "damage": 100,
    "animation": {
      "reanim": "REANIM_CONEHEAD_ZOMBIE",
      "atlas": "reanim/conehead_fast.atlas"
    },
    "ui": {
      "almanac_image": "IMAGE_ALMANAC_CONEHEAD_FAST"
    }
  }
]
```

Required fields:
- `id`, `zombie_type`, `hp`, `speed`

Reserved ranges:
- `zombie_type` >= 3000 for mods

### 5.3 Projectiles (data/projectiles.json)

```json
[
  {
    "id": "pea_plus",
    "damage": 20,
    "speed": 7.5,
    "image": "IMAGE_PROJECTILE_PEA_PLUS"
  }
]
```

### 5.4 Modes (data/modes.json)

```json
[
  {
    "id": "rush_mode",
    "name_key": "RUSH_MODE_TITLE",
    "desc_key": "RUSH_MODE_DESC",
    "base_mode": "SURVIVAL",
    "flags": {
      "fast_zombies": true,
      "no_lawnmowers": true
    },
    "waves": [
      { "time": 30, "zombies": ["zombie_basic", "conehead_fast"] }
    ]
  }
]
```

Required fields:
- `id`, `base_mode`

### 5.5 Strings (data/strings.json)

```json
{
  "PEA_SHOOTER_PLUS_NAME": "Pea Shooter Plus",
  "PEA_SHOOTER_PLUS_DESC": "Shoots stronger peas.",
  "CONEHEAD_FAST_NAME": "Conehead Sprinter",
  "CONEHEAD_FAST_DESC": "Faster but still tough."
}
```

Strings can also be provided via `resources/properties/default.xml`.
If both exist, `default.xml` has higher priority.

## 6. Resource Overlay Rules

- The resource manager should search mod resources before base resources.
- Overlay scope is per mod, but the final lookup order is mod load order.
- If a resource is missing, fall back to base resources.

Recommended paths inside `resources/`:
- `images/` for PNG or atlas textures
- `sounds/` for audio
- `reanim/` for animation atlases
- `properties/` for XML string overrides

## 7. Lua API (Minimal)

### 7.1 Global Objects

`Game`:
- `Game.Log(text)` — Log a message
- `Game.RegisterPlant(def)` — Register a custom plant
- `Game.RegisterZombie(def)` — Register a custom zombie
- `Game.RegisterMode(def)` — Register a custom game mode
- `Game.RegisterProjectile(def)` — Register a custom projectile
- `Game.GetMode()` — Get the current game mode ID
- `Game.SaveModData(key, value)` — Persist a string key-value pair for this mod
- `Game.LoadModData(key)` — Load a saved value; returns `nil` if not found

`Board`:
- `Board.SpawnZombie(zombie_id, row)` — Spawn a zombie in the given row. `zombie_id` can be a string (mod registration ID) or integer (runtime type). Returns Entity or nil.
- `Board.SpawnPlant(plant_id, row, col)` — Plant at row/col. `plant_id` can be a string or integer. Returns Entity or nil.
- `Board.GetWave()` — Current wave index

`Entity` (wrapper for a plant or zombie):
- `entity.id` — Entity ID (placeholder, always 0)
- `entity.type` — Entity type as integer (`SeedType` or `ZombieType`)
- `entity.hp` — Current health (`mPlantHealth` or `mBodyHealth`)
- `entity:Damage(amount)` — Deal damage; auto-triggers death at zero HP

`UI` (custom dialog system):
- `UI.CreateDialog(opts)` — Create and show a custom dialog. `opts` is a table: `{ title=..., body=..., modal=... }`. Returns a Dialog userdata.
- `UI.ShowMessage(title, body)` — Convenience: create a modal message box with a single OK button. Returns a Dialog userdata.

`Dialog` (userdata returned by `UI.CreateDialog` / `UI.ShowMessage`):
- `dialog:AddButton(text, callback)` — Add a button with a Lua closure callback. When clicked, the callback runs and the dialog is automatically closed.
- `dialog:Close()` — Manually close and destroy the dialog.
- `dialog:SetTitle(text)` — Change the dialog title.
- `dialog:SetBody(text)` — Change the dialog body text.

### 7.2 Lua Callbacks

All callbacks are optional. If a callback is not defined, it is silently skipped. The `plant`/`zombie` parameters are Entity objects.

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
```

Callback parameters:
- `mode_id`: integer, current game mode
- `wave_index`: integer, wave number
- `plant` / `zombie` / `target`: Entity objects with `.type`, `.hp` and `:Damage()` method
- `is_win`: boolean, `true` if level won, `false` if lost

Notes:
- Callbacks are optional.
- Errors are caught and logged without crashing the game.

## 8. Example Lua Entry

### 8.1 Basic Example

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

### 8.2 Custom Dialog Example

```lua
function OnLevelStart(mode_id)
  local dlg = UI.CreateDialog({
    title = "Level Hint",
    body = "This dialog was created by a Lua mod.",
    modal = true
  })

  dlg:AddButton("Spawn Zombie", function()
    Board.SpawnZombie(0, 2)
  end)

  dlg:AddButton("Close", function()
    Game.Log("Dialog closed")
  end)
end

function OnZombieSpawn(zombie)
  Game.Log("Zombie spawned, HP: " .. tostring(zombie.hp))
end
```

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

Optional developer-only feature:
- Reload data tables, resources, and Lua scripts.
- Require returning to title or restarting level.
- Avoid hot patching in-flight entities.

## 11. Error Handling

- Missing files: skip mod and log error.
- Invalid schema: skip record and log field name.
- Lua error: disable the callback for the current frame and log.

## 12. Versioning

- This document defines `mod_schema_version = 1`.
- Mods may optionally set:

```json
"schema_version": 1
```

The loader should warn if a mod uses a newer schema.

## 13. Future Extensions (Non-Blocking)

- ~~Custom UI panels for mods~~ (partially implemented: `UI.CreateDialog` / `UI.ShowMessage` with buttons and Lua closure callbacks; TODO: text labels, input fields)
- Modular event filters and priorities
- Network-safe mod validation for competitive modes
- Dependency version ranges

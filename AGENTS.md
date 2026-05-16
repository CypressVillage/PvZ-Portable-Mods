# PvZ-Portable — Agent Quick Reference

## What This Project Is

A cross-platform community reimplementation of Plants vs. Zombies GOTY Edition (v1.2.0.1073) using C++20, SDL2, and OpenGL ES 2.0. Runs on Linux, Windows, macOS, Android, iOS, WASM, Switch, etc. License: LGPL-3.0-or-later.

**User intent**: Develop a Lua/JSON-based Mod framework for this game, delivered as the `mods/` system with C++ scaffolding under `src/Mod/`.

---

## Build System

- **C++ standard**: C++20
- **Build**: `cmake -G Ninja -B build -DPVZ_ENABLE_LUA=ON -DCMAKE_BUILD_TYPE=Release` then `cmake --build build`
- **Key CMake options**:
  - `PVZ_ENABLE_LUA=ON` — enables mod Lua support (links `lua5.4` / liblua)
  - `PVZ_DEBUG=ON` — enables cheat keys, debug features
  - `DO_FIX_BUGS=ON` — community bug fixes
  - `LIMBO_PAGE=ON` — limbo level access (default ON)
- **Dependencies** (system or vcpkg): SDL2, libpng, libjpeg-turbo, zlib, libopenmpt, libogg, libvorbis, mpg123, lua (when enabled)
- **Rebuild after C++ changes**: `cd build && ninja`

---

## Source Layout (High-Level)

```
src/
  main.cpp                 — entry point; creates LawnApp, Init/Start
  LawnApp.h/.cpp           — main game application (inherits SexyApp)
  Lawn/                    — game logic: Board, Plant, Zombie, Coin, Projectile, widgets
  Lawn/Widget/             — UI widgets: SeedChooserScreen, AlmanacDialog, LawnDialog, GameButton
  Lawn/System/             — system utilities
  SexyAppFramework/        — engine (rendering, audio, resource mgmt, widget system, paklib)
  Sexy.TodLib/             — TodLib utilities (string file, debug/logging)
  Mod/                     — MOD FRAMEWORK (the focus area)
    ModLoader.h/.cpp       — mod scanning, manifest parsing, dependency sort, string override loading
    ModLua.h/.cpp          — Lua VM init, API bindings (Game/Board/Entity/UI), event hooks, script execution
    ModRegistry.h/.cpp     — dynamic data registry for plants/zombies/modes/projectiles, reanim registration
    ModSave.h/.cpp         — mod-specific key-value persistence in modsave/<mod_id>.json
    LuaProxyDialog.h/.cpp  — dialog proxy for Lua-created UI (modals with buttons, Lua callbacks)
    ModRegistry_patch.cpp  — registry integration patches into game engine
GameConstants.h            — game constants, enum defs (SeedType, ZombieType, etc.)
ConstEnums.h               — enum definitions
Resources.h/.cpp           — resource extraction
```

---

## Mod System Architecture (Phases & Status)

Per `TODO_ModSystem.md` — 8 phases, 6 fully complete, 2 partially:

| # | Phase | Status |
|---|-------|--------|
| 1 | Mod Loader (scan, manifest parse, dependency sort) | DONE |
| 2 | Resource Overlay & String Localization | DONE |
| 3 | Data Registry (plants, zombies, modes, projectiles) | DONE |
| 4 | Lua VM & API Bindings (Game, Board, Entity) | DONE |
| 5 | Event Hooks (10 lifecycle callbacks) | DONE |
| 6 | Mod Save & Save Data Isolation | DONE |
| 7 | Entity Extension & Deep UI Adaptation | PENDING — expand anim/resource mapping, unbundle UI from hardcoded limits |
| 8 | Custom UI Dialogs (LuaProxyDialog) | MOSTLY DONE — `dialog:AddLabel()` still pending |

Mod plant IDs: runtime `seedType >= 2000`; zombie IDs: `>= 3000`.

---

## Key Mod Source Files

| File | Purpose |
|------|---------|
| `src/Mod/ModLoader.h` | `ModManifest` struct, `ModLoader` class — reads `mod.json`, sorts by dependency+priority |
| `src/Mod/ModLua.h/cpp` | Full Lua binding: `Game.*`, `Board.*`, `Entity.*`, `UI.*`, `Dialog.*`, all callback hooks |
| `src/Mod/ModRegistry.h/cpp` | `ModPlantDef`, `ModZombieDef`, `ModModeDef`, `ModProjectileDef` — CRUD + reanim resolution |
| `src/Mod/ModSave.h/cpp` | File-based KV store per mod (`modsave/<mod_id>.json`) |
| `src/Mod/LuaProxyDialog.h/cpp` | `LuaProxyDialog` — dynamic dialog from Lua, button callbacks via `luaL_ref` |
| `src/Mod/ModRegistry_patch.cpp` | Patches into engine (Almanac, SeedChooser, Board, ReanimationType resolution) |

---

## Mod Package Format

```
mods/<mod_id>/
  mod.json                  — id, name, version, priority, dependencies, entry, data
  data/
    plants.json / *.json    — one plant per JSON file (id, cost, cooldown, reanimation/reanimFile)
    zombies.json            — zombie definitions (hp, speed, animation)
    modes.json              — game mode definitions
    projectiles.json        — projectile definitions
    strings.json            — string overrides (key→value)
  scripts/
    main.lua                — entry Lua (callbacks: OnModInit, OnLevelStart, OnZombieSpawn, etc.)
  resources/
    images/  sounds/  reanim/  properties/default.xml
```

Lua API: `Game.Log/RegisterPlant/SaveModData/LoadModData/SetSpeed/GetMenuButtonRect`, `Board.SpawnZombie/SpawnPlant/AddButton/RemoveButton/SetButtonLabel/GetWave`, `Entity:Damage/IsSun/Collect`, `UI.CreateDialog/ShowMessage`, `Dialog:AddButton/Close/SetTitle/SetBody`.

Event callbacks: `OnModInit`, `OnGameStart`, `OnLevelStart(mode)`, `OnWaveStart(wave)`, `OnPlantSpawn(plant)`, `OnZombieSpawn(zombie)`, `OnPlantAttack(plant,target)`, `OnZombieDie(zombie)`, `OnLevelEnd(isWin)`, `OnCoinSpawn(coin)`, `OnBoardButtonClick(btnId)`.

Custom plants support external reanim XML (bare `<track>` elements, no XML declaration or root wrapper) or reuse vanilla reanim paths. See `docs/custom-plants.md`.

---

## Debugging

- **Game log**: `~/.local/share/io.github.wszqkzqk/PvZPortable/userdata/log.txt`
- **C++ logging**: `TodLog("format", ...)` — requires `TodAssertInitForApp()` initialized
- **Lua logging**: `Game.Log("msg")` — writes `[Lua]` prefixed to log file
- **Mod save data**: `~/.local/share/io.github.wszqkzqk/PvZPortable/modsave/<mod_id>.json`
- Running from build dir: symlink `mods/` directory (`ln -s ../mods ./mods`) so engine finds mods
- See `DEBUGGING.md` for more

---

## Coding Conventions

- **Naming**: `m` prefix for members, `the` for parameters, `a` for locals (SexyAppFramework convention)
- **Namespaces**: `Sexy::` for framework code, game logic in global scope or `Lawn`-prefixed
- **Style**: public member data without accessor methods; minimal comments
- **No reverse-engineered code accepted** — only community-research-based reimplementation
- **Save compatibility**: `v4` mid-level save format (TLV), not raw memory dumps

---

## Existing Example Mods

| Mod | Purpose |
|-----|---------|
| `example_mod` | Validates pipeline (strings, logging, OnZombieSpawn/OnZombieDie) |
| `auto_collect_sun` | Auto-collects suns via `OnCoinSpawn` + `coin:Collect()` |
| `demo_plant` | Custom plants via JSON definitions (fire_pea, ice_melon, custom_shooter with external reanim) |
| `speed_control` | Speed button overlay using `Board.AddButton` + `Game.SetSpeed` + save persistence |
| `ui_test_mod` | Exercises `UI.CreateDialog` / `UI.ShowMessage` / `Dialog:AddButton` |

---

## Docs Folder

- `docs/modding.md` — full modding specification (English)
- `docs/modding.zh-CN.md` — Chinese version of modding spec
- `docs/custom-plants.md` — custom plants guide (JSON schema, reanim XML format)
- `docs/almanac_display_and_seed_alignment_bug.md` — known UI bug notes
- `docs/seed_chooser_pagination_crash.md` — pagination crash context
- `docs/seed_chooser_page2_and_hit_test_bug.md` — hit test bug on page 2

---

## Current Development Focus

The user is building out the mod framework. Remaining high-priority items from `TODO_ModSystem.md`:
- Phase 7: Entity extension — complete field mapping, dynamic animation/resource registration, UI unbundling from hardcoded limits
- Phase 8: `dialog:AddLabel(text)` Lua binding
- Hot reload (`-moddev` flag) — reload data/scripts/resources without restart

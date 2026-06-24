# PvZ-Portable — Agent Quick Reference

**Reminder**: After completing each task, ask the user whether to update relevant documentation.

## What This Project Is

A cross-platform community reimplementation of Plants vs. Zombies GOTY Edition (v1.2.0.1073) using C++20, SDL2, and OpenGL ES 2.0. Runs on Linux, Windows, macOS, Android, iOS, WASM, Switch, etc. License: LGPL-3.0-or-later.

**User intent**: Develop a Lua-based Mod framework, delivered as the `mods/` system with C++ scaffolding under `src/Mod/`.

**Current verified status (2026-06-24)**: The Lua Mod framework is real and builds with `PVZ_ENABLE_LUA=ON`. The implemented content pipeline is `mod.json` + Lua registration APIs + `resources/` overlays. JSON data tables such as `data/plants.json`, `data/zombies.json`, `data/modes.json`, `data/projectiles.json`, and `strings.json` are not loaded by the current engine.

---

## Build System

- **C++ standard**: C++20
- **Build**: `cmake -G Ninja -B build -DPVZ_ENABLE_LUA=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build`
- **Key CMake options**: `PVZ_ENABLE_LUA=ON`, `PVZ_DEBUG=ON`, `DO_FIX_BUGS=ON`, `LIMBO_PAGE=ON`
- **Rebuild after C++ changes**: `cd build && ninja`
- **Last verified local build**: `cmake --build build` succeeded on 2026-06-24 with `PVZ_ENABLE_LUA=ON`, `CMAKE_BUILD_TYPE=Release`

---

## Source Layout

```
src/
  main.cpp             — entry point
  LawnApp.h/.cpp       — main game app (inherits SexyApp)
  Lawn/                — game logic: Board, Plant, Zombie, Coin, Projectile
  Lawn/Widget/         — UI: SeedChooserScreen, AlmanacDialog, LawnDialog, GameButton
  Lawn/System/         — system utilities
  SexyAppFramework/    — engine (rendering, audio, widget system, resource mgmt, paklib)
  Sexy.TodLib/         — animation (Reanimator), particles, string file, debug
  Mod/                 ★ MOD FRAMEWORK (focus)
    ModLoader.h/.cpp       — scan mods/, parse manifest, dependency sort
    ModLua.h/.cpp          — Lua 5.4 VM, API bindings, event hooks
    ModTimer.h/.cpp        — frame-based timer pool (C++, replaces former _timer.lua)
    ModRegistry.h/.cpp     — dynamic data registry (plants/zombies/modes/projectiles)
    ModSave.h/.cpp         — per-mod KV persistence (modsave/<id>.json)
    LuaProxyDialog.h/.cpp  — Lua-created dialog proxy
```

Mod plant IDs: `seedType >= 2000`; zombie IDs: `>= 3000`; projectile IDs: `>= 4000`; mode IDs: `>= 5000`. All auto-assigned on registration.

Current Mod system limitations to remember:
- `priority` is parsed but cannot be relied on for conflict/override ordering.
- Missing dependencies are not fatal; only dependencies that exist are used for topological ordering.
- Duplicate manifest ids are not strictly rejected by `ModLoader`.
- Duplicate plant/zombie/mode/projectile registration ids are rejected by `ModRegistry`; later mods do not override earlier registrations.
- All mods currently share one Lua VM/global environment. Entry callbacks are captured after each mod loads, but globals can still collide; prefer `local` in examples and docs.
- Hot reload, dependency version constraints, schema gating, JSON data tables, and per-mod Lua environments are not implemented.
- Reanim XML uses element-style fields (`<track><name>...</name>`, `<t><f>0</f></t>`), not attribute-style XML (`<track name="...">`, `<t f="0"/>`).
- For mod resource images, paths normally resolve through registered `resources/` roots; use paths like `images/foo.png`, while `reanimFile` is relative to the mod root such as `resources/reanim/foo.reanim`.

---

## Debugging

- **Game log**: `~/.local/share/io.github.wszqkzqk/PvZPortable/userdata/log.txt`
- **C++ logging**: `TodLog("fmt", ...)` (after `TodAssertInitForApp()`)
- **Lua logging**: `Game.Log("msg")` → `[Lua]` prefix in log
- **Mod save data**: `<appdata>/modsave/<mod_id>.json`
- **Symlink for build dir**: `ln -s ../mods ./mods`

---

## Coding Conventions

- **Naming**: `m` prefix for members, `the` for params, `a` for locals
- **Namespaces**: `Sexy::` for framework; game logic global or `Lawn`-prefixed
- **Style**: public member data, minimal comments
- **Save format**: v4 TLV (not memory dumps)
- **No reverse-engineered code** — community-research-based only

---

## Example Mods

| Mod | What it demonstrates |
|-----|---------------------|
| `auto_collect_sun` | `OnCoinSpawn` + `coin:Collect()` |
| `demo_plant` | `Game.RegisterPlant` + `PlantHelper.SimpleAI` state machine (triple shot) |
| `peashooter_plus` | `Game.RegisterPlant` + `Game.RegisterProjectile` + vanilla reanim reuse + custom projectile image reference |
| `speed_control` | `Board.AddButton` + `Game.SetSpeed` + mod save |
| `_test_helpers` | End-to-end validation of PlantHelper module (SimpleAI, AutoShooter, TimedAction, RepeatAction) |

---

## Documentation

| Path | Content |
|------|---------|
| `docs/modding.md` | Current implementation modding spec (English) — APIs, callbacks, constants, PlantHelper, examples, known limitations |
| `docs/modding.zh-CN.md` | Current implementation modding spec (Chinese) |
| `docs/custom-plants.md` | Current custom plant Lua API + element-style Reanim XML guide |
| `docs/pvz-animation.md` | Reanim animation system reference (peashooter example, element-style XML) |
| `docs/ARCHITECTURE.md` | Current Mod framework architecture, module details, engine patch points, known limitations |
| `docs/TODO.md` | Historical task list plus current verified limitations; completed items are not API stability promises |
| `docs/bug-fixes/` | Resolved bug postmortems (Seed Chooser / Almanac) |

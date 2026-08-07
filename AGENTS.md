# PvZ-Portable — Agent Quick Reference

**Reminder**: After completing each task, ask the user whether to update relevant documentation.

## What This Project Is

A cross-platform community reimplementation of Plants vs. Zombies GOTY Edition (v1.2.0.1073) using C++20, SDL2, and OpenGL ES 2.0. Runs on Linux, Windows, macOS, Android, iOS, WASM, Switch, etc. License: LGPL-3.0-or-later.

**User intent**: Develop a Lua-based Mod framework, delivered as the `mods/` system with C++ scaffolding under `src/Mod/`.

**Current verified status (2026-08-07)**: The stabilized Lua Mod framework builds and passes CTest in both Debug and Release with `PVZ_ENABLE_LUA=ON`. The implemented content pipeline is `mod.json` + Lua registration APIs + `resources/` overlays. JSON data tables such as `data/plants.json`, `data/zombies.json`, `data/modes.json`, `data/projectiles.json`, and `strings.json` are not loaded by the current engine.

---

## Build System

- **C++ standard**: C++20
- **Build and test**: `cmake -G Ninja -B build -DPVZ_ENABLE_LUA=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure`
- **Key CMake options**: `PVZ_ENABLE_LUA=ON`, `PVZ_DEBUG=ON`, `DO_FIX_BUGS=ON`, `LIMBO_PAGE=ON`
- **Rebuild after C++ changes**: `cd build && ninja`
- **Lua requirement**: Lua 5.4 or a compatible newer version; Lua-enabled Emscripten builds are explicitly unsupported
- **Last verified local builds**: Debug and Release builds plus `pvz.mod` CTest passed on 2026-08-07 with `PVZ_ENABLE_LUA=ON`

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
tests/
  mod_core_tests.cpp       — JSON, loader, ModSave, and Lua helper tests
  plant_helper_tests.lua   — PlantHelper behavior tests
```

Mod plant IDs: `seedType >= 2000`; zombie IDs: `>= 3000`; projectile IDs: `>= 4000`; mode IDs: `>= 5000`. All auto-assigned on registration.

Current Mod system limitations to remember:
- `priority` deterministically orders dependency-ready Mods by ascending value and id, but does not provide override/conflict semantics.
- Missing dependencies are not fatal; only dependencies that exist are used for topological ordering.
- Invalid and duplicate manifest ids are rejected; circularly dependent Mods are reported and omitted.
- Duplicate plant/zombie/mode/projectile registration ids are rejected by `ModRegistry`; later mods do not override earlier registrations.
- All mods currently share one Lua VM/global environment. Captured handlers, timers, and dialogs restore their owning Mod context, but ordinary globals can still collide; prefer `local` in examples and docs.
- Hot reload, dependency version constraints, schema gating, JSON data tables, and per-mod Lua environments are not implemented.
- Custom modes map to a vanilla `baseMode` for engine rules and intentionally do not use vanilla mid-level save slots or create progression/achievement records.
- Custom zombie health, speed, damage, body state, name, and Reanim are applied; helmet and shield equipment is rejected until safe attachment/track semantics are implemented.
- Vanilla mid-level saves serialize dynamic entity types as numeric IDs, so changing enabled Mods or registration order can invalidate those references.
- Reanim XML uses element-style fields (`<track><name>...</name>`, `<t><f>0</f></t>`), not attribute-style XML (`<track name="...">`, `<t f="0"/>`).
- External Reanims are queued during Lua registration and finalized after resource roots and the base Reanimator initialize. Image paths resolve through registered `resources/` roots, while `reanimFile` is relative to the Mod root, such as `resources/reanim/foo.reanim`.

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

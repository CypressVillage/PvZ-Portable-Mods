# PvZ-Portable — Agent Quick Reference

**Reminder**: After completing each task, ask the user whether to update relevant documentation.

## What This Project Is

A cross-platform community reimplementation of Plants vs. Zombies GOTY Edition (v1.2.0.1073) using C++20, SDL2, and OpenGL ES 2.0. Runs on Linux, Windows, macOS, Android, iOS, WASM, Switch, etc. License: LGPL-3.0-or-later.

**User intent**: Develop a Lua-based Mod framework, delivered as the `mods/` system with C++ scaffolding under `src/Mod/`.

---

## Build System

- **C++ standard**: C++20
- **Build**: `cmake -G Ninja -B build -DPVZ_ENABLE_LUA=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build`
- **Key CMake options**: `PVZ_ENABLE_LUA=ON`, `PVZ_DEBUG=ON`, `DO_FIX_BUGS=ON`, `LIMBO_PAGE=ON`
- **Rebuild after C++ changes**: `cd build && ninja`

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
    ModRegistry.h/.cpp     — dynamic data registry (plants/zombies/modes/projectiles)
    ModSave.h/.cpp         — per-mod KV persistence (modsave/<id>.json)
    LuaProxyDialog.h/.cpp  — Lua-created dialog proxy
    ModRegistry_patch.cpp  — registry integrations into engine
```

Mod plant IDs: `seedType >= 2000`; zombie IDs: `>= 3000`; projectile IDs: `>= 4000`; mode IDs: `>= 5000`. All auto-assigned on registration.

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
| `example_mod` | Pipeline validation (strings, logging, zombie hooks) |
| `auto_collect_sun` | `OnCoinSpawn` + `coin:Collect()` |
| `demo_plant` | Custom plant via `Game.RegisterPlant()` + external reanim XML |
| `peashooter_plus` | `Game.RegisterPlant` + `Game.RegisterProjectile` + custom reanim |
| `speed_control` | `Board.AddButton` + `Game.SetSpeed` + mod save |
| `ui_test_mod` | `UI.CreateDialog` / `UI.ShowMessage` / `Dialog:AddButton` |

---

## Documentation

| Path | Content |
|------|---------|
| `docs/modding.md` | Full modding specification (English) |
| `docs/modding.zh-CN.md` | Modding spec (Chinese) |
| `docs/custom-plants.md` | Custom plant Lua API + reanim XML guide |
| `docs/ARCHITECTURE.md` | Mod framework architecture, module details, engine patch points |
| `docs/TODO.md` | Remaining dev tasks (Phase 7, Phase 8, hot reload) |
| `docs/bug-fixes/` | Resolved bug postmortems (Seed Chooser / Almanac) |

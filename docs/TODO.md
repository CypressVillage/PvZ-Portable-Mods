# Mod System Development TODO

All 8 phases complete.

## ✅ Completed (Phases 1–6)

| Phase | Description |
|-------|-------------|
| 1 | Mod Loader — scan, manifest parse, dependency sort |
| 2 | Resource Overlay & String Localization |
| 3 | Data Registry — plants, zombies, modes, projectiles |
| 4 | Lua VM & API Bindings — Game, Board, Entity, UI |
| 5 | Event Hooks — 11 lifecycle callbacks |
| 6 | Mod Save & Data Isolation |

## ✅ Phase 7: Entity Extension & Deep UI Adaptation

- [x] `ModPlantDef` field mapping (cost, cooldown, fire rate → `GetPlantDefinition()`) — `Plant.cpp:4975`
- [x] Dynamic `ReanimationType` registration for new entities — `ModRegistry.cpp:223`
- [x] Dynamic texture/image loading for mod plants & projectiles — `GetPlantDefinition()` + `Projectile::Draw()`
- [x] SeedChooserScreen pagination (page buttons, page-based draw/hittest) — `SeedChooserScreen.cpp`
- [x] Unbind remaining hardcoded limits: `CrazyDavePickSeeds()`, `PickRandomSeeds()`, `PreloadForUser()`, `CutScene`, `GetNumPreloadingTasks()` — all use dynamic `GetTotalAlmanacPlants()`
- [x] Deep behavior hooks (`OnPlantUpdate`, `OnGameStart`, `OnWaveStart`) for Lua-driven custom plant logic — 12 hooks total

## ✅ Phase 8: Custom UI Dialogs

- [x] `dialog:AddLabel(text)` — static text label on LuaProxyDialog

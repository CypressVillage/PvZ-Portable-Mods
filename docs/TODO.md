# Mod System Development TODO

8 phases, **6 complete**, 2 pending + hot reload.

## ✅ Completed (Phases 1–6)

| Phase | Description |
|-------|-------------|
| 1 | Mod Loader — scan, manifest parse, dependency sort |
| 2 | Resource Overlay & String Localization |
| 3 | Data Registry — plants, zombies, modes, projectiles |
| 4 | Lua VM & API Bindings — Game, Board, Entity, UI |
| 5 | Event Hooks — 11 lifecycle callbacks |
| 6 | Mod Save & Data Isolation |

## ⏳ Phase 7: Entity Extension & Deep UI Adaptation

- [ ] Extend `ModPlantDef` field mapping (cost, cooldown, fire rate → engine Definition)
- [ ] Dynamic `ReanimationType` & texture registration for new entities
- [ ] Unbind UI from hardcoded limits (`NUM_SEED_TYPES` → dynamic pagination/scroll)
- [ ] Deep behavior hooks (`OnPlantUpdate`, etc.) for Lua-driven custom plant logic

## ⏳ Phase 8: Custom UI Dialogs

- [ ] `dialog:AddLabel(text)` — static text label on LuaProxyDialog

## 🔄 Hot Reload (`-moddev` flag)

- [ ] CLI arg detection for dev mode
- [ ] Reload data tables, Lua scripts, resource indexes at runtime
- [ ] State validation (main menu / non-combat only)

---

## Known Bugs

See `docs/bugs/` for Seed Chooser / Almanac pagination & hit-test issues.

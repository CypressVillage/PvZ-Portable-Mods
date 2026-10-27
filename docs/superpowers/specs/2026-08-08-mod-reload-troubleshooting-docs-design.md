# Mod Reload Troubleshooting Documentation Design

## Goal

Add concise English and Simplified Chinese troubleshooting guides for the idle-main-menu Mod reload workflow. Readers should be able to identify the failure stage, find the relevant log or configuration file, and choose a safe recovery action without reading the implementation.

## Deliverables

- `docs/mod-reload-troubleshooting.md`
- `docs/mod-reload-troubleshooting.zh-CN.md`
- A troubleshooting link in `docs/modding.md`
- A troubleshooting link in `docs/modding.zh-CN.md`

The two guides must describe the same behavior and keep matching section order.

## Structure

1. Reload scope and prerequisites
2. Standard Apply workflow
3. Symptom, likely cause, and action table
4. Preflight failure behavior
5. Post-teardown activation failure and restart-required behavior
6. Vanilla `resources.xml` cache limitations
7. Configuration and log file locations
8. Ordered diagnostic checklist
9. Information to include in a bug report

## Required Behavior

- State that reload is only supported at an idle main menu with no active Board.
- Explain that preflight rescans manifests and dependencies and compiles enabled Lua entries before destructive teardown.
- Explain that preflight failure preserves the active runtime and attempts to restore the previous `modconfig.json` state.
- Explain that activation failure after teardown requires a process restart and does not attempt an unsafe runtime rollback.
- Explain that already-cached vanilla resources selected through `resources.xml` may still require a restart after an otherwise successful reload.
- Distinguish the active Mod state from staged configuration state.
- Reference the game log at `~/.local/share/io.github.wszqkzqk/PvZPortable/userdata/log.txt` and enabled state at `<appdata>/modconfig.json`.
- Recommend preserving logs and manifests before manually editing or deleting configuration files.

## Error Presentation

Use a symptom-first table with entries for:

- Apply is unavailable or does nothing
- Manifest or dependency validation failure
- Lua entry compilation failure
- `OnModInit` or registration failure
- `Mod reload failed; restart required`
- Reload succeeds but an image, sound, string, or Reanim appears unchanged
- The next launch enables a different Mod set than expected

Each entry provides a likely cause and the least destructive recovery action.

## Scope Exclusions

- No new runtime behavior or code changes
- No promise of in-level reload or single-Mod unload
- No platform-specific paths beyond the currently documented Linux log path and generic `<appdata>` notation
- No instructions that suggest deleting save data as a first-line recovery step
- No claim that vanilla resource overlays are always reloadable

## Validation

- Verify every lifecycle claim against `LawnApp::ReloadModsFromConfig()` and `ModLua::ValidateEntries()`.
- Verify paths and limitations against `AGENTS.md` and the existing Mod documentation.
- Ensure English and Chinese headings and troubleshooting entries match.
- Run `git diff --check` after documentation edits.
- Scan for stale statements claiming all Mod Manager changes require restart or that main-menu reload is unimplemented.

# PvZ-Portable Debugging & Development Notes

This document contains useful debugging techniques, file locations, and gotchas for developing and modding PvZ-Portable.

## 1. Log Files & Output
Unlike typical command-line applications, PvZ-Portable does not print most of its debug information to the standard output (stdout). Instead, it writes to a dedicated log file.
- **Log Location (Linux):** `~/.local/share/io.github.wszqkzqk/PvZPortable/userdata/log.txt`
- **C++ Logging:** Use `TodLog("format", ...)` to write to this file. 
- **Lua Logging:** Use `Game.Log("message")` in your mod scripts. This calls `TodLog` internally and prefixes the message with `[Lua]`.
- **Note on Initialization:** `TodLog` requires the logging system to be initialized via `TodAssertInitForApp()`. Any `TodLog` calls executed *before* this initialization (e.g., early mod loading previously) will be silently discarded.

## 2. Mod Development & Data
- **Mod Directory:** The engine looks for the `mods` folder in the resource path. If you are running the game from the `build` directory, make sure you symlink the `mods` directory (`ln -s ../mods ./mods`) so the engine can find it.
- **Mod Save Data:** When a mod calls `Game.SaveModData("key", "value")`, the data is saved in a dedicated folder separate from the main user data.
  - **Location (Linux):** `~/.local/share/io.github.wszqkzqk/PvZPortable/modsave/<mod_id>.txt`
- **Mod Loading Verification:** If you suspect a mod isn't loading but the game doesn't crash, check the `modsave` folder. If the mod writes default save data on `OnModInit` or `OnLevelStart`, the presence of its save file proves the Lua script executed successfully.

## 3. CMake & Compilation Macros
The behavior of the engine heavily depends on CMake flags defined in `CMakeCache.txt`.
- `PVZ_ENABLE_LUA=ON` : Required for loading Lua mods.
- `PVZ_DEBUG=ON` (and the `_PVZ_DEBUG` C++ macro) : Enables debug logging, assertions, and hidden features/cheats.
- **Building:** Use `cd build && ninja` (or `make`) to recompile after changing C++ source code.

## 4. Common Gotchas
- **Crash on Start / Missing Assets:** If the game fails to load `properties/resources.xml` or other assets, it is likely running in the wrong working directory. The working directory needs access to `main.pak` (or the unpacked resources) and `properties/`.
- **Silent Failures in Lua:** If a Lua script has a syntax error, `luaL_loadbuffer` or `lua_pcall` will fail. The engine catches these and outputs the error using `TodLog`. Always check `log.txt` if your mod's behavior isn't triggering.

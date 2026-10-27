# Main-Menu Mod Hot Reload Design

## Goal

Allow the native Mod manager to apply enabled-state changes without restarting
the process. The first implementation reloads the complete Mod content set at a
safe main-menu boundary; it does not unload individual Mods and does not reload
while a Board is active.

The implementation must also support the common development loop of editing an
enabled Mod's Lua entry, registrations, custom images, or external Reanimations
and pressing Apply to load the changes.

## Scope

The first version supports:

- applying dependency-aware enabled states from the main-menu Mod manager;
- rebuilding the shared Lua VM, handlers, timers, dialogs, and registrations;
- rebuilding dynamic plant, zombie, mode, and projectile definitions;
- refreshing custom images loaded through the Mod definition caches;
- rebuilding vanilla and dynamic Reanimation definitions after all live
  Reanimation instances have been destroyed;
- restoring base property maps before applying the new Mod string and property
  overrides;
- rebuilding Mod resource-root order for subsequent file loads;
- reporting success or leaving the application in a restart-required state.

The first version does not support:

- reload while a Board, level-loading operation, or another reload is active;
- individual Mod unload or per-Mod Lua environments;
- preserving Lua globals, timers, dialogs, entity handles, or other transient
  Mod runtime state across reload;
- reliable in-process replacement of already-loaded vanilla resources selected
  through `resources.xml` overlays;
- recovering the old runtime in-process after destructive activation has begun.

Mods that replace already-loaded vanilla resources may still require a process
restart before every overridden asset is updated. This limitation must be shown
in the Mod manager documentation and reload result messaging.

## Chosen Approach

Use the existing global `gModLoader`, `gModLua`, and `gModRegistry`. Add a single
reload coordinator to `LawnApp` rather than introducing a swappable `ModRuntime`
or implementing owner-scoped unload.

The Mod manager defers reload instead of executing it from
`ModManagerDialog::ButtonDepress()`. Apply saves the staged enabled state, closes
the dialog, and calls `LawnApp::RequestModReload()`. The application performs the
reload at the next main-loop safe point after deferred widget deletion has been
processed.

This approach deliberately trades transactional runtime rollback for a smaller,
auditable first implementation. Configuration is restored after a destructive
failure, but the process must then be restarted.

## Preconditions

`LawnApp` may start a reload only when all of the following are true:

- `mBoard` is null;
- startup and loading-thread work has completed successfully;
- no level transition or resource-loading operation is active;
- no earlier Mod reload is pending or running;
- the Mod manager was opened from the main-menu flow.

If a precondition is false, Apply must not modify the active runtime. It reports
that Mods can only be reloaded from an idle main menu.

## Reload Flow

### 1. Save and preflight

Before destructive teardown:

1. Keep the old disabled-ID set in memory.
2. Save the staged enabled state through the existing atomic ModConfig path.
3. Use a temporary `ModLoader` instance to rescan manifests and resolve
   dependencies without replacing the active `gModLoader` contents.
4. Compile every enabled Lua entry in a temporary Lua state without executing
   it. This validates entry-file existence and Lua syntax only.

Manifest, dependency, entry-file, or Lua syntax failures occur before teardown. On
such a failure, write the old disabled-ID set back to ModConfig and leave the
existing loader, Lua VM, registry, resources, and UI unchanged.

Preflight is intentionally not a second Mod runtime. It does not execute entry
files or stage registrations, so runtime registration errors can still occur
during activation.

### 2. Enter the reload safe point

After preflight succeeds:

1. Close the Mod manager and process deferred widget deletion.
2. Mark the application as reloading and suppress new dialogs and game-start
   actions.
3. Destroy content-dependent front-end screens, including GameSelector,
   ChallengeScreen, SeedChooserScreen, and any Almanac or Mod-created dialogs.
4. Destroy TitleScreen before clearing the global effect holders, then recreate
   it after activation.

No widget or effect that can retain a dynamic definition, image, Lua callback,
or Reanimation pointer may survive into the definition reset.

### 3. Tear down old Mod state

Teardown order is strict:

1. Invalidate and close Lua-created dialogs.
2. Shut down `ModLua`, which also unreferences and clears Mod timers.
3. Free all live EffectSystem Reanimations and attachments.
4. Dispose the Reanimator cache.
5. Restore the property-map baseline captured before startup applied any Mod
   string or `default.xml` overlays.
6. Clear engine-side dynamic plant, zombie, and projectile definition/image
   caches.
7. Free all Reanimation definitions and clear dynamic Reanimation registration
   storage.
8. Reset `ModRegistry`.

The EffectSystem must contain no live Reanimation instance before definition
arrays are freed or replaced.

### 4. Activate the new state

Activation follows the same ordering constraints as startup:

1. Reload manifests and enabled state into `gModLoader`.
2. Clear and rebuild Mod resource roots in loader order.
3. Initialize the shared Lua VM and execute enabled entries.
4. Rebuild the vanilla Reanimation definition array.
5. Rebuild the vanilla Reanimation name map.
6. Finalize and load queued dynamic Reanimations.
7. Apply Mod string overrides and Mod property overlays.
8. Recreate the Reanimator cache, required front-end screens, and TitleScreen
   effects.
9. Clear the reload flag and show a success message.

Runtime IDs are intentionally reassigned from the rebuilt registration order.
This is safe because reload is prohibited while a Board or custom mode is live.

## Required Interfaces

### LawnApp

- `RequestModReload()` records a deferred request after Apply.
- `CanReloadMods()` checks the idle-main-menu preconditions.
- `ProcessPendingModReload()` runs from a main-loop safe point.
- `ReloadModsFromConfig()` coordinates preflight, teardown, activation, and
  failure handling.
- An application state flag prevents reentrant reload and blocks game-start
  actions after a destructive failure.

The coordinator should remain one readable function unless an existing lifecycle
boundary provides a natural helper. Cleanup primitives belong to their owning
subsystems.

### ModLua

Add `ValidateEntries()` to compile enabled entry files without executing them.
It uses a temporary Lua state and must leave the active VM, handler table, entry
globals, registry, and timers unchanged.

### Dynamic engine caches

Move the current function-local static Mod caches to file scope and expose
focused reset functions:

- `Plant::ResetModDefinitions()`;
- `Zombie::ResetModDefinitions()`;
- `Projectile::ResetModDefinitions()`.

The reset functions clear only dynamic Mod entries. Vanilla definition tables
and images remain intact.

### Base property snapshot

After startup loads `LawnStrings.txt`, `properties/default.xml`, and
`properties/Layout.xml`, but before it applies any Mod string or property
overlay, `LawnApp` captures the base string, bool, integer, double, and string
vector property maps.

Reload restores those complete maps before applying the newly enabled Mod
overlays. The maps are treated as immutable content configuration after startup;
transient gameplay state must not be stored in them.

### Reanimator

Add a reset primitive that:

- asserts or verifies that no live Reanimation instances remain;
- frees loaded definitions;
- clears dynamic parameter/definition vectors and owned filename storage;
- restores the empty global definition state expected by
  `ReanimatorLoadDefinitions()`.

Dynamic Reanimation filenames must have storage whose lifetime covers the active
definition arrays. Repeated reloads must neither accumulate dynamic entries nor
retain pointers into cleared vectors.

### Resource roots

Reuse `ClearResourceRoots()` and `AddResourceRoot()` through one helper shared by
startup and reload so both paths produce identical root order. The helper does
not delete the complete ResourceManager contents.

Custom definition caches and Reanimation definitions release their image
references before stale shared images are cleaned. Already-loaded vanilla
ResourceManager entries are outside the first-version guarantee.

## Failure Handling

Failures are divided by whether destructive teardown has started.

### Preflight failure

- restore the old disabled-ID configuration;
- retain the current runtime unchanged;
- reopen or retain usable main-menu UI;
- show the specific manifest, dependency, missing-file, or Lua syntax error.

### Activation failure

- restore the old disabled-ID configuration on disk;
- log the exact failed phase and Mod error;
- mark the application as requiring restart;
- prevent starting a level or opening content screens that may rely on partial
  definitions;
- display a base-resource restart-required dialog that does not depend on Mod
  definitions;
- shut down normally after the user dismisses the dialog.

The first version does not attempt to execute the old entries again after an
activation failure.

## UI Behavior

- The Apply button text and dependency-aware checkbox behavior remain unchanged.
- Apply becomes disabled while a reload request is pending or running.
- A successful reload closes the manager and displays `Mods reloaded` with a
  note that some already-loaded vanilla resource overrides may still require a
  restart.
- A preflight error reports the error without destroying the active Mod state.
- A destructive failure displays `Mod reload failed; restart required` and
  prevents entering gameplay.

No file watcher or automatic reload shortcut is included in this version.

## Testing

### Automated tests

Add coverage for:

- valid and syntactically invalid Lua entry preflight;
- preflight leaving handlers, timers, globals, and registry contents unchanged;
- repeated registry reset and entry execution assigning IDs from the documented
  starting values without stale definitions;
- repeated dynamic Reanimation reset without count growth or stale filename
  storage;
- restoring all base property-map types and deleting keys introduced only by the
  previous Mod set;
- configuration restoration after save, preflight, and activation failures;
- existing loader, dependency, ModSave, and Lua helper tests.

Tests that require a graphics context may be covered by a focused integration or
manual smoke test rather than forcing GL initialization into `pvz.mod`.

### Manual acceptance

Verify all of the following in Debug and Release with `PVZ_ENABLE_LUA=ON`:

1. Edit a Lua callback, press Apply, and observe the new behavior.
2. Enable and disable custom plants, zombies, modes, and projectiles and verify
   rebuilt main-menu and Challenge UI.
3. Change a custom image and external Reanimation, press Apply, and verify newly
   created content uses the changed asset.
4. Reload at least ten times without a crash, duplicate registration, or growing
   dynamic Reanimation count.
5. Introduce a Lua syntax error and verify the old runtime remains usable.
6. Trigger an entry execution or registration failure and verify configuration
   rollback, gameplay lockout, and restart-required messaging.
7. Run the full CMake build and CTest suite in both configurations.

## Documentation Updates

Implementation must update:

- `docs/modding.md`;
- `docs/modding.zh-CN.md`;
- `docs/ARCHITECTURE.md`;
- `docs/TODO.md`;
- `AGENTS.md`.

The documentation must state the main-menu-only boundary, Apply behavior,
transient-state loss, failure semantics, and the limitation for already-loaded
vanilla resource overlays.

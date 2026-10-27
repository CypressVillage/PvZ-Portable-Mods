# Mod Framework Stabilization Design

## Purpose

Stabilize the existing Lua Mod framework so that its documented APIs are safe,
its implemented fields affect gameplay, and incomplete content paths fail or
degrade predictably instead of corrupting state or indexing fixed engine data
with dynamic IDs.

This work preserves the current public Lua API where practical. It does not add
a standalone custom game-rule engine or per-Mod Lua virtual machines.

## Required Order

The work is performed in this order:

1. Update the public documentation to describe the currently verified behavior,
   known defects, supported content paths, build flags, and migration notes.
2. Implement the fixes described below.
3. Update the documentation again where implementation details or limitations
   changed during verification.
4. Run Lua-enabled builds and automated tests.

The first documentation pass must not describe planned fixes as already
available.

## Compatibility Policy

- Keep existing Lua API names and callback signatures.
- Keep automatic runtime ID ranges: plants at 2000, zombies at 3000,
  projectiles at 4000, and registered mode identities at 5000.
- Registration functions continue returning `nil, error` on failure.
- Successful registration functions return the actual assigned runtime ID.
- Existing modes that only provide `id` are rejected with a useful error rather
  than being hidden or launched unsafely.
- Custom modes use an explicit vanilla `baseMode` for gameplay rules. Their
  dynamic 5000+ ID remains their registration and Challenge Screen identity.
- Existing shared Lua globals remain a documented limitation. Handler execution
  nevertheless receives the correct owning Mod context.
- No backward-compatibility layer is added for behavior that could never work
  correctly, such as successful registration returning `-1`.

## Documentation Changes

Before implementation, update these files:

- `docs/modding.md`
- `docs/modding.zh-CN.md`
- `docs/ARCHITECTURE.md`
- `docs/TODO.md`
- `DEBUGGING.md`
- `README.md`

The documentation must distinguish:

- implemented and verified features;
- known runtime defects awaiting the accompanying fixes;
- deliberately unsupported features, including independent custom mode rules,
  dependency version constraints, hot reload, JSON data tables, and per-Mod Lua
  environments;
- required build flags and Lua dependency setup;
- the requirement for custom modes to specify a safe vanilla `baseMode`;
- the save compatibility limits of dynamically assigned entity type IDs.

Incorrect completed claims in `docs/TODO.md`, such as TimedAction chaining and
stopping RepeatAction by returning false, are changed to incomplete until the
behavior exists.

## Lua Handler Ownership And Dispatch

### Handler records

Each captured callback is stored with:

- the Lua function;
- the owning Mod ID;
- the owning Mod root path.

Before a callback executes, the dispatcher saves the previous `__mod_id` and
`__mod_root`, installs the handler's values, calls the handler, then restores the
previous values on success or error. This keeps `Game.SaveModData`,
`Game.LoadModData`, and context-dependent resource registration associated with
the correct Mod.

### Argument handling

The dispatcher keeps the original callback arguments in a stable table or uses
absolute Lua stack indices. Pushing one argument must not change the index used
to retrieve later arguments. All zero-, one-, and two-argument callbacks share
the same tested dispatch path.

### Entry cleanup

Before each entry runs, all callback globals including `OnModInit` are cleared.
After a successful entry, callbacks are captured and cleared. After a load or
runtime error, partially defined callbacks are also cleared before continuing.
`OnModInit` runs only for the entry that defined it and runs under that Mod's
context.

## Registration And Runtime IDs

Registry registration methods return or expose the stored definition containing
the assigned runtime ID. Lua returns that assigned value rather than the
unchanged caller-side structure.

Registration rejects:

- duplicate string IDs;
- duplicate runtime IDs;
- IDs outside the supported dynamic range;
- plant IDs that cannot fit engine arrays;
- modes without a valid vanilla `baseMode`;
- malformed required or optional fields.

Auto-allocation skips occupied runtime IDs and fails cleanly when the supported
range is exhausted.

## Dynamic Reanimation Lifecycle

Lua registration records external Reanim requests without touching Reanimator
global arrays. After resource roots are registered and vanilla Reanimator
definitions are initialized, the application flushes the pending registrations
and loads their definitions.

The registry owns the path strings used by `ReanimationParams`; pointers passed
to Reanimator must remain valid until Reanimator shutdown. Duplicate paths map
to one dynamic definition.

Plants and zombies resolve their pending Reanim references only after the flush.
Registration failure is logged and leaves the content definition unusable
rather than indexing an invalid animation definition.

## Custom Zombies

Custom zombie initialization applies the registered fields after common zombie
defaults and before gameplay begins:

- body and head health;
- speed multiplier or configured movement speed semantics;
- damage;
- helm and shield type and health;
- `hasHead` and `hasArm`;
- display name and Reanimation.

Debug assertions accept registered dynamic zombie IDs while still rejecting
unknown dynamic IDs. Unsupported combinations are validated at registration or
logged and normalized to safe defaults.

## Custom Modes

A custom mode definition contains two distinct values:

- dynamic mode ID: the 5000+ identity used by the registry and Mod UI;
- vanilla `baseMode`: the existing engine mode whose rules and fixed challenge
  data are used.

The Challenge Screen stores both values. Selecting a Mod mode starts the vanilla
`baseMode`, while Lua-facing mode queries and level-start callbacks expose the
dynamic identity through an active Mod-mode context. Engine paths that require
fixed challenge definitions always receive the vanilla base mode.

This design intentionally does not create new challenge records, progression
records, achievements, or save slots for arbitrary rule sets.

## Event Timing

`OnProjectileSpawn` runs after all basic projectile fields and custom projectile
defaults are initialized. Values changed by the callback are not immediately
overwritten by the initializer.

`OnProjectileMiss` is emitted consistently for supported non-impact projectile
removals, including horizontal and vertical out-of-bounds paths and expiry paths
that semantically represent a miss. Impact deaths do not emit it.

## Lua Object Lifetimes

### Entities

Lua entity userdata stores a validated engine identity in addition to, or
instead of, a raw pointer. Each property or method resolves the current object
through the owning Board's generation-aware DataArray lookup. If the Board or
entity no longer exists, access returns `nil` or a Lua error according to the
existing API shape, never a stale object occupying the same slot.

### Dialogs

Dialog userdata and `LuaProxyDialog` have an explicit invalidation link. When C++
removes or destroys the dialog, it nulls the userdata pointer and releases the
self-reference. Explicit `dialog:Close()` uses the same path. Lua garbage
collection only releases remaining Lua-side state and never dereferences a
destroyed dialog.

### Shutdown

Application shutdown calls `gModLua.Shutdown()` while dependent engine objects
and logging are still valid. Timer and dialog references are released before the
Lua state closes.

## PlantHelper And Timers

- `PlantHelper.Update` advances timers even if no SimpleAI or AutoShooter entry
  exists.
- Plant death invokes unconditional helper cleanup and timer cancellation.
- Timers do not retain an entity after its validated identity becomes invalid.
- `SimpleAI` accepts an explicit initial state. Examples use it instead of Lua
  table iteration order.
- `TimedAction` supports cancellation with the documented method syntax.
- `RepeatAction` stops when its callback returns `false`.
- TimedAction chaining remains undocumented and incomplete unless implemented
  and tested in this work; it is not required for stabilization.

## Loader And Save Error Handling

Filesystem traversal uses non-throwing `std::error_code` overloads or catches
filesystem exceptions. An unreadable Mod directory is logged and does not abort
application startup.

Manifest parsing checks the return value for every optional field. A present
field with the wrong type rejects that manifest and identifies the field.

Mod save writes:

- reject or preserve a corrupt existing save instead of silently replacing all
  other keys;
- write to a temporary file in the same directory;
- verify stream completion;
- atomically replace the destination where the platform permits it.

## Build And Test Integration

README build instructions document `PVZ_ENABLE_LUA=ON` and the Lua development
dependency. At least one native CI configuration enables Lua. Emscripten either
links an explicitly supported Lua implementation or rejects
`PVZ_ENABLE_LUA=ON` at configure time with a clear message.

Automated coverage includes:

- zero-, one-, and two-argument callback dispatch;
- callback error isolation;
- per-handler Mod ID and root restoration;
- `OnModInit` and failed-entry cleanup;
- successful registration return IDs and collision rejection;
- manifest optional-field type failures;
- standalone TimedAction and RepeatAction behavior;
- corrupt Mod save preservation;
- custom mode base-mode mapping logic where it can be tested without game
  assets.

Verification commands include Lua-enabled Release and Debug builds. Full
resource-level game startup is reported separately if proprietary game resources
are unavailable.

## Completion Criteria

The stabilization is complete when:

- all findings from the 2026-08-06 review are either fixed or explicitly
  documented as an intentional limitation;
- documented successful registrations return usable IDs;
- multi-Mod callbacks use the correct save and resource context;
- no documented callback signature corrupts the Lua stack;
- external Reanim registration occurs only after engine initialization;
- custom zombies apply documented fields;
- custom modes cannot index fixed engine data with dynamic IDs;
- Lua entities and dialogs cannot dereference destroyed C++ objects;
- PlantHelper's documented timer behavior is covered by tests;
- native Lua-enabled Release and Debug builds succeed;
- public English and Chinese documentation describe the same behavior.

## Out Of Scope

- independent custom mode rule engines;
- per-Mod Lua VMs or sandboxing;
- hot reload;
- dependency version constraints;
- JSON content-table loading;
- stable symbolic serialization of active custom entities in normal game saves;
- broad refactoring unrelated to Mod framework stabilization.

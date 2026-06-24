# Custom Plants Guide

This document describes how to add custom plants to PvZ-Portable via the mod system.

Implementation status: verified against the C++ source on 2026-06-24. Custom plants are registered from Lua; JSON data files are not used. External reanim definitions use the engine's element-based Reanim XML format, not attribute-style XML.

## 1. Overview

The custom plant system allows mod authors to:

- Define new plant properties (cost, cooldown, attack rate, etc.)
- Reuse built-in vanilla animations or load custom external animations
- Have plants automatically appear in the Almanac and Seed Chooser UI
- Support pagination when the number of plants exceeds the vanilla limit of 49

The standard almanac/seed chooser plant set uses `SeedType` IDs `0` through `48` (`SEED_IMITATER`). Additional special internal seed values exist, but mod plants are automatically assigned IDs starting from `2000`.

## 2. Mod Package Layout

```
mods/
  <mod_id>/
    mod.json
    scripts/
      main.lua              # Lua entry script — plant registration here
    resources/
      reanim/
        my_plant.xml         # Optional: custom animation file(s)
      images/                # Optional: custom images
```

Plants are registered via Lua in the entry script. No JSON data files are needed.

## 3. mod.json

```json
{
  "id": "my_plant_mod",
  "name": "My Plant Mod",
  "version": "1.0.0",
  "author": "author_name",
  "description": "Adds custom plants",
  "priority": 100,
  "entry": "scripts/main.lua"
}
```

## 4. Registering Plants in Lua

Call `Game.RegisterPlant(def)` from `OnModInit()`:

### Minimal Example

```lua
function OnModInit()
    Game.RegisterPlant({
        id = "my_plant",
        name = "My Plant",
        cost = 100,
        cooldown = 500
    })
end
```

Only `id` is required. All other fields have defaults.

### Full Field Reference

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | string | **(required)** | Unique identifier (must not collide with other mods) |
| `name` | string | `"ModPlant"` | Display name |
| `cost` | int | `50` | Sun cost |
| `cooldown` | int | `750` | Cooldown time in frames (~750 frames = 25 seconds at 30fps) |
| `packetIndex` | int | `0` | Seed packet visual variant index |
| `subClass` | int | `0` | `0` = normal, `1` = shooter (determines idle/shooting behavior) |
| `launchRate` | int | `0` | Frames between projectile launches (0 = no projectile) |
| `projectileType` | string or int | `0` | Projectile ID string (registered via `Game.RegisterProjectile`) or raw integer type |
| `reanimation` | string | *(none)* | Vanilla reanim path to reuse, e.g. `"reanim/FirePea.reanim"` |
| `reanimFile` | string | *(none)* | External reanim file path (relative to mod root), e.g. `"resources/reanim/custom.xml"` |
| `image` | string | *(none)* | Custom static image path resolved through resource roots, usually relative to the mod's `resources/` directory, e.g. `"images/my_plant.png"`. Used as seed packet/almanac art when no reanimation is set. |
| `description` | string | *(none)* | Almanac description text displayed in the plant's Almanac entry |

### Animation Selection

There are two ways to assign an animation:

1. **Reuse a vanilla animation** via `reanimation`:
   ```lua
   Game.RegisterPlant({
       id = "my_fire_pea",
       reanimation = "reanim/FirePea.reanim"
   })
   ```
   The value must match an existing path in the game's reanim table. Common vanilla paths:

   | Plant | Reanim Path |
   |-------|-------------|
   | Peashooter | `reanim/PeaShooterSingle.reanim` |
   | Sunflower | `reanim/SunFlower.reanim` |
   | Snow Pea | `reanim/SnowPea.reanim` |
   | Repeater | `reanim/PeaShooter.reanim` |
   | Chomper | `reanim/Chomper.reanim` |
   | Squash | `reanim/Squash.reanim` |
   | Tall-nut | `reanim/Tallnut.reanim` |
   | Gatling Pea | `reanim/GatlingPea.reanim` |
   | Winter Melon | `reanim/WinterMelon.reanim` |
   | Fire Pea | `reanim/FirePea.reanim` |
   | Cactus | `reanim/Cactus.reanim` |
   | Cattail | `reanim/Cattail.reanim` |
   | Melon-pult | `reanim/Melonpult.reanim` |
   | Kernel-pult | `reanim/Cornpult.reanim` |

2. **Load an external reanim file** via `reanimFile`:
   ```lua
   Game.RegisterPlant({
       id = "my_custom_shooter",
       reanimFile = "resources/reanim/custom_shooter.xml"
   })
   ```
   The path is relative to the mod root directory. See Section 5 for the file format.

If both `reanimation` and `reanimFile` are specified, `reanimFile` takes priority.

If neither is specified, the plant will have no animation (invisible body).

### Static Image Fallback

If `image` is set instead of `reanimation`/`reanimFile`, the plant uses it as a static sprite for the seed packet icon:

```lua
Game.RegisterPlant({
    id = "static_plant",
    name = "Static Plant",
    cost = 50,
    image = "images/static_plant.png"
})
```

The image is loaded from disk via `SexyAppBase::GetImage()`. It is rendered in the Seed Chooser and Almanac. Note that the plant will have **no body animation** in-game unless a `reanimation` is also provided — this is best used for plants that reuse vanilla reanim logic or have no on-field presence.

### Using Custom Projectiles

Register a projectile first, then reference it by string ID:

```lua
function OnModInit()
    Game.RegisterProjectile({
        id = "super_pea",
        damage = 200,
        speed = 1.67,
        image = "IMAGE_REANIM_WINTERMELON_PROJECTILE"
    })

    Game.RegisterPlant({
        id = "super_peashooter",
        name = "Super Pea Shooter",
        cost = 175,
        subClass = 1,
        launchRate = 90,
        projectileType = "super_pea",
        reanimation = "reanim/PeaShooterSingle.reanim"
    })
end
```

## 5. Custom Reanim File Format

Custom animation files use the same XML format as the vanilla reanim system. The file is parsed by the game's built-in `DefinitionCompileAndLoad` pipeline.

**Important**: Reanim XML files do NOT have a root wrapping element. They consist of one or more `<track>` elements at the top level, plus optional `<fps>`. Do NOT include `<?xml?>` declarations. The current parser reads fields as child elements, so use `<name>`, `<x>`, `<f>`, etc.; do not use attribute-style forms like `<track name="anim_idle">` or `<t f="0"/>`.

### Structure

```xml
<track>
  <name>anim_idle</name>
  <t><f>0</f><x>0</x><y>0</y><sx>1</sx><sy>1</sy><a>1</a></t>
  <t><f>40</f><y>-3</y><sy>1.03</sy></t>
  <t><f>80</f><x>0</x><y>0</y><sx>1</sx><sy>1</sy></t>
</track>
<track>
  <name>anim_shooting</name>
  <t><f>0</f><x>0</x><y>0</y><sx>1</sx><sy>1</sy></t>
  <t><f>8</f><y>3</y><sx>1.06</sx><sy>0.94</sy></t>
  <t><f>16</f><x>0</x><y>0</y><sx>1</sx><sy>1</sy></t>
</track>
```

### Track Element

`<track><name>...</name>...</track>` defines an animation track (a named sequence of keyframes).

Common track names used by the engine:

| Track Name | Purpose |
|------------|---------|
| `anim_idle` | Default idle loop |
| `anim_shooting` | Played when the plant fires |
| `anim_blink` | Blink overlay (used by some plants) |

### Transform Element (`<t>`)

Each `<t>` element is a keyframe transform. Child elements:

| Element | Type | Description |
|-----------|------|-------------|
| `f` | float | Frame number (keyframe time position) |
| `x` | float | X translation offset |
| `y` | float | Y translation offset |
| `kx` | float | Skew X |
| `ky` | float | Skew Y |
| `sx` | float | Scale X (default 1.0) |
| `sy` | float | Scale Y (default 1.0) |
| `a` | float | Alpha/opacity (0.0 to 1.0) |
| `i` | string | Image reference (for sprite-based tracks) |
| `font` | string | Font reference |
| `text` | string | Text content |

Elements are optional on non-first keyframes. Missing values are inherited from the previous keyframe.

### FPS

The default animation FPS is `12.0`. To override, add a `<fps>` element:

```xml
<fps>15.0</fps>
<track>
  <name>anim_idle</name>
  ...
</track>
```

### Image References

To reference images in keyframes, use the `i` element:

```xml
<track>
  <name>anim_idle</name>
  <t><f>0</f><i>IMAGE_REANIM_MY_PLANT_FRAME1</i></t>
  <t><f>10</f><i>IMAGE_REANIM_MY_PLANT_FRAME2</i></t>
  <t><f>20</f><i>IMAGE_REANIM_MY_PLANT_FRAME1</i></t>
</track>
```

Images referenced with `IMAGE_REANIM_...` are resolved through the engine's resource lookup paths such as `reanim/` and `images/`, including the mod's registered `resources/` directory.

## 6. Almanac & Seed Chooser Pagination

When the total number of plants (vanilla 49 + mod plants) exceeds one page, both the Almanac and the Seed Chooser automatically show pagination controls (`<` and `>` buttons).

- The Almanac displays up to 49 plant slots per page.
- The Seed Chooser displays 40 or 48 slots per page depending on whether the current layout has 7 rows; page stepping is still based on the vanilla 49-entry indexing scheme.
- Page navigation is only visible when there are enough plants to require multiple pages
- The `<` button is disabled on the first page; the `>` button is disabled on the last page

No additional configuration is needed. Custom plants appear after all vanilla plants.

## 7. Complete Example: Demo Plant Mod

### mod.json

```json
{
  "id": "demo_plant",
  "name": "Demo Plant Mod",
  "version": "0.1.0",
  "author": "pvz-portable",
  "description": "Demonstrates how to add custom plants via the mod system",
  "priority": 200,
  "entry": "scripts/main.lua"
}
```

### scripts/main.lua

```lua
function OnModInit()
    -- Fire Pea (reusing vanilla animation)
    Game.RegisterPlant({
        id = "demo_fire_pea",
        name = "Fire Pea",
        cost = 200,
        cooldown = 750,
        subClass = 1,
        launchRate = 90,
        reanimation = "reanim/FirePea.reanim"
    })

    -- Ice Melon (reusing vanilla animation)
    Game.RegisterPlant({
        id = "demo_ice_melon",
        name = "Ice Melon",
        cost = 350,
        cooldown = 750,
        launchRate = 100,
        reanimation = "reanim/WinterMelon.reanim"
    })

    -- Custom Shooter (external animation)
    Game.RegisterPlant({
        id = "demo_custom_shooter",
        name = "Custom Shooter",
        cost = 125,
        cooldown = 500,
        subClass = 1,
        launchRate = 85,
        reanimFile = "resources/reanim/custom_pea.xml"
    })
end
```

### resources/reanim/custom_pea.xml (custom animation)

```xml
<track>
  <name>anim_idle</name>
  <t><f>0</f><x>0</x><y>0</y><sx>1</sx><sy>1</sy><a>1</a></t>
  <t><f>40</f><y>-2</y><sy>1.02</sy></t>
  <t><f>80</f><x>0</x><y>0</y><sx>1</sx><sy>1</sy></t>
</track>
<track>
  <name>anim_shooting</name>
  <t><f>0</f><x>0</x><y>0</y><sx>1</sx><sy>1</sy></t>
  <t><f>10</f><y>3</y><sx>1.05</sx><sy>0.95</sy></t>
  <t><f>20</f><x>0</x><y>0</y><sx>1</sx><sy>1</sy></t>
</track>
```

## 8. File Structure Summary

```
mods/demo_plant/
  mod.json
  scripts/
    main.lua
  resources/
    reanim/
      custom_pea.xml
```

## 9. Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| Plant doesn't appear in Almanac | `id` field missing or duplicate | Ensure `id` is unique and non-empty |
| Game crashes or dynamic reanim fails on startup | Reanim XML has `<?xml?>` header, root wrapping element, or attribute-style fields | Remove XML declaration/root wrapper; use bare `<track>` elements with child fields like `<name>` and `<t><f>0</f></t>` |
| Custom animation fails to load | Invalid XML syntax or missing file | Check the game log for "Failed to load dynamic reanim" messages |
| Plant is invisible | No animation assigned | Add `reanimation` or `reanimFile` field |
| Vanilla animation name not found | Path doesn't match reanim table entry | Use exact paths from Section 4 (e.g. `reanim/FirePea.reanim`) |
| "id already registered" | Multiple mods use the same plant id | Use unique string IDs across all mods |

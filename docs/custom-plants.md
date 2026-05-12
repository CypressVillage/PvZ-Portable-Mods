# Custom Plants Guide

This document describes how to add custom plants to PvZ-Portable via the mod system.

## 1. Overview

The custom plant system allows mod authors to:

- Define new plant properties (cost, cooldown, attack rate, etc.)
- Reuse built-in vanilla animations or load custom external animations
- Have plants automatically appear in the Almanac and Seed Chooser UI
- Support pagination when the number of plants exceeds the vanilla limit of 49

Vanilla plants use `SeedType` IDs from `0` to `52`. Mod plants are automatically assigned IDs starting from `2000`.

## 2. Mod Package Layout

```
mods/
  <mod_id>/
    mod.json
    data/
      my_plant.json          # One JSON file per plant definition
      another_plant.json
    resources/
      reanim/
        my_plant.xml         # Optional: custom animation file(s)
      images/                # Optional: custom images
      properties/
        default.xml          # Optional: string overrides
```

Each plant is defined in its own JSON file. Multiple files can be listed in `mod.json`.

## 3. mod.json

```json
{
  "id": "my_plant_mod",
  "name": "My Plant Mod",
  "version": "1.0.0",
  "author": "author_name",
  "description": "Adds custom plants",
  "priority": 100,
  "data": {
    "plants": [
      "data/fire_pea.json",
      "data/ice_melon.json",
      "data/custom_shooter.json"
    ]
  }
}
```

- `data.plants`: array of relative paths to plant definition JSON files. Each file defines exactly one plant.

## 4. Plant Definition JSON

### Minimal Example

```json
{
  "id": "my_plant",
  "name": "My Plant",
  "cost": 100,
  "cooldown": 500
}
```

Only `id` is required. All other fields have defaults.

### Full Field Reference

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `id` | string | Yes | - | Unique identifier (must not collide with other mods) |
| `name` | string | No | `"ModPlant"` | Display name |
| `cost` | int | No | `50` | Sun cost |
| `cooldown` | int | No | `750` | Cooldown time in frames (~750 frames = 25 seconds at 30fps) |
| `packetIndex` | int | No | `0` | Seed packet visual variant index |
| `subClass` | int | No | `0` | `0` = normal, `1` = shooter (determines idle/shooting behavior) |
| `launchRate` | int | No | `0` | Frames between projectile launches (0 = no projectile) |
| `reanimation` | string | No | *(none)* | Vanilla reanim path to reuse, e.g. `"reanim/FirePea.reanim"` |
| `reanimFile` | string | No | *(none)* | External reanim file path (relative to mod root), e.g. `"resources/reanim/custom.xml"` |

### Animation Selection

There are two ways to assign an animation:

1. **Reuse a vanilla animation** via `reanimation`:
   ```json
   {
     "id": "my_fire_pea",
     "reanimation": "reanim/FirePea.reanim"
   }
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
   ```json
   {
     "id": "my_custom_shooter",
     "reanimFile": "resources/reanim/custom_shooter.xml"
   }
   ```
   The path is relative to the mod root directory. See Section 5 for the file format.

If both `reanimation` and `reanimFile` are specified, `reanimFile` takes priority.

If neither is specified, the plant will have no animation (invisible body).

## 5. Custom Reanim File Format

Custom animation files use the same XML format as the vanilla reanim system. The file is parsed by the game's built-in `DefinitionCompileAndLoad` pipeline.

**Important**: Reanim XML files do NOT have a root wrapping element. They consist of one or more `<track>` elements at the top level. Do NOT include `<?xml?>` declarations.

### Structure

```xml
<track name="anim_idle">
  <t f="0" x="0" y="0" sx="1" sy="1" a="1"/>
  <t f="40" y="-3" sy="1.03"/>
  <t f="80" x="0" y="0" sx="1" sy="1"/>
</track>
<track name="anim_shooting">
  <t f="0" x="0" y="0" sx="1" sy="1"/>
  <t f="8" y="3" sx="1.06" sy="0.94"/>
  <t f="16" x="0" y="0" sx="1" sy="1"/>
</track>
```

### Track Element

`<track name="...">` defines an animation track (a named sequence of keyframes).

Common track names used by the engine:

| Track Name | Purpose |
|------------|---------|
| `anim_idle` | Default idle loop |
| `anim_shooting` | Played when the plant fires |
| `anim_blink` | Blink overlay (used by some plants) |

### Transform Element (`<t>`)

Each `<t>` element is a keyframe transform. Attributes:

| Attribute | Type | Description |
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

Attributes are optional on non-first keyframes. Missing values are inherited from the previous keyframe.

### FPS

The default animation FPS is `12.0`. To override, add a `<fps>` element:

```xml
<fps>15.0</fps>
<track name="anim_idle">
  ...
</track>
```

### Image References

To reference images in keyframes, use the `i` attribute:

```xml
<track name="anim_idle">
  <t f="0" i="images/my_plant_frame1"/>
  <t f="10" i="images/my_plant_frame2"/>
  <t f="20" i="images/my_plant_frame1"/>
</track>
```

Images are loaded from the mod's `resources/` directory.

## 6. Almanac & Seed Chooser Pagination

When the total number of plants (vanilla 49 + mod plants) exceeds one page, both the Almanac and the Seed Chooser automatically show pagination controls (`<` and `>` buttons).

- Each page displays up to 48 plants (6 rows x 8 columns)
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
  "data": {
    "plants": [
      "data/fire_pea.json",
      "data/ice_melon.json",
      "data/custom_shooter.json"
    ]
  }
}
```

### data/fire_pea.json (reusing vanilla animation)

```json
{
  "id": "demo_fire_pea",
  "name": "Fire Pea",
  "cost": 200,
  "cooldown": 750,
  "subClass": 1,
  "launchRate": 90,
  "reanimation": "reanim/FirePea.reanim"
}
```

### data/ice_melon.json (reusing vanilla animation)

```json
{
  "id": "demo_ice_melon",
  "name": "Ice Melon",
  "cost": 350,
  "cooldown": 750,
  "launchRate": 100,
  "reanimation": "reanim/WinterMelon.reanim"
}
```

### data/custom_shooter.json (external animation)

```json
{
  "id": "demo_custom_shooter",
  "name": "Custom Shooter",
  "cost": 125,
  "cooldown": 500,
  "subClass": 1,
  "launchRate": 85,
  "reanimFile": "resources/reanim/custom_pea.xml"
}
```

### resources/reanim/custom_pea.xml (custom animation)

```xml
<track name="anim_idle">
  <t f="0" x="0" y="0" sx="1" sy="1" a="1"/>
  <t f="40" y="-2" sy="1.02"/>
  <t f="80" x="0" y="0" sx="1" sy="1"/>
</track>
<track name="anim_shooting">
  <t f="0" x="0" y="0" sx="1" sy="1"/>
  <t f="10" y="3" sx="1.05" sy="0.95"/>
  <t f="20" x="0" y="0" sx="1" sy="1"/>
</track>
```

## 8. File Structure Summary

```
mods/demo_plant/
  mod.json
  data/
    fire_pea.json
    ice_melon.json
    custom_shooter.json
  resources/
    reanim/
      custom_pea.xml
```

## 9. Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| Plant doesn't appear in Almanac | `id` field missing or duplicate | Ensure `id` is unique and non-empty |
| Game crashes on startup | Reanim XML has `<?xml?>` header or root wrapping element | Remove XML declaration and root wrapper; use bare `<track>` elements |
| Custom animation fails to load | Invalid XML syntax or missing file | Check the game log for "Failed to load dynamic reanim" messages |
| Plant is invisible | No animation assigned | Add `reanimation` or `reanimFile` field |
| Vanilla animation name not found | Path doesn't match reanim table entry | Use exact paths from Section 4 (e.g. `reanim/FirePea.reanim`) |

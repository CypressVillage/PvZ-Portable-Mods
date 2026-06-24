# PvZ Reanim 骨骼动画系统 —— 以豌豆射手为例

实现状态：本文已于 2026-06-24 对照 `src/Sexy.TodLib/Definition.cpp`、`src/Sexy.TodLib/Reanimator.cpp` 和 Mod 注册代码核对。当前 Reanim XML 使用元素式字段，不使用 XML 属性字段。

## 概述

PvZ 使用 **Reanim XML** 格式的骨骼动画。每个动画由多个 **骨骼轨道（Track）** 组成，每个轨道以 `<t>`（Transform）关键帧序列驱动一个身体部件的独立 PNG 图片。

动画文件存放在 `reanim/` 目录，配套的部件 PNG 也放在 `reanim/` 下，通过 `<i>...</i>` 子元素引用。

---

## 一、文件格式

### 规则

- **无 XML 声明**（`<?xml ...?>`）—— 当前定义解析器期望直接读取定义元素，声明可能导致解析失败
- **无根元素** —— 顶层直接是 `<track>` 列表
- **无 BOM** —— 必须是纯 UTF-8
- **使用元素字段** —— 写成 `<f>0</f>`、`<x>10</x>`，不要写成 `<t f="0" x="10"/>`

### 顶层元素

```xml
<fps>12</fps>                  <!-- 可选，帧率，默认 12.0 -->
<track>...</track>             <!-- 骨骼轨道，至少一个 -->
<track>...</track>
```

### `<track>` 元素

```xml
<track>
  <name>轨道名称</name>
  <t>...</t>    <!-- 关键帧 -->
  <t>...</t>
  <t>...</t>
  ...
</track>
```

### `<t>`（Transform/关键帧）元素

| 子元素 | C++ 字段 | 类型 | 默认 | 说明 |
|------|----------|------|------|------|
| `x` | `mTransX` | float | 继承上一帧 | X 偏移（像素） |
| `y` | `mTransY` | float | 继承 | Y 偏移（像素） |
| `kx` | `mSkewX` | float | 继承 | X 扭曲/旋转（度） |
| `ky` | `mSkewY` | float | 继承 | Y 扭曲/旋转（度） |
| `sx` | `mScaleX` | float | 继承 | X 缩放倍数 |
| `sy` | `mScaleY` | float | 继承 | Y 缩放倍数 |
| `f` | `mFrame` | float | 继承 | **帧号**（>=0 显示图片的该切片；<0 隐藏） |
| `a` | `mAlpha` | float | 继承 | 透明度（0.0=透明，1.0=不透明） |
| `i` | `mImage` | image | 继承 | 引用的图片资源 ID |
| `font` | `mFont` | font | 继承 | 字体资源 |
| `text` | `mText` | string | 继承 | 渲染的文字 |

**空 `<t></t>`**：继承上一关键帧的所有字段（简化重复数据）。

`<f>-1</f>`：**隐藏帧** —— 该轨道不渲染任何内容。

---

## 二、关键帧插值

**所有插值都是线性（Lerp）**，没有贝塞尔曲线或缓动函数。

```cpp
// FloatLerp 实现
result = start + factor * (end - start)
```

### 线性插值的属性

- `x` / `y` —— 位置
- `kx` / `ky` —— 扭曲角度
- `sx` / `sy` —— 缩放
- `a` —— 透明度

### 非插值（瞬切）的属性

- `i`（图片）—— 从"前一帧"快照
- `font`（字体）—— 快照
- `text`（文字）—— 快照
- `f`（帧号）—— 快照，且有特殊优化规则

### 消失帧优化

当关键帧从可见（`f >= 0`）过渡到隐藏（`f = -1`）时，只要 `fraction > 0`，帧号立即设为 `-1`，防止图片在过渡期间残留。

---

## 三、图片引用与加载

XML 中的 `i` 子元素使用资源 ID 字符串：

```xml
<t><i>IMAGE_REANIM_PEASHOOTER_BACKLEAF</i></t>
```

引擎按以下顺序解析：

| 前缀 | 查找目录 | 示例 |
|------|----------|------|
| `IMAGE_` | （根目录） | `PEASHOOTER_BACKLEAF` |
| `IMAGE_` | `particles/` | `particles/PEASHOOTER_BACKLEAF` |
| `IMAGE_REANIM_` | **`reanim/`** | `reanim/PEASHOOTER_BACKLEAF` → 实际加载 `PeaShooter_backleaf.png` |
| `IMAGE_REANIM_` | `images/` | `images/PEASHOOTER_BACKLEAF` |

因此 `<i>IMAGE_REANIM_PEASHOOTER_BACKLEAF</i>` 最终加载的文件是 `reanim/PeaShooter_backleaf.png`。

---

## 四、豌豆射手完整资源清单

### 4.1 文件位置

| 文件 | 路径 |
|------|------|
| 动画定义 | `build/extracted/reanim/PeaShooterSingle.reanim` |
| 所有部件 PNG | `build/extracted/reanim/PeaShooter_*.png` |

### 4.2 所有部件 PNG（21 个）

| 图片 ID | 对应文件 | 用途 |
|---------|----------|------|
| `IMAGE_REANIM_PEASHOOTER_BACKLEAF` | `PeaShooter_backleaf.png` | 后叶 |
| `IMAGE_REANIM_PEASHOOTER_BACKLEAF_LEFTTIP` | `PeaShooter_backleaf_lefttip.png` | 后叶左尖 |
| `IMAGE_REANIM_PEASHOOTER_BACKLEAF_RIGHTTIP` | `PeaShooter_backleaf_righttip.png` | 后叶右尖 |
| `IMAGE_REANIM_PEASHOOTER_STALK_BOTTOM` | `PeaShooter_stalk_bottom.png` | 茎部下段 |
| `IMAGE_REANIM_PEASHOOTER_STALK_TOP` | `PeaShooter_stalk_top.png` | 茎部上段 |
| `IMAGE_REANIM_PEASHOOTER_FRONTLEAF` | `PeaShooter_frontleaf.png` | 前叶 |
| `IMAGE_REANIM_PEASHOOTER_FRONTLEAF_RIGHTTIP` | `PeaShooter_frontleaf_righttip.png` | 前叶右尖 |
| `IMAGE_REANIM_PEASHOOTER_FRONTLEAF_LEFTTIP` | `PeaShooter_frontleaf_lefttip.png` | 前叶左尖 |
| `IMAGE_REANIM_PEASHOOTER_HEAD` | `PeaShooter_Head.png` | 头部 |
| `IMAGE_REANIM_PEASHOOTER_MOUTH` | `PeaShooter_mouth.png` | 嘴巴 |
| `IMAGE_REANIM_PEASHOOTER_LIPS` | `PeaShooter_Lips.png` | 嘴唇 |
| `IMAGE_REANIM_PEASHOOTER_EYEBROW` | `PeaShooter_eyebrow.png` | 眉毛 |
| `IMAGE_REANIM_PEASHOOTER_BLINK1` | `PeaShooter_blink1.png` | 眨眼帧 1 |
| `IMAGE_REANIM_PEASHOOTER_BLINK2` | `PeaShooter_blink2.png` | 眨眼帧 2 |
| `IMAGE_REANIM_PEASHOOTER_HEADLEAF_FARTHEST` | `PeaShooter_headleaf_farthest.png` | 头冠叶最远层 |
| `IMAGE_REANIM_PEASHOOTER_HEADLEAF_3RDFARTHEST` | `PeaShooter_headleaf_3rdfarthest.png` | 头冠叶第三远 |
| `IMAGE_REANIM_PEASHOOTER_HEADLEAF_2RDFARTHEST` | `PeaShooter_headleaf_2rdfarthest.png` | 头冠叶第二远 |
| `IMAGE_REANIM_PEASHOOTER_HEADLEAF_NEAREST` | `PeaShooter_headleaf_nearest.png` | 头冠叶最近层 |
| `IMAGE_REANIM_PEASHOOTER_HEADLEAF_TIP_BOTTOM` | `PeaShooter_headleaf_tip_bottom.png` | 头冠叶尖下 |
| `IMAGE_REANIM_PEASHOOTER_HEADLEAF_TIP_TOP` | `PeaShooter_headleaf_tip_top.png` | 头冠叶尖上 |
| `IMAGE_REANIM_ANIM_SPROUT` | `anim_sprout.png` | 出土动画共用 |

---

## 五、18 个骨骼轨道详解

### 5.1 动画控制轨道（引擎按名称查找）

这些轨道的 `<t>` 关键帧控制 **哪些部件显示**（通过 `i` 切换图片）、**位置**（x/y）、**缩放**（sx/sy）等。

| # | 轨道名 | 调用时机 | 说明 |
|---|--------|----------|------|
| 1 | `anim_idle` | 默认待机 | 所有部件的基础循环。在豌豆射手中这个轨道本身是空循环，因为身体部件由各自的独立物理轨道驱动。 |
| 2 | `anim_head_idle` | 头部待机 | 头部独立待机循环，与身体动画分开播放，让头部有微动。 |
| 3 | `anim_shooting` | 发射豌豆时 | 发射动画循环。豌豆射手的是空轨道（口部由 `idle_shoot_blink` 和代码控制）。 |
| 4 | `anim_full_idle` | 全身待机 | 身体+头部的完整待机。 |
| 5 | `anim_blink` | 周期性眨眼 | 3 帧快速切换：`BLINK1` → `BLINK2` → `BLINK1` → 隐藏。 |
| 6 | `idle_shoot_blink` | 待机中的眨眼 | 比 `anim_blink` 更长间隔的眨眼，与射击动画联动。 |
| 7 | `anim_sprout` | 刚种下时 | 复杂出土动画，40+ 关键帧。使用共用图片 `IMAGE_REANIM_ANIM_SPROUT`。 |
| 8 | `anim_face` | 面部表情 | 头部上下浮动（sy 0.452~0.666），模拟呼吸。 |
| 9 | `idle_mouth` | 嘴巴待机 | 嘴巴独立微动动画。 |
| 10 | `anim_stem` | 茎部连线 | **不引用图片**，只有 x/y 坐标路径。引擎用它作为头部 reanim 的挂载点。 |

### 5.2 身体部件物理轨道（叶子/茎的摆动）

这些轨道模拟风吹摆动的效果。每个部件都有独立轨道，通过 `kx/ky`（扭曲角度）实现叶尖摆动。

| # | 轨道名 | 引用图片 | 属性变化范围 |
|---|--------|---------|-------------|
| 11 | `backleaf` | `BACKLEAF` | x: 27.5~27.7, y: 53~55.9, sx/sy: 0.429~0.610 |
| 12 | `backleaf_left_tip` | `BACKLEAF_LEFTTIP` | kx/ky: 0.0→8.8→-8.5→0.0 来回 |
| 13 | `backleaf_right_tip` | `BACKLEAF_RIGHTTIP` | kx/ky: 0.0→-8.8→8.8→0.0 |
| 14 | `stalk_bottom` | `STALK_BOTTOM` | x: 36.1→42.4, kx/ky: 0.0→37.5 （大幅摇摆） |
| 15 | `stalk_top` | `STALK_TOP` | x: 31.0→44.0, kx/ky: 0.0→23.5 |
| 16 | `frontleaf` | `FRONTLEAF` | x: 21.7→22.8, y: 54.4→56.5 |
| 17 | `frontleaf_right_tip` | `FRONTLEAF_RIGHTTIP` | kx/ky: 0.0→-9.2 |
| 18 | `frontleaf_tip_left` | `FRONTLEAF_LEFTTIP` | kx/ky: 0.0→8.3 |

### 渲染顺序

引擎按照**轨道在文件中的顺序**逐层绘制，因此：

1. 后叶系列（`backleaf` → `backleaf_left_tip` → `backleaf_right_tip`）先画 → **在最后层**
2. 茎部下段 → 茎部上段
3. 前叶系列（`frontleaf` → 叶尖）→ **在茎前面**
4. 头部 → 嘴巴 → 表情 → 眨眼 → **在最前面**

---

## 六、循环类型（ReanimLoopType）

| 枚举值 | 名称 | 行为 |
|--------|------|------|
| 0 | `REANIM_LOOP` | 标准循环，到末尾回 0 |
| 1 | `REANIM_LOOP_FULL_LAST_FRAME` | 循环但包含最后一帧 |
| 2 | `REANIM_PLAY_ONCE` | 播一次后销毁 |
| 3 | `REANIM_PLAY_ONCE_AND_HOLD` | 播一次停在最后一帧 |
| 4 | `REANIM_PLAY_ONCE_FULL_LAST_FRAME` | 播一次（含最后帧）后销毁 |
| 5 | `REANIM_PLAY_ONCE_FULL_LAST_FRAME_AND_HOLD` | 播一次（含最后帧）后停住 |

---

## 七、引擎查找的轨道命名约定

### 植物通用

| 轨道名 | 用途 |
|--------|------|
| `anim_idle` | 待机循环（必须） |
| `anim_shooting` | 射击动画（射手类） |
| `anim_head_idle` | 头部独立待机（豌豆、雪花、双发、机枪） |
| `anim_sprout` | 出土动画 |
| `anim_stem` | 头部挂载点路径 |
| `anim_blink` | 眨眼 |
| `anim_face` | 面部动画 |

### 特殊轨道前缀

- **`attacher__`**：子动画附着轨道。格式：
  ```
  attacher__REANIMNAME__TRACKNAME[rate][hold][once]
  ```
  示例：`attacher__Zombie__anim_walk[15]` 挂载一个僵尸 reanim，播放 `anim_walk` 轨道，帧率 15。

- **`_ground`**：地面参考轨道。用于僵尸走路速度同步。

### 豌豆射手不使用但其他植物会用到的轨道

| 轨道名 | 使用植物 |
|--------|---------|
| `anim_sleep` | 蘑菇类（睡眠时） |
| `anim_explode` | 樱桃炸弹、土豆雷 |
| `anim_armed` / `anim_unarmed_idle` | 土豆雷 |
| `anim_grab` / `anim_loop` | 大嘴花 |
| `anim_glow` | 火炬树桩 |
| `anim_bigidle` / `anim_bigsleep` | 大喷菇变大形态 |

---

## 八、时间与帧率计算

### 更新公式

每 tick（0.01 秒=10ms）更新一次：

```
mAnimTime += 0.01 * mAnimRate / mFrameCount
```

### 完整循环时长

- 标准模式：`(mFrameCount - 1) / mAnimRate` 秒
- FULL_LAST_FRAME 模式：`mFrameCount / mAnimRate` 秒

豌豆射手，12 FPS，叶片轨道约 25 帧：
```
(25 - 1) / 12 = 2 秒一次循环
```

### FPS 随机化

豌豆射手初始化时：
```cpp
mAnimRate = RandRangeFloat(15.0f, 20.0f);
```
因此每个实例的摆动速度略微不同，更自然。

---

## 九、Mod 开发者：如何创建自定义动画

### 方案 A：复用原版动画

```lua
Game.RegisterPlant({
    id = "super_peashooter",
    reanimation = "reanim/PeaShooterSingle.reanim",
    -- 不提供 reanimFile，直接引用引擎内置
})
```

### 方案 B：提供自定义 reanim 文件

```
mods/my_mod/
  resources/
    reanim/
      custom_plant.reanim    -- 你的动画定义
      custom_head.png        -- 头部 PNG
      custom_mouth.png       -- 嘴巴 PNG
      custom_leaf.png        -- 叶子 PNG
      ...                    -- 所有部件 PNG
  scripts/
    main.lua
  mod.json
```

```lua
Game.RegisterPlant({
    id = "my_custom_shooter",
    reanimFile = "resources/reanim/custom_plant.reanim",
})
```

### 创建步骤

1. **制作部件 PNG**：每个身体部件保存为独立的透明 PNG（不要合并成 spritesheet）
2. **编写 `.reanim` 文件**：参考 `PeaShooterSingle.reanim` 的结构
3. **确保至少包含**：`anim_idle` 轨道（引擎至少需要这个）
4. **图片 ID 引用**：`<i>IMAGE_REANIM_MYPLANT_HEAD</i>` → 引擎会按资源查找路径搜索 `reanim/MYPLANT_HEAD`、`images/MYPLANT_HEAD` 等资源
5. **mod 中路径解析**：`reanimFile` 相对于 mod 根目录，图片也在 mod 的 `resources/` 下解析

### 最小示例

```xml
<fps>12</fps>
<track>
  <name>anim_idle</name>
  <t><f>-1</f></t>
  <t><f>0</f><x>0</x><y>0</y><sx>1</sx><sy>1</sy><i>IMAGE_REANIM_MYPLANT_BODY</i></t>
  <t><f>-1</f></t>
</track>
```

---

## 十、附录：豌豆射手关键帧数据示例

### 最简单的轨道：anim_blink（眨眼）

```xml
<track>
  <name>anim_blink</name>
  <t><f>-1</f></t>                          <!-- 隐藏 -->
  <t><x>37.5</x><y>23.5</y><sx>0.555</sx><sy>0.555</sy><f>0</f><i>IMAGE_REANIM_PEASHOOTER_BLINK1</i></t>  <!-- 睁眼 -->
  <t><y>23.4</y><i>IMAGE_REANIM_PEASHOOTER_BLINK2</i></t>  <!-- 半闭 -->
  <t><y>23.5</y><i>IMAGE_REANIM_PEASHOOTER_BLINK1</i></t>  <!-- 睁眼 -->
  <t><f>-1</f></t>                          <!-- 隐藏 -->
  <!-- 后面全是空帧延长时间 -->
  ...
</track>
```

### 最复杂的轨道：anim_sprout（出土）

40+ 关键帧，使用 `IMAGE_REANIM_ANIM_SPROUT`（单个共用 sprout 图片），通过变换 x/y/kx/ky/sx/sy 模拟从土里钻出来的过程。

### 摆动轨道示例：backleaf

```xml
<track>
  <name>backleaf</name>
  <t><f>-1</f></t>                          <!-- 开头隐藏 -->
  <!-- 24 帧摆动循环：x/y/sx/sy 缓慢变化 -->
  <t><x>27.7</x><y>53</y><sx>0.555</sx><sy>0.555</sy><f>0</f><i>IMAGE_REANIM_PEASHOOTER_BACKLEAF</i></t>
  <t><y>53.3</y><sx>0.561</sx><sy>0.543</sy></t>
  <t><y>53.5</y><sx>0.566</sx><sy>0.530</sy></t>
  <!-- ... 中间帧逐渐变化 -->
  <t><y>53</y><sx>0.555</sx><sy>0.555</sy></t>  <!-- 回到起点 -->
  <t><f>-1</f></t>                          <!-- 空帧作为插值锚点 -->
  <!-- 后面镜像复制相同的关键帧作为第二遍循环 -->
</track>
```

---

## 十一、性能优化：ReanimAtlas

引擎在加载 `.reanim` 后，自动将所有引用的 PNG 打包到 **单个纹理图集（Atlas）** 中，减少绘制调用。Mod 开发者在提供自定义动画时无需关心图集——引擎会自动处理。

---

## 参考

- `src/Sexy.TodLib/Reanimator.h` —— Reanim 核心数据结构
- `src/Sexy.TodLib/Reanimator.cpp` —— 解析、更新、渲染
- `src/Sexy.TodLib/Definition.cpp` —— XML 解析器（DefField 系统）
- `src/Lawn/Plant.cpp` —— 植物 reanim 初始化与轨道调用
- `build/extracted/reanim/PeaShooterSingle.reanim` —— 豌豆射手动画文件（完整参考）
- `docs/custom-plants.md` —— Mod 自定义植物文档

# 选卡界面翻页崩溃 (SIGSEGV) Bug 总结

## 1. Bug 现象描述
在植物选卡界面（Seed Chooser Screen）中，当玩家尝试点击“上一页”或“下一页”按钮来查看 Mod 引入的新植物时，游戏发生崩溃退出。终端错误日志显示为 `SIGSEGV (地址边界错误)`。

## 2. 崩溃原因分析

这个 Bug 的发生是由两方面的原因叠加导致的：

### 2.1 核心原因：延迟加载的资源未驻留内存 (空指针异常)
在编写翻页按钮代码时，按钮使用的图片资源被设置为了图鉴的翻页按钮：
* `Sexy::IMAGE_ALMANAC_INDEXBUTTON`
* `Sexy::IMAGE_ALMANAC_INDEXBUTTONHIGHLIGHT`

在原版游戏引擎中，图鉴的图片资源属于 `DelayLoad_Almanac` (延迟加载资源组)。这意味着只有在玩家**第一次打开图鉴**时，这些图片才会被加载到内存中。由于选卡界面在图鉴之前出现，此时这几个图片指针的值为 `nullptr`。
当渲染引擎（`GameButton::Draw` -> `Graphics::DrawImage` -> `Image::GetHeight`）尝试获取这个空指针图片的高度时，发生非法内存访问，直接导致 `SIGSEGV`。

### 2.2 潜在原因：原版硬编码导致的数组越界
原版游戏大量使用宏 `NUM_SEEDS_IN_CHOOSER` (固定为 49) 来遍历植物卡片。Mod 框架动态注册了更多植物（例如增加到 50 个以上），如果直接将原版渲染逻辑应用到扩展后的植物列表，在绘制或点击检测时也会导致索引越界。

## 3. 修复方案

针对以上问题，我们进行了以下修复：

1. **替换按钮图片资源**：
   将翻页按钮 (`mNextPageButton` / `mPrevPageButton`) 的贴图修改为选卡界面必定加载的资源（如：`Sexy::IMAGE_SEEDCHOOSER_BUTTON2`）。这确保了在选卡界面渲染时资源指针绝对有效，修复了空指针引发的段错误。
   
2. **重构分页和遍历逻辑**：
   * 将选卡界面中所有硬编码的 `NUM_SEEDS_IN_CHOOSER` 替换为 `gModRegistry.GetTotalAlmanacPlants()`，以适配动态数量的植物。
   * 引入安全的 `pageStart` 和 `pageEnd` 边界检查：
     ```cpp
     int startIdx = mPlantPage * 49;
     int endIdx = startIdx + (Has7Rows() ? 48 : 40);
     if (endIdx > maxPlants) endIdx = maxPlants;
     ```
   * 仅渲染和处理当前页面 (`mPlantPage`) 范围内的植物卡片，避免因为尝试渲染不存在或超出的植物卡片引发崩溃。

3. **增加容错处理 (Null Safety)**：
   在 `Plant::DrawSeedType` 获取植物贴图时，增加了对 `GetImage()` 返回 `nullptr` 的空安全拦截，以防部分 Mod 植物缺少贴图时导致游戏闪退。

## 4. 总结
在基于原版引擎开发 UI 时，务必注意**资源的生命周期与加载时机**。跨界面复用图片时，必须确保目标界面的资源组（Resource Group）在当前界面已经被显式加载，或者使用当前界面独立拥有的资源。
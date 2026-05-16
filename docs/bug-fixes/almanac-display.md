# Mod 植物图鉴未显示与选卡错位 Bug 总结

## 1. Bug 现象描述
玩家在安装 Mod 并启动游戏后遇到以下两个问题：
1. **图鉴无卡片：** 新添加的 Mod 植物的种子卡片在图鉴（Almanac）中未能正常显示。
2. **选卡飞回错位：** 在战前选卡界面（Seed Chooser Screen）中，选中 Mod 植物卡片将其放入卡槽后，如果再次点击卡槽中的卡片将其退回，卡片没有飞回其所在的附加页正确位置，而是飞到了其他原版植物（如墓碑吞噬者）的位置，发生严重的错位堆叠。

## 2. 崩溃与错位原因分析

### 2.1 选卡界面退回卡片错位原因
在选卡界面的代码逻辑中，通过 `GetSeedPositionInChooser` 计算植物卡片在网格中的坐标。
在之前的修改中，该函数被改为了接收一个**卡片视觉索引 `int theIndex`** （范围0-50+）。然而，在“退回卡槽卡片”（`ClickedSeedInBank`）等操作时，传入的却是植物的**实际ID (`SeedType`)**。
对于原版植物，由于其视觉索引恰好等于其 ID（例如豌豆射手的 ID 是 0，索引也是 0），此时计算正常；但对于 Mod 植物，其 `SeedType` 可能被分配为类似 `2000` 的大数值。当代码尝试用 `2000 % 49` 计算行列位置时，结果为 40（对应了原版的墓碑吞噬者）。这导致所有的 Mod 卡片在退回时都被计算到了错误的网格坐标上。

### 2.2 图鉴未显示新植物卡片原因
经过对图鉴界面 (`AlmanacDialog`) 渲染逻辑 (`DrawPlants`) 和点击检测逻辑 (`SeedHitTest`) 的分析，实际上系统已经完美地实现了对新植物的分页与排版计算。
但是，导致玩家“认为”新卡片没有显示的原因极其简单——**翻页按钮没有被绘制出来**。
在 `AlmanacDialog` 的构造函数中，虽然成功创建并实例化了 `mNextPlantPageButton` 与 `mPrevPlantPageButton` 这两个翻页按钮，但在 `AlmanacDialog::Draw` 渲染流程中遗漏了对它们的绘制调用 (`->Draw(g)`)，导致玩家无法点击翻页，也就永远停留在只有原版植物的第一页，产生了新植物没有显示的错觉。

## 3. 修复方案

针对以上问题，分别进行了如下两项修复：

1. **解决卡片飞回错位：**
   在 `SeedChooserScreen.h/cpp` 中新增了一个映射函数 `int GetAlmanacIndex(SeedType theSeedType)`。该函数可以根据植物的 `SeedType` ID 反查其在图鉴扩展池中的具体视觉索引位置（0-50+）。
   将所有原来误传入 `SeedType` ID 给坐标计算的地方（如 `ClickedSeedInBank`、`ShowToolTip`等），统一修改为传入该植物的真实视觉索引：
   ```cpp
   GetSeedPositionInChooser(GetAlmanacIndex(theChosenSeed.mSeedType), theChosenSeed.mEndX, theChosenSeed.mEndY);
   ```

2. **解决图鉴翻页按钮不显示：**
   在 `src/Lawn/Widget/AlmanacDialog.cpp` 文件的 `AlmanacDialog::Draw(Graphics* g)` 方法末尾，补上对翻页按钮的渲染调用：
   ```cpp
   mNextPlantPageButton->Draw(g);
   mPrevPlantPageButton->Draw(g);
   ```
   这一修改使玩家能够正常点击图鉴右下角的翻页按钮，从而在第二页及以后的页面查看到 Mod 追加的新植物。

## 4. 总结
* UI 组件需要始终保持“生命周期闭环”，即：实例化 -> 初始化坐标 -> 更新检测 -> **渲染绘制** -> 销毁。遗漏渲染是基于代码 UI 框架中极易发生的失误。
* 当系统引入映射关系（如“视觉索引” vs “内部ID”）后，相关的坐标映射、越界计算逻辑不能再直接混用，必须进行类型隔离和显式转换，以防止出现隐式的数据污染和错位现象。
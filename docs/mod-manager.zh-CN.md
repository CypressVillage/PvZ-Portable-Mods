# 游戏内 Mod 管理

## 入口与使用

1. 将 Mod 目录放入**资源目录**下的 `mods/`。默认 Windows 构建就是 `build/mods/<包目录>/mod.json`；指定 `-resdir` 时跟随该目录。
2. 在**主菜单 → 选项 → Mods** 打开管理器。关卡内的选项菜单不提供此入口。
3. 左侧每行显示 Mod 标题和游戏原生勾选框。点击标题选择 Mod，右侧查看名称、ID、版本、作者、API 版本、依赖与简介。列表和详情均支持滚轮及原生滚动条：鼠标悬停在哪一侧就滚动哪一侧，无需先点击；悬停于勾选框或滑块时也适用。长名称在列表中省略显示，完整内容在详情中查看。
4. 点击每行右侧的勾选框暂存启用状态。启用时自动勾选必需依赖；关闭时同时取消勾选所有直接或间接依赖它的 Mod。不再提供左下角 Enable／Disable 按钮或 `[ON]`／`[OFF]` 前缀；无效 Mod 的勾选框置灰且不可启用。空列表不显示切换控件。
5. 点击 Save 保存；点击 Cancel 或按 Esc 丢弃尚未保存的修改。保存后按钮显示 Close；关闭时同样不会保存之后另作的未保存修改。Enter 等价于 Save，上下键切换选择，空格切换所选 Mod 的勾选状态。
6. **退出并重新启动游戏后生效。** 保存不会重新启动 Lua，也不会切换当前玩家档案。右侧区分 `This session`（本次实际加载状态）和 `Next launch`（下次启动的暂存选择）。

未安装任何 Mod 时，界面显示安装说明；无效清单、重复 ID、依赖缺失／循环和不兼容 API 显示诊断且不能启用。Lua 入口执行与图片资源的进一步校验仍在启动时进行，管理器不声称已经运行验证脚本。

界面使用已有 `LawnDialog`、`LawnStoneButton`、`Checkbox`、`ListWidget` 和 `ScrollbarWidget`。两侧滚动条复用原版僵尸大头贴协议页的竖向 `Slider` 贴图和绘制逻辑，不再显示框架默认的灰色轨道／箭头；支持拖动滑块、点击轨道翻页及滚轮，无需滚动时置灰。不引入网页、外部 UI 库或新美术资源。当前默认界面文字为英文。Mod 文本按 UTF-8 边界换行；可显示的字形仍取决于玩家游戏资源中的原生字体。

## 配置与存档

启用配置始终写入**原始存档根目录**的 `userdata/mods.json`，不会因为玩法 Mod 已切换到 `mod-profiles/<摘要>/` 而写入错误的位置。`-savedir` 覆盖的根目录也适用。

Windows 默认配置路径：

```text
%APPDATA%\io.github.wszqkzqk\PvZPortable\userdata\mods.json
```

管理器保留配置中其他字段，以临时文件加原子替换保存；读取／写入失败会报告错误，不将失败显示为保存成功。选择修改仅作用于暂存副本；Cancel 不写文件。重新打开管理器时会重新扫描 Mod 目录并读取已保存配置。

启用玩法 Mod 后的独立档案和禁用中途存档策略保持不变；恢复原版配置需要保存关闭状态并重启。

## 作者元数据

在现有 `mod.json` 中增加以下两个**可选字符串**即可展示作者和简介；旧清单无需修改：

```json
{
  "id": "my-mod",
  "name": "My Mod",
  "version": "1.0.0",
  "api_version": 1,
  "entry": "main.lua",
  "author": "Your name",
  "description": "What this Mod does.\nAdditional information."
}
```

## 验证

对现有 GCC／MinGW、Ninja 构建执行合约测试和 Lua 植物 API 回归：

```sh
python tools/run-mod-manager-tests.py --build build
```

提供合法游戏资源后增加原生控件集成测试及截图输出：

```sh
python tools/run-mod-manager-tests.py --build build --resources build
```

Windows 需让 MinGW 运行库位于 PATH；Linux 无显示服务器时，可将整个命令放在 `xvfb-run -a` 下运行。此脚本不支持 MSVC 的编译／链接命令。

测试使用用户临时目录下 `opencode/mod-manager-*` 中的独立资源树、启用配置和存档，不修改真实玩家数据。覆盖依赖联动、校验失败不修改选择、配置原子替换与保留其他字段、元数据兼容、原生鼠标点击入口／切换／保存、取消、重新打开、长列表和简介滚动、无效 Mod、空列表、档案隔离、运行时不热切换以及重启后应用关闭配置。资源文件只在本地引用或复制，不随代码发布。

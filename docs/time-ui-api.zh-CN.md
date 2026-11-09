# 时间倍率与简单 UI API

## `time.get_scale()`

返回当前选择的倍率，默认 `1.0`。

## `time.set_scale(scale)`

仅允许运行期且存在关卡时调用。参数必须是有限数值，范围 **0.25～4.0**；

## `ui.button(name, options, on_click)`

仅在脚本注册期调用，返回 `mod-id:name`。不设置每个 Mod 的固定按钮数量上限；名称使用框架本地名称规则，同一 Mod 不允许重复名称。不同 Mod 使用独立命名空间。

```lua
local toggle = ui.button("toggle", {
    text = "1x",
    right = 128,
    top = 0,
    width = 72,
    height = 36,
}, function()
    local scale = time.get_scale() == 1 and 2 or 1
    time.set_scale(scale)
    ui.set_text("toggle", scale == 2 and "2x" or "1x")
end)

hooks.on("level.started", function()
    ui.set_text(toggle, "1x")
end)
```

`options` 仅接受以下字段：

- `text`：必填，1～128 字节的字符串，不允许 NUL；字体可显示的字形由游戏资源决定。
- `right`：距离右边缘的偏移，默认 128。
- `top`：距离上边缘的偏移，默认 0。
- `width`：默认 80，整数范围 40～300。
- `height`：默认 36，整数范围 24～100。

坐标使用游戏原生 **800×600 逻辑坐标**；窗口缩放由引擎处理。`right`、`top` 必须为非负整数且整个按钮须位于逻辑画面内。默认放在原版菜单按钮左侧；多个 Mod 的按钮没有自动避让，作者应协调位置。

控件复用 `LawnStoneButton`，每个 Board 创建对应实例。仅在正常游玩且尚未完成关卡时显示；暂停、对话框、持有不可交互的光标对象等情况下禁用，点击不会穿透到草坪种植操作。回调在游戏线程运行，无参数；异常记录所属 Mod、按钮 ID 和 Lua traceback，不中止其他 Mod。异常前已发生的 API 副作用不回滚。

本版只提供按钮与修改文字，不提供任意绘制、布局容器、动态创建／销毁或跨 Mod 修改。

## `ui.set_text(id, text)`

仅在运行期调用，可使用本地名称或完整 ID。只能修改本 Mod 自己注册的按钮；文字规则同 `ui.button`。原生控件在下一次逻辑更新时同步文字。

## 关卡生命周期 Hook

使用已有的 `hooks.on(event, callback)` 注册，支持 `priority` 与取消订阅。这两个事件不使用植物 `type` 筛选，回调收到空上下文，不提供可写的游戏字段。

- `level.started`：创建 Board、重置倍率后触发，**早于 `InitLevel` 与选卡／开场动画**；用于重置脚本／按钮状态，不表示玩家已经开始操作。
- `level.ended`：`DisposeBoard` 离开关卡时触发；倍率已恢复 1 倍，不能再设置倍率。绑定在逻辑释放点，不依赖延迟析构，避免旧 Board 的销毁覆盖新关卡状态。

## 验证

```powershell
python tools/run-double-speed-tests.py --build build --resources build
```

省略 `--resources` 只运行 Lua／Runtime 合约测试。完整测试使用临时目录下独立资源配置和存档，覆盖参数校验、命名空间、回调异常隔离、禁用／停止清理、整数／小数倍率、原生鼠标点击、1×／2× 实际更新次数、暂停／菜单不加速及退出关卡重置，并输出 `double-speed.png` 截图。不会修改真实玩家配置。

实现入口：`src/Mod/ModRuntime.*`、`src/Lawn/Widget/ModButton.*`、`src/Lawn/Board.*` 与 `src/LawnApp.cpp`。测试脚本面向现有 Ninja + GCC/MinGW 构建；本次验证环境为 Windows，未验证移动端、Web 或其他平台运行。

local button = ui.button("toggle", {
    text = "1x",
    right = 2,
    top = 40,
    width = 72,
    height = 36,
}, function()
    local scale = time.get_scale() == 1 and 2 or 1
    time.set_scale(scale)
    ui.set_text("toggle", scale == 2 and "2x" or "1x")
end)

hooks.on("level.started", function()
    ui.set_text(button, "1x")
end)

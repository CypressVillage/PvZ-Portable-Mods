local BTN_ID = 1001

local speeds = {1, 2, 3, 5, 10}
local labels = {"1x", "2x", "3x", "5x", "10x"}
local idx = 1

function OnModInit()
    local saved = Game.LoadModData("speed_index")
    if saved then
        local n = tonumber(saved)
        if n and n >= 1 and n <= #speeds then
            idx = n
        end
    end
    Game.Log("Speed Control Mod initialized (speed=" .. labels[idx] .. ")")
end

function OnLevelStart(mode_id)
    Game.Log("Level started, setting speed to " .. labels[idx])
    local x, y, w, h = Game.GetMenuButtonRect()
    Board.AddButton(BTN_ID, x, y + h + 2, w, 36, "Speed: " .. labels[idx])
    Game.SetSpeed(speeds[idx])
end

function OnBoardButtonClick(button_id)
    if button_id == BTN_ID then
        idx = idx % #speeds + 1
        Game.SetSpeed(speeds[idx])
        Board.SetButtonLabel(BTN_ID, "Speed: " .. labels[idx])
        Game.SaveModData("speed_index", tostring(idx))
        Game.Log("Speed changed to " .. labels[idx])
    end
end

function OnLevelEnd(is_win)
    Board.RemoveButton(BTN_ID)
end

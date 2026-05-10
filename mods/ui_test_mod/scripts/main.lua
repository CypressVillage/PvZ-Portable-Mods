function OnModInit()
  Game.Log("UI Test Mod: OnModInit")
end

function OnLevelStart(mode_id)
  Game.Log("UI Test Mod: showing dialog")

  local dlg = UI.CreateDialog({
    title = "UI Test Mod",
    body = "Welcome! This dialog is created by a Lua mod.\nClick a button to continue.",
    modal = true
  })

  dlg:AddButton("Spawn Zombie", function()
    Game.Log("UI Test: spawn zombie button clicked")
    Board.SpawnZombie(0, 2)
  end)

  dlg:AddButton("Close", function()
    Game.Log("UI Test: close button clicked")
  end)
end

function OnZombieSpawn(zombie)
  Game.Log("UI Test Mod: zombie spawned, hp=" .. tostring(zombie.hp))
end

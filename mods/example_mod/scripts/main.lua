function OnModInit()
  Game.Log("Example Mod: OnModInit")
  Game.SaveModData("boot_count", "1")
end

function OnLevelStart(mode_id)
  Game.Log("Example Mod: OnLevelStart mode=" .. tostring(mode_id))
  local last = Game.LoadModData("boot_count")
  if last == nil then
    Game.SaveModData("boot_count", "1")
  end
end

function OnZombieSpawn(zombie_type, row)
  Game.Log("Example Mod: OnZombieSpawn type=" .. tostring(zombie_type) .. " row=" .. tostring(row))
end

function OnZombieDie(zombie_type)
  Game.Log("Example Mod: OnZombieDie type=" .. tostring(zombie_type))
end

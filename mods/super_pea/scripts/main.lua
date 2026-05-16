function OnModInit()
    Game.Log("[super_pea] Mod initialized")
end

function OnLevelStart(mode)
    Game.Log("[super_pea] Level started with mode: " .. tostring(mode))
end

-- Auto Collect Sun Mod
-- Collects sun automatically as soon as it spawns

function OnModInit()
    Game.Log("Auto Collect Sun mod initialized!")
end

function OnCoinSpawn(coin)
    -- Check if the coin is a sun
    if coin:IsSun() then
        -- Automatically collect it
        coin:Collect()
        Game.Log("Automatically collected a sun!")
    end
end

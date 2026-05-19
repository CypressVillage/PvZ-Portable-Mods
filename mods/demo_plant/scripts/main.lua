-- demo_plant: Triple Pea
-- A custom plant that fires three peas in a spread pattern.
--
-- Demonstrates:
--   Game.RegisterPlant / Game.RegisterProjectile
--   PlantHelper.SimpleAI  (state machine with idle → attacking → idle)
--   Board:FindTargetZombie / Board:AddProjectile
--   OnPlantSpawn / OnPlantUpdate / PlantHelper.Update
--   entity animation control via SimpleAI

function OnModInit()
    Game.Log("[demo_plant] Triple Pea mod initializing...")

    -- Register the custom projectile
    Game.RegisterProjectile({
        id = "triple_pea_bullet",
        damage = 25,
        speed = 3.0,
        image = "IMAGE_REANIM_PEA_PROJECTILE"
    })

    -- Register the custom plant
    -- subClass=0 → not a built-in shooter (SimpleAI manages everything)
    Game.RegisterPlant({
        id = "triple_pea",
        name = "Triple Pea",
        cost = 200,
        cooldown = 750,
        subClass = 0,
        reanimation = "reanim/PeaShooterSingle.reanim",
        description = "Fires three peas in a spread. Built with PlantHelper.SimpleAI."
    })

    Game.Log("[demo_plant] Triple Pea registered (seedType >= 2000)")
end

-- OnPlantSpawn: set up the SimpleAI state machine
function OnPlantSpawn(plant)
    if plant.id ~= "triple_pea" then return end

    Game.Log("[demo_plant] Triple Pea spawned at (" .. plant.col .. "," .. plant.row .. ")")

    PlantHelper.SimpleAI(plant, {
        -- idle: looping animation, waits for a target
        idle = {
            animation = "anim_idle",
            on_enter = function(p)
                Game.Log("[demo_plant] State: idle")
            end,
            on_update = function(p)
                local target = Board:FindTargetZombie(p)
                if target then
                    return "attacking"
                end
            end
        },
        -- attacking: one-shot animation, fires on finish
        attacking = {
            animation = "anim_shoot",
            loopType = ReanimLoopType.PLAY_ONCE_AND_HOLD,
            on_enter = function(p)
                Game.Log("[demo_plant] State: attacking")
            end,
            on_finish = function(p)
                -- Fire three peas: center, left, right
                local cx, cy, row = p.x, p.y, p.row
                Board:AddProjectile(cx, cy, row, ProjectileType.PEA)
                Board:AddProjectile(cx - 10, cy, row, ProjectileType.PEA)
                Board:AddProjectile(cx + 10, cy, row, ProjectileType.PEA)
                Game.Log("[demo_plant] Fired 3 peas from (" .. p.col .. "," .. p.row .. ")")
                return "idle"
            end,
            on_exit = function(p)
                Game.Log("[demo_plant] State: returning to idle")
            end
        }
    })
end

-- OnPlantUpdate: required to drive PlantHelper
function OnPlantUpdate(plant)
    if plant.id == "triple_pea" then
        PlantHelper.Update(plant)
    end
end

-- OnPlantDie: log when the plant is destroyed
function OnPlantDie(plant)
    if plant.id == "triple_pea" then
        Game.Log("[demo_plant] Triple Pea died at (" .. plant.col .. "," .. plant.row .. ")")
    end
end

-- OnZombieAttack: log when a zombie eats the plant
function OnZombieAttack(zombie, plant)
    if plant and plant.id == "triple_pea" then
        Game.Log("[demo_plant] Zombie is eating Triple Pea!")
    end
end

-- OnPlantAttack: log when the plant attacks (built-in callback)
function OnPlantAttack(plant, target)
    if plant.id == "triple_pea" then
        Game.Log("[demo_plant] Plant attacked zombie type=" .. target.type)
    end
end

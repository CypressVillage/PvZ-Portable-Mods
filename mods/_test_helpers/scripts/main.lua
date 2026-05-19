-- _test_helpers: End-to-end validation of PlantHelper module
-- Logs all events to game log for verification.
-- Expected log output is documented in comments.

local TEST_PLANT_AI = "test_state_machine"
local TEST_PLANT_SHOOTER = "test_auto_shooter"

-- ============================================================
-- Test 1: TimedAction — verify one-shot timer fires correctly
-- ============================================================
function OnModInit()
    Game.Log("[TEST] ===== PlantHelper Test Suite =====")
    Game.Log("[TEST] Registering test plants...")

    Game.RegisterPlant({
        id = TEST_PLANT_AI,
        name = "Test State Machine",
        cost = 50,
        cooldown = 300,
        subClass = 0,
        reanimation = "reanim/PeaShooterSingle.reanim",
        description = "Tests PlantHelper.SimpleAI state machine"
    })

    Game.RegisterPlant({
        id = TEST_PLANT_SHOOTER,
        name = "Test Auto Shooter",
        cost = 100,
        cooldown = 300,
        subClass = 1,
        launchRate = 90,
        projectileType = 0,
        reanimation = "reanim/PeaShooterSingle.reanim",
        description = "Tests PlantHelper.AutoShooter"
    })

    Game.RegisterProjectile({
        id = "test_pea",
        damage = 25,
        speed = 3.0,
        image = "IMAGE_REANIM_PEA_PROJECTILE"
    })

    Game.Log("[TEST] Plants registered")
end

-- ============================================================
-- Test 2: SimpleAI — state machine lifecycle
-- ============================================================
function OnPlantSpawn(plant)
    if plant.id == TEST_PLANT_AI then
        Game.Log("[TEST SimpleAI] Setting up state machine for plant at (" .. plant.col .. "," .. plant.row .. ")")

        PlantHelper.SimpleAI(plant, {
            idle = {
                animation = "anim_idle",
                on_enter = function(p)
                    Game.Log("[TEST SimpleAI] -> idle state (enter)")
                end,
                on_update = function(p)
                    local t = Board:FindTargetZombie(p)
                    if t then
                        Game.Log("[TEST SimpleAI] Target found, switching to attacking")
                        return "attacking"
                    end
                end
            },
            attacking = {
                animation = "anim_shoot",
                loopType = ReanimLoopType.PLAY_ONCE_AND_HOLD,
                on_enter = function(p)
                    Game.Log("[TEST SimpleAI] -> attacking state (enter)")
                end,
                on_finish = function(p)
                    Game.Log("[TEST SimpleAI] Attack animation finished, firing projectile")
                    Board:AddProjectile(p.x, p.y, p.row, ProjectileType.PEA)
                    return "idle"
                end,
                on_exit = function(p)
                    Game.Log("[TEST SimpleAI] <- attacking state (exit)")
                end
            }
        })
    end

    if plant.id == TEST_PLANT_SHOOTER then
        Game.Log("[TEST AutoShooter] Setting up auto-shooter for plant at (" .. plant.col .. "," .. plant.row .. ")")

        PlantHelper.AutoShooter(plant, {
            range = 400,
            fireRate = 60,
            projectileType = ProjectileType.PEA,
            projectileDamage = 25,
            onFire = function(p, target)
                Game.Log("[TEST AutoShooter] Fired pea at zombie (type=" .. target.type .. ", row=" .. target.row .. ")")
            end
        })
    end
end

-- ============================================================
-- Test 3: TimedAction — one-shot after N frames
-- ============================================================
function OnPlantUpdate(plant)
    if plant.id == TEST_PLANT_AI or plant.id == TEST_PLANT_SHOOTER then
        PlantHelper.Update(plant)
    end

    -- TimedAction test: fire once 60 frames after spawn
    if plant.id == TEST_PLANT_SHOOTER and plant.age == 1 then
        Game.Log("[TEST TimedAction] Scheduling timed action (60 frames)")
        local t = PlantHelper.TimedAction(plant, 60, function(p)
            Game.Log("[TEST TimedAction] FIRED after 60 frames (plant=" .. p.id .. ")")
        end)
        Game.Log("[TEST TimedAction] Got cancel handle: " .. tostring(t.Cancel))
    end

    -- RepeatAction test: log every 120 frames
    if plant.id == TEST_PLANT_AI and plant.age == 1 then
        Game.Log("[TEST RepeatAction] Scheduling repeat action (every 120 frames)")
        local r = PlantHelper.RepeatAction(plant, 120, function(p)
            Game.Log("[TEST RepeatAction] Tick for plant " .. p.id)
        end)
        Game.Log("[TEST RepeatAction] Got cancel handle: " .. tostring(r.Cancel))
    end
end

-- ============================================================
-- Test 4: entity._ptr stability validation
-- ============================================================
function OnPlantDie(plant)
    if plant.id == TEST_PLANT_AI or plant.id == TEST_PLANT_SHOOTER then
        -- When the plant dies, PlantHelper.Update should auto-cleanup via _cleanup
        Game.Log("[TEST] Plant died, _ptr=" .. tostring(plant._ptr))
    end
end

function OnLevelStart(mode_id)
    Game.Log("[TEST] Level started, mode=" .. mode_id)
    Game.Log("[TEST] Expected log sequence to verify:")
    Game.Log("[TEST]   1. SimpleAI idle state (enter)")
    Game.Log("[TEST]   2. SimpleAI attacking state (enter) when target found")
    Game.Log("[TEST]   3. SimpleAI attack animation finish → fire projectile")
    Game.Log("[TEST]   4. AutoShooter fires pea every 60 frames when target in range")
    Game.Log("[TEST]   5. TimedAction fires once after 60 frames")
    Game.Log("[TEST]   6. RepeatAction ticks every 120 frames")
end

function OnLevelEnd(is_win)
    Game.Log("[TEST] Level ended, is_win=" .. tostring(is_win))
    Game.Log("[TEST] ===== PlantHelper Test Suite Complete =====")
end

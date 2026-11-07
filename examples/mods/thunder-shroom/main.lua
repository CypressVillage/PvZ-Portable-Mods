local fume = plants.get("base:fume_shroom")
local gloom = plants.get("base:gloom_shroom")
local attack = fume.attack

local thunder = plants.define("thunder-shroom", {
    name = "雷蘑菇",
    description = "大喷菇升级卡。攻击前方整行，对携带金属物品的僵尸造成双倍伤害。",
    cost = 150,
    cooldown = gloom.cooldown,
    health = 300,
    upgrade = {from = fume.id},
    traits = {fungus = true, nocturnal = true},
    appearance = {
        reanim = "assets/reanim/thunder_shroom.reanim",
        animations = {idle = "anim_idle", attack = "anim_shooting", sleep = "anim_sleep"},
    },
    card = {image = "assets/thunder_shroom_card.png"},
})

local function targets(plant)
    return zombies.query({source = plant, direction = "forward", target = "ground"})
end

hooks.on("plant.updated", {type = thunder}, function(ctx)
    local state = ctx.state
    if state.launch == nil then
        state.launch = math.random(attack.initial_min, attack.initial_max)
        state.shooting = 0
    end

    -- As with the native shooter, the current attack advances before a new one starts.
    if state.shooting > 0 then
        state.shooting = state.shooting - 1
        if state.shooting == attack.duration - attack.windup then
            for _, handle in ipairs(targets(ctx.entity)) do
                local target = entities.get(handle)
                local damage = attack.damage * (target.has_metal and 2 or 1)
                zombies.damage(handle, {
                    source = ctx.entity,
                    amount = damage,
                    mode = attack.damage_mode,
                })
            end
            effects.lightning({source = ctx.entity})
        end
        if state.shooting == 0 then
            plants.play_animation(ctx.entity, "idle")
        end
    end

    state.launch = state.launch - 1
    if state.launch <= 0 then
        state.launch = attack.interval - math.random(0, attack.interval_variation - 1)
        if #targets(ctx.entity) > 0 then
            state.shooting = attack.duration
            plants.play_animation(ctx.entity, "attack")
        end
    end
end)

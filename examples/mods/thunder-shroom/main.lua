local fume = plants.get("base:fume_shroom")
local gloom = plants.get("base:gloom_shroom")
plants.define("thunder-shroom", {
    name = "Thunder-shroom",
    description = "An upgrade for Fume-shroom. Attacks all zombies ahead in its lane, dealing double damage to zombies carrying metal items.",
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
    attack = {
        timing = fume.attack,
        animation = "attack",
        animation_ticks = 90,
        targets = function(plant)
            return zombies.query({source = plant, direction = "forward", target = "ground"})
        end,
        on_strike = function(ctx)
            for _, handle in ipairs(ctx.targets) do
                local target = entities.get(handle)
                local damage = ctx.attack.damage * (target.has_metal and 2 or 1)
                zombies.damage(handle, {
                    source = ctx.entity,
                    amount = damage,
                    mode = ctx.attack.damage_mode,
                })
            end
            effects.lightning({source = ctx.entity, duration = 100})
        end,
    },
})
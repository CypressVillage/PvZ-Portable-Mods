-- No shooter, production, inheritance or per-tick callback is needed.
local stump = plants.define("stump", {
    name = "Stump",
    description = "A plain plant. No shooting or production abilities.",
    cost = 50,
    cooldown = 750, -- game ticks, not milliseconds
    health = 300,
    appearance = {image = "assets/stump.png"},
    card = {image = "assets/stump_card.png"},
})

assert(stump == "plain-plant:stump")
assert(plants.get("stump").health == 300)

hooks.on("plant.spawned", {type = stump}, function(ctx)
    mod.log("stump spawned at " .. ctx.column .. "," .. ctx.row
        .. " with health=" .. ctx.health)
end)

hooks.on("plant.removed", {type = stump}, function(ctx)
    mod.log("stump removed at " .. ctx.column .. "," .. ctx.row)
end)

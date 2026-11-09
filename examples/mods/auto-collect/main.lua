hooks.on("collectible.spawned", function(event)
    if event.type == "base:sun" or event.type == "base:gold" or event.type == "base:silver" then
        collectibles.collect(event.entity)
    end
end)

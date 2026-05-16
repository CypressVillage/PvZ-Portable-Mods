function OnModInit()
    Game.RegisterProjectile({
        id = "super_pea_bullet",
        damage = 200,
        speed = 1.67,
        image = "IMAGE_REANIM_WINTERMELON_PROJECTILE"
    })

    Game.RegisterPlant({
        id = "super_peashooter",
        name = "super pea shooter",
        cost = 0,
        cooldown = 750,
        packetIndex = 0,
        subClass = 1,
        launchRate = 90,
        projectileType = "super_pea_bullet",
        reanimation = "reanim/PeaShooterSingle.reanim",
        description = "shoots super peas that can pierce through multiple zombies. Super peas deal more damage than regular peas and can hit multiple targets in a line."
    })
end

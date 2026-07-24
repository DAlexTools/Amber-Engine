local tile = 32.0

local function patrol(enemy, player, dt)
    enemy.velocity_x = enemy.speed * enemy.direction
end

local function hopper(enemy, player, dt)
    enemy.velocity_x = enemy.speed * enemy.direction
    if enemy.on_ground and enemy.time_since_jump >= enemy.jump_cooldown then
        enemy.velocity_y = enemy.jump_velocity
        enemy.time_since_jump = 0.0
    end
end

local function sentry(enemy, player, dt)
    local distance = player.x - enemy.x
    local absDistance = math.abs(distance)

    if absDistance < enemy.alert_range then
        enemy.direction = distance >= 0.0 and 1.0 or -1.0
        enemy.velocity_x = enemy.speed * 0.35 * enemy.direction
    else
        enemy.velocity_x = enemy.speed * enemy.direction
    end
end

return {
    enemies = {
        {
            name = "red_patrol",
            x = 19.0 * tile,
            y = 15.0 * tile - 26.0,
            width = 30.0,
            height = 26.0,
            speed = 72.0,
            direction = 1.0,
            left = 18.0 * tile,
            right = 24.0 * tile,
            health = 2,
            color = { r = 174, g = 54, b = 62 },
            on_update = patrol
        },
        {
            name = "hopper",
            x = 45.0 * tile,
            y = 15.0 * tile - 28.0,
            width = 30.0,
            height = 28.0,
            speed = 54.0,
            direction = -1.0,
            left = 42.5 * tile,
            right = 49.5 * tile,
            health = 3,
            jump_cooldown = 1.15,
            jump_velocity = -360.0,
            color = { r = 90, g = 132, b = 210 },
            on_update = hopper
        },
        {
            name = "stair_patrol",
            x = 12.0 * tile,
            y = 12.0 * tile - 25.0,
            width = 28.0,
            height = 25.0,
            speed = 46.0,
            direction = 1.0,
            left = 11.0 * tile,
            right = 14.0 * tile,
            health = 2,
            color = { r = 206, g = 88, b = 94 },
            on_update = patrol
        },
        {
            name = "tower_hopper",
            x = 22.0 * tile,
            y = 8.0 * tile - 27.0,
            width = 30.0,
            height = 27.0,
            speed = 42.0,
            direction = -1.0,
            left = 21.0 * tile,
            right = 25.0 * tile,
            health = 3,
            jump_cooldown = 0.95,
            jump_velocity = -320.0,
            color = { r = 74, g = 155, b = 120 },
            can_shoot = true,
            shoot_cooldown = 1.60,
            shoot_range = 360.0,
            projectile_speed = 340.0,
            shoot_delay = 0.35,
            on_update = hopper
        },
        {
            name = "bridge_sentry",
            x = 66.0 * tile,
            y = 13.0 * tile - 28.0,
            width = 32.0,
            height = 28.0,
            speed = 48.0,
            direction = 1.0,
            left = 63.0 * tile,
            right = 71.0 * tile,
            health = 4,
            alert_range = 260.0,
            color = { r = 151, g = 83, b = 188 },
            can_shoot = true,
            shoot_cooldown = 1.05,
            shoot_range = 500.0,
            projectile_speed = 390.0,
            shoot_delay = 0.20,
            on_update = sentry
        },
        {
            name = "ridge_sentry",
            x = 56.0 * tile,
            y = 8.0 * tile - 28.0,
            width = 32.0,
            height = 28.0,
            speed = 40.0,
            direction = 1.0,
            left = 55.0 * tile,
            right = 59.0 * tile,
            health = 4,
            alert_range = 300.0,
            color = { r = 116, g = 96, b = 205 },
            can_shoot = true,
            shoot_cooldown = 1.15,
            shoot_range = 520.0,
            projectile_speed = 410.0,
            shoot_delay = 0.45,
            on_update = sentry
        },
        {
            name = "sky_patrol",
            x = 75.0 * tile,
            y = 7.0 * tile - 26.0,
            width = 30.0,
            height = 26.0,
            speed = 58.0,
            direction = -1.0,
            left = 74.0 * tile,
            right = 77.0 * tile,
            health = 3,
            color = { r = 210, g = 158, b = 64 },
            on_update = patrol
        },
        {
            name = "late_patrol",
            x = 82.0 * tile,
            y = 15.0 * tile - 30.0,
            width = 34.0,
            height = 30.0,
            speed = 68.0,
            direction = -1.0,
            left = 79.0 * tile,
            right = 87.5 * tile,
            health = 3,
            color = { r = 210, g = 115, b = 62 },
            can_shoot = true,
            shoot_cooldown = 1.35,
            shoot_range = 430.0,
            projectile_speed = 360.0,
            shoot_delay = 0.70,
            on_update = patrol
        }
    }
}

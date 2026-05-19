-- plant_helper: high-level plant behavior module for mod authors
-- Loaded automatically by the engine. Provides SimpleAI state machine,
-- AutoShooter, TimedAction, and RepeatAction.
--
-- Usage in mod scripts:
--   function OnPlantUpdate(plant)
--       PlantHelper.Update(plant)
--   end

PlantHelper = {}
PlantHelper._plants = {}

function PlantHelper.SimpleAI(plant, states)
	if plant.isDead then return end
	local ptr = plant._ptr
	local entry = {
		type = "simple_ai",
		current = nil,
		states = states,
	}
	for name, _ in pairs(states) do
		entry.current = name
		break
	end
	local state = states[entry.current]
	if state and state.animation then
		local loopType = state.loopType or 0  -- LOOP
		plant:PlayBodyReanim(state.animation, loopType)
	end
	if state and state.on_enter then
		state.on_enter(plant)
	end
	PlantHelper._plants[ptr] = entry
end

function PlantHelper.AutoShooter(plant, config)
	if plant.isDead then return end
	PlantHelper._plants[plant._ptr] = {
		type = "auto_shooter",
		config = config,
		cooldown = 0
	}
end

function PlantHelper.TimedAction(plant, frames, callback)
	local ptr = plant._ptr
	local id = _timer.New(ptr, frames, function()
		local ok, err = pcall(callback, plant)
		if not ok then
			Game.Log("TimedAction error: " .. tostring(err))
		end
	end, false, frames)
	return { Cancel = function() _timer.Cancel(id) end }
end

function PlantHelper.RepeatAction(plant, interval, callback)
	local ptr = plant._ptr
	local id = _timer.New(ptr, interval, function()
		local ok, err = pcall(callback, plant)
		if not ok then
			Game.Log("RepeatAction error: " .. tostring(err))
		end
	end, true, interval)
	return { Cancel = function() _timer.Cancel(id) end }
end

function PlantHelper.Update(plant)
	if plant.isDead then
		PlantHelper._cleanup(plant)
		return
	end
	local ptr = plant._ptr
	local entry = PlantHelper._plants[ptr]
	if not entry then return end
	_timer.TickAll(ptr)
	if entry.type == "simple_ai" then
		PlantHelper._updateSimpleAI(plant, entry)
	elseif entry.type == "auto_shooter" then
		PlantHelper._updateAutoShooter(plant, entry)
	end
end

function PlantHelper._updateSimpleAI(plant, entry)
	local state = entry.states[entry.current]
	if not state then return end
	if state.on_update then
		local next_state = state.on_update(plant)
		if next_state and next_state ~= entry.current then
			PlantHelper._switchState(plant, entry, next_state)
			return
		end
	end
	if state.on_finish then
		local reanim = plant:GetBodyReanim()
		if reanim then
			local loopCt = reanim:GetLoopCount()
			if loopCt ~= (entry._last_loop or 0) and loopCt > 0 then
				entry._last_loop = loopCt
				local next_state = state.on_finish(plant)
				if next_state and next_state ~= entry.current then
					PlantHelper._switchState(plant, entry, next_state)
				end
			end
			entry._last_loop = loopCt
		end
	end
end

function PlantHelper._switchState(plant, entry, new_state)
	local old = entry.states[entry.current]
	if old and old.on_exit then old.on_exit(plant) end
	entry.current = new_state
	local state = entry.states[new_state]
	if not state then
		Game.Log("PlantHelper: unknown state '" .. tostring(new_state) .. "'")
		return
	end
	if state.animation then
		plant:PlayBodyReanim(state.animation, state.loopType or 0)
	end
	entry._last_loop = -1
	if state.on_enter then state.on_enter(plant) end
end

function PlantHelper._updateAutoShooter(plant, entry)
	local cfg = entry.config
	local target = Board:FindTargetZombie(plant)
	if not target then
		entry.cooldown = 0
		return
	end
	if cfg.range and plant:DistanceTo(target) > cfg.range then
		entry.cooldown = 0
		return
	end
	entry.cooldown = entry.cooldown + 1
	if entry.cooldown >= cfg.fireRate then
		entry.cooldown = 0
		local proj = Board:AddProjectile(plant.x, plant.y, plant.row, cfg.projectileType)
		if proj then
			if cfg.projectileDamage then proj:SetDamage(cfg.projectileDamage) end
			if cfg.onFire then cfg.onFire(plant, target) end
		end
	end
end

function PlantHelper._cleanup(plant)
	local ptr = plant._ptr
	PlantHelper._plants[ptr] = nil
	_timer.CancelAll(ptr)
end

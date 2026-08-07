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

function PlantHelper.SimpleAI(plant, states, initialState)
	if plant.isDead then return end
	local ptr = plant._ptr
	if initialState == nil then
		local names = {}
		for name, _ in pairs(states) do
			table.insert(names, name)
		end
		table.sort(names)
		initialState = states.idle and "idle" or names[1]
	end
	if initialState == nil or states[initialState] == nil then
		error("PlantHelper.SimpleAI: unknown initial state '" .. tostring(initialState) .. "'")
	end
	local entry = {
		type = "simple_ai",
		current = initialState,
		states = states,
	}
	local state = states[entry.current]
	if state and state.animation then
		local loopType = state.loopType or 0  -- LOOP
		plant:PlayBodyReanim(state.animation, loopType)
	end
	if state and state.on_enter then
		state.on_enter(plant)
	end
	local reanim = plant:GetBodyReanim()
	entry._last_loop = reanim and reanim:GetLoopCount() or 0
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
	local handle = {}
	function handle:Cancel()
		if id ~= nil then
			_timer.Cancel(id)
			id = nil
		end
	end
	return handle
end

function PlantHelper.RepeatAction(plant, interval, callback)
	local ptr = plant._ptr
	local id
	id = _timer.New(ptr, interval, function()
		local ok, result = pcall(callback, plant)
		if not ok then
			Game.Log("RepeatAction error: " .. tostring(result))
		elseif result == false then
			_timer.Cancel(id)
			id = nil
		end
	end, true, interval)
	local handle = {}
	function handle:Cancel()
		if id ~= nil then
			_timer.Cancel(id)
			id = nil
		end
	end
	return handle
end

function PlantHelper.Update(plant)
	if plant.isDead then
		PlantHelper._cleanup(plant)
		return
	end
	local ptr = plant._ptr
	_timer.TickAll(ptr)
	if plant.isDead then
		PlantHelper._cleanup(plant)
		return
	end
	local entry = PlantHelper._plants[ptr]
	if not entry then return end
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
					if PlantHelper._switchState(plant, entry, next_state) then
						return
					end
				end
			end
			entry._last_loop = loopCt
		end
	end
end

function PlantHelper._switchState(plant, entry, new_state)
	local state = entry.states[new_state]
	if not state then
		Game.Log("PlantHelper: unknown state '" .. tostring(new_state) .. "'")
		return false
	end
	local old = entry.states[entry.current]
	if old and old.on_exit then old.on_exit(plant) end
	entry.current = new_state
	if state.animation then
		plant:PlayBodyReanim(state.animation, state.loopType or 0)
	end
	if state.on_enter then state.on_enter(plant) end
	local reanim = plant:GetBodyReanim()
	entry._last_loop = reanim and reanim:GetLoopCount() or 0
	return true
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

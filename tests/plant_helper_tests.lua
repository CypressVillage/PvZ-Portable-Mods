local timers = {}
local cancelled = {}
local cancelAll = {}
local ticked = {}
local nextId = 0
local logs = {}

_timer = {}
function _timer.New(parent, frames, callback, repeating, interval)
    nextId = nextId + 1
    timers[nextId] = {
        parent = parent,
        frames = frames,
        callback = callback,
        repeating = repeating,
        interval = interval,
    }
    return nextId
end
function _timer.Cancel(id)
    table.insert(cancelled, id)
end
function _timer.CancelAll(parent)
    table.insert(cancelAll, parent)
end
function _timer.TickAll(parent)
    table.insert(ticked, parent)
end

Game = {}
function Game.Log(message)
    table.insert(logs, message)
end

Board = {}

local function makePlant(ptr)
    local reanim = {loopCount = 0}
    function reanim:GetLoopCount()
        return self.loopCount
    end

    local plant = {
        _ptr = ptr,
        isDead = false,
        reanim = reanim,
        animations = {},
    }
    function plant:PlayBodyReanim(name, loopType)
        table.insert(self.animations, name .. ":" .. tostring(loopType))
    end
    function plant:GetBodyReanim()
        return self.reanim
    end
    return plant
end

local idlePlant = makePlant("idle")
local entered = false
PlantHelper.SimpleAI(idlePlant, {
    zeta = {},
    idle = {animation = "idle_anim", on_enter = function() entered = true end},
})
assert(PlantHelper._plants.idle.current == "idle")
assert(idlePlant.animations[1] == "idle_anim:0")
assert(entered)

local sortedPlant = makePlant("sorted")
PlantHelper.SimpleAI(sortedPlant, {zeta = {}, alpha = {}})
assert(PlantHelper._plants.sorted.current == "alpha")

local explicitPlant = makePlant("explicit")
PlantHelper.SimpleAI(explicitPlant, {idle = {}, attack = {}}, "attack")
assert(PlantHelper._plants.explicit.current == "attack")
assert(not pcall(PlantHelper.SimpleAI, makePlant("bad"), {}, nil))
assert(not pcall(PlantHelper.SimpleAI, makePlant("bad2"), {idle = {}}, "missing"))

local switchPlant = makePlant("switch")
PlantHelper.SimpleAI(switchPlant, {idle = {on_exit = function() error("must not exit") end}})
local switchEntry = PlantHelper._plants.switch
assert(not PlantHelper._switchState(switchPlant, switchEntry, "missing"))
assert(switchEntry.current == "idle")
assert(logs[#logs]:find("unknown state", 1, true))

local timedPlant = makePlant("timed")
local timedCalls = 0
local timed = PlantHelper.TimedAction(timedPlant, 5, function(plant)
    assert(plant == timedPlant)
    timedCalls = timedCalls + 1
end)
assert(timers[1].parent == "timed" and timers[1].frames == 5 and not timers[1].repeating)
timers[1].callback()
assert(timedCalls == 1)
timed:Cancel()
timed:Cancel()
assert(#cancelled == 1 and cancelled[1] == 1)

local repeatPlant = makePlant("repeat")
local repeated = PlantHelper.RepeatAction(repeatPlant, 3, function()
    return false
end)
assert(timers[2].repeating and timers[2].interval == 3)
timers[2].callback()
repeated:Cancel()
assert(#cancelled == 2 and cancelled[2] == 2)

local standalone = makePlant("standalone")
PlantHelper.Update(standalone)
assert(ticked[#ticked] == "standalone")

local dead = makePlant("dead")
PlantHelper._plants.dead = {type = "simple_ai"}
dead.isDead = true
local tickCount = #ticked
PlantHelper.Update(dead)
assert(#ticked == tickCount)
assert(PlantHelper._plants.dead == nil)
assert(cancelAll[#cancelAll] == "dead")

PlantHelper._cleanup(dead)
assert(cancelAll[#cancelAll] == "dead")

-- Step 1: registration/lifecycle only, not a playable plant yet.
local settings = require('settings')

mod.register('plant', 'sample', {
    name = 'Foundation sample',
    cost = settings.cost,
})

mod.export('settings', settings)
mod.export('describe', function(request)
    return {name = request.name, framework = mod.api_version}
end)

mod.set_state({starts = 0})
mod.on('start', function()
    local state = mod.get_state()
    state.starts = state.starts + 1
    mod.set_state(state)
    mod.log('foundation started; gameplay integration is not installed')
end)

mod.on('save', function()
    -- Put explicit, plain serializable data in mod.set_state here.
end)

mod.on('restore', function(state)
    mod.log('restored explicit state, starts=' .. state.starts)
end)

mod.on('stop', function()
    mod.log('foundation stopped')
end)

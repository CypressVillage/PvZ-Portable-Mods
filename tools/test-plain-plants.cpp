// SPDX-License-Identifier: LGPL-3.0-or-later
// Standalone runtime contract test; see docs/plain-plant-api.zh-CN.md.
#include "Mod/ModRuntime.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using Mod::Data;

static void Check(bool value, const char* message)
{
	if (!value) throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
	try
	{
		Check(argc == 3, "usage: test-plain-plants REPOSITORY EMPTY_FIXTURE_DIRECTORY");
		fs::path repo = argv[1], root = argv[2];
		Check(!fs::exists(root), "fixture directory must not already exist");
		fs::create_directories(root);
		auto add = [&](const std::string& id, const std::string& source) {
			auto path = root / id;
			fs::create_directories(path);
			std::ofstream(path / "mod.json") << Data{{"id", id}, {"name", id}, {"version", "1"}, {"api_version", 1}};
			std::ofstream(path / "main.lua") << source;
			fs::copy_file(repo / "examples/mods/plain-plant/assets/stump.png", path / "plant.png");
		};
		const std::string define = "local id = plants.define('plain', {appearance={image='plant.png'}})\n";
		add("plain", define + R"lua(
assert(id == 'plain:plain')
assert(plants.get('plain').health == 300)
assert(plants.get(id).cost == 50)
assert(plants.get('missing') == nil)
local copy = plants.get(id)
copy.health = 999
assert(plants.get(id).health == 300)
hooks.on('plant.before_initialize', {type=id}, function(ctx) ctx.health=400 end)
hooks.on('plant.before_initialize', {type='other:plain'}, function(ctx) ctx.health=1 end)
hooks.on('plant.before_initialize', {type=id, priority=10}, function(ctx) ctx.health=ctx.health+10 end)
local token
token = hooks.on('plant.spawned', function(ctx)
    local state=mod.get_state(); state.spawns=(state.spawns or 0)+1; mod.set_state(state)
    assert(hooks.off(token))
end)
hooks.on('plant.can_place', function(ctx) ctx.cancel=true end)
)lua");
		add("second", R"lua(
hooks.on('plant.before_initialize', function(ctx) ctx.health=ctx.health*2 end)
hooks.on('plant.before_initialize', {priority=5}, function(ctx) ctx.health=-1 end)
hooks.on('plant.before_initialize', {priority=6}, function(ctx) error('expected hook error') end)
)lua");
		Mod::Runtime runtime;
		Check(runtime.Start(Mod::Discover(root), {}), "empty startup failed");
		Check(runtime.Plants().empty(), "disabled content published");
		runtime.Stop();
		Check(runtime.Start(Mod::Discover(root), {"plain", "second"}), "valid startup failed");
		Check(runtime.Plants().size() == 1, "wrong playable definition count");
		Check(runtime.Contents().Find("plain:plain") != nullptr, "definition missing from foundation registry");
		Check(runtime.PlantAsset("plain:plain") == fs::canonical(root / "plain/plant.png"), "incorrect asset resolution");
		Data ctx = {{"type", "plain:plain"}, {"health", 300}, {"column", 0}, {"row", 0}};
		runtime.Dispatch("plant.before_initialize", ctx);
		Check(ctx.at("health") == 810, "hook order, filtering or rollback failed");
		Check(runtime.Diagnostics().size() == 2, "invalid hook results/errors not reported");
		runtime.Dispatch("plant.spawned", ctx);
		runtime.Dispatch("plant.spawned", ctx);
		auto snapshot = runtime.SaveState();
		Check(snapshot && (*snapshot)["mods"]["plain"]["state"]["spawns"] == 1, "hook self-unsubscription failed");
		ctx["cancel"] = false;
		runtime.Dispatch("plant.can_place", ctx);
		Check(ctx["cancel"] == true, "placement cancellation failed");
		runtime.Stop();
		Check(runtime.Plants().empty(), "definitions survived stop");

		const std::vector<std::string> invalid = {
			"plants.define('bad', {health=0, appearance={image='plant.png'}})",
			"plants.define('bad', {health=1.5, appearance={image='plant.png'}})",
			"plants.define('bad', {appearance={image='../plant.png'}})",
			"plants.define('bad', {appearance={image='/etc/passwd'}})",
			"plants.define('bad', {appearance={image='missing.png'}})",
			"plants.define('bad', {appearance={image='plant.png'}, abilities={}})",
			define + define,
			define + "mod.register('plant', 'plain', {})",
			define + "hooks.on('plant.unknown', function() end)",
			define + "mod.on('start', function() plants.define('late', {}) end)",
		};
		for (size_t i = 0; i < invalid.size(); ++i)
		{
			auto id = "invalid" + std::to_string(i);
			add(id, invalid[i]);
			Check(!runtime.Start(Mod::Discover(root), {id}), "invalid definition accepted");
			Check(runtime.Plants().empty() && runtime.Contents().Entries().empty(), "failed registration leaked content");
		}
		std::cout << "plain plant runtime contracts passed (definitions, isolation, hooks, errors, cleanup)\n";
		return 0;
	}
	catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

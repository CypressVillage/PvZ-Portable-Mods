// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_MOD
#include "ModRuntime.h"
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

namespace Mod
{
namespace
{
struct Stack
{
	lua_State* L;
	int top;
	explicit Stack(lua_State* state) : L(state), top(lua_gettop(L)) {}
	~Stack() { lua_settop(L, top); }
};

std::string String(lua_State* L, int index)
{
	if (lua_type(L, index) != LUA_TSTRING) throw std::runtime_error("expected a string");
	size_t length;
	const char* value = lua_tolstring(L, index, &length);
	return std::string(value, length);
}

Data ReadData(lua_State* L, int index, std::set<const void*>& visited, int depth)
{
	if (depth > 64) throw std::runtime_error("data nesting exceeds 64 levels");
	index = lua_absindex(L, index);
	switch (lua_type(L, index))
	{
	case LUA_TNIL: return nullptr;
	case LUA_TBOOLEAN: return lua_toboolean(L, index) != 0;
	case LUA_TNUMBER:
		if (lua_isinteger(L, index)) return lua_tointeger(L, index);
		if (!std::isfinite(lua_tonumber(L, index))) throw std::runtime_error("non-finite number is not serializable");
		return lua_tonumber(L, index);
	case LUA_TSTRING: return String(L, index);
	case LUA_TTABLE:
	{
		if (lua_getmetatable(L, index)) { lua_pop(L, 1); throw std::runtime_error("metatables are not serializable"); }
		const void* identity = lua_topointer(L, index);
		if (!visited.insert(identity).second) throw std::runtime_error("cyclic data is not serializable");
		Data object = Data::object();
		std::map<lua_Integer, Data> array;
		lua_pushnil(L);
		while (lua_next(L, index))
		{
			auto value = ReadData(L, -1, visited, depth + 1);
			if (lua_type(L, -2) == LUA_TSTRING) object[String(L, -2)] = std::move(value);
			else if (lua_isinteger(L, -2) && lua_tointeger(L, -2) > 0) array.emplace(lua_tointeger(L, -2), std::move(value));
			else throw std::runtime_error("data keys must be strings or dense positive integer indices");
			lua_pop(L, 1);
		}
		visited.erase(identity);
		if (array.empty()) return object;
		if (!object.empty()) throw std::runtime_error("mixed array/object data is not serializable");
		Data result = Data::array();
		for (auto& [key, value] : array)
		{
			if (key != static_cast<lua_Integer>(result.size() + 1)) throw std::runtime_error("sparse arrays are not serializable");
			result.push_back(std::move(value));
		}
		return result;
	}
	default: throw std::runtime_error("only plain data may cross mod/state boundaries (no functions, threads or userdata)");
	}
}

Data ReadData(lua_State* L, int index)
{
	Stack stack(L);
	std::set<const void*> visited;
	return ReadData(L, index, visited, 0);
}

void ValidateData(const Data& value, int depth = 0)
{
	if (depth > 64) throw std::runtime_error("data nesting exceeds 64 levels");
	if (value.is_number_float() && !std::isfinite(value.get<double>())) throw std::runtime_error("non-finite data");
	if (value.is_binary() || value.is_discarded()) throw std::runtime_error("unsupported data type");
	if (value.is_number_unsigned() && value.get<uint64_t>() > static_cast<uint64_t>(LUA_MAXINTEGER))
		throw std::runtime_error("integer exceeds Lua integer range");
	if (value.is_structured()) for (const auto& child : value)
	{
		// Lua nil removes a table member: refuse data we cannot round-trip.
		if (child.is_null()) throw std::runtime_error("nested null cannot round-trip through a Lua table");
		ValidateData(child, depth + 1);
	}
}

void PushData(lua_State* L, const Data& value)
{
	if (value.is_null()) lua_pushnil(L);
	else if (value.is_boolean()) lua_pushboolean(L, value.get<bool>());
	else if (value.is_number_integer()) lua_pushinteger(L, value.get<lua_Integer>());
	else if (value.is_number()) lua_pushnumber(L, value.get<double>());
	else if (value.is_string())
	{
		const auto& text = value.get_ref<const std::string&>();
		lua_pushlstring(L, text.data(), text.size());
	}
	else if (value.is_array())
	{
		lua_newtable(L);
		for (size_t i = 0; i < value.size(); ++i) { PushData(L, value[i]); lua_rawseti(L, -2, i + 1); }
	}
	else
	{
		lua_newtable(L);
		for (auto it = value.begin(); it != value.end(); ++it)
		{
			lua_pushlstring(L, it.key().data(), it.key().size());
			PushData(L, it.value());
			lua_rawset(L, -3);
		}
	}
}

int Traceback(lua_State* L)
{
	const char* message = lua_tostring(L, 1);
	luaL_traceback(L, L, message ? message : "Lua error (non-string error value)", 1);
	return 1;
}

void ProtectedCall(lua_State* L, int arguments, int results)
{
	int base = lua_gettop(L) - arguments;
	lua_pushcfunction(L, Traceback);
	lua_insert(L, base);
	int status = lua_pcall(L, arguments, results, base);
	lua_remove(L, base);
	if (status != LUA_OK)
	{
		auto error = String(L, -1);
		lua_pop(L, 1);
		throw std::runtime_error(error);
	}
}
}

struct Runtime::Impl
{
	struct Script
	{
		Impl& host;
		Manifest manifest;
		lua_State* L = nullptr;
		std::map<std::string, int> modules;
		std::set<std::string> loading;
		std::map<std::string, int> callbacks;
		std::map<std::string, Data> exports;
		std::map<std::string, int> interfaces;
		struct Hook { int token; std::string event; std::string type; int priority; int callback; };
		std::vector<Hook> hooks;
		struct UiButton { Runtime::Button view; int callback; };
		std::vector<UiButton> buttons;
		Data state = Data::object();
		std::map<std::string, Data> entityStates;
		struct Attack { Data options; int targets; int strike; };
		std::map<std::string, Attack> attacks;
		struct AttackState { int launch = 0; int shooting = 0; int animation = 0; };
		std::map<std::string, AttackState> attackStates;
		bool entered = false;

		Script(Impl& owner, Manifest info) : host(owner), manifest(std::move(info)) {}
		~Script() { if (L) lua_close(L); }

		// lua_error longjmps: it must run AFTER all C++ exception/RAII scopes.
		template<int (Script::*Method)(lua_State*)>
		static int Bridge(lua_State* L)
		{
			auto* self = static_cast<Script*>(lua_touserdata(L, lua_upvalueindex(1)));
			try { return (self->*Method)(L); }
			catch (const std::exception& error) { lua_pushstring(L, error.what()); }
			catch (...) { lua_pushliteral(L, "unknown native mod API error"); }
			return lua_error(L);
		}

		template<int (Script::*Method)(lua_State*)>
		void Bind(const char* name)
		{
			lua_pushlightuserdata(L, this);
			lua_pushcclosure(L, Bridge<Method>, 1);
			lua_setfield(L, -2, name);
		}

		void RegistrationOnly()
		{
			if (host.phase != Phase::Registering) throw std::runtime_error("operation is only allowed during registration");
		}

		void Initialize()
		{
			L = luaL_newstate();
			if (!L) throw std::runtime_error("cannot create Lua state");
			const std::pair<const char*, lua_CFunction> libraries[] = {
				{LUA_GNAME, luaopen_base}, {LUA_TABLIBNAME, luaopen_table}, {LUA_STRLIBNAME, luaopen_string},
				{LUA_MATHLIBNAME, luaopen_math}, {LUA_UTF8LIBNAME, luaopen_utf8}
			};
			for (const auto& [name, open] : libraries) { luaL_requiref(L, name, open, 1); lua_pop(L, 1); }
			for (const char* name : {"dofile", "loadfile", "load", "collectgarbage", "print"})
				{ lua_pushnil(L); lua_setglobal(L, name); }
			lua_newtable(L);
			Bind<&Script::Register>("register");
			Bind<&Script::On>("on");
			Bind<&Script::Export>("export");
			Bind<&Script::Import>("import");
			Bind<&Script::Call>("call");
			Bind<&Script::SetState>("set_state");
			Bind<&Script::GetState>("get_state");
			Bind<&Script::Log>("log");
			lua_pushlstring(L, manifest.id.data(), manifest.id.size()); lua_setfield(L, -2, "id");
			lua_pushinteger(L, ApiVersion); lua_setfield(L, -2, "api_version");
			lua_setglobal(L, "mod");
			lua_newtable(L);
			Bind<&Script::DefinePlant>("define");
			Bind<&Script::GetPlant>("get");
			Bind<&Script::PlayAnimation>("play_animation");
			lua_setglobal(L, "plants");
			lua_newtable(L);
			Bind<&Script::GetEntity>("get");
			lua_setglobal(L, "entities");
			lua_newtable(L);
			Bind<&Script::CollectCollectible>("collect");
			lua_setglobal(L, "collectibles");
			lua_newtable(L);
			Bind<&Script::QueryZombies>("query");
			Bind<&Script::DamageZombie>("damage");
			lua_setglobal(L, "zombies");
			lua_newtable(L);
			Bind<&Script::Lightning>("lightning");
			lua_setglobal(L, "effects");
			lua_newtable(L);
			Bind<&Script::HookOn>("on");
			Bind<&Script::HookOff>("off");
			lua_setglobal(L, "hooks");
			lua_newtable(L);
			Bind<&Script::SetTimeScale>("set_scale");
			Bind<&Script::GetTimeScale>("get_scale");
			lua_setglobal(L, "time");
			lua_newtable(L);
			Bind<&Script::CreateButton>("button");
			Bind<&Script::SetButtonText>("set_text");
			lua_setglobal(L, "ui");
			lua_pushlightuserdata(L, this);
			lua_pushcclosure(L, Bridge<&Script::Require>, 1);
			lua_setglobal(L, "require");
		}

		void Execute()
		{
			Stack stack(L);
			auto text = ReadText(ResolveFile(manifest.directory, manifest.entry));
			std::string source = "@" + manifest.id + "/" + manifest.entry;
			if (luaL_loadbufferx(L, text.data(), text.size(), source.c_str(), "t") != LUA_OK)
				throw std::runtime_error(String(L, -1));
			ProtectedCall(L, 0, 0);
		}

		void RunningOnly()
		{
			if (host.phase != Phase::Running) throw std::runtime_error("operation requires a running runtime");
		}

		int SetTimeScale(lua_State* state)
		{
			RunningOnly();
			if (!host.boardActive) throw std::runtime_error("time scale requires an active board");
			if (lua_type(state, 1) != LUA_TNUMBER) throw std::runtime_error("scale must be a number");
			double value = lua_tonumber(state, 1);
			if (!std::isfinite(value) || value < 0.25 || value > 4.0) throw std::runtime_error("scale must be finite and in 0.25..4");
			host.timeScale = value;
			return 0;
		}

		int GetTimeScale(lua_State* state) { lua_pushnumber(state, host.timeScale); return 1; }

		int CreateButton(lua_State* state)
		{
			RegistrationOnly();
			auto name = String(state, 1);
			if (!ValidName(name)) throw std::runtime_error("invalid button name");
			auto id = manifest.id + ":" + name;
			for (const auto& button : buttons) if (button.view.id == id) throw std::runtime_error("duplicate button: " + id);
			auto options = ReadData(state, 2);
			if (!options.is_object()) throw std::runtime_error("button options must be an object");
			for (auto it = options.begin(); it != options.end(); ++it)
				if (it.key() != "text" && it.key() != "right" && it.key() != "top" && it.key() != "width" && it.key() != "height")
					throw std::runtime_error("unsupported button option: " + it.key());
			auto text = options.at("text").get<std::string>();
			if (text.empty() || text.size() > 128 || text.find('\0') != std::string::npos) throw std::runtime_error("button text must contain 1..128 bytes without NUL");
			auto integer = [&](const char* key, int fallback, int low, int high) {
				Data value = options.value(key, Data(fallback));
				if (!value.is_number_integer() || value < low || value > high) throw std::runtime_error(std::string("invalid button ") + key);
				return value.get<int>();
			};
			int width = integer("width", 80, 40, 300), height = integer("height", 36, 24, 100);
			int right = integer("right", 128, 0, 800 - width), top = integer("top", 0, 0, 600 - height);
			if (!lua_isfunction(state, 3)) throw std::runtime_error("button callback must be a function");
			lua_pushvalue(state, 3);
			buttons.push_back({{id, text, right, top, width, height}, luaL_ref(state, LUA_REGISTRYINDEX)});
			lua_pushlstring(state, id.data(), id.size());
			return 1;
		}

		int SetButtonText(lua_State* state)
		{
			RunningOnly();
			auto id = String(state, 1), text = String(state, 2);
			if (id.find(':') == std::string::npos) id = manifest.id + ":" + id;
			if (text.empty() || text.size() > 128 || text.find('\0') != std::string::npos) throw std::runtime_error("invalid button text");
			for (auto& button : buttons) if (button.view.id == id) { button.view.text = text; return 0; }
			throw std::runtime_error("button is missing or belongs to another mod: " + id);
		}

		int Require(lua_State* state)
		{
			auto name = String(state, 1);
			if (!ValidName(name) || name.find('-') != std::string::npos) throw std::runtime_error("invalid module name");
			if (auto it = modules.find(name); it != modules.end()) { lua_rawgeti(L, LUA_REGISTRYINDEX, it->second); return 1; }
			if (!loading.insert(name).second) throw std::runtime_error("module cycle: " + name);
			try
			{
				std::string relative = name;
				std::replace(relative.begin(), relative.end(), '.', '/');
				relative += ".lua";
				auto text = ReadText(ResolveFile(manifest.directory, relative));
				auto source = "@" + manifest.id + "/" + relative;
				if (luaL_loadbufferx(L, text.data(), text.size(), source.c_str(), "t") != LUA_OK)
					throw std::runtime_error(String(L, -1));
				ProtectedCall(L, 0, 1);
				if (lua_isnil(L, -1)) { lua_pop(L, 1); lua_pushboolean(L, 1); }
				lua_pushvalue(L, -1);
				modules.emplace(name, luaL_ref(L, LUA_REGISTRYINDEX));
				loading.erase(name);
				return 1;
			}
			catch (...) { loading.erase(name); throw; }
		}

		int Register(lua_State* state)
		{
			RegistrationOnly();
			host.staging.Add(manifest.id, String(state, 1), String(state, 2), ReadData(state, 3));
			return 0;
		}

		int DefinePlant(lua_State* state)
		{
			RegistrationOnly();
			auto name = String(state, 1);
			if (!lua_istable(state, 2)) throw std::runtime_error("plant definition must be an object");
			lua_newtable(state);
			int plain = lua_gettop(state);
			lua_pushnil(state);
			while (lua_next(state, 2))
			{
				if (String(state, -2) != "attack")
				{
					lua_pushvalue(state, -2);
					lua_pushvalue(state, -2);
					lua_rawset(state, plain);
				}
				lua_pop(state, 1);
			}
			auto definition = ReadData(state, plain);
			lua_pop(state, 1);
			lua_getfield(state, 2, "attack");
			if (!lua_isnil(state, -1))
			{
				Data options = Data::object();
				for (const char* field : {"timing", "animation", "animation_ticks"})
				{
					lua_getfield(state, -1, field);
					options[field] = ReadData(state, -1);
					lua_pop(state, 1);
				}
				lua_getfield(state, -1, "targets");
				int targets = luaL_ref(state, LUA_REGISTRYINDEX);
				lua_getfield(state, -1, "on_strike");
				int strike = luaL_ref(state, LUA_REGISTRYINDEX);
				attacks.emplace(manifest.id + ":" + name, Attack{options, targets, strike});
				definition["attack"] = std::move(options);
			}
			lua_pop(state, 1);
			if (!definition.is_object()) throw std::runtime_error("plant definition must be an object");
			const std::set<std::string> fields = {"name", "description", "cost", "cooldown", "health", "appearance", "card", "upgrade", "traits", "attack"};
			for (auto it = definition.begin(); it != definition.end(); ++it)
				if (!fields.count(it.key())) throw std::runtime_error("unsupported plant field: " + it.key());
			for (const char* field : {"name", "description"})
			{
				if (!definition.contains(field)) definition[field] = field == std::string("name") ? name : "";
				if (!definition[field].is_string() || (field == std::string("name") && definition[field].get<std::string>().empty()))
					throw std::runtime_error(std::string(field) + " must be a string (name must not be empty)");
			}
			for (const auto& [field, fallback, minimum, maximum] : {
				std::tuple{"cost", 50, 0, 9990}, std::tuple{"cooldown", 750, 1, 1000000}, std::tuple{"health", 300, 1, 100000000}})
			{
				if (!definition.contains(field)) definition[field] = fallback;
				if (!definition[field].is_number_integer() || definition[field] < minimum || definition[field] > maximum)
					throw std::runtime_error(std::string(field) + " must be an integer in " + std::to_string(minimum) + ".." + std::to_string(maximum));
			}
			if (definition.contains("upgrade"))
			{
				const auto& upgrade = definition.at("upgrade");
				if (!upgrade.is_object() || upgrade.size() != 1 || !upgrade.at("from").is_string())
					throw std::runtime_error("upgrade must contain only from = 'plant-id'");
			}
			if (definition.contains("traits"))
			{
				if (!definition["traits"].is_object()) throw std::runtime_error("traits must be an object");
				for (auto it = definition["traits"].begin(); it != definition["traits"].end(); ++it)
					if ((it.key() != "fungus" && it.key() != "nocturnal") || !it.value().is_boolean())
						throw std::runtime_error("traits supports only fungus and nocturnal booleans");
			}
			for (const char* field : {"appearance", "card"})
			{
				if (field == std::string("card") && !definition.contains(field))
				{
					if (definition.at("appearance").contains("reanim")) throw std::runtime_error("Reanim plants require a card image");
					definition[field] = definition.at("appearance");
				}
				const auto& asset = definition.at(field);
				if (field == std::string("appearance") && asset.contains("reanim"))
				{
					for (auto it = asset.begin(); it != asset.end(); ++it)
						if (it.key() != "reanim" && it.key() != "animations") throw std::runtime_error("unsupported Reanim appearance field: " + it.key());
					if (ResolveFile(manifest.directory, asset.at("reanim").get<std::string>()).extension() != ".reanim")
						throw std::runtime_error("external animations must be .reanim XML files");
					const auto& actions = asset.at("animations");
					if (!actions.is_object() || actions.size() != 3) throw std::runtime_error("animations must contain idle, attack and sleep");
					for (const char* action : {"idle", "attack", "sleep"})
						if (!actions.at(action).is_string() || actions.at(action).get<std::string>().empty())
							throw std::runtime_error("animation actions must name Reanim tracks");
					continue;
				}
				if (!asset.is_object() || asset.size() != 1 || !asset.contains("image") || !asset["image"].is_string())
					throw std::runtime_error(std::string(field) + " must contain only image = 'relative/path.png'");
				auto path = ResolveFile(manifest.directory, asset["image"].get<std::string>());
				if (path.extension() != ".png") throw std::runtime_error("plant images must be PNG files");
			}
			host.staging.Add(manifest.id, "plant", name, definition);
			auto id = manifest.id + ":" + name;
			host.plants.emplace(id, Content{id, "plant", manifest.id, std::move(definition)});
			lua_pushlstring(state, id.data(), id.size());
			return 1;
		}

		int Random(int minimum, int maximum)
		{
			lua_getglobal(L, "math");
			lua_getfield(L, -1, "random");
			lua_remove(L, -2);
			lua_pushinteger(L, minimum);
			lua_pushinteger(L, maximum);
			ProtectedCall(L, 2, 1);
			int result = lua_tointeger(L, -1);
			lua_pop(L, 1);
			return result;
		}

		Data AttackTargets(const Attack& attack, const std::string& entity)
		{
			lua_rawgeti(L, LUA_REGISTRYINDEX, attack.targets);
			PushData(L, entity);
			ProtectedCall(L, 1, 1);
			auto result = ReadData(L, -1);
			lua_pop(L, 1);
			return result;
		}

		void UpdateAttack(const Attack& attack, const Data& context)
		{
			const auto& timing = attack.options.at("timing");
			auto entity = context.at("entity").get<std::string>();
			auto [entry, inserted] = attackStates.try_emplace(entity);
			auto& state = entry->second;
			if (inserted) state.launch = Random(timing.at("initial_min"), timing.at("initial_max"));
			if (state.animation > 0 && --state.animation == 0)
				host.gameApi("plants.play_animation", {{"entity", entity}, {"action", "idle"}});
			// Advance the previous attack before starting a new one, as native shooters do.
			if (state.shooting > 0 && --state.shooting == timing.at("duration").get<int>() - timing.at("windup").get<int>())
			{
				auto input = context;
				input["targets"] = AttackTargets(attack, entity);
				input["attack"] = timing;
				lua_rawgeti(L, LUA_REGISTRYINDEX, attack.strike);
				PushData(L, input);
				ProtectedCall(L, 1, 0);
			}
			if (--state.launch <= 0)
			{
				state.launch = timing.at("interval").get<int>() - Random(0, timing.at("interval_variation").get<int>() - 1);
				if (!AttackTargets(attack, entity).empty())
				{
					state.shooting = timing.at("duration");
					state.animation = attack.options.at("animation_ticks");
					host.gameApi("plants.play_animation", {{"entity", entity}, {"action", attack.options.at("animation")}});
				}
			}
		}

		int GetPlant(lua_State* state)
		{
			auto id = String(state, 1);
			if (id.find(':') == std::string::npos) id = manifest.id + ":" + id;
			if (id.starts_with("base:")) { PushData(state, host.gameApi("plants.get", id)); return 1; }
			auto found = host.plants.find(id);
			if (found == host.plants.end()) { lua_pushnil(state); return 1; }
			auto value = found->second.definition;
			value["id"] = id;
			PushData(state, value);
			return 1;
		}

		int GameCall(lua_State* state, const char* operation, Data arguments)
		{
			if (host.phase != Phase::Running || !host.boardActive) throw std::runtime_error("game operation requires an active level");
			PushData(state, host.gameApi(operation, arguments));
			return 1;
		}
		int GetEntity(lua_State* state) { return GameCall(state, "entities.get", String(state, 1)); }
		int CollectCollectible(lua_State* state) { return GameCall(state, "collectibles.collect", String(state, 1)); }
		int QueryZombies(lua_State* state) { return GameCall(state, "zombies.query", ReadData(state, 1)); }
		int DamageZombie(lua_State* state) { return GameCall(state, "zombies.damage", {{"entity", String(state, 1)}, {"options", ReadData(state, 2)}}); }
		int PlayAnimation(lua_State* state) { return GameCall(state, "plants.play_animation", {{"entity", String(state, 1)}, {"action", String(state, 2)}}); }
		int Lightning(lua_State* state) { return GameCall(state, "effects.lightning", ReadData(state, 1)); }

		int HookOn(lua_State* state)
		{
			RegistrationOnly();
			auto event = String(state, 1);
			static const std::set<std::string> events = {"plant.before_initialize", "plant.spawned", "plant.removed", "plant.can_place", "plant.updated", "collectible.spawned", "level.started", "level.ended"};
			if (!events.count(event)) throw std::runtime_error("unsupported hook: " + event);
			int callback = 2, priority = 0;
			std::string type;
			if (lua_istable(state, 2))
			{
				auto options = ReadData(state, 2);
				for (auto it = options.begin(); it != options.end(); ++it)
					if (it.key() != "type" && it.key() != "priority") throw std::runtime_error("unknown hook option: " + it.key());
				if (options.contains("type"))
				{
					type = options.at("type").get<std::string>();
					if (type.find(':') == std::string::npos) type = manifest.id + ":" + type;
				}
				if (options.contains("priority"))
				{
					const auto& value = options.at("priority");
					if (!value.is_number_integer() || value < -1000000 || value > 1000000) throw std::runtime_error("invalid hook priority");
					priority = value.get<int>();
				}
				callback = 3;
			}
			if (!lua_isfunction(state, callback)) throw std::runtime_error("hook callback must be a function");
			lua_pushvalue(state, callback);
			int token = ++host.nextHook;
			hooks.push_back({token, event, type, priority, luaL_ref(state, LUA_REGISTRYINDEX)});
			lua_pushinteger(state, token);
			return 1;
		}

		int HookOff(lua_State* state)
		{
			if (!lua_isinteger(state, 1)) throw std::runtime_error("hook token must be an integer");
			auto token = lua_tointeger(state, 1);
			for (auto it = hooks.begin(); it != hooks.end(); ++it) if (it->token == token)
			{
				luaL_unref(state, LUA_REGISTRYINDEX, it->callback);
				hooks.erase(it);
				lua_pushboolean(state, true);
				return 1;
			}
			lua_pushboolean(state, false);
			return 1;
		}

		int On(lua_State* state)
		{
			RegistrationOnly();
			auto event = String(state, 1);
			if (event != "start" && event != "stop" && event != "save" && event != "restore")
				throw std::runtime_error("unknown lifecycle event: " + event);
			if (!lua_isfunction(state, 2)) throw std::runtime_error("callback must be a function");
			if (callbacks.count(event)) throw std::runtime_error("duplicate lifecycle callback: " + event);
			lua_pushvalue(L, 2);
			callbacks.emplace(event, luaL_ref(L, LUA_REGISTRYINDEX));
			return 0;
		}

		int Export(lua_State* state)
		{
			RegistrationOnly();
			auto name = String(state, 1);
			if (!ValidName(name) || exports.count(name) || interfaces.count(name)) throw std::runtime_error("invalid/duplicate export: " + name);
			if (lua_isfunction(state, 2))
			{
				lua_pushvalue(L, 2);
				interfaces.emplace(name, luaL_ref(L, LUA_REGISTRYINDEX));
			}
			else exports.emplace(name, ReadData(state, 2));
			return 0;
		}

		Script& Dependency(lua_State* state)
		{
			if (host.phase == Phase::Empty || host.phase == Phase::Failed)
				throw std::runtime_error("mod interfaces are no longer available");
			auto id = String(state, 1);
			if (std::find(manifest.dependencies.begin(), manifest.dependencies.end(), id) == manifest.dependencies.end())
				throw std::runtime_error("import/call requires a declared direct dependency: " + id);
			for (auto& script : host.scripts) if (script->manifest.id == id) return *script;
			throw std::runtime_error("dependency is unavailable: " + id);
		}

		int Import(lua_State* state)
		{
			auto& dependency = Dependency(state);
			auto name = String(state, 2);
			auto found = dependency.exports.find(name);
			if (found == dependency.exports.end()) throw std::runtime_error("missing data export: " + dependency.manifest.id + ":" + name);
			PushData(state, found->second);
			return 1;
		}

		int Call(lua_State* state)
		{
			if (host.phase == Phase::Empty || host.phase == Phase::Failed)
				throw std::runtime_error("mod interfaces are no longer available");
			auto& dependency = Dependency(state);
			auto name = String(state, 2);
			auto found = dependency.interfaces.find(name);
			if (found == dependency.interfaces.end()) throw std::runtime_error("missing interface export: " + name);
			if (host.callDepth >= 32) throw std::runtime_error("cross-mod call depth exceeded");
			auto argument = ReadData(state, 3);
			Stack stack(dependency.L);
			++host.callDepth;
			try
			{
				lua_rawgeti(dependency.L, LUA_REGISTRYINDEX, found->second);
				PushData(dependency.L, argument);
				ProtectedCall(dependency.L, 1, 1);
				auto result = ReadData(dependency.L, -1);
				PushData(state, result);
				--host.callDepth;
				return 1;
			}
			catch (const std::exception& error)
			{
				--host.callDepth;
				throw Error(dependency.manifest.id, "export " + name, error.what());
			}
		}

		int SetState(lua_State* state) { this->state = ReadData(state, 1); return 0; }
		int GetState(lua_State* state) { PushData(state, this->state); return 1; }
		int Log(lua_State* state)
		{
			auto message = String(state, 1);
			if (host.logger) host.logger(manifest.id, message);
			return 0;
		}

		void Callback(const std::string& event, const Data* argument = nullptr)
		{
			auto found = callbacks.find(event);
			if (found == callbacks.end()) return;
			Stack stack(L);
			try
			{
				lua_rawgeti(L, LUA_REGISTRYINDEX, found->second);
				if (argument) PushData(L, *argument);
				ProtectedCall(L, argument ? 1 : 0, 0);
			}
			catch (const std::exception& error) { throw Error(manifest.id, event, error.what()); }
		}
	};

	Logger logger;
	Phase phase = Phase::Empty;
	Registry staging;
	Registry committed;
	std::map<std::string, Content> plants;
	int nextHook = 0;
	std::optional<Error> error;
	std::vector<std::string> diagnostics;
	std::vector<std::unique_ptr<Script>> scripts;
	int callDepth = 0;
	bool boardActive = false;
	GameApi gameApi;
	double timeScale = 1.0, timeRemainder = 0.0;

	explicit Impl(Logger sink) : logger(std::move(sink)) {}

	void Cleanup()
	{
		phase = Phase::Stopping;
		for (auto it = scripts.rbegin(); it != scripts.rend(); ++it)
		{
			if (!(*it)->entered) continue;
			try { (*it)->Callback("stop"); }
			catch (const std::exception& failure) { diagnostics.emplace_back(failure.what()); }
		}
		// lua_close can run user __gc finalizers. All cross-VM calls must be
		// disabled before any VM is destroyed (including failure cleanup).
		phase = Phase::Empty;
		scripts.clear();
		staging.Clear();
		committed.Clear();
		plants.clear();
		boardActive = false;
		timeScale = 1.0;
		timeRemainder = 0.0;
		nextHook = 0;
		callDepth = 0;
	}

	void Fail(const Error& failure)
	{
		error = failure;
		diagnostics.emplace_back(failure.what());
		Cleanup();
		phase = Phase::Failed;
	}
};

Runtime::Runtime(Logger logger) : mImpl(std::make_unique<Impl>(std::move(logger))) {}
Runtime::~Runtime() { Stop(); }
Phase Runtime::GetPhase() const { return mImpl->phase; }
const Registry& Runtime::Contents() const { return mImpl->committed; }
const std::optional<Error>& Runtime::LastError() const { return mImpl->error; }
const std::vector<std::string>& Runtime::Diagnostics() const { return mImpl->diagnostics; }
const std::map<std::string, Content>& Runtime::Plants() const { return mImpl->plants; }
void Runtime::SetGameApi(GameApi api) { mImpl->gameApi = std::move(api); }
void Runtime::ForgetEntity(const std::string& entity)
{
	for (auto& script : mImpl->scripts)
	{
		script->entityStates.erase(entity);
		script->attackStates.erase(entity);
	}
}

bool Runtime::HasGameHooks() const
{
	for (const auto& script : mImpl->scripts) if (!script->hooks.empty() || !script->buttons.empty()) return true;
	return false;
}

std::vector<Runtime::Button> Runtime::Buttons() const
{
	std::vector<Button> result;
	if (mImpl->phase == Phase::Running)
		for (const auto& script : mImpl->scripts) for (const auto& button : script->buttons) result.push_back(button.view);
	return result;
}

void Runtime::ClickButton(const std::string& id)
{
	if (mImpl->phase != Phase::Running || !mImpl->boardActive) return;
	for (auto& script : mImpl->scripts) for (const auto& button : script->buttons) if (button.view.id == id)
	{
		Stack stack(script->L);
		try { lua_rawgeti(script->L, LUA_REGISTRYINDEX, button.callback); ProtectedCall(script->L, 0, 0); }
		catch (const std::exception& failure)
		{
			Error error(script->manifest.id, "ui.click " + id, failure.what());
			mImpl->diagnostics.emplace_back(error.what());
			if (mImpl->logger) mImpl->logger(script->manifest.id, error.what());
		}
		return;
	}
}

double Runtime::TimeScale() const { return mImpl->timeScale; }
int Runtime::AdvanceTime(bool active)
{
	if (!active || !mImpl->boardActive || mImpl->phase != Phase::Running) { mImpl->timeRemainder = 0; return 1; }
	mImpl->timeRemainder += mImpl->timeScale;
	int steps = static_cast<int>(mImpl->timeRemainder);
	mImpl->timeRemainder -= steps;
	return steps;
}
void Runtime::BeginBoard()
{
	mImpl->boardActive = true;
	mImpl->timeScale = 1;
	mImpl->timeRemainder = 0;
	Data context = Data::object();
	Dispatch("level.started", context);
}
void Runtime::EndBoard()
{
	mImpl->boardActive = false;
	mImpl->timeScale = 1;
	mImpl->timeRemainder = 0;
	Data context = Data::object();
	Dispatch("level.ended", context);
	for (auto& script : mImpl->scripts)
	{
		script->entityStates.clear();
		script->attackStates.clear();
	}
}

std::string Runtime::ConfigurationIdentity() const
{
	Data identity = Data::array();
	for (const auto& script : mImpl->scripts)
		identity.push_back({{"id", script->manifest.id}, {"version", script->manifest.version}});
	return identity.dump();
}

std::filesystem::path Runtime::PlantAsset(const std::string& id, bool card) const
{
	const auto& plant = mImpl->plants.at(id);
	for (const auto& script : mImpl->scripts) if (script->manifest.id == plant.owner)
	{
		const auto& asset = plant.definition.at(card ? "card" : "appearance");
		return ResolveFile(script->manifest.directory, asset.at(asset.contains("reanim") ? "reanim" : "image").get<std::string>());
	}
	throw std::runtime_error("plant owner is unavailable: " + id);
}

void Runtime::Dispatch(const std::string& event, Data& context)
{
	auto& impl = *mImpl;
	if (impl.phase != Phase::Running) return;
	if (event == "plant.updated") for (auto& script : impl.scripts)
	{
		auto attack = script->attacks.find(context.at("type").get<std::string>());
		if (attack == script->attacks.end()) continue;
		Stack stack(script->L);
		try { script->UpdateAttack(attack->second, context); }
		catch (const std::exception& failure)
		{
			Error error(script->manifest.id, "plant.attack", failure.what());
			impl.diagnostics.emplace_back(error.what());
			if (impl.logger) impl.logger(script->manifest.id, error.what());
		}
	}
	struct Entry { Impl::Script* script; int token; int priority; };
	std::vector<Entry> entries;
	for (auto& script : impl.scripts) for (const auto& hook : script->hooks)
		if (hook.event == event && (hook.type.empty() || hook.type == context.value("type", std::string())))
			entries.push_back({script.get(), hook.token, hook.priority});
	std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.priority < b.priority; });
	for (const auto& entry : entries)
	{
		auto& script = *entry.script;
		auto found = std::find_if(script.hooks.begin(), script.hooks.end(), [&](const auto& hook) { return hook.token == entry.token; });
		if (found == script.hooks.end()) continue;
		Stack stack(script.L);
		try
		{
			lua_rawgeti(script.L, LUA_REGISTRYINDEX, found->callback);
			auto input = context;
			if (event == "plant.updated")
			{
				auto& state = script.entityStates[context.at("entity").get<std::string>()];
				if (state.is_null()) state = Data::object();
				input["state"] = state;
			}
			PushData(script.L, input);
			lua_pushvalue(script.L, -1);
			int reference = luaL_ref(script.L, LUA_REGISTRYINDEX);
			try { ProtectedCall(script.L, 1, 0); }
			catch (...) { luaL_unref(script.L, LUA_REGISTRYINDEX, reference); throw; }
			lua_rawgeti(script.L, LUA_REGISTRYINDEX, reference);
			luaL_unref(script.L, LUA_REGISTRYINDEX, reference);
			auto changed = ReadData(script.L, -1);
			if (event == "plant.updated") script.entityStates[context.at("entity").get<std::string>()] = changed.at("state");
			// Identity and coordinates are observational. Only documented fields mutate.
			if (event == "plant.before_initialize")
			{
				const auto& health = changed.at("health");
				if (!health.is_number_integer() || health < 1 || health > 100000000) throw std::runtime_error("hook health must be an integer in 1..100000000");
				context["health"] = health;
			}
			else if (event == "plant.can_place")
			{
				if (!changed.at("cancel").is_boolean()) throw std::runtime_error("hook cancel must be boolean");
				context["cancel"] = changed.at("cancel");
			}
		}
		catch (const std::exception& failure)
		{
			Error error(script.manifest.id, event, failure.what());
			impl.diagnostics.emplace_back(error.what());
			if (impl.logger) impl.logger(script.manifest.id, error.what());
		}
	}
}

bool Runtime::Start(const Discovery& discovery, const std::vector<std::string>& enabled)
{
	auto& impl = *mImpl;
	if (impl.phase != Phase::Empty && impl.phase != Phase::Failed) return false;
	impl.error.reset();
	impl.diagnostics.clear();
	try
	{
		auto ordered = ResolveOrder(discovery, enabled);
		impl.phase = Phase::Registering;
		for (const auto& manifest : ordered)
		{
			try
			{
				impl.scripts.push_back(std::make_unique<Impl::Script>(impl, manifest));
				auto& script = *impl.scripts.back();
				script.Initialize();
				script.entered = true;
				script.Execute();
			}
			catch (const std::exception& failure) { throw Error(manifest.id, "register", failure.what()); }
		}
		impl.phase = Phase::Starting;
		for (auto& script : impl.scripts) script->Callback("start");
		impl.committed = std::move(impl.staging);
		impl.staging.Clear();
		impl.phase = Phase::Running;
		return true;
	}
	catch (const Error& failure) { impl.Fail(failure); }
	catch (const std::exception& failure) { impl.Fail(Error("framework", "start", failure.what())); }
	return false;
}

void Runtime::Stop()
{
	if (mImpl->phase == Phase::Empty || mImpl->phase == Phase::Failed) return;
	mImpl->Cleanup();
	mImpl->phase = Phase::Empty;
}

std::optional<Data> Runtime::SaveState()
{
	auto& impl = *mImpl;
	if (impl.phase != Phase::Running) return std::nullopt;
	try
	{
		impl.phase = Phase::Saving;
		Data snapshot = {{"format", 1}, {"mods", Data::object()}};
		for (auto& script : impl.scripts)
		{
			script->Callback("save");
			snapshot["mods"][script->manifest.id] = {{"version", script->manifest.version}, {"state", script->state}};
		}
		impl.phase = Phase::Running;
		return snapshot;
	}
	catch (const Error& failure) { impl.Fail(failure); }
	catch (const std::exception& failure) { impl.Fail(Error("framework", "save", failure.what())); }
	return std::nullopt;
}

bool Runtime::RestoreState(const Data& snapshot)
{
	auto& impl = *mImpl;
	if (impl.phase != Phase::Running) return false;
	try
	{
		if (snapshot.at("format") != 1 || !snapshot.at("mods").is_object()) throw std::runtime_error("invalid state envelope");
		for (auto it = snapshot.at("mods").begin(); it != snapshot.at("mods").end(); ++it)
		{
			auto script = std::find_if(impl.scripts.begin(), impl.scripts.end(), [&it](const auto& item) { return item->manifest.id == it.key(); });
			if (script == impl.scripts.end()) throw Error(it.key(), "restore preflight", "required mod is missing/disabled");
			if (it.value().at("version") != (*script)->manifest.version) throw Error(it.key(), "restore preflight", "exact mod version required");
			ValidateData(it.value().at("state"));
		}
	}
	catch (const std::exception& failure)
	{
		// Preflight is read-only; the caller may retry with a different snapshot.
		impl.error = Error("framework", "restore preflight", failure.what());
		impl.diagnostics.emplace_back(impl.error->what());
		return false;
	}
	try
	{
		impl.phase = Phase::Restoring;
		for (auto& script : impl.scripts)
			if (snapshot["mods"].contains(script->manifest.id)) script->state = snapshot["mods"][script->manifest.id]["state"];
		for (auto& script : impl.scripts)
			if (snapshot["mods"].contains(script->manifest.id)) script->Callback("restore", &script->state);
		impl.phase = Phase::Running;
		impl.error.reset();
		return true;
	}
	catch (const Error& failure) { impl.Fail(failure); }
	catch (const std::exception& failure) { impl.Fail(Error("framework", "restore", failure.what())); }
	return false;
}
}
#endif

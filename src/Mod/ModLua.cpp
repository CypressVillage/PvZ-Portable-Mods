/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModLua.h"
#include "ModSave.h"
#include "ModRegistry.h"
#include "../LawnApp.h"
#include "../Lawn/Board.h"
#include "../Lawn/Plant.h"
#include "../Lawn/Zombie.h"
#include "Sexy.TodLib/TodDebug.h"
#include "SexyAppFramework/Common.h"
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(PVZ_ENABLE_LUA)
#include <lua.hpp>
#endif

ModLua gModLua;

namespace
{
	std::string ReadFileText(const std::string& path)
	{
		std::ifstream file(Sexy::PathFromU8(path), std::ios::in | std::ios::binary);
		if (!file)
			return std::string();
		std::ostringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}

#if defined(PVZ_ENABLE_LUA)
	int Lua_GameLog(lua_State* L)
	{
		const char* msg = lua_tostring(L, 1);
		if (msg && msg[0] != '\0')
			TodLog("[Lua] %s", msg);
		return 0;
	}

	std::string Lua_GetModId(lua_State* L)
	{
		std::string modId;
		lua_getglobal(L, "__mod_id");
		if (lua_isstring(L, -1))
			modId = lua_tostring(L, -1);
		lua_pop(L, 1);
		return modId;
	}

	int Lua_GameSaveModData(lua_State* L)
	{
		const char* key = lua_tostring(L, 1);
		const char* value = lua_tostring(L, 2);
		if (!key || !value)
			return 0;

		std::string modId = Lua_GetModId(L);
		if (!modId.empty())
			ModSave::SaveValue(modId, key, value);
		return 0;
	}

	int Lua_GameLoadModData(lua_State* L)
	{
		const char* key = lua_tostring(L, 1);
		if (!key)
		{
			lua_pushnil(L);
			return 1;
		}

		std::string modId = Lua_GetModId(L);
		std::string value;
		if (!modId.empty() && ModSave::LoadValue(modId, key, value))
		{
			lua_pushlstring(L, value.c_str(), value.size());
			return 1;
		}

		lua_pushnil(L);
		return 1;
	}

	void Lua_SetStringGlobal(lua_State* L, const char* name, const std::string& value)
	{
		lua_pushlstring(L, value.c_str(), value.size());
		lua_setglobal(L, name);
	}

	// Entity wrapper
	struct LuaEntity {
		int type; // 0 = plant, 1 = zombie
		union {
			Plant* plant;
			Zombie* zombie;
		} ptr;
	};

	int Lua_EntityIndex(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		const char* key = luaL_checkstring(L, 2);

		if (strcmp(key, "Damage") == 0) {
			lua_getmetatable(L, 1);
			lua_getfield(L, -1, "Damage");
			return 1;
		}

		if (ent->type == 0 && ent->ptr.plant) {
			if (strcmp(key, "type") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.plant->mSeedType)); return 1; }
			if (strcmp(key, "hp") == 0) { lua_pushinteger(L, ent->ptr.plant->mPlantHealth); return 1; }
			if (strcmp(key, "id") == 0) { lua_pushinteger(L, 0); return 1; } // No direct ID equivalent
		} else if (ent->type == 1 && ent->ptr.zombie) {
			if (strcmp(key, "type") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.zombie->mZombieType)); return 1; }
			if (strcmp(key, "hp") == 0) { lua_pushinteger(L, ent->ptr.zombie->mBodyHealth); return 1; }
			if (strcmp(key, "id") == 0) { lua_pushinteger(L, 0); return 1; } // No direct ID equivalent
		}
		
		lua_pushnil(L);
		return 1;
	}

	int Lua_EntityDamage(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		int amount = luaL_checkinteger(L, 2);

		if (ent->type == 0 && ent->ptr.plant) {
			ent->ptr.plant->mPlantHealth -= amount;
			if (ent->ptr.plant->mPlantHealth <= 0) {
				ent->ptr.plant->Die();
			}
		} else if (ent->type == 1 && ent->ptr.zombie) {
			ent->ptr.zombie->TakeBodyDamage(amount, 0);
		}
		return 0;
	}

	void PushEntity(lua_State* L, Plant* plant) {
		if (!plant) { lua_pushnil(L); return; }
		LuaEntity* ent = (LuaEntity*)lua_newuserdata(L, sizeof(LuaEntity));
		ent->type = 0;
		ent->ptr.plant = plant;
		luaL_getmetatable(L, "Game.Entity");
		lua_setmetatable(L, -2);
	}

	void PushEntity(lua_State* L, Zombie* zombie) {
		if (!zombie) { lua_pushnil(L); return; }
		LuaEntity* ent = (LuaEntity*)lua_newuserdata(L, sizeof(LuaEntity));
		ent->type = 1;
		ent->ptr.zombie = zombie;
		luaL_getmetatable(L, "Game.Entity");
		lua_setmetatable(L, -2);
	}

	// Board table
	int Lua_BoardSpawnZombie(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int type = -1;
		if (lua_isstring(L, 1)) {
			std::string id = lua_tostring(L, 1);
			auto modDef = gModRegistry.FindZombie(id);
			if (modDef) type = modDef->zombieType;
		} else {
			type = luaL_checkinteger(L, 1);
		}
		int row = luaL_checkinteger(L, 2);
		
		if (type >= 0 && row >= 0 && row < 6) {
			Zombie* z = gLawnApp->mBoard->AddZombieInRow((ZombieType)type, row, gLawnApp->mBoard->mCurrentWave);
			PushEntity(L, z);
			return 1;
		}
		return 0;
	}

	int Lua_BoardSpawnPlant(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int type = -1;
		if (lua_isstring(L, 1)) {
			std::string id = lua_tostring(L, 1);
			auto modDef = gModRegistry.FindPlant(id);
			if (modDef) type = modDef->seedType;
		} else {
			type = luaL_checkinteger(L, 1);
		}
		int row = luaL_checkinteger(L, 2);
		int col = luaL_checkinteger(L, 3);
		
		if (type >= 0 && row >= 0 && row < 6 && col >= 0 && col < 9) {
			Plant* p = gLawnApp->mBoard->AddPlant(col, row, (SeedType)type);
			PushEntity(L, p);
			return 1;
		}
		return 0;
	}

	int Lua_BoardGetWave(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) {
			lua_pushinteger(L, 0);
			return 1;
		}
		lua_pushinteger(L, gLawnApp->mBoard->mCurrentWave);
		return 1;
	}

	void Lua_RegisterGameTable(lua_State* L)
	{
		lua_newtable(L);
		lua_pushcfunction(L, Lua_GameLog);
		lua_setfield(L, -2, "Log");
		lua_pushcfunction(L, Lua_GameSaveModData);
		lua_setfield(L, -2, "SaveModData");
		lua_pushcfunction(L, Lua_GameLoadModData);
		lua_setfield(L, -2, "LoadModData");
		lua_setglobal(L, "Game");

		// Board
		lua_newtable(L);
		lua_pushcfunction(L, Lua_BoardSpawnZombie);
		lua_setfield(L, -2, "SpawnZombie");
		lua_pushcfunction(L, Lua_BoardSpawnPlant);
		lua_setfield(L, -2, "SpawnPlant");
		lua_pushcfunction(L, Lua_BoardGetWave);
		lua_setfield(L, -2, "GetWave");
		lua_setglobal(L, "Board");

		// Entity metatable
		luaL_newmetatable(L, "Game.Entity");
		lua_pushcfunction(L, Lua_EntityIndex);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, Lua_EntityDamage);
		lua_setfield(L, -2, "Damage");
		lua_pop(L, 1);
	}

	void Lua_CallGlobal(lua_State* L, const char* name, int argc)
	{
		int base = lua_gettop(L) - argc;
		lua_getglobal(L, name);
		if (!lua_isfunction(L, -1))
		{
			lua_settop(L, base);
			return;
		}
		lua_insert(L, -1 - argc);
		if (lua_pcall(L, argc, 0, 0) != 0)
		{
			TodLog("Lua %s failed: %s", name, lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}
#endif
}

bool ModLua::Init()
{
#if defined(PVZ_ENABLE_LUA)
	if (mState != nullptr)
		return true;

	lua_State* L = luaL_newstate();
	if (L == nullptr)
		return false;

	luaL_openlibs(L);
	Lua_RegisterGameTable(L);
	mState = L;
	return true;
#else
	TodLog("Lua disabled: build without PVZ_ENABLE_LUA");
	return false;
#endif
}

void ModLua::Shutdown()
{
#if defined(PVZ_ENABLE_LUA)
	if (mState != nullptr)
	{
		lua_State* L = static_cast<lua_State*>(mState);
		lua_close(L);
		mState = nullptr;
	}
#endif
}

void ModLua::RunEntries(const std::vector<ModManifest>& manifests)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;

	for (const ModManifest& manifest : manifests)
	{
		if (manifest.entry.empty())
			continue;

		std::filesystem::path entryPath = Sexy::PathFromU8(manifest.rootPath) / Sexy::PathFromU8(manifest.entry);
		std::string entryText = ReadFileText(Sexy::PathToU8(entryPath));
		if (entryText.empty())
		{
			TodLog("Lua entry missing: %s", Sexy::PathToU8(entryPath).c_str());
			continue;
		}

		Lua_SetStringGlobal(L, "__mod_id", manifest.id);
		Lua_SetStringGlobal(L, "__mod_root", manifest.rootPath);

		if (luaL_loadbuffer(L, entryText.c_str(), entryText.size(), manifest.id.c_str()) != 0)
		{
			TodLog("Lua load failed: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			continue;
		}
		if (lua_pcall(L, 0, 0, 0) != 0)
		{
			TodLog("Lua run failed: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			continue;
		}

		lua_getglobal(L, "OnModInit");
		if (lua_isfunction(L, -1))
		{
			if (lua_pcall(L, 0, 0, 0) != 0)
			{
				TodLog("Lua OnModInit failed: %s", lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		else
		{
			lua_pop(L, 1);
		}
	}
#else
	(void)manifests;
#endif
}

void ModLua::CallOnLevelStart(int gameMode)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	lua_pushinteger(L, gameMode);
	Lua_CallGlobal(L, "OnLevelStart", 1);
#else
	(void)gameMode;
#endif
}

void ModLua::CallOnZombieSpawn(int zombieType, int row)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	lua_pushinteger(L, zombieType);
	lua_pushinteger(L, row);
	Lua_CallGlobal(L, "OnZombieSpawn", 2);
#else
	(void)zombieType;
	(void)row;
#endif
}

void ModLua::CallOnZombieDie(int zombieType)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	lua_pushinteger(L, zombieType);
	Lua_CallGlobal(L, "OnZombieDie", 1);
#else
	(void)zombieType;
#endif
}

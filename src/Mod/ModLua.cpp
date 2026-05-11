/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModLua.h"
#include "ModSave.h"
#include "ModRegistry.h"
#include "LuaProxyDialog.h"
#include "../LawnApp.h"
#include "../Lawn/Board.h"
#include "../Lawn/Plant.h"
#include "../Lawn/Zombie.h"
#include "../Lawn/Widget/GameButton.h"
#include "Sexy.TodLib/TodDebug.h"
#include "SexyAppFramework/Common.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

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

	struct LuaDialogUD
	{
		LuaProxyDialog* dialog;
		int selfRef;
	};

	int Lua_DialogIndex(lua_State* L)
	{
		LuaDialogUD* ud = (LuaDialogUD*)luaL_checkudata(L, 1, "Game.Dialog");
		const char* key = luaL_checkstring(L, 2);

		if (!ud->dialog || ud->dialog->mDestroyed)
		{
			lua_pushnil(L);
			return 1;
		}

		if (strcmp(key, "AddButton") == 0)
		{
			lua_getmetatable(L, 1);
			lua_getfield(L, -1, "AddButton");
			return 1;
		}
		if (strcmp(key, "Close") == 0)
		{
			lua_getmetatable(L, 1);
			lua_getfield(L, -1, "Close");
			return 1;
		}
		if (strcmp(key, "SetTitle") == 0)
		{
			lua_getmetatable(L, 1);
			lua_getfield(L, -1, "SetTitle");
			return 1;
		}
		if (strcmp(key, "SetBody") == 0)
		{
			lua_getmetatable(L, 1);
			lua_getfield(L, -1, "SetBody");
			return 1;
		}

		lua_pushnil(L);
		return 1;
	}

	int Lua_DialogGC(lua_State* L)
	{
		LuaDialogUD* ud = (LuaDialogUD*)luaL_checkudata(L, 1, "Game.Dialog");
		if (ud->selfRef != LUA_NOREF)
		{
			luaL_unref(L, LUA_REGISTRYINDEX, ud->selfRef);
			ud->selfRef = LUA_NOREF;
		}
		return 0;
	}

	int Lua_DialogAddButton(lua_State* L)
	{
		LuaDialogUD* ud = (LuaDialogUD*)luaL_checkudata(L, 1, "Game.Dialog");
		if (!ud->dialog || ud->dialog->mDestroyed)
			return 0;

		const char* text = luaL_checkstring(L, 2);
		if (lua_isfunction(L, 3))
		{
			lua_pushvalue(L, 3);
			int ref = luaL_ref(L, LUA_REGISTRYINDEX);
			ud->dialog->AddLuaButton(text, ref);
		}
		return 0;
	}

	int Lua_DialogClose(lua_State* L)
	{
		LuaDialogUD* ud = (LuaDialogUD*)luaL_checkudata(L, 1, "Game.Dialog");
		if (!ud->dialog || ud->dialog->mDestroyed)
			return 0;

		int dialogId = ud->dialog->mId;
		ud->dialog = nullptr;
		gLawnApp->KillDialog(dialogId);
		return 0;
	}

	int Lua_DialogSetTitle(lua_State* L)
	{
		LuaDialogUD* ud = (LuaDialogUD*)luaL_checkudata(L, 1, "Game.Dialog");
		if (!ud->dialog || ud->dialog->mDestroyed)
			return 0;
		const char* text = luaL_checkstring(L, 2);
		ud->dialog->mDialogHeader = text;
		return 0;
	}

	int Lua_DialogSetBody(lua_State* L)
	{
		LuaDialogUD* ud = (LuaDialogUD*)luaL_checkudata(L, 1, "Game.Dialog");
		if (!ud->dialog || ud->dialog->mDestroyed)
			return 0;
		const char* text = luaL_checkstring(L, 2);
		ud->dialog->mDialogLines = text;
		return 0;
	}

	void PushDialogUD(lua_State* L, LuaProxyDialog* dialog)
	{
		if (!dialog) { lua_pushnil(L); return; }
		LuaDialogUD* ud = (LuaDialogUD*)lua_newuserdata(L, sizeof(LuaDialogUD));
		ud->dialog = dialog;
		ud->selfRef = LUA_NOREF;
		luaL_getmetatable(L, "Game.Dialog");
		lua_setmetatable(L, -2);
		lua_pushvalue(L, -1);
		ud->selfRef = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	int Lua_UICreateDialog(lua_State* L)
	{
		if (!gLawnApp) return 0;

		std::string title = "Mod Dialog";
		std::string body;
		bool modal = true;

		if (lua_istable(L, 1))
		{
			lua_getfield(L, 1, "title");
			if (lua_isstring(L, -1)) title = lua_tostring(L, -1);
			lua_pop(L, 1);

			lua_getfield(L, 1, "body");
			if (lua_isstring(L, -1)) body = lua_tostring(L, -1);
			lua_pop(L, 1);

			lua_getfield(L, 1, "modal");
			if (lua_isboolean(L, -1)) modal = lua_toboolean(L, -1);
			lua_pop(L, 1);
		}

		LuaProxyDialog* dialog = new LuaProxyDialog(gLawnApp, title, body, modal);
		dialog->SetLuaState(L);

		gLawnApp->KillDialog(dialog->mId);
		gLawnApp->CenterDialog(dialog, dialog->mWidth, dialog->mHeight);
		gLawnApp->AddDialog(dialog->mId, dialog);

		PushDialogUD(L, dialog);
		return 1;
	}

	int Lua_NoopCallback(lua_State* L)
	{
		return 0;
	}

	int Lua_UIShowMessage(lua_State* L)
	{
		if (!gLawnApp) return 0;

		const char* title = luaL_checkstring(L, 1);
		const char* body = luaL_optstring(L, 2, "");

		LuaProxyDialog* dialog = new LuaProxyDialog(gLawnApp, title, body, true);
		dialog->SetLuaState(L);

		lua_pushcfunction(L, Lua_NoopCallback);
		int ref = luaL_ref(L, LUA_REGISTRYINDEX);
		dialog->AddLuaButton("[DIALOG_BUTTON_OK]", ref);

		gLawnApp->KillDialog(dialog->mId);
		gLawnApp->CenterDialog(dialog, dialog->mWidth, dialog->mHeight);
		gLawnApp->AddDialog(dialog->mId, dialog);

		PushDialogUD(L, dialog);
		return 1;
	}

	int Lua_GameSetSpeed(lua_State* L)
	{
		int multiplier = luaL_checkinteger(L, 1);
		if (gLawnApp)
			gLawnApp->SetGameSpeed(multiplier);
		return 0;
	}

	int Lua_GameGetSpeed(lua_State* L)
	{
		if (gLawnApp)
			lua_pushinteger(L, gLawnApp->GetGameSpeed());
		else
			lua_pushinteger(L, 1);
		return 1;
	}

	int Lua_GameGetMenuButtonRect(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard || !gLawnApp->mBoard->mMenuButton)
		{
			lua_pushinteger(L, 0);
			lua_pushinteger(L, 0);
			lua_pushinteger(L, 0);
			lua_pushinteger(L, 0);
			return 4;
		}
		GameButton* btn = gLawnApp->mBoard->mMenuButton;
		lua_pushinteger(L, btn->mX);
		lua_pushinteger(L, btn->mY);
		lua_pushinteger(L, btn->mWidth);
		lua_pushinteger(L, btn->mHeight);
		return 4;
	}

	int Lua_BoardAddButton(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int id = luaL_checkinteger(L, 1);
		int x = luaL_checkinteger(L, 2);
		int y = luaL_checkinteger(L, 3);
		int w = luaL_checkinteger(L, 4);
		int h = luaL_checkinteger(L, 5);
		const char* label = luaL_checkstring(L, 6);

		for (auto& existing : gLawnApp->mBoard->mLuaButtons)
		{
			if (existing.id == id)
			{
				existing.x = x;
				existing.y = y;
				existing.w = w;
				existing.h = h;
				existing.label = label;
				existing.visible = true;
				return 0;
			}
		}

		gLawnApp->mBoard->mLuaButtons.push_back({id, x, y, w, h, std::string(label), true});
		return 0;
	}

	int Lua_BoardRemoveButton(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int id = luaL_checkinteger(L, 1);
		auto& btns = gLawnApp->mBoard->mLuaButtons;
		btns.erase(std::remove_if(btns.begin(), btns.end(), [id](const LuaBoardButton& b) { return b.id == id; }), btns.end());
		return 0;
	}

	int Lua_BoardSetButtonVisible(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int id = luaL_checkinteger(L, 1);
		bool visible = lua_toboolean(L, 2) != 0;
		for (auto& btn : gLawnApp->mBoard->mLuaButtons)
		{
			if (btn.id == id)
			{
				btn.visible = visible;
				break;
			}
		}
		return 0;
	}

	int Lua_BoardSetButtonLabel(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int id = luaL_checkinteger(L, 1);
		const char* label = luaL_checkstring(L, 2);
		for (auto& btn : gLawnApp->mBoard->mLuaButtons)
		{
			if (btn.id == id)
			{
				btn.label = label;
				break;
			}
		}
		return 0;
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
		lua_pushcfunction(L, Lua_GameSetSpeed);
		lua_setfield(L, -2, "SetSpeed");
		lua_pushcfunction(L, Lua_GameGetSpeed);
		lua_setfield(L, -2, "GetSpeed");
		lua_pushcfunction(L, Lua_GameGetMenuButtonRect);
		lua_setfield(L, -2, "GetMenuButtonRect");
		lua_setglobal(L, "Game");

		// Board
		lua_newtable(L);
		lua_pushcfunction(L, Lua_BoardSpawnZombie);
		lua_setfield(L, -2, "SpawnZombie");
		lua_pushcfunction(L, Lua_BoardSpawnPlant);
		lua_setfield(L, -2, "SpawnPlant");
		lua_pushcfunction(L, Lua_BoardGetWave);
		lua_setfield(L, -2, "GetWave");
		lua_pushcfunction(L, Lua_BoardAddButton);
		lua_setfield(L, -2, "AddButton");
		lua_pushcfunction(L, Lua_BoardRemoveButton);
		lua_setfield(L, -2, "RemoveButton");
		lua_pushcfunction(L, Lua_BoardSetButtonVisible);
		lua_setfield(L, -2, "SetButtonVisible");
		lua_pushcfunction(L, Lua_BoardSetButtonLabel);
		lua_setfield(L, -2, "SetButtonLabel");
		lua_setglobal(L, "Board");

		// Entity metatable
		luaL_newmetatable(L, "Game.Entity");
		lua_pushcfunction(L, Lua_EntityIndex);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, Lua_EntityDamage);
		lua_setfield(L, -2, "Damage");
		lua_pop(L, 1);

		// Dialog metatable
		luaL_newmetatable(L, "Game.Dialog");
		lua_pushcfunction(L, Lua_DialogIndex);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, Lua_DialogGC);
		lua_setfield(L, -2, "__gc");
		lua_pushcfunction(L, Lua_DialogAddButton);
		lua_setfield(L, -2, "AddButton");
		lua_pushcfunction(L, Lua_DialogClose);
		lua_setfield(L, -2, "Close");
		lua_pushcfunction(L, Lua_DialogSetTitle);
		lua_setfield(L, -2, "SetTitle");
		lua_pushcfunction(L, Lua_DialogSetBody);
		lua_setfield(L, -2, "SetBody");
		lua_pop(L, 1);

		// UI table
		lua_newtable(L);
		lua_pushcfunction(L, Lua_UICreateDialog);
		lua_setfield(L, -2, "CreateDialog");
		lua_pushcfunction(L, Lua_UIShowMessage);
		lua_setfield(L, -2, "ShowMessage");
		lua_setglobal(L, "UI");
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

void ModLua::CallOnZombieSpawn(Zombie* zombie)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	PushEntity(L, zombie);
	Lua_CallGlobal(L, "OnZombieSpawn", 1);
#else
	(void)zombie;
#endif
}

void ModLua::CallOnZombieDie(Zombie* zombie)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	PushEntity(L, zombie);
	Lua_CallGlobal(L, "OnZombieDie", 1);
#else
	(void)zombie;
#endif
}

void ModLua::CallOnPlantSpawn(Plant* plant)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	PushEntity(L, plant);
	Lua_CallGlobal(L, "OnPlantSpawn", 1);
#else
	(void)plant;
#endif
}

void ModLua::CallOnPlantAttack(Plant* plant, Zombie* target)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	PushEntity(L, plant);
	PushEntity(L, target);
	Lua_CallGlobal(L, "OnPlantAttack", 2);
#else
	(void)plant;
	(void)target;
#endif
}

void ModLua::CallOnLevelEnd(bool isWin)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	lua_pushboolean(L, isWin);
	Lua_CallGlobal(L, "OnLevelEnd", 1);
#else
	(void)isWin;
#endif
}

void ModLua::CallOnBoardButtonClick(int buttonId)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	lua_pushinteger(L, buttonId);
	Lua_CallGlobal(L, "OnBoardButtonClick", 1);
#else
	(void)buttonId;
#endif
}

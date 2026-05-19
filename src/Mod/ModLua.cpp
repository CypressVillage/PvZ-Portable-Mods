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
#include "../GameConstants.h"
#include "../Lawn/Board.h"
#include "../Lawn/Plant.h"
#include "Sexy.TodLib/Reanimator.h"
#include "../Lawn/Zombie.h"
#include "../Lawn/Coin.h"
#include "../Lawn/Projectile.h"
#include "../Lawn/Widget/GameButton.h"
#include "Sexy.TodLib/TodDebug.h"
#include "SexyAppFramework/Common.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

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

	int Lua_GameRegisterProjectile(lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TTABLE);

		ModProjectileDef def;

		lua_getfield(L, 1, "id");
		if (lua_isstring(L, -1))
			def.id = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "damage");
		if (lua_isinteger(L, -1))
			def.damage = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "speed");
		if (lua_isnumber(L, -1))
			def.speed = static_cast<float>(lua_tonumber(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "image");
		if (lua_isstring(L, -1))
			def.imageName = lua_tostring(L, -1);
		lua_pop(L, 1);

		std::string error;
		if (gModRegistry.RegisterProjectile(def, &error))
		{
			lua_pushinteger(L, def.projectileType);
			return 1;
		}

		lua_pushnil(L);
		lua_pushstring(L, error.c_str());
		return 2;
	}

	int Lua_GameRegisterPlant(lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TTABLE);

		ModPlantDef def;
		def.seedType = -1;

		lua_getfield(L, 1, "id");
		if (lua_isstring(L, -1))
			def.id = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "name");
		if (lua_isstring(L, -1))
			def.plantName = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "cost");
		if (lua_isinteger(L, -1))
			def.seedCost = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "cooldown");
		if (lua_isinteger(L, -1))
			def.refreshTime = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "packetIndex");
		if (lua_isinteger(L, -1))
			def.packetIndex = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "subClass");
		if (lua_isinteger(L, -1))
			def.subClass = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "launchRate");
		if (lua_isinteger(L, -1))
			def.launchRate = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "description");
		if (lua_isstring(L, -1))
			def.description = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "image");
		if (lua_isstring(L, -1))
			def.imageName = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "projectileType");
		if (lua_isstring(L, -1))
		{
			std::string projId = lua_tostring(L, -1);
			const ModProjectileDef* projDef = gModRegistry.FindProjectile(projId);
			if (projDef && projDef->projectileType >= 0)
				def.projectileType = projDef->projectileType;
		}
		else if (lua_isinteger(L, -1))
		{
			def.projectileType = static_cast<int>(lua_tointeger(L, -1));
		}
		lua_pop(L, 1);

		lua_getfield(L, 1, "reanimation");
		if (lua_isstring(L, -1))
			def.reanimationName = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "reanimFile");
		if (lua_isstring(L, -1))
		{
			std::string relPath = lua_tostring(L, -1);
			if (!relPath.empty())
			{
				lua_getglobal(L, "__mod_root");
				if (lua_isstring(L, -1))
				{
					std::string modRoot = lua_tostring(L, -1);
					std::filesystem::path absPath = Sexy::PathFromU8(modRoot) / Sexy::PathFromU8(relPath);
					def.reanimationName = Sexy::PathToU8(absPath);
					gModRegistry.RegisterDynamicReanim(def.reanimationName);
				}
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);

		std::string error;
		if (gModRegistry.RegisterPlant(def, &error))
		{
			lua_pushinteger(L, def.seedType);
			return 1;
		}

		lua_pushnil(L);
		lua_pushstring(L, error.c_str());
		return 2;
	}

	int Lua_GameRegisterZombie(lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TTABLE);

		ModZombieDef def;
		def.zombieType = -1;

		lua_getfield(L, 1, "id");
		if (lua_isstring(L, -1))
			def.id = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "bodyHealth");
		if (lua_isinteger(L, -1))
			def.bodyHealth = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "headHealth");
		if (lua_isinteger(L, -1))
			def.headHealth = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "speed");
		if (lua_isnumber(L, -1))
			def.speed = static_cast<float>(lua_tonumber(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "damage");
		if (lua_isinteger(L, -1))
			def.damage = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "reanimation");
		if (lua_isstring(L, -1))
		{
			def.reanimationName = lua_tostring(L, -1);
			if (!def.reanimationName.empty())
				gModRegistry.RegisterDynamicReanim(def.reanimationName);
		}
		lua_pop(L, 1);

		lua_getfield(L, 1, "helmType");
		if (lua_isinteger(L, -1))
			def.helmType = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "helmHealth");
		if (lua_isinteger(L, -1))
			def.helmHealth = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "shieldType");
		if (lua_isinteger(L, -1))
			def.shieldType = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "shieldHealth");
		if (lua_isinteger(L, -1))
			def.shieldHealth = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "hasHead");
		if (lua_isboolean(L, -1))
			def.hasHead = lua_toboolean(L, -1) != 0;
		lua_pop(L, 1);

		lua_getfield(L, 1, "hasArm");
		if (lua_isboolean(L, -1))
			def.hasArm = lua_toboolean(L, -1) != 0;
		lua_pop(L, 1);

		lua_getfield(L, 1, "name");
		if (lua_isstring(L, -1))
			def.zombieName = lua_tostring(L, -1);
		lua_pop(L, 1);

		std::string error;
		if (gModRegistry.RegisterZombie(def, &error))
		{
			lua_pushinteger(L, def.zombieType);
			return 1;
		}

		lua_pushnil(L);
		lua_pushstring(L, error.c_str());
		return 2;
	}

	int Lua_GameRegisterMode(lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TTABLE);

		ModModeDef def;
		def.baseMode = -1;

		lua_getfield(L, 1, "id");
		if (lua_isstring(L, -1))
			def.id = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "name");
		if (lua_isstring(L, -1))
			def.challengeName = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "page");
		if (lua_isinteger(L, -1))
			def.challengePage = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "row");
		if (lua_isinteger(L, -1))
			def.challengeRow = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "col");
		if (lua_isinteger(L, -1))
			def.challengeCol = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		lua_getfield(L, 1, "iconIndex");
		if (lua_isinteger(L, -1))
			def.challengeIconIndex = static_cast<int>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		std::string error;
		if (gModRegistry.RegisterMode(def, &error))
		{
			lua_pushinteger(L, def.baseMode);
			return 1;
		}

		lua_pushnil(L);
		lua_pushstring(L, error.c_str());
		return 2;
	}

	// Entity wrapper
	struct LuaEntity {
		int type; // 0 = plant, 1 = zombie, 2 = coin, 3 = projectile
		union {
			Plant* plant;
			Zombie* zombie;
			Coin* coin;
			Projectile* projectile;
		} ptr;
	};

	int Lua_EntityIndex(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		const char* key = luaL_checkstring(L, 2);

		if (strcmp(key, "Damage") == 0 || strcmp(key, "IsSun") == 0 || strcmp(key, "Collect") == 0 ||
			strcmp(key, "DistanceTo") == 0 ||
			strcmp(key, "GetBodyReanim") == 0 || strcmp(key, "PlayBodyReanim") == 0 || strcmp(key, "PlayIdleAnim") == 0 ||
			strcmp(key, "GetBodyReanimProgress") == 0 || strcmp(key, "SetBodyReanimRate") == 0 || strcmp(key, "GetBodyReanimRate") == 0 ||
			strcmp(key, "IsAnimPlaying") == 0 || strcmp(key, "TrackExists") == 0 ||
			strcmp(key, "GetBodyReanimLoopType") == 0 || strcmp(key, "GetBodyReanimLoopCount") == 0 ||
			strcmp(key, "SetDamage") == 0 || strcmp(key, "SetVelocity") == 0 || strcmp(key, "SetDamageFlags") == 0 ||
			strcmp(key, "SetMotionType") == 0 || strcmp(key, "SetTargetZombie") == 0 ||
			strcmp(key, "Die") == 0 || strcmp(key, "Squish") == 0 || strcmp(key, "SetSleeping") == 0 ||
			strcmp(key, "GetCost") == 0 || strcmp(key, "GetName") == 0 ||
			strcmp(key, "IsNocturnal") == 0 || strcmp(key, "IsFungus") == 0 || strcmp(key, "IsAquatic") == 0 ||
			strcmp(key, "IsUpgrade") == 0 || strcmp(key, "IsFlying") == 0 ||
			strcmp(key, "SetRow") == 0 || strcmp(key, "ApplyChill") == 0 ||
			strcmp(key, "ApplyButter") == 0 || strcmp(key, "RemoveButter") == 0 ||
			strcmp(key, "StartMindControlled") == 0 || strcmp(key, "DieNoLoot") == 0 || strcmp(key, "DieWithLoot") == 0 ||
			strcmp(key, "TakeHelmDamage") == 0 || strcmp(key, "TakeShieldDamage") == 0 ||
			strcmp(key, "IsOnHighGround") == 0 || strcmp(key, "IsImmobilized") == 0 ||
			strcmp(key, "GetValue") == 0 || strcmp(key, "StartFade") == 0) {
			lua_getmetatable(L, 1);
			lua_getfield(L, -1, key);
			return 1;
		}

		if (ent->type == 0 && ent->ptr.plant) {
			if (strcmp(key, "type") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.plant->mSeedType)); return 1; }
			if (strcmp(key, "hp") == 0) { lua_pushinteger(L, ent->ptr.plant->mPlantHealth); return 1; }
			if (strcmp(key, "id") == 0) {
				int seedType = static_cast<int>(ent->ptr.plant->mSeedType);
				if (seedType >= 2000) {
					const ModPlantDef* def = gModRegistry.FindPlantByRuntimeId(seedType);
					if (def) { lua_pushstring(L, def->id.c_str()); return 1; }
				}
				lua_pushstring(L, ""); return 1;
			}
			if (strcmp(key, "row") == 0) { lua_pushinteger(L, ent->ptr.plant->mRow); return 1; }
			if (strcmp(key, "col") == 0) { lua_pushinteger(L, ent->ptr.plant->mPlantCol); return 1; }
			if (strcmp(key, "x") == 0) { lua_pushnumber(L, static_cast<float>(ent->ptr.plant->mX)); return 1; }
			if (strcmp(key, "y") == 0) { lua_pushnumber(L, static_cast<float>(ent->ptr.plant->mY)); return 1; }
			if (strcmp(key, "maxHp") == 0) { lua_pushinteger(L, ent->ptr.plant->mPlantMaxHealth); return 1; }
			if (strcmp(key, "state") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.plant->mState)); return 1; }
			if (strcmp(key, "subClass") == 0) { lua_pushinteger(L, ent->ptr.plant->mSubclass); return 1; }
			if (strcmp(key, "isAsleep") == 0) { lua_pushboolean(L, ent->ptr.plant->mIsAsleep ? 1 : 0); return 1; }
			if (strcmp(key, "isDead") == 0) { lua_pushboolean(L, ent->ptr.plant->mDead ? 1 : 0); return 1; }
			if (strcmp(key, "launchCounter") == 0) { lua_pushinteger(L, ent->ptr.plant->mLaunchCounter); return 1; }
			if (strcmp(key, "launchRate") == 0) { lua_pushinteger(L, ent->ptr.plant->mLaunchRate); return 1; }
			if (strcmp(key, "age") == 0) { lua_pushinteger(L, ent->ptr.plant->mAnimCounter); return 1; }
			if (strcmp(key, "imitaterType") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.plant->mImitaterType)); return 1; }
			if (strcmp(key, "recentlyEaten") == 0) { lua_pushinteger(L, ent->ptr.plant->mRecentlyEatenCountdown); return 1; }
			if (strcmp(key, "squished") == 0) { lua_pushboolean(L, ent->ptr.plant->mSquished ? 1 : 0); return 1; }
		} else if (ent->type == 1 && ent->ptr.zombie) {
			if (strcmp(key, "type") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.zombie->mZombieType)); return 1; }
			if (strcmp(key, "hp") == 0) { lua_pushinteger(L, ent->ptr.zombie->mBodyHealth); return 1; }
			if (strcmp(key, "id") == 0) {
				int zombieType = static_cast<int>(ent->ptr.zombie->mZombieType);
				if (zombieType >= 3000) {
					const ModZombieDef* def = gModRegistry.FindZombieByRuntimeId(zombieType);
					if (def) { lua_pushstring(L, def->id.c_str()); return 1; }
				}
				lua_pushstring(L, ""); return 1;
			}
			if (strcmp(key, "row") == 0) { lua_pushinteger(L, ent->ptr.zombie->mRow); return 1; }
			if (strcmp(key, "x") == 0) { lua_pushnumber(L, ent->ptr.zombie->mPosX); return 1; }
			if (strcmp(key, "y") == 0) { lua_pushnumber(L, ent->ptr.zombie->mPosY); return 1; }
			if (strcmp(key, "velX") == 0) { lua_pushnumber(L, ent->ptr.zombie->mVelX); return 1; }
			if (strcmp(key, "maxHp") == 0) { lua_pushinteger(L, ent->ptr.zombie->mBodyMaxHealth); return 1; }
			if (strcmp(key, "phase") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.zombie->mZombiePhase)); return 1; }
			if (strcmp(key, "isEating") == 0) { lua_pushboolean(L, ent->ptr.zombie->mIsEating ? 1 : 0); return 1; }
			if (strcmp(key, "isDead") == 0) { lua_pushboolean(L, ent->ptr.zombie->mDead ? 1 : 0); return 1; }
			if (strcmp(key, "age") == 0) { lua_pushinteger(L, ent->ptr.zombie->mZombieAge); return 1; }
			if (strcmp(key, "chilled") == 0) { lua_pushboolean(L, ent->ptr.zombie->mChilledCounter > 0 ? 1 : 0); return 1; }
			if (strcmp(key, "buttered") == 0) { lua_pushboolean(L, ent->ptr.zombie->mButteredCounter > 0 ? 1 : 0); return 1; }
			if (strcmp(key, "mindControlled") == 0) { lua_pushboolean(L, ent->ptr.zombie->mMindControlled ? 1 : 0); return 1; }
			if (strcmp(key, "helmType") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.zombie->mHelmType)); return 1; }
			if (strcmp(key, "helmHp") == 0) { lua_pushinteger(L, ent->ptr.zombie->mHelmHealth); return 1; }
			if (strcmp(key, "shieldType") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.zombie->mShieldType)); return 1; }
			if (strcmp(key, "shieldHp") == 0) { lua_pushinteger(L, ent->ptr.zombie->mShieldHealth); return 1; }
			if (strcmp(key, "hasHead") == 0) { lua_pushboolean(L, ent->ptr.zombie->mHasHead ? 1 : 0); return 1; }
			if (strcmp(key, "hasArm") == 0) { lua_pushboolean(L, ent->ptr.zombie->mHasArm ? 1 : 0); return 1; }
			if (strcmp(key, "inPool") == 0) { lua_pushboolean(L, ent->ptr.zombie->mInPool ? 1 : 0); return 1; }
			if (strcmp(key, "onHighGround") == 0) { lua_pushboolean(L, ent->ptr.zombie->mOnHighGround ? 1 : 0); return 1; }
			if (strcmp(key, "altitude") == 0) { lua_pushnumber(L, ent->ptr.zombie->mAltitude); return 1; }
		} else if (ent->type == 2 && ent->ptr.coin) {
			if (strcmp(key, "type") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.coin->mType)); return 1; }
			if (strcmp(key, "hp") == 0) { lua_pushinteger(L, 0); return 1; }
			if (strcmp(key, "id") == 0) { lua_pushstring(L, ""); return 1; }
			if (strcmp(key, "x") == 0) { lua_pushnumber(L, ent->ptr.coin->mPosX); return 1; }
			if (strcmp(key, "y") == 0) { lua_pushnumber(L, ent->ptr.coin->mPosY); return 1; }
			if (strcmp(key, "velX") == 0) { lua_pushnumber(L, ent->ptr.coin->mVelX); return 1; }
			if (strcmp(key, "velY") == 0) { lua_pushnumber(L, ent->ptr.coin->mVelY); return 1; }
			if (strcmp(key, "age") == 0) { lua_pushinteger(L, ent->ptr.coin->mCoinAge); return 1; }
			if (strcmp(key, "value") == 0) {
				if (ent->ptr.coin->IsSun()) { lua_pushinteger(L, ent->ptr.coin->GetSunValue()); return 1; }
				lua_pushinteger(L, Coin::GetCoinValue(ent->ptr.coin->mType)); return 1;
			}
			if (strcmp(key, "isBeingCollected") == 0) { lua_pushboolean(L, ent->ptr.coin->mIsBeingCollected ? 1 : 0); return 1; }
			if (strcmp(key, "coinMotion") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.coin->mCoinMotion)); return 1; }
			if (strcmp(key, "isMoney") == 0) { lua_pushboolean(L, ent->ptr.coin->IsMoney() ? 1 : 0); return 1; }
			if (strcmp(key, "scale") == 0) { lua_pushnumber(L, ent->ptr.coin->mScale); return 1; }
		} else if (ent->type == 3 && ent->ptr.projectile) {
			if (strcmp(key, "type") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.projectile->mProjectileType)); return 1; }
			if (strcmp(key, "x") == 0) { lua_pushnumber(L, ent->ptr.projectile->mPosX); return 1; }
			if (strcmp(key, "y") == 0) { lua_pushnumber(L, ent->ptr.projectile->mPosY); return 1; }
			if (strcmp(key, "z") == 0) { lua_pushnumber(L, ent->ptr.projectile->mPosZ); return 1; }
			if (strcmp(key, "row") == 0) { lua_pushinteger(L, ent->ptr.projectile->mRow); return 1; }
			if (strcmp(key, "motionType") == 0) { lua_pushinteger(L, static_cast<int>(ent->ptr.projectile->mMotionType)); return 1; }
			if (strcmp(key, "age") == 0) { lua_pushinteger(L, ent->ptr.projectile->mProjectileAge); return 1; }
			if (strcmp(key, "hp") == 0) { lua_pushinteger(L, 1); return 1; }
			if (strcmp(key, "id") == 0) {
				int projectileType = static_cast<int>(ent->ptr.projectile->mProjectileType);
				if (projectileType >= 4000) {
					const ModProjectileDef* def = gModRegistry.FindProjectileByRuntimeId(projectileType);
					if (def) { lua_pushstring(L, def->id.c_str()); return 1; }
				}
				lua_pushstring(L, ""); return 1;
			}
			if (strcmp(key, "damage") == 0) { lua_pushinteger(L, ent->ptr.projectile->mDamageOverride >= 0 ? ent->ptr.projectile->mDamageOverride : ent->ptr.projectile->GetProjectileDef().mDamage); return 1; }
			if (strcmp(key, "velX") == 0) { lua_pushnumber(L, ent->ptr.projectile->mVelX); return 1; }
			if (strcmp(key, "velY") == 0) { lua_pushnumber(L, ent->ptr.projectile->mVelY); return 1; }
			if (strcmp(key, "velZ") == 0) { lua_pushnumber(L, ent->ptr.projectile->mVelZ); return 1; }
			if (strcmp(key, "isDead") == 0) { lua_pushboolean(L, ent->ptr.projectile->mDead ? 1 : 0); return 1; }
			if (strcmp(key, "rotation") == 0) { lua_pushnumber(L, ent->ptr.projectile->mRotation); return 1; }
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

	int Lua_EntityIsSun(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 2 && ent->ptr.coin) {
			lua_pushboolean(L, ent->ptr.coin->IsSun() ? 1 : 0);
		} else {
			lua_pushboolean(L, 0);
		}
		return 1;
	}

	int Lua_EntityCollect(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 2 && ent->ptr.coin && !ent->ptr.coin->mIsBeingCollected && !ent->ptr.coin->mDead) {
			ent->ptr.coin->PlayCollectSound();
			ent->ptr.coin->Collect();
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

	void PushEntity(lua_State* L, Coin* coin) {
		if (!coin) { lua_pushnil(L); return; }
		LuaEntity* ent = (LuaEntity*)lua_newuserdata(L, sizeof(LuaEntity));
		ent->type = 2;
		ent->ptr.coin = coin;
		luaL_getmetatable(L, "Game.Entity");
		lua_setmetatable(L, -2);
	}

	void PushEntity(lua_State* L, Projectile* projectile) {
		if (!projectile) { lua_pushnil(L); return; }
		LuaEntity* ent = (LuaEntity*)lua_newuserdata(L, sizeof(LuaEntity));
		ent->type = 3;
		ent->ptr.projectile = projectile;
		luaL_getmetatable(L, "Game.Entity");
		lua_setmetatable(L, -2);
	}

	int Lua_EntityDistanceTo(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		LuaEntity* other = (LuaEntity*)luaL_checkudata(L, 2, "Game.Entity");

		auto getPos = [](LuaEntity* e, float& x, float& y) -> bool {
			if (e->type == 0 && e->ptr.plant) {
				x = static_cast<float>(e->ptr.plant->mX + 40);
				y = static_cast<float>(e->ptr.plant->mY + 40);
				return true;
			}
			if (e->type == 1 && e->ptr.zombie) {
				x = e->ptr.zombie->mPosX;
				y = e->ptr.zombie->mPosY;
				return true;
			}
			if (e->type == 2 && e->ptr.coin) {
				x = e->ptr.coin->mPosX;
				y = e->ptr.coin->mPosY;
				return true;
			}
			return false;
		};

		float ex, ey, ox, oy;
		if (!getPos(ent, ex, ey) || !getPos(other, ox, oy)) {
			lua_pushnumber(L, 0.0);
			return 1;
		}
		float dx = ex - ox;
		float dy = ey - oy;
		lua_pushnumber(L, std::sqrt(dx * dx + dy * dy));
		return 1;
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

	int Lua_BoardFindTargetZombie(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant) {
			lua_pushnil(L);
			return 1;
		}
		Plant* plant = ent->ptr.plant;
		Zombie* target = plant->FindTargetZombie(plant->mRow, PlantWeapon::WEAPON_PRIMARY);
		PushEntity(L, target);
		return 1;
	}

	int Lua_BoardGetZombiesInRow(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int row = luaL_checkinteger(L, 1);
		if (row < 0 || row >= 6) { lua_newtable(L); return 1; }

		lua_newtable(L);
		int idx = 1;
		Zombie* z = nullptr;
		while (gLawnApp->mBoard->mZombies.IterateNext(z))
		{
			if (z->mRow == row && !z->mDead)
			{
				PushEntity(L, z);
				lua_rawseti(L, -2, idx++);
			}
		}
		return 1;
	}

	int Lua_BoardGetPlantsInRow(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int row = luaL_checkinteger(L, 1);
		if (row < 0 || row >= 6) { lua_newtable(L); return 1; }

		lua_newtable(L);
		int idx = 1;
		Plant* p = nullptr;
		while (gLawnApp->mBoard->mPlants.IterateNext(p))
		{
			if (p->mRow == row && !p->mDead)
			{
				PushEntity(L, p);
				lua_rawseti(L, -2, idx++);
			}
		}
		return 1;
	}

	int Lua_BoardGetZombieAt(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int col = luaL_checkinteger(L, 1);
		int row = luaL_checkinteger(L, 2);

		int bestCol = -1;
		Zombie* best = nullptr;
		int bestDist = 999999;
		Zombie* z = nullptr;
		while (gLawnApp->mBoard->mZombies.IterateNext(z))
		{
			if (z->mRow != row || z->mDead) continue;
			int zCol = gLawnApp->mBoard->PixelToGridX(z->mPosX + 40.0f, z->mPosY);
			if (zCol == col)
			{
				int dist = std::abs((int)(z->mPosX + 40.0f) - gLawnApp->mBoard->GridToPixelX(col, row));
				if (dist < bestDist)
				{
					bestDist = dist;
					best = z;
				}
			}
		}
		PushEntity(L, best);
		return 1;
	}

	int Lua_BoardGetPlantAt(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int col = luaL_checkinteger(L, 1);
		int row = luaL_checkinteger(L, 2);
		Plant* p = gLawnApp->mBoard->GetTopPlantAt(col, row, TOPPLANT_BUNGEE_ORDER);
		PushEntity(L, p);
		return 1;
	}

	int Lua_BoardGetAllZombies(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		lua_newtable(L);
		int idx = 1;
		Zombie* z = nullptr;
		while (gLawnApp->mBoard->mZombies.IterateNext(z))
		{
			if (!z->mDead)
			{
				PushEntity(L, z);
				lua_rawseti(L, -2, idx++);
			}
		}
		return 1;
	}

	int Lua_BoardGetAllPlants(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		lua_newtable(L);
		int idx = 1;
		Plant* p = nullptr;
		while (gLawnApp->mBoard->mPlants.IterateNext(p))
		{
			if (!p->mDead)
			{
				PushEntity(L, p);
				lua_rawseti(L, -2, idx++);
			}
		}
		return 1;
	}

	int Lua_BoardAddProjectile(lua_State* L)
	{
		if (!gLawnApp || !gLawnApp->mBoard) return 0;
		int x = luaL_checkinteger(L, 1);
		int y = luaL_checkinteger(L, 2);
		int row = luaL_checkinteger(L, 3);
		int projType = luaL_checkinteger(L, 4);

		if (row < 0 || row >= 6) { lua_pushnil(L); return 1; }

		Projectile* p = gLawnApp->mBoard->AddProjectile(x, y, 0, row, static_cast<ProjectileType>(projType));
		PushEntity(L, p);
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
		if (strcmp(key, "AddLabel") == 0)
		{
			lua_getmetatable(L, 1);
			lua_getfield(L, -1, "AddLabel");
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

	int Lua_DialogAddLabel(lua_State* L)
	{
		LuaDialogUD* ud = (LuaDialogUD*)luaL_checkudata(L, 1, "Game.Dialog");
		if (!ud->dialog || ud->dialog->mDestroyed)
			return 0;
		const char* text = luaL_checkstring(L, 2);
		int x = luaL_checkinteger(L, 3);
		int y = luaL_checkinteger(L, 4);
		ud->dialog->AddLuaLabel(text, x, y);
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

	// === Reanimation userdata ===
	struct LuaReanimUD
	{
		Reanimation* reanim;
	};

	int Lua_ReanimIndex(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		const char* key = luaL_checkstring(L, 2);

		if (!ud->reanim)
		{
			lua_pushnil(L);
			return 1;
		}

		if (strcmp(key, "Play") == 0 || strcmp(key, "GetRate") == 0 || strcmp(key, "SetRate") == 0 ||
			strcmp(key, "GetProgress") == 0 || strcmp(key, "SetProgress") == 0 ||
			strcmp(key, "IsPlaying") == 0 || strcmp(key, "TrackExists") == 0 ||
			strcmp(key, "GetLoopType") == 0 || strcmp(key, "GetLoopCount") == 0 ||
			strcmp(key, "SetPosition") == 0 || strcmp(key, "OverrideScale") == 0 ||
			strcmp(key, "ShowOnlyTrack") == 0)
		{
			lua_getmetatable(L, 1);
			lua_getfield(L, -1, key);
			return 1;
		}

		lua_pushnil(L);
		return 1;
	}

	int Lua_ReanimPlay(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) return 0;
		const char* trackName = luaL_checkstring(L, 2);
		int loopType = luaL_checkinteger(L, 3);
		int blendTime = luaL_optinteger(L, 4, 0);
		float animRate = (float)luaL_optnumber(L, 5, 0.0);
		ud->reanim->PlayReanim(trackName, (ReanimLoopType)loopType, blendTime, animRate);
		return 0;
	}

	int Lua_ReanimGetRate(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) { lua_pushnumber(L, 0); return 1; }
		lua_pushnumber(L, ud->reanim->mAnimRate);
		return 1;
	}

	int Lua_ReanimSetRate(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) return 0;
		ud->reanim->mAnimRate = (float)luaL_checknumber(L, 2);
		return 0;
	}

	int Lua_ReanimGetProgress(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) { lua_pushnumber(L, 0); return 1; }
		lua_pushnumber(L, ud->reanim->mAnimTime);
		return 1;
	}

	int Lua_ReanimSetProgress(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) return 0;
		ud->reanim->mAnimTime = (float)luaL_checknumber(L, 2);
		return 0;
	}

	int Lua_ReanimIsPlaying(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) { lua_pushboolean(L, 0); return 1; }
		const char* trackName = luaL_checkstring(L, 2);
		lua_pushboolean(L, ud->reanim->IsAnimPlaying(trackName) ? 1 : 0);
		return 1;
	}

	int Lua_ReanimTrackExists(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) { lua_pushboolean(L, 0); return 1; }
		const char* trackName = luaL_checkstring(L, 2);
		lua_pushboolean(L, ud->reanim->TrackExists(trackName) ? 1 : 0);
		return 1;
	}

	int Lua_ReanimGetLoopType(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) { lua_pushinteger(L, 0); return 1; }
		lua_pushinteger(L, ud->reanim->mLoopType);
		return 1;
	}

	int Lua_ReanimGetLoopCount(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) { lua_pushinteger(L, 0); return 1; }
		lua_pushinteger(L, ud->reanim->mLoopCount);
		return 1;
	}

	int Lua_ReanimSetPosition(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) return 0;
		float x = (float)luaL_checknumber(L, 2);
		float y = (float)luaL_checknumber(L, 3);
		ud->reanim->SetPosition(x, y);
		return 0;
	}

	int Lua_ReanimOverrideScale(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) return 0;
		float sx = (float)luaL_checknumber(L, 2);
		float sy = (float)luaL_optnumber(L, 3, sx);
		ud->reanim->OverrideScale(sx, sy);
		return 0;
	}

	int Lua_ReanimShowOnlyTrack(lua_State* L)
	{
		LuaReanimUD* ud = (LuaReanimUD*)luaL_checkudata(L, 1, "Game.Reanim");
		if (!ud->reanim) return 0;
		const char* trackName = luaL_checkstring(L, 2);
		ud->reanim->ShowOnlyTrack(trackName);
		return 0;
	}

	void PushReanimUD(lua_State* L, Reanimation* reanim)
	{
		if (!reanim) { lua_pushnil(L); return; }
		LuaReanimUD* ud = (LuaReanimUD*)lua_newuserdata(L, sizeof(LuaReanimUD));
		ud->reanim = reanim;
		luaL_getmetatable(L, "Game.Reanim");
		lua_setmetatable(L, -2);
	}

	// Plant entity reanimation methods
	int Lua_EntityGetBodyReanim(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant)
		{
			lua_pushnil(L);
			return 1;
		}
		Reanimation* reanim = gLawnApp->ReanimationGet(ent->ptr.plant->mBodyReanimID);
		PushReanimUD(L, reanim);
		return 1;
	}

	int Lua_EntityPlayBodyReanim(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant) return 0;
		const char* trackName = luaL_checkstring(L, 2);
		int loopType = luaL_checkinteger(L, 3);
		int blendTime = luaL_optinteger(L, 4, 0);
		float animRate = (float)luaL_optnumber(L, 5, 0.0);
		ent->ptr.plant->PlayBodyReanim(trackName, (ReanimLoopType)loopType, blendTime, animRate);
		return 0;
	}

	int Lua_EntityPlayIdleAnim(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant) return 0;
		float animRate = (float)luaL_optnumber(L, 2, 12.0);
		ent->ptr.plant->PlayIdleAnim(animRate);
		return 0;
	}

	int Lua_EntityGetBodyReanimProgress(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant) { lua_pushnumber(L, 0); return 1; }
		Reanimation* reanim = gLawnApp->ReanimationGet(ent->ptr.plant->mBodyReanimID);
		lua_pushnumber(L, reanim ? reanim->mAnimTime : 0.0f);
		return 1;
	}

	int Lua_EntitySetBodyReanimRate(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant) return 0;
		float rate = (float)luaL_checknumber(L, 2);
		Reanimation* reanim = gLawnApp->ReanimationGet(ent->ptr.plant->mBodyReanimID);
		if (reanim) reanim->mAnimRate = rate;
		return 0;
	}

	int Lua_EntityGetBodyReanimRate(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant) { lua_pushnumber(L, 0); return 1; }
		Reanimation* reanim = gLawnApp->ReanimationGet(ent->ptr.plant->mBodyReanimID);
		lua_pushnumber(L, reanim ? reanim->mAnimRate : 0.0f);
		return 1;
	}

	int Lua_EntityIsAnimPlaying(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant) { lua_pushboolean(L, 0); return 1; }
		const char* trackName = luaL_checkstring(L, 2);
		Reanimation* reanim = gLawnApp->ReanimationGet(ent->ptr.plant->mBodyReanimID);
		lua_pushboolean(L, reanim && reanim->IsAnimPlaying(trackName) ? 1 : 0);
		return 1;
	}

	int Lua_EntityTrackExists(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant) { lua_pushboolean(L, 0); return 1; }
		const char* trackName = luaL_checkstring(L, 2);
		Reanimation* reanim = gLawnApp->ReanimationGet(ent->ptr.plant->mBodyReanimID);
		lua_pushboolean(L, reanim && reanim->TrackExists(trackName) ? 1 : 0);
		return 1;
	}

	int Lua_EntityGetBodyReanimLoopType(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant) { lua_pushinteger(L, 0); return 1; }
		Reanimation* reanim = gLawnApp->ReanimationGet(ent->ptr.plant->mBodyReanimID);
		lua_pushinteger(L, reanim ? (int)reanim->mLoopType : 0);
		return 1;
	}

	int Lua_EntityGetBodyReanimLoopCount(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 0 || !ent->ptr.plant) { lua_pushinteger(L, 0); return 1; }
		Reanimation* reanim = gLawnApp->ReanimationGet(ent->ptr.plant->mBodyReanimID);
		lua_pushinteger(L, reanim ? reanim->mLoopCount : 0);
		return 1;
	}

	int Lua_EntitySetDamage(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 3 || !ent->ptr.projectile) return 0;
		int amount = luaL_checkinteger(L, 2);
		ent->ptr.projectile->mDamageOverride = amount;
		return 0;
	}

	int Lua_EntitySetVelocity(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 3 || !ent->ptr.projectile) return 0;
		float vx = (float)luaL_checknumber(L, 2);
		float vy = (float)luaL_checknumber(L, 3);
		float vz = (float)luaL_optnumber(L, 4, 0.0);
		ent->ptr.projectile->mVelX = vx;
		ent->ptr.projectile->mVelY = vy;
		ent->ptr.projectile->mVelZ = vz;
		return 0;
	}

	int Lua_EntitySetDamageFlags(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 3 || !ent->ptr.projectile) return 0;
		int flags = luaL_checkinteger(L, 2);
		ent->ptr.projectile->mDamageRangeFlags = flags;
		return 0;
	}

	int Lua_EntitySetMotionType(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 3 || !ent->ptr.projectile) return 0;
		int motionType = luaL_checkinteger(L, 2);
		ent->ptr.projectile->mMotionType = static_cast<ProjectileMotion>(motionType);
		return 0;
	}

	int Lua_EntitySetTargetZombie(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type != 3 || !ent->ptr.projectile || !gLawnApp || !gLawnApp->mBoard) return 0;
		LuaEntity* target = (LuaEntity*)luaL_checkudata(L, 2, "Game.Entity");
		if (target->type != 1 || !target->ptr.zombie) return 0;
		ent->ptr.projectile->mTargetZombieID = gLawnApp->mBoard->ZombieGetID(target->ptr.zombie);
		return 0;
	}

	int Lua_EntityDie(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 0 && ent->ptr.plant) ent->ptr.plant->Die();
		else if (ent->type == 2 && ent->ptr.coin) ent->ptr.coin->Die();
		return 0;
	}

	int Lua_EntitySquish(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 0 && ent->ptr.plant) ent->ptr.plant->Squish();
		return 0;
	}

	int Lua_EntitySetSleeping(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 0 && ent->ptr.plant) ent->ptr.plant->SetSleeping(lua_toboolean(L, 2) != 0);
		return 0;
	}

	int Lua_EntityGetCost(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 0 && ent->ptr.plant) {
			lua_pushinteger(L, Plant::GetCost(ent->ptr.plant->mSeedType, ent->ptr.plant->mImitaterType));
		} else { lua_pushinteger(L, 0); }
		return 1;
	}

	int Lua_EntityGetName(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 0 && ent->ptr.plant) {
			lua_pushstring(L, Plant::GetNameString(ent->ptr.plant->mSeedType, ent->ptr.plant->mImitaterType).c_str());
		} else { lua_pushstring(L, ""); }
		return 1;
	}

	int Lua_EntityIsNocturnal(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 0 && ent->ptr.plant) {
			lua_pushboolean(L, Plant::IsNocturnal(ent->ptr.plant->mSeedType) ? 1 : 0);
		} else { lua_pushboolean(L, 0); }
		return 1;
	}

	int Lua_EntityIsFungus(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 0 && ent->ptr.plant) {
			lua_pushboolean(L, Plant::IsFungus(ent->ptr.plant->mSeedType) ? 1 : 0);
		} else { lua_pushboolean(L, 0); }
		return 1;
	}

	int Lua_EntityIsAquatic(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 0 && ent->ptr.plant) {
			lua_pushboolean(L, Plant::IsAquatic(ent->ptr.plant->mSeedType) ? 1 : 0);
		} else { lua_pushboolean(L, 0); }
		return 1;
	}

	int Lua_EntityIsUpgrade(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 0 && ent->ptr.plant) {
			lua_pushboolean(L, Plant::IsUpgrade(ent->ptr.plant->mSeedType) ? 1 : 0);
		} else { lua_pushboolean(L, 0); }
		return 1;
	}

	int Lua_EntityIsFlying(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 0 && ent->ptr.plant) {
			lua_pushboolean(L, Plant::IsFlying(ent->ptr.plant->mSeedType) ? 1 : 0);
		} else if (ent->type == 1 && ent->ptr.zombie) {
			lua_pushboolean(L, ent->ptr.zombie->IsFlying() ? 1 : 0);
		} else { lua_pushboolean(L, 0); }
		return 1;
	}

	int Lua_EntitySetRow(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) {
			ent->ptr.zombie->SetRow(luaL_checkinteger(L, 2));
		}
		return 0;
	}

	int Lua_EntityApplyChill(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) {
			ent->ptr.zombie->ApplyChill(lua_toboolean(L, 2) != 0);
		}
		return 0;
	}

	int Lua_EntityApplyButter(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) ent->ptr.zombie->ApplyButter();
		return 0;
	}

	int Lua_EntityRemoveButter(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) ent->ptr.zombie->RemoveButter();
		return 0;
	}

	int Lua_EntityStartMindControlled(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) ent->ptr.zombie->StartMindControlled();
		return 0;
	}

	int Lua_EntityDieNoLoot(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) ent->ptr.zombie->DieNoLoot();
		return 0;
	}

	int Lua_EntityDieWithLoot(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) ent->ptr.zombie->DieWithLoot();
		return 0;
	}

	int Lua_EntityTakeHelmDamage(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) {
			ent->ptr.zombie->TakeHelmDamage(luaL_checkinteger(L, 2), 0);
		}
		return 0;
	}

	int Lua_EntityTakeShieldDamage(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) {
			ent->ptr.zombie->TakeShieldDamage(luaL_checkinteger(L, 2), 0);
		}
		return 0;
	}

	int Lua_EntityIsOnHighGround(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) {
			lua_pushboolean(L, ent->ptr.zombie->IsOnHighGround() ? 1 : 0);
		} else { lua_pushboolean(L, 0); }
		return 1;
	}

	int Lua_EntityIsImmobilized(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 1 && ent->ptr.zombie) {
			lua_pushboolean(L, ent->ptr.zombie->IsImmobilizied() ? 1 : 0);
		} else { lua_pushboolean(L, 0); }
		return 1;
	}

	int Lua_EntityCoinGetValue(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 2 && ent->ptr.coin) {
			if (ent->ptr.coin->IsSun()) {
				lua_pushinteger(L, ent->ptr.coin->GetSunValue());
			} else {
				lua_pushinteger(L, Coin::GetCoinValue(ent->ptr.coin->mType));
			}
		} else { lua_pushinteger(L, 0); }
		return 1;
	}

	int Lua_EntityStartFade(lua_State* L)
	{
		LuaEntity* ent = (LuaEntity*)luaL_checkudata(L, 1, "Game.Entity");
		if (ent->type == 2 && ent->ptr.coin) ent->ptr.coin->StartFade();
		return 0;
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

	int Lua_GameGetMode(lua_State* L)
	{
		if (gLawnApp)
			lua_pushinteger(L, static_cast<int>(gLawnApp->mGameMode));
		else
			lua_pushinteger(L, -1);
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

	void Lua_RegisterConstants(lua_State* L)
	{
		// PlantType (SeedType)
		lua_newtable(L);
		lua_pushinteger(L, SEED_PEASHOOTER); lua_setfield(L, -2, "PEASHOOTER");
		lua_pushinteger(L, SEED_SUNFLOWER); lua_setfield(L, -2, "SUNFLOWER");
		lua_pushinteger(L, SEED_CHERRYBOMB); lua_setfield(L, -2, "CHERRYBOMB");
		lua_pushinteger(L, SEED_WALLNUT); lua_setfield(L, -2, "WALLNUT");
		lua_pushinteger(L, SEED_POTATOMINE); lua_setfield(L, -2, "POTATOMINE");
		lua_pushinteger(L, SEED_SNOWPEA); lua_setfield(L, -2, "SNOWPEA");
		lua_pushinteger(L, SEED_CHOMPER); lua_setfield(L, -2, "CHOMPER");
		lua_pushinteger(L, SEED_REPEATER); lua_setfield(L, -2, "REPEATER");
		lua_pushinteger(L, SEED_PUFFSHROOM); lua_setfield(L, -2, "PUFFSHROOM");
		lua_pushinteger(L, SEED_SUNSHROOM); lua_setfield(L, -2, "SUNSHROOM");
		lua_pushinteger(L, SEED_FUMESHROOM); lua_setfield(L, -2, "FUMESHROOM");
		lua_pushinteger(L, SEED_GRAVEBUSTER); lua_setfield(L, -2, "GRAVEBUSTER");
		lua_pushinteger(L, SEED_HYPNOSHROOM); lua_setfield(L, -2, "HYPNOSHROOM");
		lua_pushinteger(L, SEED_SCAREDYSHROOM); lua_setfield(L, -2, "SCAREDYSHROOM");
		lua_pushinteger(L, SEED_ICESHROOM); lua_setfield(L, -2, "ICESHROOM");
		lua_pushinteger(L, SEED_DOOMSHROOM); lua_setfield(L, -2, "DOOMSHROOM");
		lua_pushinteger(L, SEED_LILYPAD); lua_setfield(L, -2, "LILYPAD");
		lua_pushinteger(L, SEED_SQUASH); lua_setfield(L, -2, "SQUASH");
		lua_pushinteger(L, SEED_THREEPEATER); lua_setfield(L, -2, "THREEPEATER");
		lua_pushinteger(L, SEED_TANGLEKELP); lua_setfield(L, -2, "TANGLEKELP");
		lua_pushinteger(L, SEED_JALAPENO); lua_setfield(L, -2, "JALAPENO");
		lua_pushinteger(L, SEED_SPIKEWEED); lua_setfield(L, -2, "SPIKEWEED");
		lua_pushinteger(L, SEED_TORCHWOOD); lua_setfield(L, -2, "TORCHWOOD");
		lua_pushinteger(L, SEED_TALLNUT); lua_setfield(L, -2, "TALLNUT");
		lua_pushinteger(L, SEED_SEASHROOM); lua_setfield(L, -2, "SEASHROOM");
		lua_pushinteger(L, SEED_PLANTERN); lua_setfield(L, -2, "PLANTERN");
		lua_pushinteger(L, SEED_CACTUS); lua_setfield(L, -2, "CACTUS");
		lua_pushinteger(L, SEED_BLOVER); lua_setfield(L, -2, "BLOVER");
		lua_pushinteger(L, SEED_SPLITPEA); lua_setfield(L, -2, "SPLITPEA");
		lua_pushinteger(L, SEED_STARFRUIT); lua_setfield(L, -2, "STARFRUIT");
		lua_pushinteger(L, SEED_PUMPKINSHELL); lua_setfield(L, -2, "PUMPKIN");
		lua_pushinteger(L, SEED_MAGNETSHROOM); lua_setfield(L, -2, "MAGNETSHROOM");
		lua_pushinteger(L, SEED_CABBAGEPULT); lua_setfield(L, -2, "CABBAGEPULT");
		lua_pushinteger(L, SEED_FLOWERPOT); lua_setfield(L, -2, "FLOWERPOT");
		lua_pushinteger(L, SEED_KERNELPULT); lua_setfield(L, -2, "KERNELPULT");
		lua_pushinteger(L, SEED_INSTANT_COFFEE); lua_setfield(L, -2, "COFFEEBEAN");
		lua_pushinteger(L, SEED_GARLIC); lua_setfield(L, -2, "GARLIC");
		lua_pushinteger(L, SEED_UMBRELLA); lua_setfield(L, -2, "UMBRELLA");
		lua_pushinteger(L, SEED_MARIGOLD); lua_setfield(L, -2, "MARIGOLD");
		lua_pushinteger(L, SEED_MELONPULT); lua_setfield(L, -2, "MELONPULT");
		lua_pushinteger(L, SEED_GATLINGPEA); lua_setfield(L, -2, "GATLINGPEA");
		lua_pushinteger(L, SEED_TWINSUNFLOWER); lua_setfield(L, -2, "TWINSUNFLOWER");
		lua_pushinteger(L, SEED_GLOOMSHROOM); lua_setfield(L, -2, "GLOOMSHROOM");
		lua_pushinteger(L, SEED_CATTAIL); lua_setfield(L, -2, "CATTAIL");
		lua_pushinteger(L, SEED_WINTERMELON); lua_setfield(L, -2, "WINTERMELON");
		lua_pushinteger(L, SEED_GOLD_MAGNET); lua_setfield(L, -2, "GOLD_MAGNET");
		lua_pushinteger(L, SEED_SPIKEROCK); lua_setfield(L, -2, "SPIKEROCK");
		lua_pushinteger(L, SEED_COBCANNON); lua_setfield(L, -2, "COBCANNON");
		lua_pushinteger(L, SEED_IMITATER); lua_setfield(L, -2, "IMITATER");
		lua_pushinteger(L, SEED_NONE); lua_setfield(L, -2, "NONE");
		lua_pushinteger(L, NUM_SEED_TYPES); lua_setfield(L, -2, "NUM_TYPES");
		lua_setglobal(L, "PlantType");

		// ZombieType
		lua_newtable(L);
		lua_pushinteger(L, ZOMBIE_INVALID); lua_setfield(L, -2, "INVALID");
		lua_pushinteger(L, ZOMBIE_NORMAL); lua_setfield(L, -2, "NORMAL");
		lua_pushinteger(L, ZOMBIE_FLAG); lua_setfield(L, -2, "FLAG");
		lua_pushinteger(L, ZOMBIE_TRAFFIC_CONE); lua_setfield(L, -2, "TRAFFIC_CONE");
		lua_pushinteger(L, ZOMBIE_POLEVAULTER); lua_setfield(L, -2, "POLEVAULTER");
		lua_pushinteger(L, ZOMBIE_PAIL); lua_setfield(L, -2, "PAIL");
		lua_pushinteger(L, ZOMBIE_NEWSPAPER); lua_setfield(L, -2, "NEWSPAPER");
		lua_pushinteger(L, ZOMBIE_DOOR); lua_setfield(L, -2, "DOOR");
		lua_pushinteger(L, ZOMBIE_FOOTBALL); lua_setfield(L, -2, "FOOTBALL");
		lua_pushinteger(L, ZOMBIE_DANCER); lua_setfield(L, -2, "DANCER");
		lua_pushinteger(L, ZOMBIE_BACKUP_DANCER); lua_setfield(L, -2, "BACKUP_DANCER");
		lua_pushinteger(L, ZOMBIE_DUCKY_TUBE); lua_setfield(L, -2, "DUCKY_TUBE");
		lua_pushinteger(L, ZOMBIE_SNORKEL); lua_setfield(L, -2, "SNORKEL");
		lua_pushinteger(L, ZOMBIE_ZAMBONI); lua_setfield(L, -2, "ZAMBONI");
		lua_pushinteger(L, ZOMBIE_BOBSLED); lua_setfield(L, -2, "BOBSLED");
		lua_pushinteger(L, ZOMBIE_DOLPHIN_RIDER); lua_setfield(L, -2, "DOLPHIN_RIDER");
		lua_pushinteger(L, ZOMBIE_JACK_IN_THE_BOX); lua_setfield(L, -2, "JACK_IN_THE_BOX");
		lua_pushinteger(L, ZOMBIE_BALLOON); lua_setfield(L, -2, "BALLOON");
		lua_pushinteger(L, ZOMBIE_DIGGER); lua_setfield(L, -2, "DIGGER");
		lua_pushinteger(L, ZOMBIE_POGO); lua_setfield(L, -2, "POGO");
		lua_pushinteger(L, ZOMBIE_YETI); lua_setfield(L, -2, "YETI");
		lua_pushinteger(L, ZOMBIE_BUNGEE); lua_setfield(L, -2, "BUNGEE");
		lua_pushinteger(L, ZOMBIE_LADDER); lua_setfield(L, -2, "LADDER");
		lua_pushinteger(L, ZOMBIE_CATAPULT); lua_setfield(L, -2, "CATAPULT");
		lua_pushinteger(L, ZOMBIE_GARGANTUAR); lua_setfield(L, -2, "GARGANTUAR");
		lua_pushinteger(L, ZOMBIE_IMP); lua_setfield(L, -2, "IMP");
		lua_pushinteger(L, ZOMBIE_BOSS); lua_setfield(L, -2, "BOSS");
		lua_pushinteger(L, ZOMBIE_PEA_HEAD); lua_setfield(L, -2, "PEA_HEAD");
		lua_pushinteger(L, ZOMBIE_WALLNUT_HEAD); lua_setfield(L, -2, "WALLNUT_HEAD");
		lua_pushinteger(L, ZOMBIE_JALAPENO_HEAD); lua_setfield(L, -2, "JALAPENO_HEAD");
		lua_pushinteger(L, ZOMBIE_GATLING_HEAD); lua_setfield(L, -2, "GATLING_HEAD");
		lua_pushinteger(L, ZOMBIE_SQUASH_HEAD); lua_setfield(L, -2, "SQUASH_HEAD");
		lua_pushinteger(L, ZOMBIE_TALLNUT_HEAD); lua_setfield(L, -2, "TALLNUT_HEAD");
		lua_pushinteger(L, ZOMBIE_REDEYE_GARGANTUAR); lua_setfield(L, -2, "REDEYE_GARGANTUAR");
		lua_pushinteger(L, NUM_ZOMBIE_TYPES); lua_setfield(L, -2, "NUM_TYPES");
		lua_setglobal(L, "ZombieType");

		// ProjectileType
		lua_newtable(L);
		lua_pushinteger(L, PROJECTILE_PEA); lua_setfield(L, -2, "PEA");
		lua_pushinteger(L, PROJECTILE_SNOWPEA); lua_setfield(L, -2, "SNOWPEA");
		lua_pushinteger(L, PROJECTILE_CABBAGE); lua_setfield(L, -2, "CABBAGE");
		lua_pushinteger(L, PROJECTILE_MELON); lua_setfield(L, -2, "MELON");
		lua_pushinteger(L, PROJECTILE_PUFF); lua_setfield(L, -2, "PUFF");
		lua_pushinteger(L, PROJECTILE_WINTERMELON); lua_setfield(L, -2, "WINTERMELON");
		lua_pushinteger(L, PROJECTILE_FIREBALL); lua_setfield(L, -2, "FIREBALL");
		lua_pushinteger(L, PROJECTILE_STAR); lua_setfield(L, -2, "STAR");
		lua_pushinteger(L, PROJECTILE_SPIKE); lua_setfield(L, -2, "SPIKE");
		lua_pushinteger(L, PROJECTILE_BASKETBALL); lua_setfield(L, -2, "BASKETBALL");
		lua_pushinteger(L, PROJECTILE_KERNEL); lua_setfield(L, -2, "KERNEL");
		lua_pushinteger(L, PROJECTILE_COBBIG); lua_setfield(L, -2, "COBBIG");
		lua_pushinteger(L, PROJECTILE_BUTTER); lua_setfield(L, -2, "BUTTER");
		lua_pushinteger(L, PROJECTILE_ZOMBIE_PEA); lua_setfield(L, -2, "ZOMBIE_PEA");
		lua_pushinteger(L, NUM_PROJECTILES); lua_setfield(L, -2, "NUM_TYPES");
		lua_setglobal(L, "ProjectileType");

		// CoinType
		lua_newtable(L);
		lua_pushinteger(L, COIN_NONE); lua_setfield(L, -2, "NONE");
		lua_pushinteger(L, COIN_SILVER); lua_setfield(L, -2, "SILVER");
		lua_pushinteger(L, COIN_GOLD); lua_setfield(L, -2, "GOLD");
		lua_pushinteger(L, COIN_DIAMOND); lua_setfield(L, -2, "DIAMOND");
		lua_pushinteger(L, COIN_SUN); lua_setfield(L, -2, "SUN");
		lua_pushinteger(L, COIN_SMALLSUN); lua_setfield(L, -2, "SMALLSUN");
		lua_pushinteger(L, COIN_LARGESUN); lua_setfield(L, -2, "LARGESUN");
		lua_pushinteger(L, COIN_FINAL_SEED_PACKET); lua_setfield(L, -2, "FINAL_SEED_PACKET");
		lua_pushinteger(L, COIN_TROPHY); lua_setfield(L, -2, "TROPHY");
		lua_pushinteger(L, COIN_SHOVEL); lua_setfield(L, -2, "SHOVEL");
		lua_pushinteger(L, COIN_ALMANAC); lua_setfield(L, -2, "ALMANAC");
		lua_pushinteger(L, COIN_CARKEYS); lua_setfield(L, -2, "CARKEYS");
		lua_pushinteger(L, COIN_VASE); lua_setfield(L, -2, "VASE");
		lua_pushinteger(L, COIN_WATERING_CAN); lua_setfield(L, -2, "WATERING_CAN");
		lua_pushinteger(L, COIN_TACO); lua_setfield(L, -2, "TACO");
		lua_pushinteger(L, COIN_NOTE); lua_setfield(L, -2, "NOTE");
		lua_pushinteger(L, COIN_USABLE_SEED_PACKET); lua_setfield(L, -2, "USABLE_SEED_PACKET");
		lua_pushinteger(L, COIN_PRESENT_PLANT); lua_setfield(L, -2, "PRESENT_PLANT");
		lua_pushinteger(L, COIN_AWARD_MONEY_BAG); lua_setfield(L, -2, "AWARD_MONEY_BAG");
		lua_pushinteger(L, COIN_AWARD_PRESENT); lua_setfield(L, -2, "AWARD_PRESENT");
		lua_pushinteger(L, COIN_AWARD_BAG_DIAMOND); lua_setfield(L, -2, "AWARD_BAG_DIAMOND");
		lua_pushinteger(L, COIN_AWARD_SILVER_SUNFLOWER); lua_setfield(L, -2, "AWARD_SILVER_SUNFLOWER");
		lua_pushinteger(L, COIN_AWARD_GOLD_SUNFLOWER); lua_setfield(L, -2, "AWARD_GOLD_SUNFLOWER");
		lua_pushinteger(L, COIN_CHOCOLATE); lua_setfield(L, -2, "CHOCOLATE");
		lua_pushinteger(L, COIN_AWARD_CHOCOLATE); lua_setfield(L, -2, "AWARD_CHOCOLATE");
		lua_pushinteger(L, COIN_PRESENT_MINIGAMES); lua_setfield(L, -2, "PRESENT_MINIGAMES");
		lua_pushinteger(L, COIN_PRESENT_PUZZLE_MODE); lua_setfield(L, -2, "PRESENT_PUZZLE_MODE");
		lua_pushinteger(L, COIN_PRESENT_SURVIVAL_MODE); lua_setfield(L, -2, "PRESENT_SURVIVAL_MODE");
		lua_setglobal(L, "CoinType");

		// GameMode
		lua_newtable(L);
		lua_pushinteger(L, GAMEMODE_ADVENTURE); lua_setfield(L, -2, "ADVENTURE");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_NORMAL_STAGE_1); lua_setfield(L, -2, "SURVIVAL_NORMAL_STAGE_1");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_NORMAL_STAGE_2); lua_setfield(L, -2, "SURVIVAL_NORMAL_STAGE_2");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_NORMAL_STAGE_3); lua_setfield(L, -2, "SURVIVAL_NORMAL_STAGE_3");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_NORMAL_STAGE_4); lua_setfield(L, -2, "SURVIVAL_NORMAL_STAGE_4");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_NORMAL_STAGE_5); lua_setfield(L, -2, "SURVIVAL_NORMAL_STAGE_5");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_HARD_STAGE_1); lua_setfield(L, -2, "SURVIVAL_HARD_STAGE_1");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_HARD_STAGE_2); lua_setfield(L, -2, "SURVIVAL_HARD_STAGE_2");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_HARD_STAGE_3); lua_setfield(L, -2, "SURVIVAL_HARD_STAGE_3");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_HARD_STAGE_4); lua_setfield(L, -2, "SURVIVAL_HARD_STAGE_4");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_HARD_STAGE_5); lua_setfield(L, -2, "SURVIVAL_HARD_STAGE_5");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_ENDLESS_STAGE_1); lua_setfield(L, -2, "SURVIVAL_ENDLESS_STAGE_1");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_ENDLESS_STAGE_2); lua_setfield(L, -2, "SURVIVAL_ENDLESS_STAGE_2");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_ENDLESS_STAGE_3); lua_setfield(L, -2, "SURVIVAL_ENDLESS_STAGE_3");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_ENDLESS_STAGE_4); lua_setfield(L, -2, "SURVIVAL_ENDLESS_STAGE_4");
		lua_pushinteger(L, GAMEMODE_SURVIVAL_ENDLESS_STAGE_5); lua_setfield(L, -2, "SURVIVAL_ENDLESS_STAGE_5");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_WAR_AND_PEAS); lua_setfield(L, -2, "CHALLENGE_WAR_AND_PEAS");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_WALLNUT_BOWLING); lua_setfield(L, -2, "CHALLENGE_WALLNUT_BOWLING");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_SLOT_MACHINE); lua_setfield(L, -2, "CHALLENGE_SLOT_MACHINE");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_RAINING_SEEDS); lua_setfield(L, -2, "CHALLENGE_RAINING_SEEDS");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_BEGHOULED); lua_setfield(L, -2, "CHALLENGE_BEGHOULED");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_INVISIGHOUL); lua_setfield(L, -2, "CHALLENGE_INVISIGHOUL");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_SEEING_STARS); lua_setfield(L, -2, "CHALLENGE_SEEING_STARS");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_ZOMBIQUARIUM); lua_setfield(L, -2, "CHALLENGE_ZOMBIQUARIUM");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_BEGHOULED_TWIST); lua_setfield(L, -2, "CHALLENGE_BEGHOULED_TWIST");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_LITTLE_TROUBLE); lua_setfield(L, -2, "CHALLENGE_LITTLE_TROUBLE");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_PORTAL_COMBAT); lua_setfield(L, -2, "CHALLENGE_PORTAL_COMBAT");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_COLUMN); lua_setfield(L, -2, "CHALLENGE_COLUMN");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_BOBSLED_BONANZA); lua_setfield(L, -2, "CHALLENGE_BOBSLED_BONANZA");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_SPEED); lua_setfield(L, -2, "CHALLENGE_SPEED");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_WHACK_A_ZOMBIE); lua_setfield(L, -2, "CHALLENGE_WHACK_A_ZOMBIE");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_LAST_STAND); lua_setfield(L, -2, "CHALLENGE_LAST_STAND");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_WAR_AND_PEAS_2); lua_setfield(L, -2, "CHALLENGE_WAR_AND_PEAS_2");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_WALLNUT_BOWLING_2); lua_setfield(L, -2, "CHALLENGE_WALLNUT_BOWLING_2");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_POGO_PARTY); lua_setfield(L, -2, "CHALLENGE_POGO_PARTY");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_FINAL_BOSS); lua_setfield(L, -2, "CHALLENGE_FINAL_BOSS");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT); lua_setfield(L, -2, "CHALLENGE_ART_CHALLENGE_WALLNUT");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_SUNNY_DAY); lua_setfield(L, -2, "CHALLENGE_SUNNY_DAY");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_RESODDED); lua_setfield(L, -2, "CHALLENGE_RESODDED");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_BIG_TIME); lua_setfield(L, -2, "CHALLENGE_BIG_TIME");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER); lua_setfield(L, -2, "CHALLENGE_ART_CHALLENGE_SUNFLOWER");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_AIR_RAID); lua_setfield(L, -2, "CHALLENGE_AIR_RAID");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_ICE); lua_setfield(L, -2, "CHALLENGE_ICE");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_ZEN_GARDEN); lua_setfield(L, -2, "CHALLENGE_ZEN_GARDEN");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_HIGH_GRAVITY); lua_setfield(L, -2, "CHALLENGE_HIGH_GRAVITY");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_GRAVE_DANGER); lua_setfield(L, -2, "CHALLENGE_GRAVE_DANGER");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_SHOVEL); lua_setfield(L, -2, "CHALLENGE_SHOVEL");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_STORMY_NIGHT); lua_setfield(L, -2, "CHALLENGE_STORMY_NIGHT");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_BUNGEE_BLITZ); lua_setfield(L, -2, "CHALLENGE_BUNGEE_BLITZ");
		lua_pushinteger(L, GAMEMODE_CHALLENGE_SQUIRREL); lua_setfield(L, -2, "CHALLENGE_SQUIRREL");
		lua_pushinteger(L, GAMEMODE_TREE_OF_WISDOM); lua_setfield(L, -2, "TREE_OF_WISDOM");
		lua_pushinteger(L, GAMEMODE_SCARY_POTTER_1); lua_setfield(L, -2, "SCARY_POTTER_1");
		lua_pushinteger(L, GAMEMODE_SCARY_POTTER_2); lua_setfield(L, -2, "SCARY_POTTER_2");
		lua_pushinteger(L, GAMEMODE_SCARY_POTTER_3); lua_setfield(L, -2, "SCARY_POTTER_3");
		lua_pushinteger(L, GAMEMODE_SCARY_POTTER_4); lua_setfield(L, -2, "SCARY_POTTER_4");
		lua_pushinteger(L, GAMEMODE_SCARY_POTTER_5); lua_setfield(L, -2, "SCARY_POTTER_5");
		lua_pushinteger(L, GAMEMODE_SCARY_POTTER_6); lua_setfield(L, -2, "SCARY_POTTER_6");
		lua_pushinteger(L, GAMEMODE_SCARY_POTTER_7); lua_setfield(L, -2, "SCARY_POTTER_7");
		lua_pushinteger(L, GAMEMODE_SCARY_POTTER_8); lua_setfield(L, -2, "SCARY_POTTER_8");
		lua_pushinteger(L, GAMEMODE_SCARY_POTTER_9); lua_setfield(L, -2, "SCARY_POTTER_9");
		lua_pushinteger(L, GAMEMODE_SCARY_POTTER_ENDLESS); lua_setfield(L, -2, "SCARY_POTTER_ENDLESS");
		lua_pushinteger(L, GAMEMODE_PUZZLE_I_ZOMBIE_1); lua_setfield(L, -2, "PUZZLE_I_ZOMBIE_1");
		lua_pushinteger(L, GAMEMODE_PUZZLE_I_ZOMBIE_2); lua_setfield(L, -2, "PUZZLE_I_ZOMBIE_2");
		lua_pushinteger(L, GAMEMODE_PUZZLE_I_ZOMBIE_3); lua_setfield(L, -2, "PUZZLE_I_ZOMBIE_3");
		lua_pushinteger(L, GAMEMODE_PUZZLE_I_ZOMBIE_4); lua_setfield(L, -2, "PUZZLE_I_ZOMBIE_4");
		lua_pushinteger(L, GAMEMODE_PUZZLE_I_ZOMBIE_5); lua_setfield(L, -2, "PUZZLE_I_ZOMBIE_5");
		lua_pushinteger(L, GAMEMODE_PUZZLE_I_ZOMBIE_6); lua_setfield(L, -2, "PUZZLE_I_ZOMBIE_6");
		lua_pushinteger(L, GAMEMODE_PUZZLE_I_ZOMBIE_7); lua_setfield(L, -2, "PUZZLE_I_ZOMBIE_7");
		lua_pushinteger(L, GAMEMODE_PUZZLE_I_ZOMBIE_8); lua_setfield(L, -2, "PUZZLE_I_ZOMBIE_8");
		lua_pushinteger(L, GAMEMODE_PUZZLE_I_ZOMBIE_9); lua_setfield(L, -2, "PUZZLE_I_ZOMBIE_9");
		lua_pushinteger(L, GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS); lua_setfield(L, -2, "PUZZLE_I_ZOMBIE_ENDLESS");
		lua_pushinteger(L, GAMEMODE_UPSELL); lua_setfield(L, -2, "UPSELL");
		lua_pushinteger(L, GAMEMODE_INTRO); lua_setfield(L, -2, "INTRO");
		lua_pushinteger(L, NUM_GAME_MODES); lua_setfield(L, -2, "NUM_MODES");
		lua_setglobal(L, "GameMode");

		// PlantSubClass
		lua_newtable(L);
		lua_pushinteger(L, SUBCLASS_NORMAL); lua_setfield(L, -2, "NORMAL");
		lua_pushinteger(L, SUBCLASS_SHOOTER); lua_setfield(L, -2, "SHOOTER");
		lua_setglobal(L, "PlantSubClass");

		// PlantState
		lua_newtable(L);
		lua_pushinteger(L, STATE_NOTREADY); lua_setfield(L, -2, "NOTREADY");
		lua_pushinteger(L, STATE_READY); lua_setfield(L, -2, "READY");
		lua_pushinteger(L, STATE_DOINGSPECIAL); lua_setfield(L, -2, "DOINGSPECIAL");
		lua_pushinteger(L, STATE_SQUASH_LOOK); lua_setfield(L, -2, "SQUASH_LOOK");
		lua_pushinteger(L, STATE_SQUASH_PRE_LAUNCH); lua_setfield(L, -2, "SQUASH_PRE_LAUNCH");
		lua_pushinteger(L, STATE_SQUASH_RISING); lua_setfield(L, -2, "SQUASH_RISING");
		lua_pushinteger(L, STATE_SQUASH_FALLING); lua_setfield(L, -2, "SQUASH_FALLING");
		lua_pushinteger(L, STATE_SQUASH_DONE_FALLING); lua_setfield(L, -2, "SQUASH_DONE_FALLING");
		lua_pushinteger(L, STATE_GRAVEBUSTER_LANDING); lua_setfield(L, -2, "GRAVEBUSTER_LANDING");
		lua_pushinteger(L, STATE_GRAVEBUSTER_EATING); lua_setfield(L, -2, "GRAVEBUSTER_EATING");
		lua_pushinteger(L, STATE_CHOMPER_BITING); lua_setfield(L, -2, "CHOMPER_BITING");
		lua_pushinteger(L, STATE_CHOMPER_BITING_GOT_ONE); lua_setfield(L, -2, "CHOMPER_BITING_GOT_ONE");
		lua_pushinteger(L, STATE_CHOMPER_BITING_MISSED); lua_setfield(L, -2, "CHOMPER_BITING_MISSED");
		lua_pushinteger(L, STATE_CHOMPER_DIGESTING); lua_setfield(L, -2, "CHOMPER_DIGESTING");
		lua_pushinteger(L, STATE_CHOMPER_SWALLOWING); lua_setfield(L, -2, "CHOMPER_SWALLOWING");
		lua_pushinteger(L, STATE_POTATO_RISING); lua_setfield(L, -2, "POTATO_RISING");
		lua_pushinteger(L, STATE_POTATO_ARMED); lua_setfield(L, -2, "POTATO_ARMED");
		lua_pushinteger(L, STATE_POTATO_MASHED); lua_setfield(L, -2, "POTATO_MASHED");
		lua_pushinteger(L, STATE_SPIKEWEED_ATTACKING); lua_setfield(L, -2, "SPIKEWEED_ATTACKING");
		lua_pushinteger(L, STATE_SPIKEWEED_ATTACKING_2); lua_setfield(L, -2, "SPIKEWEED_ATTACKING_2");
		lua_pushinteger(L, STATE_SCAREDYSHROOM_LOWERING); lua_setfield(L, -2, "SCAREDYSHROOM_LOWERING");
		lua_pushinteger(L, STATE_SCAREDYSHROOM_SCARED); lua_setfield(L, -2, "SCAREDYSHROOM_SCARED");
		lua_pushinteger(L, STATE_SCAREDYSHROOM_RAISING); lua_setfield(L, -2, "SCAREDYSHROOM_RAISING");
		lua_pushinteger(L, STATE_SUNSHROOM_SMALL); lua_setfield(L, -2, "SUNSHROOM_SMALL");
		lua_pushinteger(L, STATE_SUNSHROOM_GROWING); lua_setfield(L, -2, "SUNSHROOM_GROWING");
		lua_pushinteger(L, STATE_SUNSHROOM_BIG); lua_setfield(L, -2, "SUNSHROOM_BIG");
		lua_pushinteger(L, STATE_MAGNETSHROOM_SUCKING); lua_setfield(L, -2, "MAGNETSHROOM_SUCKING");
		lua_pushinteger(L, STATE_MAGNETSHROOM_CHARGING); lua_setfield(L, -2, "MAGNETSHROOM_CHARGING");
		lua_pushinteger(L, STATE_BOWLING_UP); lua_setfield(L, -2, "BOWLING_UP");
		lua_pushinteger(L, STATE_BOWLING_DOWN); lua_setfield(L, -2, "BOWLING_DOWN");
		lua_pushinteger(L, STATE_CACTUS_LOW); lua_setfield(L, -2, "CACTUS_LOW");
		lua_pushinteger(L, STATE_CACTUS_RISING); lua_setfield(L, -2, "CACTUS_RISING");
		lua_pushinteger(L, STATE_CACTUS_HIGH); lua_setfield(L, -2, "CACTUS_HIGH");
		lua_pushinteger(L, STATE_CACTUS_LOWERING); lua_setfield(L, -2, "CACTUS_LOWERING");
		lua_pushinteger(L, STATE_TANGLEKELP_GRABBING); lua_setfield(L, -2, "TANGLEKELP_GRABBING");
		lua_pushinteger(L, STATE_COBCANNON_ARMING); lua_setfield(L, -2, "COBCANNON_ARMING");
		lua_pushinteger(L, STATE_COBCANNON_LOADING); lua_setfield(L, -2, "COBCANNON_LOADING");
		lua_pushinteger(L, STATE_COBCANNON_READY); lua_setfield(L, -2, "COBCANNON_READY");
		lua_pushinteger(L, STATE_COBCANNON_FIRING); lua_setfield(L, -2, "COBCANNON_FIRING");
		lua_pushinteger(L, STATE_KERNELPULT_BUTTER); lua_setfield(L, -2, "KERNELPULT_BUTTER");
		lua_pushinteger(L, STATE_UMBRELLA_TRIGGERED); lua_setfield(L, -2, "UMBRELLA_TRIGGERED");
		lua_pushinteger(L, STATE_UMBRELLA_REFLECTING); lua_setfield(L, -2, "UMBRELLA_REFLECTING");
		lua_pushinteger(L, STATE_IMITATER_MORPHING); lua_setfield(L, -2, "IMITATER_MORPHING");
		lua_pushinteger(L, STATE_ZEN_GARDEN_WATERED); lua_setfield(L, -2, "ZEN_GARDEN_WATERED");
		lua_pushinteger(L, STATE_ZEN_GARDEN_NEEDY); lua_setfield(L, -2, "ZEN_GARDEN_NEEDY");
		lua_pushinteger(L, STATE_ZEN_GARDEN_HAPPY); lua_setfield(L, -2, "ZEN_GARDEN_HAPPY");
		lua_pushinteger(L, STATE_MARIGOLD_ENDING); lua_setfield(L, -2, "MARIGOLD_ENDING");
		lua_pushinteger(L, STATE_FLOWERPOT_INVULNERABLE); lua_setfield(L, -2, "FLOWERPOT_INVULNERABLE");
		lua_pushinteger(L, STATE_LILYPAD_INVULNERABLE); lua_setfield(L, -2, "LILYPAD_INVULNERABLE");
		lua_setglobal(L, "PlantState");

		// ZombiePhase
		lua_newtable(L);
		lua_pushinteger(L, PHASE_ZOMBIE_NORMAL); lua_setfield(L, -2, "ZOMBIE_NORMAL");
		lua_pushinteger(L, PHASE_ZOMBIE_DYING); lua_setfield(L, -2, "ZOMBIE_DYING");
		lua_pushinteger(L, PHASE_ZOMBIE_BURNED); lua_setfield(L, -2, "ZOMBIE_BURNED");
		lua_pushinteger(L, PHASE_ZOMBIE_MOWERED); lua_setfield(L, -2, "ZOMBIE_MOWERED");
		lua_pushinteger(L, PHASE_BUNGEE_DIVING); lua_setfield(L, -2, "BUNGEE_DIVING");
		lua_pushinteger(L, PHASE_BUNGEE_DIVING_SCREAMING); lua_setfield(L, -2, "BUNGEE_DIVING_SCREAMING");
		lua_pushinteger(L, PHASE_BUNGEE_AT_BOTTOM); lua_setfield(L, -2, "BUNGEE_AT_BOTTOM");
		lua_pushinteger(L, PHASE_BUNGEE_GRABBING); lua_setfield(L, -2, "BUNGEE_GRABBING");
		lua_pushinteger(L, PHASE_BUNGEE_RISING); lua_setfield(L, -2, "BUNGEE_RISING");
		lua_pushinteger(L, PHASE_BUNGEE_HIT_OUCHY); lua_setfield(L, -2, "BUNGEE_HIT_OUCHY");
		lua_pushinteger(L, PHASE_BUNGEE_CUTSCENE); lua_setfield(L, -2, "BUNGEE_CUTSCENE");
		lua_pushinteger(L, PHASE_POLEVAULTER_PRE_VAULT); lua_setfield(L, -2, "POLEVAULTER_PRE_VAULT");
		lua_pushinteger(L, PHASE_POLEVAULTER_IN_VAULT); lua_setfield(L, -2, "POLEVAULTER_IN_VAULT");
		lua_pushinteger(L, PHASE_POLEVAULTER_POST_VAULT); lua_setfield(L, -2, "POLEVAULTER_POST_VAULT");
		lua_pushinteger(L, PHASE_RISING_FROM_GRAVE); lua_setfield(L, -2, "RISING_FROM_GRAVE");
		lua_pushinteger(L, PHASE_JACK_IN_THE_BOX_RUNNING); lua_setfield(L, -2, "JACK_IN_THE_BOX_RUNNING");
		lua_pushinteger(L, PHASE_JACK_IN_THE_BOX_POPPING); lua_setfield(L, -2, "JACK_IN_THE_BOX_POPPING");
		lua_pushinteger(L, PHASE_BOBSLED_SLIDING); lua_setfield(L, -2, "BOBSLED_SLIDING");
		lua_pushinteger(L, PHASE_BOBSLED_BOARDING); lua_setfield(L, -2, "BOBSLED_BOARDING");
		lua_pushinteger(L, PHASE_BOBSLED_CRASHING); lua_setfield(L, -2, "BOBSLED_CRASHING");
		lua_pushinteger(L, PHASE_POGO_BOUNCING); lua_setfield(L, -2, "POGO_BOUNCING");
		lua_pushinteger(L, PHASE_POGO_HIGH_BOUNCE_1); lua_setfield(L, -2, "POGO_HIGH_BOUNCE_1");
		lua_pushinteger(L, PHASE_POGO_HIGH_BOUNCE_2); lua_setfield(L, -2, "POGO_HIGH_BOUNCE_2");
		lua_pushinteger(L, PHASE_POGO_HIGH_BOUNCE_3); lua_setfield(L, -2, "POGO_HIGH_BOUNCE_3");
		lua_pushinteger(L, PHASE_POGO_HIGH_BOUNCE_4); lua_setfield(L, -2, "POGO_HIGH_BOUNCE_4");
		lua_pushinteger(L, PHASE_POGO_HIGH_BOUNCE_5); lua_setfield(L, -2, "POGO_HIGH_BOUNCE_5");
		lua_pushinteger(L, PHASE_POGO_HIGH_BOUNCE_6); lua_setfield(L, -2, "POGO_HIGH_BOUNCE_6");
		lua_pushinteger(L, PHASE_POGO_FORWARD_BOUNCE_2); lua_setfield(L, -2, "POGO_FORWARD_BOUNCE_2");
		lua_pushinteger(L, PHASE_POGO_FORWARD_BOUNCE_7); lua_setfield(L, -2, "POGO_FORWARD_BOUNCE_7");
		lua_pushinteger(L, PHASE_NEWSPAPER_READING); lua_setfield(L, -2, "NEWSPAPER_READING");
		lua_pushinteger(L, PHASE_NEWSPAPER_MADDENING); lua_setfield(L, -2, "NEWSPAPER_MADDENING");
		lua_pushinteger(L, PHASE_NEWSPAPER_MAD); lua_setfield(L, -2, "NEWSPAPER_MAD");
		lua_pushinteger(L, PHASE_DIGGER_TUNNELING); lua_setfield(L, -2, "DIGGER_TUNNELING");
		lua_pushinteger(L, PHASE_DIGGER_RISING); lua_setfield(L, -2, "DIGGER_RISING");
		lua_pushinteger(L, PHASE_DIGGER_TUNNELING_PAUSE_WITHOUT_AXE); lua_setfield(L, -2, "DIGGER_TUNNELING_PAUSE_WITHOUT_AXE");
		lua_pushinteger(L, PHASE_DIGGER_RISE_WITHOUT_AXE); lua_setfield(L, -2, "DIGGER_RISE_WITHOUT_AXE");
		lua_pushinteger(L, PHASE_DIGGER_STUNNED); lua_setfield(L, -2, "DIGGER_STUNNED");
		lua_pushinteger(L, PHASE_DIGGER_WALKING); lua_setfield(L, -2, "DIGGER_WALKING");
		lua_pushinteger(L, PHASE_DIGGER_WALKING_WITHOUT_AXE); lua_setfield(L, -2, "DIGGER_WALKING_WITHOUT_AXE");
		lua_pushinteger(L, PHASE_DIGGER_CUTSCENE); lua_setfield(L, -2, "DIGGER_CUTSCENE");
		lua_pushinteger(L, PHASE_DANCER_DANCING_IN); lua_setfield(L, -2, "DANCER_DANCING_IN");
		lua_pushinteger(L, PHASE_DANCER_SNAPPING_FINGERS); lua_setfield(L, -2, "DANCER_SNAPPING_FINGERS");
		lua_pushinteger(L, PHASE_DANCER_SNAPPING_FINGERS_WITH_LIGHT); lua_setfield(L, -2, "DANCER_SNAPPING_FINGERS_WITH_LIGHT");
		lua_pushinteger(L, PHASE_DANCER_SNAPPING_FINGERS_HOLD); lua_setfield(L, -2, "DANCER_SNAPPING_FINGERS_HOLD");
		lua_pushinteger(L, PHASE_DANCER_DANCING_LEFT); lua_setfield(L, -2, "DANCER_DANCING_LEFT");
		lua_pushinteger(L, PHASE_DANCER_WALK_TO_RAISE); lua_setfield(L, -2, "DANCER_WALK_TO_RAISE");
		lua_pushinteger(L, PHASE_DANCER_RAISE_LEFT_1); lua_setfield(L, -2, "DANCER_RAISE_LEFT_1");
		lua_pushinteger(L, PHASE_DANCER_RAISE_RIGHT_1); lua_setfield(L, -2, "DANCER_RAISE_RIGHT_1");
		lua_pushinteger(L, PHASE_DANCER_RAISE_LEFT_2); lua_setfield(L, -2, "DANCER_RAISE_LEFT_2");
		lua_pushinteger(L, PHASE_DANCER_RAISE_RIGHT_2); lua_setfield(L, -2, "DANCER_RAISE_RIGHT_2");
		lua_pushinteger(L, PHASE_DANCER_RISING); lua_setfield(L, -2, "DANCER_RISING");
		lua_pushinteger(L, PHASE_DOLPHIN_WALKING); lua_setfield(L, -2, "DOLPHIN_WALKING");
		lua_pushinteger(L, PHASE_DOLPHIN_INTO_POOL); lua_setfield(L, -2, "DOLPHIN_INTO_POOL");
		lua_pushinteger(L, PHASE_DOLPHIN_RIDING); lua_setfield(L, -2, "DOLPHIN_RIDING");
		lua_pushinteger(L, PHASE_DOLPHIN_IN_JUMP); lua_setfield(L, -2, "DOLPHIN_IN_JUMP");
		lua_pushinteger(L, PHASE_DOLPHIN_WALKING_IN_POOL); lua_setfield(L, -2, "DOLPHIN_WALKING_IN_POOL");
		lua_pushinteger(L, PHASE_DOLPHIN_WALKING_WITHOUT_DOLPHIN); lua_setfield(L, -2, "DOLPHIN_WALKING_WITHOUT_DOLPHIN");
		lua_pushinteger(L, PHASE_SNORKEL_WALKING); lua_setfield(L, -2, "SNORKEL_WALKING");
		lua_pushinteger(L, PHASE_SNORKEL_INTO_POOL); lua_setfield(L, -2, "SNORKEL_INTO_POOL");
		lua_pushinteger(L, PHASE_SNORKEL_WALKING_IN_POOL); lua_setfield(L, -2, "SNORKEL_WALKING_IN_POOL");
		lua_pushinteger(L, PHASE_SNORKEL_UP_TO_EAT); lua_setfield(L, -2, "SNORKEL_UP_TO_EAT");
		lua_pushinteger(L, PHASE_SNORKEL_EATING_IN_POOL); lua_setfield(L, -2, "SNORKEL_EATING_IN_POOL");
		lua_pushinteger(L, PHASE_SNORKEL_DOWN_FROM_EAT); lua_setfield(L, -2, "SNORKEL_DOWN_FROM_EAT");
		lua_pushinteger(L, PHASE_ZOMBIQUARIUM_ACCEL); lua_setfield(L, -2, "ZOMBIQUARIUM_ACCEL");
		lua_pushinteger(L, PHASE_ZOMBIQUARIUM_DRIFT); lua_setfield(L, -2, "ZOMBIQUARIUM_DRIFT");
		lua_pushinteger(L, PHASE_ZOMBIQUARIUM_BACK_AND_FORTH); lua_setfield(L, -2, "ZOMBIQUARIUM_BACK_AND_FORTH");
		lua_pushinteger(L, PHASE_ZOMBIQUARIUM_BITE); lua_setfield(L, -2, "ZOMBIQUARIUM_BITE");
		lua_pushinteger(L, PHASE_CATAPULT_LAUNCHING); lua_setfield(L, -2, "CATAPULT_LAUNCHING");
		lua_pushinteger(L, PHASE_CATAPULT_RELOADING); lua_setfield(L, -2, "CATAPULT_RELOADING");
		lua_pushinteger(L, PHASE_GARGANTUAR_THROWING); lua_setfield(L, -2, "GARGANTUAR_THROWING");
		lua_pushinteger(L, PHASE_GARGANTUAR_SMASHING); lua_setfield(L, -2, "GARGANTUAR_SMASHING");
		lua_pushinteger(L, PHASE_IMP_GETTING_THROWN); lua_setfield(L, -2, "IMP_GETTING_THROWN");
		lua_pushinteger(L, PHASE_IMP_LANDING); lua_setfield(L, -2, "IMP_LANDING");
		lua_pushinteger(L, PHASE_BALLOON_FLYING); lua_setfield(L, -2, "BALLOON_FLYING");
		lua_pushinteger(L, PHASE_BALLOON_POPPING); lua_setfield(L, -2, "BALLOON_POPPING");
		lua_pushinteger(L, PHASE_BALLOON_WALKING); lua_setfield(L, -2, "BALLOON_WALKING");
		lua_pushinteger(L, PHASE_LADDER_CARRYING); lua_setfield(L, -2, "LADDER_CARRYING");
		lua_pushinteger(L, PHASE_LADDER_PLACING); lua_setfield(L, -2, "LADDER_PLACING");
		lua_pushinteger(L, PHASE_BOSS_ENTER); lua_setfield(L, -2, "BOSS_ENTER");
		lua_pushinteger(L, PHASE_BOSS_IDLE); lua_setfield(L, -2, "BOSS_IDLE");
		lua_pushinteger(L, PHASE_BOSS_SPAWNING); lua_setfield(L, -2, "BOSS_SPAWNING");
		lua_pushinteger(L, PHASE_BOSS_STOMPING); lua_setfield(L, -2, "BOSS_STOMPING");
		lua_pushinteger(L, PHASE_BOSS_BUNGEES_ENTER); lua_setfield(L, -2, "BOSS_BUNGEES_ENTER");
		lua_pushinteger(L, PHASE_BOSS_BUNGEES_DROP); lua_setfield(L, -2, "BOSS_BUNGEES_DROP");
		lua_pushinteger(L, PHASE_BOSS_BUNGEES_LEAVE); lua_setfield(L, -2, "BOSS_BUNGEES_LEAVE");
		lua_pushinteger(L, PHASE_BOSS_DROP_RV); lua_setfield(L, -2, "BOSS_DROP_RV");
		lua_pushinteger(L, PHASE_BOSS_HEAD_ENTER); lua_setfield(L, -2, "BOSS_HEAD_ENTER");
		lua_pushinteger(L, PHASE_BOSS_HEAD_IDLE_BEFORE_SPIT); lua_setfield(L, -2, "BOSS_HEAD_IDLE_BEFORE_SPIT");
		lua_pushinteger(L, PHASE_BOSS_HEAD_IDLE_AFTER_SPIT); lua_setfield(L, -2, "BOSS_HEAD_IDLE_AFTER_SPIT");
		lua_pushinteger(L, PHASE_BOSS_HEAD_SPIT); lua_setfield(L, -2, "BOSS_HEAD_SPIT");
		lua_pushinteger(L, PHASE_BOSS_HEAD_LEAVE); lua_setfield(L, -2, "BOSS_HEAD_LEAVE");
		lua_pushinteger(L, PHASE_YETI_RUNNING); lua_setfield(L, -2, "YETI_RUNNING");
		lua_pushinteger(L, PHASE_SQUASH_PRE_LAUNCH); lua_setfield(L, -2, "SQUASH_PRE_LAUNCH");
		lua_pushinteger(L, PHASE_SQUASH_RISING); lua_setfield(L, -2, "SQUASH_RISING");
		lua_pushinteger(L, PHASE_SQUASH_FALLING); lua_setfield(L, -2, "SQUASH_FALLING");
		lua_pushinteger(L, PHASE_SQUASH_DONE_FALLING); lua_setfield(L, -2, "SQUASH_DONE_FALLING");
		lua_setglobal(L, "ZombiePhase");

		// ProjectileMotion
		lua_newtable(L);
		lua_pushinteger(L, MOTION_STRAIGHT); lua_setfield(L, -2, "STRAIGHT");
		lua_pushinteger(L, MOTION_LOBBED); lua_setfield(L, -2, "LOBBED");
		lua_pushinteger(L, MOTION_THREEPEATER); lua_setfield(L, -2, "THREEPEATER");
		lua_pushinteger(L, MOTION_BEE); lua_setfield(L, -2, "BEE");
		lua_pushinteger(L, MOTION_BEE_BACKWARDS); lua_setfield(L, -2, "BEE_BACKWARDS");
		lua_pushinteger(L, MOTION_PUFF); lua_setfield(L, -2, "PUFF");
		lua_pushinteger(L, MOTION_BACKWARDS); lua_setfield(L, -2, "BACKWARDS");
		lua_pushinteger(L, MOTION_STAR); lua_setfield(L, -2, "STAR");
		lua_pushinteger(L, MOTION_FLOAT_OVER); lua_setfield(L, -2, "FLOAT_OVER");
		lua_pushinteger(L, MOTION_HOMING); lua_setfield(L, -2, "HOMING");
		lua_setglobal(L, "ProjectileMotion");

		// DamageFlags
		lua_newtable(L);
		lua_pushinteger(L, DAMAGE_BYPASSES_SHIELD); lua_setfield(L, -2, "BYPASSES_SHIELD");
		lua_pushinteger(L, DAMAGE_HITS_SHIELD_AND_BODY); lua_setfield(L, -2, "HITS_SHIELD_AND_BODY");
		lua_pushinteger(L, DAMAGE_FREEZE); lua_setfield(L, -2, "FREEZE");
		lua_pushinteger(L, DAMAGE_DOESNT_CAUSE_FLASH); lua_setfield(L, -2, "DOESNT_CAUSE_FLASH");
		lua_pushinteger(L, DAMAGE_DOESNT_LEAVE_BODY); lua_setfield(L, -2, "DOESNT_LEAVE_BODY");
		lua_pushinteger(L, DAMAGE_SPIKE); lua_setfield(L, -2, "SPIKE");
		lua_setglobal(L, "DamageFlags");

		// BackgroundType
		lua_newtable(L);
		lua_pushinteger(L, BACKGROUND_1_DAY); lua_setfield(L, -2, "DAY");
		lua_pushinteger(L, BACKGROUND_2_NIGHT); lua_setfield(L, -2, "NIGHT");
		lua_pushinteger(L, BACKGROUND_3_POOL); lua_setfield(L, -2, "POOL");
		lua_pushinteger(L, BACKGROUND_4_FOG); lua_setfield(L, -2, "FOG");
		lua_pushinteger(L, BACKGROUND_5_ROOF); lua_setfield(L, -2, "ROOF");
		lua_pushinteger(L, BACKGROUND_6_BOSS); lua_setfield(L, -2, "BOSS");
		lua_pushinteger(L, BACKGROUND_MUSHROOM_GARDEN); lua_setfield(L, -2, "MUSHROOM_GARDEN");
		lua_pushinteger(L, BACKGROUND_GREENHOUSE); lua_setfield(L, -2, "GREENHOUSE");
		lua_pushinteger(L, BACKGROUND_ZOMBIQUARIUM); lua_setfield(L, -2, "ZOMBIQUARIUM");
		lua_pushinteger(L, BACKGROUND_TREEOFWISDOM); lua_setfield(L, -2, "TREEOFWISDOM");
		lua_setglobal(L, "BackgroundType");

		// HelmType
		lua_newtable(L);
		lua_pushinteger(L, HELMTYPE_NONE); lua_setfield(L, -2, "NONE");
		lua_pushinteger(L, HELMTYPE_TRAFFIC_CONE); lua_setfield(L, -2, "TRAFFIC_CONE");
		lua_pushinteger(L, HELMTYPE_PAIL); lua_setfield(L, -2, "PAIL");
		lua_pushinteger(L, HELMTYPE_FOOTBALL); lua_setfield(L, -2, "FOOTBALL");
		lua_pushinteger(L, HELMTYPE_DIGGER); lua_setfield(L, -2, "DIGGER");
		lua_pushinteger(L, HELMTYPE_REDEYES); lua_setfield(L, -2, "REDEYES");
		lua_pushinteger(L, HELMTYPE_HEADBAND); lua_setfield(L, -2, "HEADBAND");
		lua_pushinteger(L, HELMTYPE_BOBSLED); lua_setfield(L, -2, "BOBSLED");
		lua_pushinteger(L, HELMTYPE_WALLNUT); lua_setfield(L, -2, "WALLNUT");
		lua_pushinteger(L, HELMTYPE_TALLNUT); lua_setfield(L, -2, "TALLNUT");
		lua_setglobal(L, "HelmType");

		// ShieldType
		lua_newtable(L);
		lua_pushinteger(L, SHIELDTYPE_NONE); lua_setfield(L, -2, "NONE");
		lua_pushinteger(L, SHIELDTYPE_DOOR); lua_setfield(L, -2, "DOOR");
		lua_pushinteger(L, SHIELDTYPE_NEWSPAPER); lua_setfield(L, -2, "NEWSPAPER");
		lua_pushinteger(L, SHIELDTYPE_LADDER); lua_setfield(L, -2, "LADDER");
		lua_setglobal(L, "ShieldType");

		// ReanimLoopType
		lua_newtable(L);
		lua_pushinteger(L, REANIM_LOOP); lua_setfield(L, -2, "LOOP");
		lua_pushinteger(L, REANIM_LOOP_FULL_LAST_FRAME); lua_setfield(L, -2, "LOOP_FULL_LAST_FRAME");
		lua_pushinteger(L, REANIM_PLAY_ONCE); lua_setfield(L, -2, "PLAY_ONCE");
		lua_pushinteger(L, REANIM_PLAY_ONCE_AND_HOLD); lua_setfield(L, -2, "PLAY_ONCE_AND_HOLD");
		lua_pushinteger(L, REANIM_PLAY_ONCE_FULL_LAST_FRAME); lua_setfield(L, -2, "PLAY_ONCE_FULL_LAST_FRAME");
		lua_pushinteger(L, REANIM_PLAY_ONCE_FULL_LAST_FRAME_AND_HOLD); lua_setfield(L, -2, "PLAY_ONCE_FULL_LAST_FRAME_AND_HOLD");
		lua_setglobal(L, "ReanimLoopType");

		// GridConstants
		lua_newtable(L);
		lua_pushinteger(L, MAX_GRID_SIZE_X); lua_setfield(L, -2, "COLS");
		lua_pushinteger(L, MAX_GRID_SIZE_Y); lua_setfield(L, -2, "ROWS");
		lua_pushinteger(L, BOARD_WIDTH); lua_setfield(L, -2, "BOARD_WIDTH");
		lua_pushinteger(L, BOARD_HEIGHT); lua_setfield(L, -2, "BOARD_HEIGHT");
		lua_pushinteger(L, LAWN_XMIN); lua_setfield(L, -2, "LAWN_XMIN");
		lua_pushinteger(L, LAWN_YMIN); lua_setfield(L, -2, "LAWN_YMIN");
		lua_pushinteger(L, BOARD_OFFSET); lua_setfield(L, -2, "BOARD_OFFSET");
		lua_pushinteger(L, SEEDBANK_MAX); lua_setfield(L, -2, "SEEDBANK_MAX");
		lua_setglobal(L, "GridConstants");
	}

	void Lua_RegisterGameTable(lua_State* L)
	{
		Lua_RegisterConstants(L);
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
		lua_pushcfunction(L, Lua_GameGetMode);
		lua_setfield(L, -2, "GetMode");
		lua_pushcfunction(L, Lua_GameGetMenuButtonRect);
		lua_setfield(L, -2, "GetMenuButtonRect");
		lua_pushcfunction(L, Lua_GameRegisterProjectile);
		lua_setfield(L, -2, "RegisterProjectile");
		lua_pushcfunction(L, Lua_GameRegisterPlant);
		lua_setfield(L, -2, "RegisterPlant");
		lua_pushcfunction(L, Lua_GameRegisterZombie);
		lua_setfield(L, -2, "RegisterZombie");
		lua_pushcfunction(L, Lua_GameRegisterMode);
		lua_setfield(L, -2, "RegisterMode");
		lua_setglobal(L, "Game");

		// Board
		lua_newtable(L);
		lua_pushcfunction(L, Lua_BoardSpawnZombie);
		lua_setfield(L, -2, "SpawnZombie");
		lua_pushcfunction(L, Lua_BoardSpawnPlant);
		lua_setfield(L, -2, "SpawnPlant");
		lua_pushcfunction(L, Lua_BoardGetWave);
		lua_setfield(L, -2, "GetWave");
		lua_pushcfunction(L, Lua_BoardFindTargetZombie);
		lua_setfield(L, -2, "FindTargetZombie");
		lua_pushcfunction(L, Lua_BoardGetZombiesInRow);
		lua_setfield(L, -2, "GetZombiesInRow");
		lua_pushcfunction(L, Lua_BoardGetPlantsInRow);
		lua_setfield(L, -2, "GetPlantsInRow");
		lua_pushcfunction(L, Lua_BoardGetZombieAt);
		lua_setfield(L, -2, "GetZombieAt");
		lua_pushcfunction(L, Lua_BoardGetPlantAt);
		lua_setfield(L, -2, "GetPlantAt");
		lua_pushcfunction(L, Lua_BoardGetAllZombies);
		lua_setfield(L, -2, "GetAllZombies");
		lua_pushcfunction(L, Lua_BoardGetAllPlants);
		lua_setfield(L, -2, "GetAllPlants");
		lua_pushcfunction(L, Lua_BoardAddProjectile);
		lua_setfield(L, -2, "AddProjectile");
		lua_pushcfunction(L, Lua_BoardAddButton);
		lua_setfield(L, -2, "AddButton");
		lua_pushcfunction(L, Lua_BoardRemoveButton);
		lua_setfield(L, -2, "RemoveButton");
		lua_pushcfunction(L, Lua_BoardSetButtonVisible);
		lua_setfield(L, -2, "SetButtonVisible");
		lua_pushcfunction(L, Lua_BoardSetButtonLabel);
		lua_setfield(L, -2, "SetButtonLabel");
		lua_setglobal(L, "Board");

		// Reanim metatable
		luaL_newmetatable(L, "Game.Reanim");
		lua_pushcfunction(L, Lua_ReanimIndex);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, Lua_ReanimPlay);
		lua_setfield(L, -2, "Play");
		lua_pushcfunction(L, Lua_ReanimGetRate);
		lua_setfield(L, -2, "GetRate");
		lua_pushcfunction(L, Lua_ReanimSetRate);
		lua_setfield(L, -2, "SetRate");
		lua_pushcfunction(L, Lua_ReanimGetProgress);
		lua_setfield(L, -2, "GetProgress");
		lua_pushcfunction(L, Lua_ReanimSetProgress);
		lua_setfield(L, -2, "SetProgress");
		lua_pushcfunction(L, Lua_ReanimIsPlaying);
		lua_setfield(L, -2, "IsPlaying");
		lua_pushcfunction(L, Lua_ReanimTrackExists);
		lua_setfield(L, -2, "TrackExists");
		lua_pushcfunction(L, Lua_ReanimGetLoopType);
		lua_setfield(L, -2, "GetLoopType");
		lua_pushcfunction(L, Lua_ReanimGetLoopCount);
		lua_setfield(L, -2, "GetLoopCount");
		lua_pushcfunction(L, Lua_ReanimSetPosition);
		lua_setfield(L, -2, "SetPosition");
		lua_pushcfunction(L, Lua_ReanimOverrideScale);
		lua_setfield(L, -2, "OverrideScale");
		lua_pushcfunction(L, Lua_ReanimShowOnlyTrack);
		lua_setfield(L, -2, "ShowOnlyTrack");
		lua_pop(L, 1);

		// Entity metatable
		luaL_newmetatable(L, "Game.Entity");
		lua_pushcfunction(L, Lua_EntityIndex);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, Lua_EntityDamage);
		lua_setfield(L, -2, "Damage");
		lua_pushcfunction(L, Lua_EntityIsSun);
		lua_setfield(L, -2, "IsSun");
		lua_pushcfunction(L, Lua_EntityCollect);
		lua_setfield(L, -2, "Collect");
		lua_pushcfunction(L, Lua_EntityGetBodyReanim);
		lua_setfield(L, -2, "GetBodyReanim");
		lua_pushcfunction(L, Lua_EntityPlayBodyReanim);
		lua_setfield(L, -2, "PlayBodyReanim");
		lua_pushcfunction(L, Lua_EntityPlayIdleAnim);
		lua_setfield(L, -2, "PlayIdleAnim");
		lua_pushcfunction(L, Lua_EntityGetBodyReanimProgress);
		lua_setfield(L, -2, "GetBodyReanimProgress");
		lua_pushcfunction(L, Lua_EntitySetBodyReanimRate);
		lua_setfield(L, -2, "SetBodyReanimRate");
		lua_pushcfunction(L, Lua_EntityGetBodyReanimRate);
		lua_setfield(L, -2, "GetBodyReanimRate");
		lua_pushcfunction(L, Lua_EntityIsAnimPlaying);
		lua_setfield(L, -2, "IsAnimPlaying");
		lua_pushcfunction(L, Lua_EntityTrackExists);
		lua_setfield(L, -2, "TrackExists");
		lua_pushcfunction(L, Lua_EntityGetBodyReanimLoopType);
		lua_setfield(L, -2, "GetBodyReanimLoopType");
		lua_pushcfunction(L, Lua_EntityGetBodyReanimLoopCount);
		lua_setfield(L, -2, "GetBodyReanimLoopCount");
		lua_pushcfunction(L, Lua_EntityDistanceTo);
		lua_setfield(L, -2, "DistanceTo");
		lua_pushcfunction(L, Lua_EntitySetDamage);
		lua_setfield(L, -2, "SetDamage");
		lua_pushcfunction(L, Lua_EntitySetVelocity);
		lua_setfield(L, -2, "SetVelocity");
		lua_pushcfunction(L, Lua_EntitySetDamageFlags);
		lua_setfield(L, -2, "SetDamageFlags");
		lua_pushcfunction(L, Lua_EntitySetMotionType);
		lua_setfield(L, -2, "SetMotionType");
		lua_pushcfunction(L, Lua_EntitySetTargetZombie);
		lua_setfield(L, -2, "SetTargetZombie");
		lua_pushcfunction(L, Lua_EntityDie);
		lua_setfield(L, -2, "Die");
		lua_pushcfunction(L, Lua_EntitySquish);
		lua_setfield(L, -2, "Squish");
		lua_pushcfunction(L, Lua_EntitySetSleeping);
		lua_setfield(L, -2, "SetSleeping");
		lua_pushcfunction(L, Lua_EntityGetCost);
		lua_setfield(L, -2, "GetCost");
		lua_pushcfunction(L, Lua_EntityGetName);
		lua_setfield(L, -2, "GetName");
		lua_pushcfunction(L, Lua_EntityIsNocturnal);
		lua_setfield(L, -2, "IsNocturnal");
		lua_pushcfunction(L, Lua_EntityIsFungus);
		lua_setfield(L, -2, "IsFungus");
		lua_pushcfunction(L, Lua_EntityIsAquatic);
		lua_setfield(L, -2, "IsAquatic");
		lua_pushcfunction(L, Lua_EntityIsUpgrade);
		lua_setfield(L, -2, "IsUpgrade");
		lua_pushcfunction(L, Lua_EntityIsFlying);
		lua_setfield(L, -2, "IsFlying");
		lua_pushcfunction(L, Lua_EntitySetRow);
		lua_setfield(L, -2, "SetRow");
		lua_pushcfunction(L, Lua_EntityApplyChill);
		lua_setfield(L, -2, "ApplyChill");
		lua_pushcfunction(L, Lua_EntityApplyButter);
		lua_setfield(L, -2, "ApplyButter");
		lua_pushcfunction(L, Lua_EntityRemoveButter);
		lua_setfield(L, -2, "RemoveButter");
		lua_pushcfunction(L, Lua_EntityStartMindControlled);
		lua_setfield(L, -2, "StartMindControlled");
		lua_pushcfunction(L, Lua_EntityDieNoLoot);
		lua_setfield(L, -2, "DieNoLoot");
		lua_pushcfunction(L, Lua_EntityDieWithLoot);
		lua_setfield(L, -2, "DieWithLoot");
		lua_pushcfunction(L, Lua_EntityTakeHelmDamage);
		lua_setfield(L, -2, "TakeHelmDamage");
		lua_pushcfunction(L, Lua_EntityTakeShieldDamage);
		lua_setfield(L, -2, "TakeShieldDamage");
		lua_pushcfunction(L, Lua_EntityIsOnHighGround);
		lua_setfield(L, -2, "IsOnHighGround");
		lua_pushcfunction(L, Lua_EntityIsImmobilized);
		lua_setfield(L, -2, "IsImmobilized");
		lua_pushcfunction(L, Lua_EntityCoinGetValue);
		lua_setfield(L, -2, "GetValue");
		lua_pushcfunction(L, Lua_EntityStartFade);
		lua_setfield(L, -2, "StartFade");
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
		lua_pushcfunction(L, Lua_DialogAddLabel);
		lua_setfield(L, -2, "AddLabel");
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
	lua_pushboolean(L, isWin ? 1 : 0);
	Lua_CallGlobal(L, "OnLevelEnd", 1);
#else
	(void)isWin;
#endif
}

void ModLua::CallOnCoinSpawn(Coin* coin)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	PushEntity(L, coin);
	Lua_CallGlobal(L, "OnCoinSpawn", 1);
#else
	(void)coin;
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

void ModLua::CallOnGameStart()
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	Lua_CallGlobal(L, "OnGameStart", 0);
#endif
}

void ModLua::CallOnWaveStart(int waveIndex)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	lua_pushinteger(L, waveIndex);
	Lua_CallGlobal(L, "OnWaveStart", 1);
#else
	(void)waveIndex;
#endif
}

void ModLua::CallOnPlantUpdate(Plant* plant)
{
#if defined(PVZ_ENABLE_LUA)
	lua_State* L = static_cast<lua_State*>(mState);
	if (L == nullptr)
		return;
	PushEntity(L, plant);
	Lua_CallGlobal(L, "OnPlantUpdate", 1);
#else
	(void)plant;
#endif
}

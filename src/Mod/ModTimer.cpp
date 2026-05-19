/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModTimer.h"
#include "Sexy.TodLib/TodDebug.h"

#if defined(PVZ_ENABLE_LUA)
#include <lua.hpp>
#endif

ModTimer gModTimer;

int ModTimer::New(void* parentPtr, int frames, int callbackRef, bool isRepeat, int interval)
{
	mNextId++;
	int id = mNextId;
	mTimers[id] = { id, parentPtr, frames, interval, isRepeat, callbackRef };
	return id;
}

bool ModTimer::Cancel(int id, lua_State* L)
{
	auto it = mTimers.find(id);
	if (it == mTimers.end())
		return false;

#if defined(PVZ_ENABLE_LUA)
	luaL_unref(L, LUA_REGISTRYINDEX, it->second.callbackRef);
#endif
	mTimers.erase(it);
	return true;
}

void ModTimer::CancelAll(void* parentPtr, lua_State* L)
{
#if defined(PVZ_ENABLE_LUA)
	for (auto it = mTimers.begin(); it != mTimers.end(); )
	{
		if (it->second.parentPtr == parentPtr)
		{
			luaL_unref(L, LUA_REGISTRYINDEX, it->second.callbackRef);
			it = mTimers.erase(it);
		}
		else
		{
			++it;
		}
	}
#else
	(void)parentPtr;
	(void)L;
#endif
}

void ModTimer::TickAll(void* parentPtr, lua_State* L)
{
#if defined(PVZ_ENABLE_LUA)
	// First pass: collect IDs of timers that expired this frame
	std::vector<int> expired;
	for (auto& [id, t] : mTimers)
	{
		if (t.parentPtr == parentPtr)
		{
			t.remaining--;
			if (t.remaining <= 0)
				expired.push_back(id);
		}
	}

	// Second pass: fire callbacks and cleanup
	for (int id : expired)
	{
		auto it = mTimers.find(id);
		if (it == mTimers.end())
			continue;

		lua_rawgeti(L, LUA_REGISTRYINDEX, it->second.callbackRef);
		if (lua_pcall(L, 0, 0, 0) != 0)
		{
			TodLog("_timer callback error: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
		}

		it = mTimers.find(id);
		if (it == mTimers.end())
			continue;

		if (it->second.isRepeat)
		{
			it->second.remaining = it->second.interval;
		}
		else
		{
			luaL_unref(L, LUA_REGISTRYINDEX, it->second.callbackRef);
			mTimers.erase(it);
		}
	}
#else
	(void)parentPtr;
	(void)L;
#endif
}

void ModTimer::Shutdown(lua_State* L)
{
#if defined(PVZ_ENABLE_LUA)
	for (auto& [id, t] : mTimers)
		luaL_unref(L, LUA_REGISTRYINDEX, t.callbackRef);
	mTimers.clear();
#else
	(void)L;
#endif
}

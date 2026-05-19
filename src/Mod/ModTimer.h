/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __MODTIMER_H__
#define __MODTIMER_H__

#include <unordered_map>
#include <vector>

struct lua_State;

class ModTimer
{
public:
	struct TimerEntry
	{
		int id;
		void* parentPtr;
		int remaining;
		int interval;
		bool isRepeat;
		int callbackRef;
	};

	int New(void* parentPtr, int frames, int callbackRef, bool isRepeat, int interval);
	bool Cancel(int id, lua_State* L);
	void CancelAll(void* parentPtr, lua_State* L);
	void TickAll(void* parentPtr, lua_State* L);
	void Shutdown(lua_State* L);

private:
	int mNextId = 0;
	std::unordered_map<int, TimerEntry> mTimers;
};

extern ModTimer gModTimer;

#endif

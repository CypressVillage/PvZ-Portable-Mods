/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __MODLUA_H__
#define __MODLUA_H__

#include "ModLoader.h"

class ModLua
{
public:
	bool Init();
	void Shutdown();
	void RunEntries(const std::vector<ModManifest>& manifests);
	void CallOnLevelStart(int gameMode);
	void CallOnZombieSpawn(int zombieType, int row);
	void CallOnZombieDie(int zombieType);

private:
	void* mState = nullptr;
};

extern ModLua gModLua;

#endif

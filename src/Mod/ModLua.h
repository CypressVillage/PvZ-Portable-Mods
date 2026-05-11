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
	void CallOnZombieSpawn(class Zombie* zombie);
	void CallOnZombieDie(class Zombie* zombie);
	void CallOnPlantSpawn(class Plant* plant);
	void CallOnPlantAttack(class Plant* plant, class Zombie* target);
	void CallOnLevelEnd(bool isWin);
	void CallOnBoardButtonClick(int buttonId);
	void CallOnCoinSpawn(class Coin* coin);

private:
	void* mState = nullptr;
};

extern ModLua gModLua;

#endif

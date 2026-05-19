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
	void CallOnGameStart();
	void CallOnWaveStart(int waveIndex);
	void CallOnPlantUpdate(class Plant* plant);
	void CallOnPlantDie(class Plant* plant);
	void CallOnZombieAttack(class Zombie* zombie, class Plant* plant);
	void CallOnProjectileSpawn(class Projectile* proj);
	void CallOnProjectileHit(class Projectile* proj, class Zombie* zombie);
	void CallOnProjectileMiss(class Projectile* proj);
	void CallOnCoinCollect(class Coin* coin);
	void CallOnZombieReachHouse(class Zombie* zombie);
	void CallOnPlantEaten(class Plant* plant, class Zombie* zombie);
	void CallOnPlantProduce(class Plant* plant);
	void CallOnPlantUpgrade(class Plant* plant, int oldType);
	void CallOnZombieFrozen(class Zombie* zombie, bool isFrozen);
	void CallOnZombieButtered(class Zombie* zombie);
	void CallOnZombieMindControl(class Zombie* zombie);
	void CallOnCoinExpire(class Coin* coin);
	void CallOnFlagRaise(int waveIndex);
	void CallOnMowerTriggered(int row, int mowerType);
	void CallOnSunCountChange(int oldAmount, int newAmount);

private:
	void* mState = nullptr;
};

extern ModLua gModLua;

#endif

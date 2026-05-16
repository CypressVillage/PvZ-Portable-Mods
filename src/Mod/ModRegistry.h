/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __MODREGISTRY_H__
#define __MODREGISTRY_H__

#include <map>
#include <string>
#include <vector>

struct ModPlantDef
{
	std::string id;
	int seedType = -1;
	int seedCost = 50;
	int refreshTime = 750;
	int packetIndex = 0;
	int subClass = 0;
	int launchRate = 0;
	int projectileType = 0;
	std::string plantName;
	std::string reanimationName;
};

struct ModZombieDef
{
	std::string id;
	int zombieType = -1;
};

struct ModModeDef
{
	std::string id;
	int baseMode = -1;
};

struct ModProjectileDef
{
	std::string id;
	int damage = 0;
	float speed = 3.0f;
	std::string imageName;
	int projectileType = -1;
};

class ModRegistry
{
public:
	void Reset();

	bool RegisterPlant(const ModPlantDef& def, std::string* outError);
	bool RegisterZombie(const ModZombieDef& def, std::string* outError);
	bool RegisterMode(const ModModeDef& def, std::string* outError);
	bool RegisterProjectile(const ModProjectileDef& def, std::string* outError);

	const ModPlantDef* FindPlant(const std::string& id) const;
	const ModZombieDef* FindZombie(const std::string& id) const;
	const ModModeDef* FindMode(const std::string& id) const;
	const ModProjectileDef* FindProjectile(const std::string& id) const;

	const ModPlantDef* FindPlantByRuntimeId(int seedType) const;
	const ModZombieDef* FindZombieByRuntimeId(int zombieType) const;
	const ModModeDef* FindModeByRuntimeId(int baseMode) const;
	const ModProjectileDef* FindProjectileByRuntimeId(int projectileType) const;

	const std::vector<std::string>& GetErrors() const;

	int GetTotalAlmanacPlants() const;
	int GetAlmanacPlantAt(int index) const;
	int ResolveReanimationType(const std::string& reanimName) const;

	void BuildReanimNameMap();
	unsigned int RegisterDynamicReanim(const std::string& reanimFilePath);

private:
	bool ValidateId(const std::string& id, std::string* outError) const;

	std::map<std::string, ModPlantDef> mPlants;
	std::map<std::string, ModZombieDef> mZombies;
	std::map<std::string, ModModeDef> mModes;
	std::map<std::string, ModProjectileDef> mProjectiles;

	std::map<int, ModPlantDef> mRuntimePlants;
	std::map<int, ModZombieDef> mRuntimeZombies;
	std::map<int, ModModeDef> mRuntimeModes;
	std::map<int, ModProjectileDef> mRuntimeProjectiles;
	int mNextProjectileType = 4000;
	std::map<std::string, int> mReanimNameMap;
	std::vector<std::string> mErrors;
};

extern ModRegistry gModRegistry;

#endif

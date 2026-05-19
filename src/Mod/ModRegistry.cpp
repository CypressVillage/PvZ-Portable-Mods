/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModRegistry.h"
#include "../SexyAppFramework/SexyAppBase.h"
#include "../Sexy.TodLib/Reanimator.h"

static bool HasFileExtension(const std::string& path)
{
	auto dot = path.rfind('.');
	if (dot == std::string::npos) return false;
	auto slash = path.rfind('/');
	auto bslash = path.rfind('\\');
	size_t lastSep = (slash != std::string::npos && bslash != std::string::npos) ? std::max(slash, bslash) : (slash != std::string::npos ? slash : bslash);
	return lastSep == std::string::npos || dot > lastSep;
}

ModRegistry gModRegistry;

void ModRegistry::Reset()
{
	mPlants.clear();
	mZombies.clear();
	mModes.clear();
	mProjectiles.clear();
	mRuntimePlants.clear();
	mRuntimeZombies.clear();
	mRuntimeModes.clear();
	mRuntimeProjectiles.clear();
	mNextSeedType = 2000;
	mNextZombieType = 3000;
	mNextModeType = 5000;
	mNextProjectileType = 4000;
	mErrors.clear();
}

bool ModRegistry::RegisterPlant(const ModPlantDef& def, std::string* outError)
{
	if (!ValidateId(def.id, outError))
		return false;
	if (mPlants.find(def.id) != mPlants.end())
	{
		if (outError)
			*outError = "Plant id already registered: " + def.id;
		mErrors.push_back("Plant id already registered: " + def.id);
		return false;
	}
	ModPlantDef storedDef = def;
	if (storedDef.seedType < 0)
		storedDef.seedType = mNextSeedType++;
	mPlants[def.id] = storedDef;
	mRuntimePlants[storedDef.seedType] = storedDef;
	return true;
}

bool ModRegistry::RegisterZombie(const ModZombieDef& def, std::string* outError)
{
	if (!ValidateId(def.id, outError))
		return false;
	if (mZombies.find(def.id) != mZombies.end())
	{
		if (outError)
			*outError = "Zombie id already registered: " + def.id;
		mErrors.push_back("Zombie id already registered: " + def.id);
		return false;
	}
	ModZombieDef storedDef = def;
	if (storedDef.zombieType < 0)
		storedDef.zombieType = mNextZombieType++;
	mZombies[def.id] = storedDef;
	mRuntimeZombies[storedDef.zombieType] = storedDef;
	return true;
}

bool ModRegistry::RegisterMode(const ModModeDef& def, std::string* outError)
{
	if (!ValidateId(def.id, outError))
		return false;
	if (mModes.find(def.id) != mModes.end())
	{
		if (outError)
			*outError = "Mode id already registered: " + def.id;
		mErrors.push_back("Mode id already registered: " + def.id);
		return false;
	}
	ModModeDef storedDef = def;
	if (storedDef.baseMode < 0)
		storedDef.baseMode = mNextModeType++;
	mModes[def.id] = storedDef;
	mRuntimeModes[storedDef.baseMode] = storedDef;
	return true;
}

bool ModRegistry::RegisterProjectile(const ModProjectileDef& def, std::string* outError)
{
	if (!ValidateId(def.id, outError))
		return false;
	if (mProjectiles.find(def.id) != mProjectiles.end())
	{
		if (outError)
			*outError = "Projectile id already registered: " + def.id;
		mErrors.push_back("Projectile id already registered: " + def.id);
		return false;
	}

	ModProjectileDef storedDef = def;
	if (storedDef.projectileType < 0)
	{
		storedDef.projectileType = mNextProjectileType++;
	}

	mProjectiles[def.id] = storedDef;
	mRuntimeProjectiles[storedDef.projectileType] = storedDef;
	return true;
}

const ModPlantDef* ModRegistry::FindPlant(const std::string& id) const
{
	auto it = mPlants.find(id);
	return it == mPlants.end() ? nullptr : &it->second;
}

const ModZombieDef* ModRegistry::FindZombie(const std::string& id) const
{
	auto it = mZombies.find(id);
	return it == mZombies.end() ? nullptr : &it->second;
}

const ModModeDef* ModRegistry::FindMode(const std::string& id) const
{
	auto it = mModes.find(id);
	return it == mModes.end() ? nullptr : &it->second;
}

const ModProjectileDef* ModRegistry::FindProjectile(const std::string& id) const
{
	auto it = mProjectiles.find(id);
	return it == mProjectiles.end() ? nullptr : &it->second;
}

const ModPlantDef* ModRegistry::FindPlantByRuntimeId(int seedType) const
{
	auto it = mRuntimePlants.find(seedType);
	return it == mRuntimePlants.end() ? nullptr : &it->second;
}

const ModZombieDef* ModRegistry::FindZombieByRuntimeId(int zombieType) const
{
	auto it = mRuntimeZombies.find(zombieType);
	return it == mRuntimeZombies.end() ? nullptr : &it->second;
}

const ModModeDef* ModRegistry::FindModeByRuntimeId(int baseMode) const
{
	auto it = mRuntimeModes.find(baseMode);
	return it == mRuntimeModes.end() ? nullptr : &it->second;
}

const ModProjectileDef* ModRegistry::FindProjectileByRuntimeId(int projectileType) const
{
	auto it = mRuntimeProjectiles.find(projectileType);
	return it == mRuntimeProjectiles.end() ? nullptr : &it->second;
}

std::vector<ModModeChallengeDef> ModRegistry::GetModeChallengeDefs() const
{
	std::vector<ModModeChallengeDef> out;
	for (const auto& pair : mRuntimeModes)
	{
		const ModModeDef& def = pair.second;
		if (!def.challengeName.empty())
		{
			out.push_back({ def.baseMode, def.challengePage, def.challengeRow, def.challengeCol,
				def.challengeIconIndex, def.challengeName });
		}
	}
	return out;
}

const std::vector<std::string>& ModRegistry::GetErrors() const
{
	return mErrors;
}

int ModRegistry::GetTotalAlmanacPlants() const
{
	return 49 + mRuntimePlants.size();
}

int ModRegistry::GetAlmanacPlantAt(int index) const
{
	if (index < 49)
		return index; // Vanilla seed types 0-48

	int modIndex = index - 49;
	auto it = mRuntimePlants.begin();
	std::advance(it, modIndex);
	if (it != mRuntimePlants.end())
		return it->second.seedType;

	return 0; // Fallback
}

bool ModRegistry::ValidateId(const std::string& id, std::string* outError) const
{
	if (id.empty())
	{
		if (outError)
			*outError = "Id is empty";
		return false;
	}
	return true;
}

void ModRegistry::BuildReanimNameMap()
{
	extern ReanimationParams gLawnReanimationArray[];
	mReanimNameMap.clear();
	for (int i = 0; i < ReanimationType::NUM_REANIMS; i++)
	{
		mReanimNameMap[gLawnReanimationArray[i].mReanimFileName] = i;
	}
}

int ModRegistry::ResolveReanimationType(const std::string& reanimName) const
{
	if (reanimName.empty())
		return ReanimationType::REANIM_NONE;

	auto it = mReanimNameMap.find(reanimName);
	if (it != mReanimNameMap.end())
		return it->second;

	std::string withPrefix = reanimName;
	if (withPrefix.find("reanim/") != 0 && withPrefix.find("reanim\\") != 0)
	{
		withPrefix = "reanim/" + withPrefix;
		if (!HasFileExtension(withPrefix))
			withPrefix += ".reanim";
	}
	it = mReanimNameMap.find(withPrefix);
	if (it != mReanimNameMap.end())
		return it->second;

	return static_cast<int>(ReanimationType::REANIM_NONE);
}

unsigned int ModRegistry::RegisterDynamicReanim(const std::string& reanimFilePath)
{
	auto it = mReanimNameMap.find(reanimFilePath);
	if (it != mReanimNameMap.end())
		return it->second;

	unsigned int dynamicIndex = ReanimatorRegisterDynamic(reanimFilePath.c_str(), 0);
	mReanimNameMap[reanimFilePath] = dynamicIndex;
	ReanimatorEnsureDynamicDefinitionLoaded(dynamicIndex);
	return dynamicIndex;
}

void ModRegistry::InjectPlantStringOverrides(Sexy::SexyAppBase* app)
{
	for (auto& pair : mPlants)
	{
		ModPlantDef& def = pair.second;
		if (!def.plantName.empty())
			app->SetString(def.plantName, def.plantName);
		if (!def.description.empty())
		{
			app->SetString(def.plantName + "_DESCRIPTION", def.description);
			app->SetString(def.plantName + "_TOOLTIP", def.description);
		}
	}
}

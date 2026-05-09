/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModRegistry.h"

ModRegistry gModRegistry;

void ModRegistry::Reset()
{
	mPlants.clear();
	mZombies.clear();
	mModes.clear();
	mProjectiles.clear();
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
	mPlants[def.id] = def;
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
	mZombies[def.id] = def;
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
	mModes[def.id] = def;
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
	mProjectiles[def.id] = def;
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

const std::vector<std::string>& ModRegistry::GetErrors() const
{
	return mErrors;
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

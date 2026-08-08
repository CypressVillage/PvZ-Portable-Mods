/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __MODLOADER_H__
#define __MODLOADER_H__

#include <string>
#include <set>
#include <vector>

namespace Sexy
{
	class SexyAppBase;
}

struct ModManifest
{
	std::string id;
	std::string name;
	std::string version;
	std::string author;
	std::string description;
	std::string entry;
	std::string rootPath;
	int priority = 0;
	std::vector<std::string> dependencies;
};

struct ModInfo
{
	ModManifest manifest;
	std::string directoryName;
	std::string manifestPath;
	std::string error;
	bool valid = false;
	bool enabled = true;
	bool loaded = false;
};

class ModLoader
{
public:
	bool LoadAll();
	const std::vector<ModManifest>& GetManifests() const;
	const std::vector<ModInfo>& GetMods() const;
	const std::vector<std::string>& GetErrors() const;
	bool SetEnabledState(std::vector<bool>& states, size_t index, bool enabled, std::vector<size_t>& affected, std::string& outError) const;
	bool SaveEnabledStates(const std::vector<bool>& states, std::string& outError);
	void ApplyStringOverrides(Sexy::SexyAppBase* app) const;

private:
	bool LoadManifestFromFile(ModInfo& outInfo);
	void SetModError(ModInfo& info, const std::string& error);

private:
	std::vector<ModManifest> mManifests;
	std::vector<ModInfo> mMods;
	std::vector<std::string> mErrors;
	std::set<std::string> mDisabledIds;
};

extern ModLoader gModLoader;

bool ModIsValidId(const std::string& id);

#endif

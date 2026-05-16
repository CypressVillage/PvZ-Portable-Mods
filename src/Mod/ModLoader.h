/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __MODLOADER_H__
#define __MODLOADER_H__

#include <map>
#include <string>
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
	std::map<std::string, std::vector<std::string>> dataFiles;
};

class ModLoader
{
public:
	bool LoadAll();
	const std::vector<ModManifest>& GetManifests() const;
	const std::vector<std::string>& GetErrors() const;
	void ApplyStringOverrides(Sexy::SexyAppBase* app) const;

	bool LoadPlantDefs();
	bool LoadProjectileDefs();

private:
	bool LoadManifestFromFile(const std::string& manifestPath, const std::string& rootPath);

private:
	std::vector<ModManifest> mManifests;
	std::vector<std::string> mErrors;
};

extern ModLoader gModLoader;

#endif

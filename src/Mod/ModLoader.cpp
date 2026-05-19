/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModLoader.h"
#include "ModJson.h"
#include "ModRegistry.h"
#include "SexyAppFramework/Common.h"
#include "SexyAppFramework/SexyAppBase.h"
#include "Sexy.TodLib/TodDebug.h"
#include <algorithm>
#include <filesystem>

using namespace Sexy;

ModLoader gModLoader;

bool ModLoader::LoadAll()
{
	mManifests.clear();
	mErrors.clear();

	std::string modsRoot = GetResourcePath("mods");
	printf("LOOKING FOR MODS AT: %s\n", modsRoot.c_str());
	std::filesystem::path rootPath = PathFromU8(modsRoot);
	if (!std::filesystem::exists(rootPath)) {
		printf("MODS FOLDER DOES NOT EXIST!\n");
	} else {
		printf("MODS FOLDER EXISTS.\n");
		for (const auto& entry : std::filesystem::directory_iterator(rootPath)) {
			printf("FOUND ENTRY: %s\n", entry.path().string().c_str());
		}
	}
	if (!std::filesystem::exists(rootPath))
	{
		return true;
	}

	bool allSuccess = true;
	for (const auto& entry : std::filesystem::directory_iterator(rootPath))
	{
		if (!entry.is_directory())
			continue;

		std::filesystem::path manifestPath = entry.path() / "mod.json";
		if (!std::filesystem::exists(manifestPath))
			continue;

		std::string manifestPathStr = PathToU8(manifestPath);
		std::string modRootStr = PathToU8(entry.path());
		if (!LoadManifestFromFile(manifestPathStr, modRootStr))
		{
			TodLog("Mod manifest failed: %s", manifestPathStr.c_str());
			allSuccess = false;
			continue;
		}
	}

	// Topological sort based on dependencies (Kahn's algorithm)
	{
		std::map<std::string, int> inDegree;
		std::map<std::string, std::vector<std::string>> dependents;
		std::map<std::string, const ModManifest*> manifestById;

		for (const auto& m : mManifests)
		{
			inDegree[m.id] = 0;
			manifestById[m.id] = &m;
		}

		for (const auto& m : mManifests)
		{
			for (const auto& dep : m.dependencies)
			{
				if (manifestById.find(dep) != manifestById.end())
				{
					inDegree[m.id]++;
					dependents[dep].push_back(m.id);
				}
			}
		}

		std::vector<std::string> sorted;
		{
			std::vector<std::string> queue;
			for (const auto& pair : inDegree)
			{
				if (pair.second == 0)
					queue.push_back(pair.first);
			}
			while (!queue.empty())
			{
				std::string node = queue.back();
				queue.pop_back();
				sorted.push_back(node);
				for (const auto& dep : dependents[node])
				{
					if (--inDegree[dep] == 0)
						queue.push_back(dep);
				}
			}
		}

		// If cycle detected (some mods not sorted), append them unsorted
		if (sorted.size() < mManifests.size())
		{
			for (const auto& pair : inDegree)
			{
				if (pair.second > 0 && std::find(sorted.begin(), sorted.end(), pair.first) == sorted.end())
					sorted.push_back(pair.first);
			}
			mErrors.push_back("Circular dependency detected among mods");
		}

		// Reorder mManifests to match topological order, then stable_sort by priority within
		std::stable_sort(mManifests.begin(), mManifests.end(),
			[&sorted](const ModManifest& a, const ModManifest& b) {
				auto aIt = std::find(sorted.begin(), sorted.end(), a.id);
				auto bIt = std::find(sorted.begin(), sorted.end(), b.id);
				int aOrder = static_cast<int>(aIt - sorted.begin());
				int bOrder = static_cast<int>(bIt - sorted.begin());
				if (aOrder != bOrder)
					return aOrder < bOrder;
				return a.priority < b.priority;
			});
	}

	return mErrors.empty();
}

const std::vector<ModManifest>& ModLoader::GetManifests() const
{
	return mManifests;
}

const std::vector<std::string>& ModLoader::GetErrors() const
{
	return mErrors;
}

void ModLoader::ApplyStringOverrides(Sexy::SexyAppBase* app) const
{
	if (app == nullptr)
		return;

	gModRegistry.InjectPlantStringOverrides(app);
}

bool ModLoader::LoadManifestFromFile(const std::string& manifestPath, const std::string& rootPath)
{
	std::string jsonText;
	if (!ModJsonReadFile(manifestPath, jsonText))
	{
		mErrors.push_back("Failed to read: " + manifestPath);
		return false;
	}

	ModJsonParser parser(jsonText);
	ModJsonValue root;
	std::string error;
	if (!parser.Parse(root, error))
	{
		mErrors.push_back("Parse error in " + manifestPath + ": " + error);
		return false;
	}
	if (root.type != ModJsonType::Object)
	{
		mErrors.push_back("Root JSON must be object: " + manifestPath);
		return false;
	}

	ModManifest manifest;
	manifest.rootPath = rootPath;

	if (!ModJsonGetObjectString(root, "id", manifest.id, true, error))
	{
		mErrors.push_back(manifestPath + ": " + error);
		return false;
	}
	ModJsonGetObjectString(root, "name", manifest.name, false, error);
	ModJsonGetObjectString(root, "version", manifest.version, false, error);
	ModJsonGetObjectString(root, "author", manifest.author, false, error);
	ModJsonGetObjectString(root, "description", manifest.description, false, error);
	ModJsonGetObjectString(root, "entry", manifest.entry, false, error);
	ModJsonGetObjectInt(root, "priority", manifest.priority, false, error);

	auto depIt = root.objectValue.find("dependencies");
	if (depIt != root.objectValue.end())
	{
		std::vector<std::string> deps;
		if (!ModJsonGetStringArray(depIt->second, deps, error))
		{
			mErrors.push_back(manifestPath + ": dependencies: " + error);
			return false;
		}
		manifest.dependencies = deps;
	}

	mManifests.push_back(std::move(manifest));
	return true;
}



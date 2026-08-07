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
#include <map>

using namespace Sexy;

ModLoader gModLoader;

bool ModIsValidId(const std::string& id)
{
	if (id.empty())
		return false;

	return std::all_of(id.begin(), id.end(), [](unsigned char c) {
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
	});
}

bool ModLoader::LoadAll()
{
	mManifests.clear();
	mErrors.clear();

	std::string modsRoot = GetResourcePath("mods");
	std::filesystem::path rootPath = PathFromU8(modsRoot);
	std::error_code error;
	bool rootExists = std::filesystem::exists(rootPath, error);
	if (error)
	{
		mErrors.push_back("Failed to inspect Mods directory " + modsRoot + ": " + error.message());
		return false;
	}
	if (!rootExists)
	{
		return true;
	}

	bool allSuccess = true;
	std::filesystem::directory_iterator entry(rootPath, error);
	std::filesystem::directory_iterator end;
	if (error)
	{
		mErrors.push_back("Failed to read Mods directory " + modsRoot + ": " + error.message());
		return false;
	}
	while (entry != end)
	{
		const std::filesystem::directory_entry current = *entry;
		entry.increment(error);
		if (error)
		{
			mErrors.push_back("Failed while reading Mods directory " + modsRoot + ": " + error.message());
			allSuccess = false;
			break;
		}

		bool isDirectory = current.is_directory(error);
		if (error)
		{
			mErrors.push_back("Failed to inspect Mod entry " + PathToU8(current.path()) + ": " + error.message());
			allSuccess = false;
			error.clear();
			continue;
		}
		if (!isDirectory)
			continue;

		std::filesystem::path manifestPath = current.path() / "mod.json";
		bool manifestExists = std::filesystem::exists(manifestPath, error);
		if (error)
		{
			mErrors.push_back("Failed to inspect Mod manifest " + PathToU8(manifestPath) + ": " + error.message());
			allSuccess = false;
			error.clear();
			continue;
		}
		if (!manifestExists)
			continue;

		std::string manifestPathStr = PathToU8(manifestPath);
		std::string modRootStr = PathToU8(current.path());
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
			std::vector<std::string> ready;
			for (const auto& pair : inDegree)
			{
				if (pair.second == 0)
					ready.push_back(pair.first);
			}
			auto readyBefore = [&manifestById](const std::string& a, const std::string& b) {
				const ModManifest* aManifest = manifestById.at(a);
				const ModManifest* bManifest = manifestById.at(b);
				if (aManifest->priority != bManifest->priority)
					return aManifest->priority < bManifest->priority;
				return a < b;
			};
			while (!ready.empty())
			{
				std::sort(ready.begin(), ready.end(), readyBefore);
				std::string node = ready.front();
				ready.erase(ready.begin());
				sorted.push_back(node);
				for (const auto& dep : dependents[node])
				{
					if (--inDegree[dep] == 0)
						ready.push_back(dep);
				}
			}
		}

		// Cyclic Mods cannot be loaded in a dependency-safe order.
		if (sorted.size() < mManifests.size())
		{
			mErrors.push_back("Circular dependency detected among mods");
		}

		std::vector<ModManifest> ordered;
		ordered.reserve(sorted.size());
		for (const std::string& id : sorted)
			ordered.push_back(*manifestById.at(id));
		mManifests = std::move(ordered);
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
	if (!ModIsValidId(manifest.id))
	{
		mErrors.push_back(manifestPath + ": id may contain only letters, digits, '.', '_', and '-'");
		return false;
	}
	for (const ModManifest& loaded : mManifests)
	{
		if (loaded.id == manifest.id)
		{
			mErrors.push_back(manifestPath + ": duplicate Mod id '" + manifest.id + "' (already loaded from " + loaded.rootPath + ")");
			return false;
		}
	}

	auto readString = [&](const char* key, std::string& value) {
		if (ModJsonGetObjectString(root, key, value, false, error))
			return true;
		mErrors.push_back(manifestPath + ": " + error);
		return false;
	};
	if (!readString("name", manifest.name) || !readString("version", manifest.version) ||
		!readString("author", manifest.author) || !readString("description", manifest.description) ||
		!readString("entry", manifest.entry))
		return false;
	if (!ModJsonGetObjectInt(root, "priority", manifest.priority, false, error))
	{
		mErrors.push_back(manifestPath + ": " + error);
		return false;
	}

	auto depIt = root.objectValue.find("dependencies");
	if (depIt != root.objectValue.end())
	{
		std::vector<std::string> deps;
		if (!ModJsonGetStringArray(depIt->second, deps, error))
		{
			mErrors.push_back(manifestPath + ": dependencies: " + error);
			return false;
		}
		for (const std::string& dependency : deps)
		{
			if (!ModIsValidId(dependency))
			{
				mErrors.push_back(manifestPath + ": invalid dependency id '" + dependency + "'");
				return false;
			}
		}
		manifest.dependencies = deps;
	}

	mManifests.push_back(std::move(manifest));
	return true;
}

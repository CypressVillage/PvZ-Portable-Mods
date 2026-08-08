/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModLoader.h"
#include "ModConfig.h"
#include "ModJson.h"
#include "ModRegistry.h"
#include "SexyAppFramework/Common.h"
#include "SexyAppFramework/SexyAppBase.h"
#include "Sexy.TodLib/TodDebug.h"
#include <algorithm>
#include <filesystem>
#include <functional>
#include <map>
#include <set>

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
	mMods.clear();
	mErrors.clear();
	mDisabledIds.clear();
	std::string configError;
	if (!ModConfig::LoadDisabledIds(mDisabledIds, configError))
	{
		mErrors.push_back(configError);
		mDisabledIds.clear();
	}

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
		return mErrors.empty();
	}

	std::vector<std::filesystem::path> modDirectories;
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
			break;
		}

		bool isDirectory = current.is_directory(error);
		if (error)
		{
			mErrors.push_back("Failed to inspect Mod entry " + PathToU8(current.path()) + ": " + error.message());
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
			error.clear();
			continue;
		}
		if (!manifestExists)
			continue;

		modDirectories.push_back(current.path());
	}
	std::sort(modDirectories.begin(), modDirectories.end());
	for (const std::filesystem::path& modDirectory : modDirectories)
	{
		ModInfo info;
		info.directoryName = PathToU8(modDirectory.filename());
		info.manifestPath = PathToU8(modDirectory / "mod.json");
		info.manifest.rootPath = PathToU8(modDirectory);
		LoadManifestFromFile(info);
		mMods.push_back(std::move(info));
	}

	std::map<std::string, std::vector<size_t>> entriesById;
	for (size_t i = 0; i < mMods.size(); i++)
	{
		if (!mMods[i].manifest.id.empty())
			entriesById[mMods[i].manifest.id].push_back(i);
	}
	for (const auto& pair : entriesById)
	{
		if (pair.second.size() < 2)
			continue;
		for (size_t index : pair.second)
			SetModError(mMods[index], "Duplicate Mod id '" + pair.first + "'");
	}

	auto invalidateUnavailableDependencies = [&]() {
		bool changed = false;
		for (ModInfo& info : mMods)
		{
			if (!info.valid)
				continue;
			for (const std::string& dependency : info.manifest.dependencies)
			{
				auto found = entriesById.find(dependency);
				if (found == entriesById.end())
				{
					SetModError(info, "Missing dependency '" + dependency + "'");
					changed = true;
					break;
				}
				bool available = found->second.size() == 1 && mMods[found->second.front()].valid;
				if (!available)
				{
					SetModError(info, "Dependency '" + dependency + "' is unavailable");
					changed = true;
					break;
				}
			}
		}
		return changed;
	};
	while (invalidateUnavailableDependencies())
	{
	}

	std::map<std::string, size_t> validById;
	for (size_t i = 0; i < mMods.size(); i++)
	{
		if (mMods[i].valid)
			validById[mMods[i].manifest.id] = i;
	}
	std::map<std::string, int> visitState;
	std::vector<std::string> visitStack;
	std::set<std::string> cycleIds;
	std::function<void(const std::string&)> visit = [&](const std::string& id) {
		visitState[id] = 1;
		visitStack.push_back(id);
		const ModInfo& info = mMods[validById.at(id)];
		for (const std::string& dependency : info.manifest.dependencies)
		{
			if (visitState[dependency] == 0)
				visit(dependency);
			else if (visitState[dependency] == 1)
			{
				auto start = std::find(visitStack.begin(), visitStack.end(), dependency);
				cycleIds.insert(start, visitStack.end());
			}
		}
		visitStack.pop_back();
		visitState[id] = 2;
	};
	for (const auto& pair : validById)
	{
		if (visitState[pair.first] == 0)
			visit(pair.first);
	}
	for (const std::string& id : cycleIds)
		SetModError(mMods[validById.at(id)], "Circular dependency involving '" + id + "'");
	while (invalidateUnavailableDependencies())
	{
	}

	validById.clear();
	for (size_t i = 0; i < mMods.size(); i++)
	{
		ModInfo& info = mMods[i];
		info.enabled = info.valid && !mDisabledIds.contains(info.manifest.id);
		if (info.valid)
			validById[info.manifest.id] = i;
	}
	bool enabledChanged;
	do
	{
		enabledChanged = false;
		for (ModInfo& info : mMods)
		{
			if (!info.enabled)
				continue;
			for (const std::string& dependency : info.manifest.dependencies)
			{
				if (!mMods[validById.at(dependency)].enabled)
				{
					info.enabled = false;
					enabledChanged = true;
					break;
				}
			}
		}
	} while (enabledChanged);

	// Stable topological order for all valid Mods, including disabled entries shown by the manager.
	{
		std::map<std::string, int> inDegree;
		std::map<std::string, std::vector<std::string>> dependents;
		for (const auto& pair : validById)
		{
			inDegree[pair.first] = 0;
		}
		for (const auto& pair : validById)
		{
			const ModManifest& manifest = mMods[pair.second].manifest;
			for (const std::string& dependency : manifest.dependencies)
			{
				inDegree[manifest.id]++;
				dependents[dependency].push_back(manifest.id);
			}
		}
		std::vector<std::string> ready;
		for (const auto& pair : inDegree)
		{
			if (pair.second == 0)
				ready.push_back(pair.first);
		}
		auto readyBefore = [&](const std::string& a, const std::string& b) {
			const ModManifest& aManifest = mMods[validById.at(a)].manifest;
			const ModManifest& bManifest = mMods[validById.at(b)].manifest;
			if (aManifest.priority != bManifest.priority)
				return aManifest.priority < bManifest.priority;
			return a < b;
		};
		std::vector<size_t> orderedIndices;
		while (!ready.empty())
		{
			std::sort(ready.begin(), ready.end(), readyBefore);
			std::string id = ready.front();
			ready.erase(ready.begin());
			orderedIndices.push_back(validById.at(id));
			for (const std::string& dependent : dependents[id])
			{
				if (--inDegree[dependent] == 0)
					ready.push_back(dependent);
			}
		}
		std::vector<size_t> invalidIndices;
		for (size_t i = 0; i < mMods.size(); i++)
		{
			if (!mMods[i].valid)
				invalidIndices.push_back(i);
		}
		std::sort(invalidIndices.begin(), invalidIndices.end(), [&](size_t a, size_t b) {
			std::string aKey = mMods[a].manifest.id.empty() ? mMods[a].directoryName : mMods[a].manifest.id;
			std::string bKey = mMods[b].manifest.id.empty() ? mMods[b].directoryName : mMods[b].manifest.id;
			return aKey < bKey;
		});
		orderedIndices.insert(orderedIndices.end(), invalidIndices.begin(), invalidIndices.end());
		std::vector<ModInfo> orderedMods;
		orderedMods.reserve(mMods.size());
		for (size_t index : orderedIndices)
			orderedMods.push_back(std::move(mMods[index]));
		mMods = std::move(orderedMods);
	}
	for (ModInfo& info : mMods)
	{
		info.loaded = info.valid && info.enabled;
		if (info.loaded)
			mManifests.push_back(info.manifest);
	}

	return mErrors.empty();
}

const std::vector<ModManifest>& ModLoader::GetManifests() const
{
	return mManifests;
}

const std::vector<ModInfo>& ModLoader::GetMods() const
{
	return mMods;
}

const std::vector<std::string>& ModLoader::GetErrors() const
{
	return mErrors;
}

bool ModLoader::SetEnabledState(std::vector<bool>& states, size_t index, bool enabled, std::vector<size_t>& affected, std::string& outError) const
{
	affected.clear();
	outError.clear();
	if (states.size() != mMods.size() || index >= mMods.size() || !mMods[index].valid)
	{
		outError = "Mod cannot be changed";
		return false;
	}
	std::map<std::string, size_t> byId;
	for (size_t i = 0; i < mMods.size(); i++)
	{
		if (mMods[i].valid)
			byId[mMods[i].manifest.id] = i;
	}
	std::function<void(size_t)> enableWithDependencies = [&](size_t current) {
		if (!states[current])
		{
			states[current] = true;
			affected.push_back(current);
		}
		for (const std::string& dependency : mMods[current].manifest.dependencies)
			enableWithDependencies(byId.at(dependency));
	};
	std::function<void(size_t)> disableWithDependents = [&](size_t current) {
		if (states[current])
		{
			states[current] = false;
			affected.push_back(current);
		}
		const std::string& id = mMods[current].manifest.id;
		for (size_t i = 0; i < mMods.size(); i++)
		{
			if (!mMods[i].valid || !states[i])
				continue;
			if (std::find(mMods[i].manifest.dependencies.begin(), mMods[i].manifest.dependencies.end(), id) != mMods[i].manifest.dependencies.end())
				disableWithDependents(i);
		}
	};
	if (enabled)
		enableWithDependencies(index);
	else
		disableWithDependents(index);
	return true;
}

bool ModLoader::SaveEnabledStates(const std::vector<bool>& states, std::string& outError)
{
	if (states.size() != mMods.size())
	{
		outError = "Invalid Mod state list";
		return false;
	}
	ModConfig::DisabledIds disabled = mDisabledIds;
	for (size_t i = 0; i < mMods.size(); i++)
	{
		if (!mMods[i].valid)
			continue;
		if (states[i])
			disabled.erase(mMods[i].manifest.id);
		else
			disabled.insert(mMods[i].manifest.id);
	}
	if (!ModConfig::SaveDisabledIds(disabled, outError))
		return false;
	mDisabledIds = std::move(disabled);
	for (size_t i = 0; i < mMods.size(); i++)
	{
		if (mMods[i].valid)
			mMods[i].enabled = states[i];
	}
	return true;
}

void ModLoader::ApplyStringOverrides(Sexy::SexyAppBase* app) const
{
	if (app == nullptr)
		return;

	gModRegistry.InjectPlantStringOverrides(app);
}

void ModLoader::SetModError(ModInfo& info, const std::string& error)
{
	if (!info.error.empty())
		return;
	info.valid = false;
	info.error = error;
	mErrors.push_back(info.manifestPath + ": " + error);
}

bool ModLoader::LoadManifestFromFile(ModInfo& outInfo)
{
	std::string jsonText;
	if (!ModJsonReadFile(outInfo.manifestPath, jsonText))
	{
		SetModError(outInfo, "Failed to read manifest");
		return false;
	}

	ModJsonParser parser(jsonText);
	ModJsonValue root;
	std::string error;
	if (!parser.Parse(root, error))
	{
		SetModError(outInfo, "Parse error: " + error);
		return false;
	}
	if (root.type != ModJsonType::Object)
	{
		SetModError(outInfo, "Root JSON must be object");
		return false;
	}

	ModManifest& manifest = outInfo.manifest;

	if (!ModJsonGetObjectString(root, "id", manifest.id, true, error))
	{
		SetModError(outInfo, error);
		return false;
	}
	if (!ModIsValidId(manifest.id))
	{
		SetModError(outInfo, "id may contain only letters, digits, '.', '_', and '-'");
		return false;
	}

	auto readString = [&](const char* key, std::string& value) {
		if (ModJsonGetObjectString(root, key, value, false, error))
			return true;
		SetModError(outInfo, error);
		return false;
	};
	if (!readString("name", manifest.name) || !readString("version", manifest.version) ||
		!readString("author", manifest.author) || !readString("description", manifest.description) ||
		!readString("entry", manifest.entry))
		return false;
	if (!ModJsonGetObjectInt(root, "priority", manifest.priority, false, error))
	{
		SetModError(outInfo, error);
		return false;
	}

	auto depIt = root.objectValue.find("dependencies");
	if (depIt != root.objectValue.end())
	{
		std::vector<std::string> deps;
		if (!ModJsonGetStringArray(depIt->second, deps, error))
		{
			SetModError(outInfo, "dependencies: " + error);
			return false;
		}
		std::set<std::string> seenDependencies;
		for (const std::string& dependency : deps)
		{
			if (!ModIsValidId(dependency))
			{
				SetModError(outInfo, "invalid dependency id '" + dependency + "'");
				return false;
			}
			if (!seenDependencies.insert(dependency).second)
			{
				SetModError(outInfo, "duplicate dependency id '" + dependency + "'");
				return false;
			}
		}
		manifest.dependencies = deps;
	}

	outInfo.valid = true;
	return true;
}

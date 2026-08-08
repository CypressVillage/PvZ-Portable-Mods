/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModConfig.h"
#include "ModJson.h"
#include "ModLoader.h"
#include "SexyAppFramework/Common.h"
#include <filesystem>
#include <fstream>

namespace
{
	bool ReplaceFile(const std::filesystem::path& tempPath, const std::filesystem::path& filePath, std::string& outError)
	{
		std::error_code error;
		std::filesystem::rename(tempPath, filePath, error);
		if (!error)
			return true;

		std::filesystem::path backupPath = filePath;
		backupPath += ".bak";
		std::error_code cleanupError;
		std::filesystem::remove(backupPath, cleanupError);
		error.clear();
		std::filesystem::rename(filePath, backupPath, error);
		if (error)
		{
			outError = "Failed to replace Mod config: " + error.message();
			return false;
		}

		error.clear();
		std::filesystem::rename(tempPath, filePath, error);
		if (error)
		{
			std::error_code restoreError;
			std::filesystem::rename(backupPath, filePath, restoreError);
			outError = "Failed to install Mod config: " + error.message();
			return false;
		}

		std::filesystem::remove(backupPath, cleanupError);
		return true;
	}
}

std::string ModConfig::GetPath()
{
	return Sexy::GetAppDataPath("modconfig.json");
}

bool ModConfig::LoadDisabledIds(DisabledIds& outIds, std::string& outError)
{
	outIds.clear();
	outError.clear();
	std::filesystem::path path = Sexy::PathFromU8(GetPath());
	std::error_code fsError;
	if (!std::filesystem::exists(path, fsError))
	{
		if (fsError)
		{
			outError = "Failed to inspect Mod config: " + fsError.message();
			return false;
		}
		return true;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		outError = "Failed to read Mod config: " + GetPath();
		return false;
	}
	std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
	ModJsonValue root;
	ModJsonParser parser(text);
	if (!parser.Parse(root, outError))
	{
		outError = "Invalid Mod config: " + outError;
		return false;
	}
	if (root.type != ModJsonType::Object)
	{
		outError = "Invalid Mod config: root must be an object";
		return false;
	}

	auto version = root.objectValue.find("version");
	if (version != root.objectValue.end() && (version->second.type != ModJsonType::Number || version->second.numberValue != 1))
	{
		outError = "Unsupported Mod config version";
		return false;
	}
	auto disabled = root.objectValue.find("disabled");
	if (disabled == root.objectValue.end())
		return true;

	std::vector<std::string> ids;
	if (!ModJsonGetStringArray(disabled->second, ids, outError))
	{
		outError = "Invalid Mod config disabled list: " + outError;
		return false;
	}
	for (const std::string& id : ids)
	{
		if (!ModIsValidId(id))
		{
			outError = "Invalid Mod id in config: " + id;
			outIds.clear();
			return false;
		}
		outIds.insert(id);
	}
	return true;
}

bool ModConfig::SaveDisabledIds(const DisabledIds& ids, std::string& outError)
{
	outError.clear();
	std::filesystem::path filePath = Sexy::PathFromU8(GetPath());
	std::filesystem::path parent = filePath.parent_path();
	std::error_code fsError;
	std::filesystem::create_directories(parent, fsError);
	if (fsError)
	{
		outError = "Failed to create Mod config directory: " + fsError.message();
		return false;
	}

	std::string json = "{\n  \"version\": 1,\n  \"disabled\": [";
	bool first = true;
	for (const std::string& id : ids)
	{
		if (!first)
			json += ',';
		json += "\n    \"" + ModJsonEscape(id) + "\"";
		first = false;
	}
	if (!ids.empty())
		json += '\n';
	json += "  ]\n}\n";

	std::filesystem::path tempPath = filePath;
	tempPath += ".tmp";
	std::ofstream file(tempPath, std::ios::binary | std::ios::trunc);
	if (!file)
	{
		outError = "Failed to write Mod config temporary file";
		return false;
	}
	file << json;
	file.flush();
	bool writeSucceeded = file.good();
	file.close();
	writeSucceeded = writeSucceeded && !file.fail();
	if (!writeSucceeded)
	{
		std::filesystem::remove(tempPath, fsError);
		outError = "Failed to write Mod config";
		return false;
	}
	if (!ReplaceFile(tempPath, filePath, outError))
	{
		std::filesystem::remove(tempPath, fsError);
		return false;
	}
	return true;
}

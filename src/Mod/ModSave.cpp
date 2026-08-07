/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModSave.h"
#include "ModJson.h"
#include "ModLoader.h"
#include "SexyAppFramework/Common.h"
#include "Sexy.TodLib/TodDebug.h"
#include <filesystem>
#include <fstream>
#include <map>

namespace
{
	std::filesystem::path GetModSaveDir()
	{
		return Sexy::PathFromU8(Sexy::GetAppDataPath("modsave"));
	}

	bool ReplaceFile(const std::filesystem::path& tempPath, const std::filesystem::path& filePath)
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
			return false;

		error.clear();
		std::filesystem::rename(tempPath, filePath, error);
		if (error)
		{
			std::error_code restoreError;
			std::filesystem::rename(backupPath, filePath, restoreError);
			return false;
		}

		std::filesystem::remove(backupPath, cleanupError);
		return true;
	}
}

std::string ModSave::GetModSavePath(const std::string& modId)
{
	if (!ModIsValidId(modId))
		return std::string();
	std::filesystem::path dir = GetModSaveDir();
	std::filesystem::path file = dir / (modId + ".json");
	return Sexy::PathToU8(file);
}

bool ModSave::SaveValue(const std::string& modId, const std::string& key, const std::string& value)
{
	if (!ModIsValidId(modId) || key.empty())
		return false;

	std::filesystem::path dir = GetModSaveDir();
	Sexy::MkDir(Sexy::PathToU8(dir));
	std::error_code error;
	if (!std::filesystem::is_directory(dir, error) || error)
	{
		TodLog("ModSave directory unavailable: %s", Sexy::PathToU8(dir).c_str());
		return false;
	}
	std::filesystem::path filePath = dir / (modId + ".json");

	std::string content;
	std::map<std::string, std::string> data;

	if (ModJsonReadFile(Sexy::PathToU8(filePath), content) && !ModJsonDeserializeMap(content, data))
	{
		TodLog("ModSave contains invalid JSON; preserving existing file: %s", Sexy::PathToU8(filePath).c_str());
		return false;
	}

	data[key] = value;
	std::string json = ModJsonSerializeMap(data);

	std::filesystem::path tempPath = filePath;
	tempPath += ".tmp";
	std::ofstream file(tempPath, std::ios::out | std::ios::trunc | std::ios::binary);
	if (!file)
	{
		TodLog("ModSave temporary write failed: %s", Sexy::PathToU8(tempPath).c_str());
		return false;
	}
	file << json;
	file.flush();
	bool writeSucceeded = file.good();
	file.close();
	writeSucceeded = writeSucceeded && !file.fail();
	if (!writeSucceeded)
	{
		std::filesystem::remove(tempPath, error);
		TodLog("ModSave write failed: %s", Sexy::PathToU8(tempPath).c_str());
		return false;
	}
	if (!ReplaceFile(tempPath, filePath))
	{
		std::filesystem::remove(tempPath, error);
		TodLog("ModSave replace failed: %s", Sexy::PathToU8(filePath).c_str());
		return false;
	}
	return true;
}

bool ModSave::LoadValue(const std::string& modId, const std::string& key, std::string& outValue)
{
	outValue.clear();
	if (!ModIsValidId(modId) || key.empty())
		return false;

	std::filesystem::path dir = GetModSaveDir();
	std::filesystem::path filePath = dir / (modId + ".json");

	std::string content;
	if (!ModJsonReadFile(Sexy::PathToU8(filePath), content))
		return false;

	std::map<std::string, std::string> data;
	if (!ModJsonDeserializeMap(content, data))
		return false;

	auto it = data.find(key);
	if (it == data.end())
		return false;

	outValue = it->second;
	return true;
}

/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModSave.h"
#include "ModJson.h"
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
}

std::string ModSave::GetModSavePath(const std::string& modId)
{
	std::filesystem::path dir = GetModSaveDir();
	std::filesystem::path file = dir / (modId + ".json");
	return Sexy::PathToU8(file);
}

bool ModSave::SaveValue(const std::string& modId, const std::string& key, const std::string& value)
{
	if (modId.empty() || key.empty())
		return false;

	std::filesystem::path dir = GetModSaveDir();
	Sexy::MkDir(Sexy::PathToU8(dir));
	std::filesystem::path filePath = dir / (modId + ".json");

	std::string content;
	std::map<std::string, std::string> data;

	if (ModJsonReadFile(Sexy::PathToU8(filePath), content))
		ModJsonDeserializeMap(content, data);

	data[key] = value;
	std::string json = ModJsonSerializeMap(data);

	std::ofstream file(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
	if (!file)
	{
		TodLog("ModSave write failed: %s", Sexy::PathToU8(filePath).c_str());
		return false;
	}
	file << json;
	return true;
}

bool ModSave::LoadValue(const std::string& modId, const std::string& key, std::string& outValue)
{
	outValue.clear();
	if (modId.empty() || key.empty())
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

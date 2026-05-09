/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModSave.h"
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

	bool LoadFileMap(const std::filesystem::path& filePath, std::map<std::string, std::string>& outMap)
	{
		outMap.clear();
		if (!Sexy::FileExists(Sexy::PathToU8(filePath)))
			return true;

		std::ifstream file(filePath, std::ios::in | std::ios::binary);
		if (!file)
			return false;

		std::string line;
		while (std::getline(file, line))
		{
			if (line.empty() || line[0] == '#')
				continue;
			size_t pos = line.find('=');
			if (pos == std::string::npos)
				continue;
			std::string key = Sexy::Trim(line.substr(0, pos));
			std::string value = line.substr(pos + 1);
			if (!key.empty())
				outMap[key] = value;
		}
		return true;
	}

	bool SaveFileMap(const std::filesystem::path& filePath, const std::map<std::string, std::string>& data)
	{
		std::ofstream file(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!file)
			return false;

		for (const auto& kv : data)
			file << kv.first << "=" << kv.second << "\n";

		return true;
	}
}

std::string ModSave::GetModSavePath(const std::string& modId)
{
	std::filesystem::path dir = GetModSaveDir();
	std::filesystem::path file = dir / (modId + ".txt");
	return Sexy::PathToU8(file);
}

bool ModSave::SaveValue(const std::string& modId, const std::string& key, const std::string& value)
{
	if (modId.empty() || key.empty())
		return false;

	std::filesystem::path dir = GetModSaveDir();
	Sexy::MkDir(Sexy::PathToU8(dir));
	std::filesystem::path filePath = dir / (modId + ".txt");

	std::map<std::string, std::string> data;
	if (!LoadFileMap(filePath, data))
		return false;

	data[key] = value;
	if (!SaveFileMap(filePath, data))
	{
		TodLog("ModSave write failed: %s", Sexy::PathToU8(filePath).c_str());
		return false;
	}

	return true;
}

bool ModSave::LoadValue(const std::string& modId, const std::string& key, std::string& outValue)
{
	outValue.clear();
	if (modId.empty() || key.empty())
		return false;

	std::filesystem::path dir = GetModSaveDir();
	std::filesystem::path filePath = dir / (modId + ".txt");

	std::map<std::string, std::string> data;
	if (!LoadFileMap(filePath, data))
		return false;

	auto it = data.find(key);
	if (it == data.end())
		return false;

	outValue = it->second;
	return true;
}

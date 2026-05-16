/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModLoader.h"
#include "ModRegistry.h"
#include "SexyAppFramework/Common.h"
#include "SexyAppFramework/SexyAppBase.h"
#include "Sexy.TodLib/TodDebug.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace Sexy;

namespace
{
	enum class JsonType
	{
		Null,
		Bool,
		Number,
		String,
		Array,
		Object
	};

	struct JsonValue
	{
		JsonType type = JsonType::Null;
		bool boolValue = false;
		int numberValue = 0;
		std::string stringValue;
		std::vector<JsonValue> arrayValue;
		std::map<std::string, JsonValue> objectValue;
	};

	class JsonParser
	{
	public:
		explicit JsonParser(const std::string& input)
			: mInput(input)
		{
		}

		bool Parse(JsonValue& outValue, std::string& outError)
		{
			SkipWhitespace();
			if (!ParseValue(outValue, outError))
				return false;
			SkipWhitespace();
			if (mPos != mInput.size())
			{
				outError = "Trailing characters after JSON value";
				return false;
			}
			return true;
		}

	private:
		bool ParseValue(JsonValue& outValue, std::string& outError)
		{
			SkipWhitespace();
			if (mPos >= mInput.size())
			{
				outError = "Unexpected end of input";
				return false;
			}

			char c = mInput[mPos];
			if (c == '{')
				return ParseObject(outValue, outError);
			if (c == '[')
				return ParseArray(outValue, outError);
			if (c == '"')
				return ParseString(outValue, outError);
			if (c == '-' || std::isdigit(static_cast<unsigned char>(c)))
				return ParseNumber(outValue, outError);
			if (StartsWith("true"))
			{
				mPos += 4;
				outValue.type = JsonType::Bool;
				outValue.boolValue = true;
				return true;
			}
			if (StartsWith("false"))
			{
				mPos += 5;
				outValue.type = JsonType::Bool;
				outValue.boolValue = false;
				return true;
			}
			if (StartsWith("null"))
			{
				mPos += 4;
				outValue.type = JsonType::Null;
				return true;
			}

			outError = "Invalid JSON token";
			return false;
		}

		bool ParseObject(JsonValue& outValue, std::string& outError)
		{
			if (!Consume('{', outError))
				return false;

			outValue.type = JsonType::Object;
			SkipWhitespace();
			if (TryConsume('}'))
				return true;

			while (true)
			{
				JsonValue keyValue;
				if (!ParseString(keyValue, outError))
					return false;
				SkipWhitespace();
				if (!Consume(':', outError))
					return false;

				JsonValue value;
				if (!ParseValue(value, outError))
					return false;

				outValue.objectValue[keyValue.stringValue] = value;
				SkipWhitespace();
				if (TryConsume('}'))
					break;
				if (!Consume(',', outError))
					return false;
			}

			return true;
		}

		bool ParseArray(JsonValue& outValue, std::string& outError)
		{
			if (!Consume('[', outError))
				return false;

			outValue.type = JsonType::Array;
			SkipWhitespace();
			if (TryConsume(']'))
				return true;

			while (true)
			{
				JsonValue item;
				if (!ParseValue(item, outError))
					return false;
				outValue.arrayValue.push_back(item);
				SkipWhitespace();
				if (TryConsume(']'))
					break;
				if (!Consume(',', outError))
					return false;
			}

			return true;
		}

		bool ParseString(JsonValue& outValue, std::string& outError)
		{
			if (!Consume('"', outError))
				return false;

			std::string result;
			while (mPos < mInput.size())
			{
				char c = mInput[mPos++];
				if (c == '"')
				{
					outValue.type = JsonType::String;
					outValue.stringValue = result;
					return true;
				}
				if (c == '\\')
				{
					if (mPos >= mInput.size())
					{
						outError = "Invalid escape sequence";
						return false;
					}
					char esc = mInput[mPos++];
					switch (esc)
					{
						case '"': result.push_back('"'); break;
						case '\\': result.push_back('\\'); break;
						case '/': result.push_back('/'); break;
						case 'b': result.push_back('\b'); break;
						case 'f': result.push_back('\f'); break;
						case 'n': result.push_back('\n'); break;
						case 'r': result.push_back('\r'); break;
						case 't': result.push_back('\t'); break;
						default:
							outError = "Unsupported escape sequence";
							return false;
					}
					continue;
				}
				result.push_back(c);
			}

			outError = "Unterminated string";
			return false;
		}

		bool ParseNumber(JsonValue& outValue, std::string& outError)
		{
			size_t start = mPos;
			if (mInput[mPos] == '-')
				++mPos;
			while (mPos < mInput.size() && std::isdigit(static_cast<unsigned char>(mInput[mPos])))
				++mPos;
			if (start == mPos)
			{
				outError = "Invalid number";
				return false;
			}

			int value = 0;
			try
			{
				value = std::stoi(mInput.substr(start, mPos - start));
			}
			catch (...)
			{
				outError = "Invalid number";
				return false;
			}
			outValue.type = JsonType::Number;
			outValue.numberValue = value;
			return true;
		}

		bool Consume(char expected, std::string& outError)
		{
			SkipWhitespace();
			if (mPos >= mInput.size() || mInput[mPos] != expected)
			{
				outError = "Expected character '" + std::string(1, expected) + "'";
				return false;
			}
			++mPos;
			return true;
		}

		bool TryConsume(char expected)
		{
			SkipWhitespace();
			if (mPos < mInput.size() && mInput[mPos] == expected)
			{
				++mPos;
				return true;
			}
			return false;
		}

		bool StartsWith(const char* text) const
		{
			size_t len = std::strlen(text);
			if (mPos + len > mInput.size())
				return false;
			return mInput.compare(mPos, len, text) == 0;
		}

		void SkipWhitespace()
		{
			while (mPos < mInput.size())
			{
				char c = mInput[mPos];
				if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
					++mPos;
				else
					break;
			}
		}

	private:
		const std::string& mInput;
		size_t mPos = 0;
	};

	bool ReadFileToString(const std::string& filePath, std::string& outText)
	{
		std::ifstream file(PathFromU8(filePath), std::ios::in | std::ios::binary);
		if (!file)
			return false;
		std::ostringstream ss;
		ss << file.rdbuf();
		outText = ss.str();
		return true;
	}

	bool GetObjectString(const JsonValue& object, const char* key, std::string& outValue, bool required, std::string& outError)
	{
		auto it = object.objectValue.find(key);
		if (it == object.objectValue.end())
		{
			if (required)
				outError = std::string("Missing required field: ") + key;
			return !required;
		}
		if (it->second.type != JsonType::String)
		{
			outError = std::string("Field is not string: ") + key;
			return false;
		}
		outValue = it->second.stringValue;
		return true;
	}

	bool GetObjectInt(const JsonValue& object, const char* key, int& outValue, bool required, std::string& outError)
	{
		auto it = object.objectValue.find(key);
		if (it == object.objectValue.end())
		{
			if (required)
				outError = std::string("Missing required field: ") + key;
			return !required;
		}
		if (it->second.type != JsonType::Number)
		{
			outError = std::string("Field is not number: ") + key;
			return false;
		}
		outValue = it->second.numberValue;
		return true;
	}

	bool GetObjectFloat(const JsonValue& object, const char* key, float& outValue, bool required, std::string& outError)
	{
		int intValue = 0;
		if (!GetObjectInt(object, key, intValue, required, outError))
			return false;
		outValue = static_cast<float>(intValue);
		return true;
	}

	bool GetStringArray(const JsonValue& value, std::vector<std::string>& outValues, std::string& outError)
	{
		if (value.type != JsonType::Array)
		{
			outError = "Expected array";
			return false;
		}
		outValues.clear();
		for (const JsonValue& item : value.arrayValue)
		{
			if (item.type != JsonType::String)
			{
				outError = "Array item is not string";
				return false;
			}
			outValues.push_back(item.stringValue);
		}
		return true;
	}

	bool GetStringMap(const JsonValue& value, std::map<std::string, std::string>& outMap, std::string& outError)
	{
		if (value.type != JsonType::Object)
		{
			outError = "Expected object";
			return false;
		}
		outMap.clear();
		for (const auto& item : value.objectValue)
		{
			if (item.second.type != JsonType::String)
			{
				outError = "Object value is not string";
				return false;
			}
			outMap[item.first] = item.second.stringValue;
		}
		return true;
	}
}

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

	std::stable_sort(mManifests.begin(), mManifests.end(), [](const ModManifest& a, const ModManifest& b) {
		return a.priority < b.priority;
	});

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

	for (const ModManifest& manifest : mManifests)
	{
		auto it = manifest.dataFiles.find("strings");
		if (it == manifest.dataFiles.end())
			continue;

		for (const std::string& relativePath : it->second)
		{
			std::filesystem::path filePath = PathFromU8(manifest.rootPath) / PathFromU8(relativePath);
			std::string jsonText;
			if (!ReadFileToString(PathToU8(filePath), jsonText))
			{
				TodLog("Mod strings read failed: %s", PathToU8(filePath).c_str());
				continue;
			}

			JsonParser parser(jsonText);
			JsonValue root;
			std::string error;
			if (!parser.Parse(root, error))
			{
				TodLog("Mod strings parse error: %s", error.c_str());
				continue;
			}

			std::map<std::string, std::string> stringMap;
			if (!GetStringMap(root, stringMap, error))
			{
				TodLog("Mod strings invalid: %s", error.c_str());
				continue;
			}

			for (const auto& kv : stringMap)
				app->SetString(kv.first, kv.second);
		}
	}
}

bool ModLoader::LoadManifestFromFile(const std::string& manifestPath, const std::string& rootPath)
{
	std::string jsonText;
	if (!ReadFileToString(manifestPath, jsonText))
	{
		mErrors.push_back("Failed to read: " + manifestPath);
		return false;
	}

	JsonParser parser(jsonText);
	JsonValue root;
	std::string error;
	if (!parser.Parse(root, error))
	{
		mErrors.push_back("Parse error in " + manifestPath + ": " + error);
		return false;
	}
	if (root.type != JsonType::Object)
	{
		mErrors.push_back("Root JSON must be object: " + manifestPath);
		return false;
	}

	ModManifest manifest;
	manifest.rootPath = rootPath;

	if (!GetObjectString(root, "id", manifest.id, true, error))
	{
		mErrors.push_back(manifestPath + ": " + error);
		return false;
	}
	GetObjectString(root, "name", manifest.name, false, error);
	GetObjectString(root, "version", manifest.version, false, error);
	GetObjectString(root, "author", manifest.author, false, error);
	GetObjectString(root, "description", manifest.description, false, error);
	GetObjectString(root, "entry", manifest.entry, false, error);
	GetObjectInt(root, "priority", manifest.priority, false, error);

	auto depIt = root.objectValue.find("dependencies");
	if (depIt != root.objectValue.end())
	{
		std::vector<std::string> deps;
		if (!GetStringArray(depIt->second, deps, error))
		{
			mErrors.push_back(manifestPath + ": dependencies: " + error);
			return false;
		}
		manifest.dependencies = deps;
	}

	auto dataIt = root.objectValue.find("data");
	if (dataIt != root.objectValue.end())
	{
		if (dataIt->second.type != JsonType::Object)
		{
			mErrors.push_back(manifestPath + ": data: expected object");
			return false;
		}
		for (const auto& item : dataIt->second.objectValue)
		{
			std::vector<std::string> files;
			if (!GetStringArray(item.second, files, error))
			{
				mErrors.push_back(manifestPath + ": data." + item.first + ": " + error);
				return false;
			}
			manifest.dataFiles[item.first] = files;
		}
	}

	mManifests.push_back(std::move(manifest));
	return true;
}

bool ModLoader::LoadPlantDefs()
{
	int nextSeedType = 2000;

	for (const ModManifest& manifest : mManifests)
	{
		auto it = manifest.dataFiles.find("plants");
		if (it == manifest.dataFiles.end())
			continue;

		for (const std::string& relativePath : it->second)
		{
			std::filesystem::path filePath = PathFromU8(manifest.rootPath) / PathFromU8(relativePath);
			std::string jsonText;
			if (!ReadFileToString(PathToU8(filePath), jsonText))
			{
				mErrors.push_back("Failed to read plant file: " + PathToU8(filePath));
				continue;
			}

			JsonParser parser(jsonText);
			JsonValue root;
			std::string error;
			if (!parser.Parse(root, error))
			{
				mErrors.push_back("Parse error in " + PathToU8(filePath) + ": " + error);
				continue;
			}
			if (root.type != JsonType::Object)
			{
				mErrors.push_back("Plant file root must be object: " + PathToU8(filePath));
				continue;
			}

			ModPlantDef plantDef;
			plantDef.seedType = nextSeedType;

			if (!GetObjectString(root, "id", plantDef.id, true, error))
			{
				mErrors.push_back(PathToU8(filePath) + ": " + error);
				continue;
			}

			GetObjectInt(root, "cost", plantDef.seedCost, false, error);
			GetObjectInt(root, "cooldown", plantDef.refreshTime, false, error);
			GetObjectInt(root, "packetIndex", plantDef.packetIndex, false, error);
			GetObjectInt(root, "subClass", plantDef.subClass, false, error);
			GetObjectInt(root, "launchRate", plantDef.launchRate, false, error);

			std::string projTypeStr;
			if (GetObjectString(root, "projectileType", projTypeStr, false, error) && !projTypeStr.empty())
			{
				const ModProjectileDef* projDef = gModRegistry.FindProjectile(projTypeStr);
				if (projDef && projDef->projectileType >= 0)
				{
					plantDef.projectileType = projDef->projectileType;
				}
				else
				{
					mErrors.push_back(PathToU8(filePath) + ": projectileType string '" + projTypeStr + "' not found in registered projectiles");
				}
			}
			else
			{
				GetObjectInt(root, "projectileType", plantDef.projectileType, false, error);
			}
			GetObjectString(root, "name", plantDef.plantName, false, error);
			GetObjectString(root, "reanimation", plantDef.reanimationName, false, error);
			GetObjectString(root, "image", plantDef.imageName, false, error);

			std::string reanimFile;
			if (GetObjectString(root, "reanimFile", reanimFile, false, error) && !reanimFile.empty())
			{
				std::filesystem::path absReanimPath = PathFromU8(manifest.rootPath) / PathFromU8(reanimFile);
				plantDef.reanimationName = PathToU8(absReanimPath);
				gModRegistry.RegisterDynamicReanim(plantDef.reanimationName);
			}

			std::string outError;
			if (!gModRegistry.RegisterPlant(plantDef, &outError))
			{
				mErrors.push_back("Plant registration failed: " + outError);
				continue;
			}

			nextSeedType++;
		}
	}

	return true;
}

bool ModLoader::LoadProjectileDefs()
{
	for (const ModManifest& manifest : mManifests)
	{
		auto it = manifest.dataFiles.find("projectiles");
		if (it == manifest.dataFiles.end())
			continue;

		for (const std::string& relativePath : it->second)
		{
			std::filesystem::path filePath = PathFromU8(manifest.rootPath) / PathFromU8(relativePath);
			std::string jsonText;
			if (!ReadFileToString(PathToU8(filePath), jsonText))
			{
				mErrors.push_back("Failed to read projectile file: " + PathToU8(filePath));
				continue;
			}

			JsonParser parser(jsonText);
			JsonValue root;
			std::string error;
			if (!parser.Parse(root, error))
			{
				mErrors.push_back("Parse error in " + PathToU8(filePath) + ": " + error);
				continue;
			}

			if (root.type == JsonType::Object)
			{
				ModProjectileDef projDef;
				if (!GetObjectString(root, "id", projDef.id, true, error))
				{
					mErrors.push_back(PathToU8(filePath) + ": " + error);
					continue;
				}
				GetObjectInt(root, "damage", projDef.damage, false, error);
				GetObjectFloat(root, "speed", projDef.speed, false, error);
				GetObjectString(root, "image", projDef.imageName, false, error);

				std::string outError;
				if (!gModRegistry.RegisterProjectile(projDef, &outError))
				{
					mErrors.push_back("Projectile registration failed: " + outError);
					continue;
				}
			}
			else if (root.type == JsonType::Array)
			{
				for (const JsonValue& item : root.arrayValue)
				{
					if (item.type != JsonType::Object)
					{
						mErrors.push_back("Projectile array item must be object in " + PathToU8(filePath));
						continue;
					}

					ModProjectileDef projDef;
					if (!GetObjectString(item, "id", projDef.id, true, error))
					{
						mErrors.push_back(PathToU8(filePath) + ": " + error);
						continue;
					}
					GetObjectInt(item, "damage", projDef.damage, false, error);
					GetObjectFloat(item, "speed", projDef.speed, false, error);
					GetObjectString(item, "image", projDef.imageName, false, error);

					std::string outError;
					if (!gModRegistry.RegisterProjectile(projDef, &outError))
					{
						mErrors.push_back("Projectile registration failed: " + outError);
						continue;
					}
				}
			}
			else
			{
				mErrors.push_back("Projectile file root must be object or array: " + PathToU8(filePath));
			}
		}
	}

	return true;
}

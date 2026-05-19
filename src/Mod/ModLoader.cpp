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
#include <map>
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

	mManifests.push_back(std::move(manifest));
	return true;
}



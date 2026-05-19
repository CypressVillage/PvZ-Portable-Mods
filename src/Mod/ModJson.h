/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __MODJSON_H__
#define __MODJSON_H__

#include <map>
#include <string>
#include <vector>

enum class ModJsonType
{
	Null,
	Bool,
	Number,
	String,
	Array,
	Object
};

struct ModJsonValue
{
	ModJsonType type = ModJsonType::Null;
	bool boolValue = false;
	int numberValue = 0;
	std::string stringValue;
	std::vector<ModJsonValue> arrayValue;
	std::map<std::string, ModJsonValue> objectValue;
};

class ModJsonParser
{
public:
	explicit ModJsonParser(const std::string& input);

	bool Parse(ModJsonValue& outValue, std::string& outError);

private:
	bool ParseValue(ModJsonValue& outValue, std::string& outError);
	bool ParseObject(ModJsonValue& outValue, std::string& outError);
	bool ParseArray(ModJsonValue& outValue, std::string& outError);
	bool ParseString(ModJsonValue& outValue, std::string& outError);
	bool ParseNumber(ModJsonValue& outValue, std::string& outError);
	bool Consume(char expected, std::string& outError);
	bool TryConsume(char expected);
	bool StartsWith(const char* text) const;
	void SkipWhitespace();

	const std::string& mInput;
	size_t mPos = 0;
};

bool ModJsonGetObjectString(const ModJsonValue& object, const char* key, std::string& outValue, bool required, std::string& outError);
bool ModJsonGetObjectInt(const ModJsonValue& object, const char* key, int& outValue, bool required, std::string& outError);
bool ModJsonGetStringArray(const ModJsonValue& value, std::vector<std::string>& outValues, std::string& outError);

std::string ModJsonEscape(const std::string& s);
std::string ModJsonUnescape(const std::string& s);
std::string ModJsonSerializeMap(const std::map<std::string, std::string>& data);
bool ModJsonDeserializeMap(const std::string& json, std::map<std::string, std::string>& outMap);
bool ModJsonReadFile(const std::string& filePath, std::string& outText);

#endif

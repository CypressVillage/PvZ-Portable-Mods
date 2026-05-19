/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModJson.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>

ModJsonParser::ModJsonParser(const std::string& input)
	: mInput(input)
{
}

bool ModJsonParser::Parse(ModJsonValue& outValue, std::string& outError)
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

bool ModJsonParser::ParseValue(ModJsonValue& outValue, std::string& outError)
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
		outValue.type = ModJsonType::Bool;
		outValue.boolValue = true;
		return true;
	}
	if (StartsWith("false"))
	{
		mPos += 5;
		outValue.type = ModJsonType::Bool;
		outValue.boolValue = false;
		return true;
	}
	if (StartsWith("null"))
	{
		mPos += 4;
		outValue.type = ModJsonType::Null;
		return true;
	}

	outError = "Invalid JSON token";
	return false;
}

bool ModJsonParser::ParseObject(ModJsonValue& outValue, std::string& outError)
{
	if (!Consume('{', outError))
		return false;

	outValue.type = ModJsonType::Object;
	SkipWhitespace();
	if (TryConsume('}'))
		return true;

	while (true)
	{
		ModJsonValue keyValue;
		if (!ParseString(keyValue, outError))
			return false;
		SkipWhitespace();
		if (!Consume(':', outError))
			return false;

		ModJsonValue value;
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

bool ModJsonParser::ParseArray(ModJsonValue& outValue, std::string& outError)
{
	if (!Consume('[', outError))
		return false;

	outValue.type = ModJsonType::Array;
	SkipWhitespace();
	if (TryConsume(']'))
		return true;

	while (true)
	{
		ModJsonValue item;
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

bool ModJsonParser::ParseString(ModJsonValue& outValue, std::string& outError)
{
	if (!Consume('"', outError))
		return false;

	std::string result;
	while (mPos < mInput.size())
	{
		char c = mInput[mPos++];
		if (c == '"')
		{
			outValue.type = ModJsonType::String;
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

bool ModJsonParser::ParseNumber(ModJsonValue& outValue, std::string& outError)
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
	outValue.type = ModJsonType::Number;
	outValue.numberValue = value;
	return true;
}

bool ModJsonParser::Consume(char expected, std::string& outError)
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

bool ModJsonParser::TryConsume(char expected)
{
	SkipWhitespace();
	if (mPos < mInput.size() && mInput[mPos] == expected)
	{
		++mPos;
		return true;
	}
	return false;
}

bool ModJsonParser::StartsWith(const char* text) const
{
	size_t len = std::strlen(text);
	if (mPos + len > mInput.size())
		return false;
	return mInput.compare(mPos, len, text) == 0;
}

void ModJsonParser::SkipWhitespace()
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

bool ModJsonGetObjectString(const ModJsonValue& object, const char* key, std::string& outValue, bool required, std::string& outError)
{
	auto it = object.objectValue.find(key);
	if (it == object.objectValue.end())
	{
		if (required)
			outError = std::string("Missing required field: ") + key;
		return !required;
	}
	if (it->second.type != ModJsonType::String)
	{
		outError = std::string("Field is not string: ") + key;
		return false;
	}
	outValue = it->second.stringValue;
	return true;
}

bool ModJsonGetObjectInt(const ModJsonValue& object, const char* key, int& outValue, bool required, std::string& outError)
{
	auto it = object.objectValue.find(key);
	if (it == object.objectValue.end())
	{
		if (required)
			outError = std::string("Missing required field: ") + key;
		return !required;
	}
	if (it->second.type != ModJsonType::Number)
	{
		outError = std::string("Field is not number: ") + key;
		return false;
	}
	outValue = it->second.numberValue;
	return true;
}

bool ModJsonGetStringArray(const ModJsonValue& value, std::vector<std::string>& outValues, std::string& outError)
{
	if (value.type != ModJsonType::Array)
	{
		outError = "Expected array";
		return false;
	}
	outValues.clear();
	for (const ModJsonValue& item : value.arrayValue)
	{
		if (item.type != ModJsonType::String)
		{
			outError = "Array item is not string";
			return false;
		}
		outValues.push_back(item.stringValue);
	}
	return true;
}

std::string ModJsonEscape(const std::string& s)
{
	std::string out;
	out.reserve(s.size() + 2);
	for (char c : s)
	{
		switch (c)
		{
		case '"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\b': out += "\\b"; break;
		case '\f': out += "\\f"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default: out += c;
		}
	}
	return out;
}

std::string ModJsonUnescape(const std::string& s)
{
	std::string out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size(); i++)
	{
		if (s[i] == '\\' && i + 1 < s.size())
		{
			switch (s[++i])
			{
			case '"': out += '"'; break;
			case '\\': out += '\\'; break;
			case '/': out += '/'; break;
			case 'b': out += '\b'; break;
			case 'f': out += '\f'; break;
			case 'n': out += '\n'; break;
			case 'r': out += '\r'; break;
			case 't': out += '\t'; break;
			case 'u':
				{
					if (i + 4 < s.size())
					{
						std::string hex = s.substr(i + 1, 4);
						char* end = nullptr;
						long cp = std::strtol(hex.c_str(), &end, 16);
						if (cp > 0 && cp < 128)
							out += static_cast<char>(cp);
						i += 4;
					}
					break;
				}
			default: out += s[i];
			}
		}
		else
		{
			out += s[i];
		}
	}
	return out;
}

std::string ModJsonSerializeMap(const std::map<std::string, std::string>& data)
{
	std::ostringstream ss;
	ss << "{\n";
	bool first = true;
	for (const auto& kv : data)
	{
		if (!first)
			ss << ",\n";
		first = false;
		ss << "\t\"" << ModJsonEscape(kv.first) << "\": \"" << ModJsonEscape(kv.second) << "\"";
	}
	ss << "\n}\n";
	return ss.str();
}

bool ModJsonDeserializeMap(const std::string& json, std::map<std::string, std::string>& outMap)
{
	outMap.clear();

	auto trim = [](const std::string& s) -> std::string {
		size_t start = 0;
		while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n'))
			start++;
		size_t end = s.size();
		while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n'))
			end--;
		return s.substr(start, end - start);
	};

	std::string content = trim(json);
	if (content.empty() || content[0] != '{')
		return false;

	size_t pos = 1;
	while (pos < content.size() && content[pos] != '}')
	{
		while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t' || content[pos] == '\n' || content[pos] == '\r' || content[pos] == ','))
			pos++;

		if (pos >= content.size() || content[pos] == '}')
			break;

		if (content[pos] != '"')
			return false;
		pos++;

		size_t keyStart = pos;
		while (pos < content.size() && content[pos] != '"')
		{
			if (content[pos] == '\\') pos++;
			pos++;
		}
		if (pos >= content.size()) return false;
		std::string key = ModJsonUnescape(content.substr(keyStart, pos - keyStart));
		pos++;

		while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t' || content[pos] == '\n' || content[pos] == '\r'))
			pos++;
		if (pos >= content.size() || content[pos] != ':') return false;
		pos++;

		while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t' || content[pos] == '\n' || content[pos] == '\r'))
			pos++;
		if (pos >= content.size() || content[pos] != '"') return false;
		pos++;

		size_t valStart = pos;
		while (pos < content.size() && content[pos] != '"')
		{
			if (content[pos] == '\\') pos++;
			pos++;
		}
		if (pos >= content.size()) return false;
		std::string value = ModJsonUnescape(content.substr(valStart, pos - valStart));
		pos++;

		outMap[key] = value;
	}

	return true;
}

bool ModJsonReadFile(const std::string& filePath, std::string& outText)
{
	std::ifstream file(filePath, std::ios::in | std::ios::binary);
	if (!file)
		return false;
	std::ostringstream ss;
	ss << file.rdbuf();
	outText = ss.str();
	return true;
}

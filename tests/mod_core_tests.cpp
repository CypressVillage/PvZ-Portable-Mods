#include "Mod/ModJson.h"
#include "Mod/ModLoader.h"
#include "Mod/ModSave.h"
#include "SexyAppFramework/Common.h"

#include <lua.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{
	int gFailures = 0;

	void Check(bool condition, const std::string& message)
	{
		if (!condition)
		{
			std::cerr << "FAIL: " << message << '\n';
			gFailures++;
		}
	}

	bool Contains(const std::vector<std::string>& values, const std::string& text)
	{
		for (const std::string& value : values)
		{
			if (value.find(text) != std::string::npos)
				return true;
		}
		return false;
	}

	struct TempDir
	{
		std::filesystem::path path;

		TempDir()
		{
			auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
			path = std::filesystem::temp_directory_path() / ("pvz-mod-tests-" + std::to_string(stamp));
			std::filesystem::create_directories(path);
		}

		~TempDir()
		{
			std::error_code error;
			std::filesystem::remove_all(path, error);
		}
	};

	void WriteFile(const std::filesystem::path& path, const std::string& contents)
	{
		std::filesystem::create_directories(path.parent_path());
		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		file << contents;
	}

	std::string ReadFile(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}

	void AddManifest(const std::filesystem::path& root, const std::string& directory, const std::string& json)
	{
		WriteFile(root / "mods" / directory / "mod.json", json);
	}

	void TestJson()
	{
		ModJsonValue value;
		std::string error;
		ModJsonParser parser(R"({"id":"demo","priority":2,"dependencies":["base"],"enabled":true})");
		Check(parser.Parse(value, error), "representative JSON parses");
		Check(value.type == ModJsonType::Object, "JSON root is an object");
		Check(value.objectValue["priority"].numberValue == 2, "JSON integer is retained");

		for (const std::string& invalid : {std::string(), std::string("{"), std::string("{\"a\":1,}"), std::string("[] trailing")})
		{
			ModJsonParser invalidParser(invalid);
			Check(!invalidParser.Parse(value, error), "invalid JSON is rejected: " + invalid);
		}

		std::map<std::string, std::string> original = {{"key\"\\", "line\n\tvalue"}, {"other", "two"}};
		std::map<std::string, std::string> decoded;
		Check(ModJsonDeserializeMap(ModJsonSerializeMap(original), decoded), "save map round-trips");
		Check(decoded == original, "save map preserves escaped strings");
		decoded["stale"] = "value";
		Check(!ModJsonDeserializeMap(R"({"key":1})", decoded), "save map rejects non-string values");
		Check(decoded.empty(), "failed save map parse clears output");
		Check(!ModJsonDeserializeMap(R"({"key":"value"} trailing)", decoded), "save map rejects trailing data");
	}

	void TestIds()
	{
		for (const std::string& valid : {"a", "Demo.Mod_2-test", "123"})
			Check(ModIsValidId(valid), "valid Mod id accepted: " + valid);
		for (const std::string& invalid : {"", "a/b", "a\\b", "a b", "a:b", "m\xC3\xB6" "d"})
			Check(!ModIsValidId(invalid), "invalid Mod id rejected: " + invalid);
	}

	void TestLoader()
	{
		TempDir temp;
		Sexy::SetResourceFolder(Sexy::PathToU8(temp.path));
		ModLoader loader;
		Check(loader.LoadAll(), "missing Mods directory succeeds");
		Check(loader.GetManifests().empty(), "missing Mods directory loads nothing");

		AddManifest(temp.path, "dependent", R"({"id":"dependent","priority":-10,"dependencies":["base"]})");
		AddManifest(temp.path, "base", R"({"id":"base","priority":20})");
		AddManifest(temp.path, "alpha", R"({"id":"alpha","priority":0})");
		AddManifest(temp.path, "beta", R"({"id":"beta","priority":0,"dependencies":["missing"]})");
		Check(loader.LoadAll(), "valid dependency graph loads");
		std::vector<std::string> ids;
		for (const ModManifest& manifest : loader.GetManifests())
			ids.push_back(manifest.id);
		Check(ids == std::vector<std::string>({"alpha", "beta", "base", "dependent"}),
			"loader orders ready Mods by priority and id while preserving dependencies");

		AddManifest(temp.path, "invalid", R"({"id":"../bad"})");
		Check(!loader.LoadAll(), "invalid manifest id fails the load");
		Check(Contains(loader.GetErrors(), "id may contain only"), "invalid id reports a useful error");
		std::filesystem::remove_all(temp.path / "mods" / "invalid");

		AddManifest(temp.path, "wrong-type", R"({"id":"wrong-type","entry":7})");
		Check(!loader.LoadAll(), "wrong optional field type fails the load");
		Check(Contains(loader.GetErrors(), "entry"), "wrong optional field names the field");
		std::filesystem::remove_all(temp.path / "mods" / "wrong-type");

		AddManifest(temp.path, "duplicate", R"({"id":"alpha"})");
		Check(!loader.LoadAll(), "duplicate Mod id fails the load");
		Check(Contains(loader.GetErrors(), "duplicate Mod id"), "duplicate id reports a useful error");
		std::filesystem::remove_all(temp.path / "mods" / "duplicate");

		AddManifest(temp.path, "cycle-a", R"({"id":"cycle-a","dependencies":["cycle-b"]})");
		AddManifest(temp.path, "cycle-b", R"({"id":"cycle-b","dependencies":["cycle-a"]})");
		Check(!loader.LoadAll(), "circular dependency fails the load");
		Check(Contains(loader.GetErrors(), "Circular dependency"), "cycle reports a useful error");
		for (const ModManifest& manifest : loader.GetManifests())
			Check(manifest.id != "cycle-a" && manifest.id != "cycle-b", "cyclic Mods are omitted");
	}

	void TestSave()
	{
		TempDir temp;
		Sexy::SetAppDataFolder(Sexy::PathToU8(temp.path));
		Check(ModSave::GetModSavePath("valid.id") == Sexy::PathToU8(temp.path / "modsave" / "valid.id.json"),
			"Mod save path stays inside app data");
		Check(ModSave::GetModSavePath("../bad").empty(), "invalid save id has no path");
		Check(!ModSave::SaveValue("../bad", "key", "value"), "invalid save id is rejected");
		Check(!ModSave::SaveValue("valid", "", "value"), "empty save key is rejected");

		Check(ModSave::SaveValue("valid", "one", "first"), "first value saves");
		Check(ModSave::SaveValue("valid", "two", "line\n\tvalue"), "second value saves");
		std::string value;
		Check(ModSave::LoadValue("valid", "one", value) && value == "first", "first value is preserved");
		Check(ModSave::LoadValue("valid", "two", value) && value == "line\n\tvalue", "escaped value round-trips");

		std::filesystem::path savePath = temp.path / "modsave" / "corrupt.json";
		WriteFile(savePath, "{broken");
		Check(!ModSave::SaveValue("corrupt", "key", "value"), "corrupt save is not overwritten");
		Check(ReadFile(savePath) == "{broken", "corrupt save bytes are preserved");
		Check(!std::filesystem::exists(savePath.string() + ".tmp"), "failed save leaves no temporary file");

		value = "stale";
		Check(!ModSave::LoadValue("missing", "key", value) && value.empty(), "missing value clears output");
	}

	void TestPlantHelper(const std::filesystem::path& sourceRoot)
	{
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		std::filesystem::path helper = sourceRoot / "src" / "Mod" / "Scripts" / "plant_helper.lua";
		std::filesystem::path tests = sourceRoot / "tests" / "plant_helper_tests.lua";
		if (luaL_dofile(L, Sexy::PathToU8(helper).c_str()) != LUA_OK ||
			luaL_dofile(L, Sexy::PathToU8(tests).c_str()) != LUA_OK)
		{
			const char* error = lua_tostring(L, -1);
			Check(false, std::string("PlantHelper Lua tests: ") + (error ? error : "unknown error"));
		}
		lua_close(L);
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "usage: pvz-mod-tests <source-root>\n";
		return 2;
	}

	TestJson();
	TestIds();
	TestLoader();
	TestSave();
	TestPlantHelper(std::filesystem::path(argv[1]));

	if (gFailures != 0)
	{
		std::cerr << gFailures << " test(s) failed\n";
		return 1;
	}
	std::cout << "All Mod framework tests passed\n";
	return 0;
}

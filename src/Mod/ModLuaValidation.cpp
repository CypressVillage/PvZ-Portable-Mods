/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModLuaValidation.h"

#include <fstream>
#include <sstream>

#if defined(PVZ_ENABLE_LUA)
#include <lua.hpp>
#endif

bool ModLuaValidateEntry(const std::string& path, const std::string& chunkName, std::string& outError)
{
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file)
	{
		outError = "Lua entry missing for Mod '" + chunkName + "': " + path;
		return false;
	}
	std::ostringstream stream;
	stream << file.rdbuf();
	std::string text = stream.str();

#if defined(PVZ_ENABLE_LUA)
	lua_State* L = luaL_newstate();
	if (L == nullptr)
	{
		outError = "Failed to create Lua validation state";
		return false;
	}
	if (luaL_loadbuffer(L, text.c_str(), text.size(), chunkName.c_str()) != 0)
	{
		const char* error = lua_tostring(L, -1);
		outError = "Lua syntax error in Mod '" + chunkName + "': " + (error ? error : "unknown error");
		lua_close(L);
		return false;
	}
	lua_close(L);
#else
	(void)text;
#endif
	outError.clear();
	return true;
}

# The same interpreter sources are compiled on every target: no JIT, dlopen,
# system Lua ABI or host executables (including on Web/iOS).
set(pvz_lua_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/Mod/lua")
file(GLOB LUA_SOURCES "${pvz_lua_SOURCE_DIR}/src/*.c")
list(REMOVE_ITEM LUA_SOURCES "${pvz_lua_SOURCE_DIR}/src/lua.c" "${pvz_lua_SOURCE_DIR}/src/luac.c")
add_library(pvz_lua STATIC ${LUA_SOURCES})
set_target_properties(pvz_lua PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(pvz_lua PUBLIC "${pvz_lua_SOURCE_DIR}/src")
if(UNIX)
	target_link_libraries(pvz_lua PUBLIC m)
endif()

find_package(nlohmann_json 3.12.0 CONFIG REQUIRED)

add_library(pvz_mod STATIC
	${PROJECT_SOURCE_DIR}/src/Mod/ModData.cpp
	${PROJECT_SOURCE_DIR}/src/Mod/ModRuntime.cpp
)
set_target_properties(pvz_mod PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_EXTENSIONS OFF)
target_compile_features(pvz_mod PUBLIC cxx_std_20)
target_compile_definitions(pvz_mod PUBLIC PVZ_MOD)
target_include_directories(pvz_mod PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(pvz_mod PUBLIC nlohmann_json::nlohmann_json PRIVATE pvz_lua)
if(EMSCRIPTEN)
	# The root's legacy global flags are set later; this target must retain
	# C++ exception handling for native bindings and fail-closed lifecycle.
	target_compile_options(pvz_mod PRIVATE -fexceptions)
endif()
if(MSVC)
	target_compile_options(pvz_mod PRIVATE /utf-8)
endif()

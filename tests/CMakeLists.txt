add_executable(pvz-mod-tests
	mod_core_tests.cpp
	mod_test_stubs.cpp
	${PROJECT_SOURCE_DIR}/src/Mod/ModConfig.cpp
	${PROJECT_SOURCE_DIR}/src/Mod/ModJson.cpp
	${PROJECT_SOURCE_DIR}/src/Mod/ModLoader.cpp
	${PROJECT_SOURCE_DIR}/src/Mod/ModLuaValidation.cpp
	${PROJECT_SOURCE_DIR}/src/Mod/ModSave.cpp
)

target_include_directories(pvz-mod-tests PRIVATE
	${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/src/SexyAppFramework
	${LUA_INCLUDE_DIR}
)
target_link_libraries(pvz-mod-tests PRIVATE ${LUA_LIBRARIES})
target_compile_definitions(pvz-mod-tests PRIVATE PVZ_ENABLE_LUA)
target_compile_features(pvz-mod-tests PRIVATE cxx_std_20)
set_target_properties(pvz-mod-tests PROPERTIES CXX_EXTENSIONS OFF)

add_test(
	NAME pvz.mod
	COMMAND pvz-mod-tests ${PROJECT_SOURCE_DIR}
)
set_tests_properties(pvz.mod PROPERTIES TIMEOUT 30)

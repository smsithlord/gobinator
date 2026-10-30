# Gobinator: v1.1 in progress (src/), and v1.0 as it was (src/v1.0/) for comparison tests.
#
#   cmake -G "Visual Studio 17 2022" -A Win32 -B build
#   cmake --build build --config Release        -> build/Release/gobinator.exe, gobinator-v1.0.exe
#   ctest --test-dir build -C Release           -> the unit tests
cmake_minimum_required(VERSION 3.20)
project(gobinator C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(UNICODE _UNICODE)

# The static runtime, so the EXEs run without a redistributable (v1.0 needed VS 2013's debug runtime DLLs).
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# v1.1
add_library(gobinator-core STATIC src/config.cpp src/files.cpp src/launch.cpp src/modfolder.cpp
	src/install.cpp src/gob.cpp)
target_link_libraries(gobinator-core PUBLIC normaliz)
# No console window. The .rc holds the icon, the packaged ini and the version; firstrun.cpp is the folder picker.
add_executable(gobinator WIN32 src/main.cpp src/firstrun.cpp src/gobinator.rc)
target_link_libraries(gobinator PRIVATE gobinator-core ole32 shell32 uuid)

# v1.0, unchanged: a 32-bit Unicode console app, as the installed one is (x86, console, _tmain -> wmain).
add_executable(gobinator-v1.0 src/v1.0/gobinator.cpp)
set_target_properties(gobinator-v1.0 PROPERTIES CXX_STANDARD 14)	# it uses <codecvt>, deprecated in C++17

# Tests
enable_testing()
# A stand-in for jk.exe / inject.exe that only logs how it was launched: the unit tests and
# tests/run-v1-scenarios.ps1 start it.
add_executable(fakejk WIN32 tests/fakejk.c)
# The same, with a .bind section: it passes for Steam's DRM'd JediKnight.exe (IsSteamStub).
add_executable(fakesteamjk WIN32 tests/fakejk.c)
target_compile_definitions(fakesteamjk PRIVATE FAKE_STEAM_STUB)

add_executable(gobinator-tests tests/tests_main.cpp tests/config_tests.cpp tests/launch_tests.cpp
	tests/modfolder_tests.cpp tests/gob_tests.cpp)
target_link_libraries(gobinator-tests PRIVATE gobinator-core)
target_compile_definitions(gobinator-tests PRIVATE
	GOBINATOR_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
	FAKEJK_PATH="$<TARGET_FILE:fakejk>"
	FAKESTEAMJK_PATH="$<TARGET_FILE:fakesteamjk>")
add_dependencies(gobinator-tests fakejk fakesteamjk)
add_test(NAME unit COMMAND gobinator-tests)

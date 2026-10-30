// Gobinator's unit tests. Run: ctest --test-dir build -C Release
// gobinator-tests --detect <folder> prints what DetectInstall finds there, and for OpenJKDF2 which of the autostart
// switches its EXE knows, and starts nothing: a read-only look at a real install.
#include "check.h"
#include "../src/files.h"
#include "../src/install.h"
#include "../src/launch.h"

#include <cwchar>
#include <string>

int failures = 0;

void RunConfigTests();
void RunLaunchTests();
void RunModFolderTests();
void RunGobTests();

int wmain(int argc, wchar_t* argv[])
{
	if (argc == 3 && std::wstring(argv[1]) == L"--detect") {
		Install install;
		if (!DetectInstall(argv[2], install)) {
			std::wprintf(L"no JK program found\n");
			return 1;
		}
		std::wprintf(L"%ls: %ls\n", KindName(install.kind), install.program.c_str());
		std::string exe;
		if (install.kind == InstallKind::OpenJkdf2 && ReadFileBytes(install.program, exe)) {
			std::set<std::string> known = KnownSwitches(exe);
			for (const char* s : { "-autostart", "-sp", "-episode", "-map", "-donotresumelast" })
				std::wprintf(L"  %hs: %ls\n", s, known.count(s) ? L"known" : L"not known");
		}
		else if (ReadFileBytes(install.program, exe)) {
			bool stub = IsSteamStub(exe);
			std::wprintf(L"  Steam's DRM (.bind): %ls\n", stub ? L"yes" : L"no");
			std::wstring steam = SteamJkFolder();
			if (stub)
				std::wprintf(L"  Steam starts: %ls (%ls)\n", steam.empty() ? L"(not registered)" : steam.c_str(),
					!steam.empty() && SameFolder(steam, argv[2]) ? L"this folder" : L"not this folder");
		}
		return 0;
	}

	RunConfigTests();
	RunLaunchTests();
	RunModFolderTests();
	RunGobTests();
	if (failures)
		std::fprintf(stderr, "%d check(s) failed\n", failures);
	else
		std::printf("unit tests: all passed\n");
	return failures ? 1 : 0;
}

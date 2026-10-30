// Gobinator v1.1: double-click an episode GOB to play it in Jedi Knight (docs/PLAN.md).
// No console window: a message only when something fails, and gobinator.log beside the EXE for the last run.
#include "config.h"
#include "files.h"
#include "firstrun.h"
#include "gob.h"
#include "install.h"
#include "launch.h"
#include "modfolder.h"
#include "version.h"

#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
	const wchar_t* const kVersion = GOBINATOR_WIDE(GOBINATOR_VERSION_TEXT);	// src/version.h

	// The last run, written to gobinator.log when Gobinator exits.
	struct Log
	{
		std::wstring text;
		void Line(const std::wstring& line) { text += line + L"\r\n"; }
	};

	std::wstring Now()
	{
		SYSTEMTIME t;
		GetLocalTime(&t);
		wchar_t s[32];
		swprintf_s(s, L"%04u-%02u-%02u %02u:%02u:%02u", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
		return s;
	}

	void LogPlacement(const Placement& placement, Log& log)
	{
		for (const auto& name : placement.removed)
			log.Line(L"removed " + name);
		for (const auto& note : placement.notes)
			log.Line(L"note: " + note);
	}

	// Steam's own JK (IsSteamStub) starts the install Steam registered, so a GOB copied into another folder's mod
	// would not be listed. Returns the message for the player when that is so, or nothing.
	std::wstring CheckSteam(const LaunchSpec& launch, const std::wstring& jkFolder, Log& log)
	{
		std::string exe;
		if (launch.kind == InstallKind::OpenJkdf2 || !ReadFileBytes(launch.program, exe) || !IsSteamStub(exe))
			return L"";
		std::wstring name = FileName(launch.program);
		std::wstring steamFolder = SteamJkFolder();
		if (steamFolder.empty() || !GetFileInfo(steamFolder).folder) {
			log.Line(L"note: " + name + L" is Steam's, which hands the start to Steam, and Steam's JK install could "
				L"not be found (\"Install Path\" in the registry), so Gobinator cannot check that Steam starts this "
				L"folder");
			return L"";
		}
		steamFolder = FullPath(steamFolder);
		if (SameFolder(steamFolder, jkFolder)) {
			log.Line(L"Steam's JK: Steam starts this folder");
			return L"";
		}
		return name + L" is Steam's: it hands every start to Steam, and Steam starts the JK in\n" + steamFolder +
			L"\nnot this one:\n" + jkFolder + L"\nThe episode would not be in its list. Put gobinator.exe and "
			L"gobinator.ini in Steam's JK folder, or set JK_FOLDER = " + steamFolder + L" in gobinator.ini.";
	}

	// One double-click. Returns the message for the player, or nothing when JK started.
	std::wstring Run(const std::vector<std::wstring>& args, const std::wstring& ownFolder, Log& log)
	{
		// The settings: gobinator.ini beside the EXE, or the packaged one's defaults when there is none (it is then
		// written once JK is found, so the player has the file to edit).
		std::wstring iniPath = JoinPath(ownFolder, L"gobinator.ini");
		std::string bytes;
		bool iniExists = ReadFileBytes(iniPath, bytes);
		if (!iniExists)
			bytes = PackagedIni();
		Config config = ParseConfig(bytes);
		log.Line(iniExists ? L"settings: " + iniPath + (config.legacyForm ? L" (v1.0's form)" : L"") :
			L"settings: no gobinator.ini beside gobinator.exe, so the defaults");
		for (const auto& note : config.notes)
			log.Line(L"note: " + note);
		if (!config.errors.empty()) {
			std::wstring message = L"gobinator.ini needs a fix (" + iniPath + L"):";
			for (const auto& error : config.errors)
				message += L"\n\n" + error;
			return message;
		}

		// Where JK is: JK_FOLDER, else Gobinator's own folder. When neither holds a JK program, the player is asked,
		// until a folder holds one or they give up, and the answer is kept as JK_FOLDER (docs/DECISIONS.md,
		// 2026-10-30). JK must be there before anything in the game's folders changes.
		std::wstring jkFolder = ResolveJkFolder(config, ownFolder);
		LaunchSpec launch;
		std::wstring error;
		bool planned = PlanLaunch(config, jkFolder, launch, error);
		bool writeIni = !iniExists;
		if (!planned && !config.jkExecutable) {
			log.Line(L"no JK program in " + jkFolder + L": asking where JK is");
			std::wstring steam = SteamJkFolder();
			std::wstring start = !steam.empty() && GetFileInfo(steam).folder ? steam : jkFolder;
			std::wstring chosen;
			while (PickJkFolder(start, chosen)) {
				chosen = FullPath(chosen);
				Install install;
				if (DetectInstall(chosen, install)) {
					log.Line(L"the player chose " + chosen);
					jkFolder = chosen;
					bytes = SetIniValue(bytes, L"JK_FOLDER", chosen);
					writeIni = true;
					planned = PlanLaunch(config, jkFolder, launch, error);
					break;
				}
				log.Line(L"the player chose " + chosen + L", which holds no JK program");
				if (!AskToPickAgain(chosen))
					break;
				start = chosen;
			}
			if (!planned)
				log.Line(L"no folder chosen");
		}
		if (!planned || !ProgramExists(launch, error))
			return error;
		log.Line(L"JK folder: " + jkFolder);
		log.Line(L"JK: " + std::wstring(KindName(launch.kind)) + (launch.detected ? L", found in the JK folder" :
			L", from JK_EXECUTABLE"));

		error = CheckSteam(launch, jkFolder, log);
		if (!error.empty())
			return error;

		if (writeIni) {
			if (WriteFileBytes(iniPath, bytes))
				log.Line(iniExists ? L"wrote JK_FOLDER = " + jkFolder + L" into " + iniPath :
					L"wrote " + iniPath + (jkFolder == ownFolder ? L" with the default settings" :
						L" with the default settings and JK_FOLDER = " + jkFolder));
			else
				log.Line(L"note: could not write " + iniPath + L", so Gobinator will ask again next time");
		}

		// The GOB goes into Episode, where it stays, and an empty folder of its name lists it first; JK starts with
		// -path <mod folder>, the player's own, which loads on top of it (docs/DECISIONS.md, 2026-10-27).
		std::wstring episodes = JoinPath(jkFolder, L"Episode");
		std::wstring modFolder = JoinPath(jkFolder, config.modFolder.value_or(L"mod"));
		if (!GetFileInfo(modFolder).exists && CreateDirectoryW(LongPath(modFolder).c_str(), nullptr))
			log.Line(L"made the mod folder " + modFolder + L", for your own GOBs");

		// Copies earlier v1.1 builds made in the mod folder: from there an episode hides every other one.
		Placement earlier;
		RemoveCopies(modFolder, earlier);
		LogPlacement(earlier, log);

		std::string episodeJk;
		GobEntry entry = GobEntry::Found;
		if (args.size() > 1) {
			if (!CheckGobFile(args[1], error))
				return error;
			entry = FindGobEntry(FullPath(args[1]), "episode.jk", episodeJk);
		}
		if (entry != GobEntry::Found) {
			// A GOB with no episode.jk is a mod, not an episode; one that cannot be read as a GOB cannot say which it
			// is. Either way it stays where it is, nothing in the game's folders changes, and JK starts at its menu as
			// usual (docs/DECISIONS.md, 2026-10-29).
			std::wstring name = FileName(FullPath(args[1]));
			log.Line(entry == GobEntry::Missing ?
				L"note: " + name + L" holds no episode.jk: it is a mod, not an episode. Gobinator left it where it is; "
					L"to use it, put it in " + modFolder :
				L"note: " + name + L" could not be read as a GOB (damaged, or still being written?). Gobinator left "
					L"it where it is");
			log.Line(L"no autostart: JK starts at its menu");
		}
		else if (args.size() > 1) {
			Placement placement;
			if (!PlaceGob(args[1], episodes, placement, error))
				return error;
			log.Line((placement.copied ? L"copied " : L"using ") + placement.name + L" in " + FullPath(episodes));
			std::wstring episode = EpisodeName(placement.name);
			bool listed = ListFirst(episodes, episode, placement, error);
			LogPlacement(placement, log);
			if (!listed)
				return error;

			std::wstring autostart;
			AddAutostart(launch, JoinPath(episodes, placement.name), episode, autostart);
			log.Line(autostart);
		}
		else {
			// gobinator.exe on its own is the clean-up: the folder that listed the last episode first comes out (the
			// GOB stays one of the episodes), and JK starts with its usual list.
			Placement cleanup;
			UnlistAll(episodes, cleanup);
			LogPlacement(cleanup, log);
			log.Line(cleanup.removed.empty() ? L"no GOB given, and nothing of Gobinator's to clean up" :
				L"no GOB given: JK starts with its usual episode list");
		}

		log.Line(L"starting " + launch.program + L" as: " + CommandLine(launch));
		bool elevated = false;
		if (!Launch(launch, error, &elevated))
			return error;
		log.Line(elevated ? L"started with admin rights, which Windows said it needs" : L"started");
		return L"";
	}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::vector<std::wstring> args(argv, argv + argc);
	LocalFree(argv);

	std::wstring ownFolder = OwnFolder();
	Log log;
	log.Line(L"Gobinator " + std::wstring(kVersion) + L", " + Now());
	log.Line(L"GOB: " + (args.size() > 1 ? args[1] : L"(none)"));
	std::wstring failure = Run(args, ownFolder, log);
	if (!failure.empty())
		log.Line(L"error: " + failure);

	std::wstring logPath = JoinPath(ownFolder, L"gobinator.log");
	bool logged = WriteFileBytes(logPath, "\xEF\xBB\xBF" + ToUtf8(log.text));
	if (failure.empty())
		return 0;

	// GOBINATOR_NO_MESSAGES keeps the message in the log only, for the scenario tests.
	if (GetEnvironmentVariableW(L"GOBINATOR_NO_MESSAGES", nullptr, 0) == 0) {
		std::wstring message = failure + (logged ? L"\n\nDetails: " + logPath : L"");
		MessageBoxW(nullptr, message.c_str(), L"Gobinator", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
	}
	return 1;
}

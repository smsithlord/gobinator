// Starting JK without a .bat (docs/PLAN.md, part 2).
#pragma once

#include "config.h"
#include "install.h"

#include <set>
#include <string>

// What to start and how.
struct LaunchSpec
{
	std::wstring program;		// the full path, so Windows never searches PATH for it
	std::wstring arguments;		// what follows the program's name: [options] -path <mod folder>
	std::wstring folder;		// the working directory: the JK folder
	InstallKind kind = InstallKind::Other;
	bool detected = false;		// found in the JK folder, rather than set as JK_EXECUTABLE
};

// The launch the settings ask for: JK_EXECUTABLE if set (relative to the JK folder unless absolute), else the program
// DetectInstall finds there; ADDITIONAL_LAUNCH_OPTIONS if set, else -devmode (which puts Debug in JK's menu, the way
// to a level v1.0 relied on). False, with error for the player, when there is nothing to start.
bool PlanLaunch(const Config& config, const std::wstring& jkFolder, LaunchSpec& spec, std::wstring& error);

// The switches an EXE holds, in lower case: each string in it that starts with '-' and stands on its own, a NUL
// before and after. That is how OpenJKDF2 keeps the switches it knows ("-autostart"); it compares them in any case,
// and quits with "Error in arguments" on a switch it does not know.
std::set<std::string> KnownSwitches(const std::string& exe);

// For OpenJKDF2: start straight into the GOB's first level, read from the episode.jk inside it, by adding
// "-autostart <mode> -episode <episode> -map <level>", when the EXE knows them (OpenJKDF2 v0.9.1 and later), and
// -doNotResumeLast when it knows that too (the user's fork, which would otherwise resume its last session instead).
// The mode follows the episode's TYPE: -sp for single player (1), -mp for deathmatch (2), which hosts a game on this
// PC with the player in it; any other TYPE (Capture the Flag, 8, included) opens at the menu (docs/DECISIONS.md,
// 2026-10-29). episode is the GOB's entry in JK's list (its name without .gob), which OpenJKDF2 takes as one word: no
// spaces. Retail JK has no such switches. False, with note saying why, when it does not apply or cannot be done; the
// launch then goes ahead without it.
bool AddAutostart(LaunchSpec& spec, const std::wstring& gobPath, const std::wstring& episode, std::wstring& note);

// The command line the program gets: its bare file name, then the arguments. That is what v1.0's START produced
// ("inject.exe -devmode -path mod"), so JkGfxMod's inject.exe and JK see the same line as before.
std::wstring CommandLine(const LaunchSpec& spec);

// Whether the program is there to start. If not, error says so, in words for the player.
bool ProgramExists(const LaunchSpec& spec, std::wstring& error);

// Starts the program and returns at once; elevated, if given, says whether Windows needed admin rights for it (GOG's
// JK.EXE does: Gobinator then starts it through Windows' prompt). On failure, error says why, in words for the player.
bool Launch(const LaunchSpec& spec, std::wstring& error, bool* elevated = nullptr);

// Windows' own text for an error code, without the trailing line break.
std::wstring SystemMessage(unsigned long code);

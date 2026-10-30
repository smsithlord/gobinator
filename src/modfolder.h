// Putting the double-clicked GOB where JK lists it first, and taking out only what Gobinator put there itself
// (docs/PLAN.md, parts 1 and 3; docs/DECISIONS.md, 2026-10-27).
//
// The GOB goes into JK's Episode folder, where it stays, and an empty folder of the same name beside it puts it first
// in JK's episode list: JK lists Episode's subfolders before its GOB files, and an entry named X loads Episode\X.gob
// as its episode GOB, so the folder also gets it past the list's limit (measured,
// tests/run-episode-folder-experiment.ps1). JK still starts with -path mod, so the player's own mods there load on
// top of it.
#pragma once

#include <string>
#include <vector>

// The file a record is kept in: in Episode, the folders Gobinator made there (one name a line); in the mod folder,
// the GOBs earlier v1.1 builds copied there (name, size and time written).
extern const wchar_t* const kStateFileName;

struct Placement
{
	std::wstring name;					// the GOB's file name in Episode: what JK loads
	bool copied = false;				// false when it was there already
	std::vector<std::wstring> removed;	// what was taken out: earlier builds' copies, or the folders of earlier episodes
	std::vector<std::wstring> notes;	// for the log
};

// The name a GOB is copied under: letters, digits, '.', '_' and '-' kept, accented letters as their plain letter,
// anything else '_' (JK opens files in the ANSI code page, and OpenJKDF2's -episode is one word), and at most 31
// characters before the extension (JK keeps an episode's name in 32 bytes).
std::wstring SafeGobName(const std::wstring& fileName);

// The GOB's name without its extension: its entry in JK's episode list, and what -episode names.
std::wstring EpisodeName(const std::wstring& gobName);

// Whether gobPath is a file named .gob. If not, error says so, in words for the player.
bool CheckGobFile(const std::wstring& gobPath, std::wstring& error);

// Makes the GOB at gobPath (CheckGobFile) one of the episodes in episodeFolder, which is created if missing:
// - a GOB already in that folder is used where it is;
// - otherwise it is copied in under SafeGobName and stays there;
// - a file of that name is never overwritten: an identical one is used as it is, a different one makes the copy take
//   another name (Episode_2.gob).
// On failure, error says why, in words for the player, and nothing has changed.
bool PlaceGob(const std::wstring& gobPath, const std::wstring& episodeFolder, Placement& placement,
	std::wstring& error);

// Puts the episode first in JK's list: makes the empty folder episodeFolder\<episode> if there is none (and records
// it), and removes every other folder Gobinator made there. A folder that was there already is the player's: it is
// used and never removed.
bool ListFirst(const std::wstring& episodeFolder, const std::wstring& episode, Placement& placement,
	std::wstring& error);

// Removes every folder Gobinator made in episodeFolder: what gobinator.exe does when it starts with no GOB. Only an
// empty folder is removed; one the player has put something in is kept and forgotten.
void UnlistAll(const std::wstring& episodeFolder, Placement& placement);

// Removes the GOBs an earlier v1.1 build copied into folder (the mod folder) and recorded there, while unchanged:
// from mod, an episode hides every other one.
void RemoveCopies(const std::wstring& folder, Placement& placement);

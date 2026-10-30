// gobinator.ini: the settings file beside gobinator.exe (docs/PLAN.md, part 4).
#pragma once

#include <optional>
#include <string>
#include <vector>

// The settings as read. A setting left out stays empty, so the caller can tell "not set" (use the default or detect
// it) from "set to nothing" (ADDITIONAL_LAUNCH_OPTIONS = with no value means no options at all).
struct Config
{
	std::optional<std::wstring> jkFolder;		// JK_FOLDER: absolute, or relative to Gobinator's own folder
	std::optional<std::wstring> modFolder;		// MOD_FOLDER: a folder inside the JK folder, passed as -path
	std::optional<std::wstring> jkExecutable;	// JK_EXECUTABLE: the program to start, in the JK folder or a full path
	std::optional<std::wstring> launchOptions;	// ADDITIONAL_LAUNCH_OPTIONS: put before -path

	bool legacyForm = false;					// v1.0's form: a line with the key, then a line with its value
	std::vector<std::wstring> notes;			// what was skipped or guessed, for the log
	std::vector<std::wstring> errors;			// what makes the settings unusable, for the player
};

// Reads the bytes of a gobinator.ini. Takes either form (key = value, or v1.0's pairs of lines), in UTF-8 with or
// without a BOM, UTF-16 with a BOM, or the Windows code page.
Config ParseConfig(const std::string& bytes);

// The JK folder: JK_FOLDER (relative to Gobinator's own folder unless absolute), else Gobinator's own folder.
std::wstring ResolveJkFolder(const Config& config, const std::wstring& ownFolder);

// The bytes of a gobinator.ini with one setting set: in key = value form the setting's line is replaced (the last one
// set, else the commented-out example, so its note stays above it), or appended; in v1.0's form the value line after
// the key is replaced, or inserted, or key and value are appended. The file keeps its form and its line endings and
// comes back as UTF-8 with a BOM, whatever it was read in. For the folder picker's answer (docs/DECISIONS.md,
// 2026-10-30).
std::string SetIniValue(const std::string& bytes, const std::wstring& key, const std::wstring& value);

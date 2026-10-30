// A fresh install: the folder picker for when no JK program is found where Gobinator looks, and the gobinator.ini the
// EXE carries for when there is none beside it (docs/DECISIONS.md, 2026-10-30).
#pragma once

#include <string>

// Windows' folder picker, titled "Gobinator: where is your Jedi Knight folder?", opened on startFolder when it exists.
// chosen is the folder picked; false when the player cancels. Under GOBINATOR_NO_MESSAGES (the scenario tests) no
// dialog opens: GOBINATOR_PICK_FOLDER is the answer, and unset or empty means cancel.
bool PickJkFolder(const std::wstring& startFolder, std::wstring& chosen);

// After a folder with no JK program in it: says so, names what Gobinator looks for, and asks whether to choose again.
// True to pick again. Under GOBINATOR_NO_MESSAGES: false.
bool AskToPickAgain(const std::wstring& folder);

// The gobinator.ini shipped in the zip (package/gobinator.ini), built into the EXE (IDR_PACKAGED_INI): every setting
// a comment, so the defaults apply.
std::string PackagedIni();

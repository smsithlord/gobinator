// Telling the JK installs apart (docs/PLAN.md, part 5).
#pragma once

#include <string>

enum class InstallKind { Remastered, OpenJkdf2, Steam, Retail, Other };

// A JK install's program: what to start, and the kind of install it belongs to.
struct Install
{
	InstallKind kind = InstallKind::Other;
	std::wstring program;		// the full path, with the file name as it is on disk
};

// Looks in the JK folder for, in this order: JkGfxMod's inject.exe with jkgm.json beside it (JK Remastered),
// openjkdf2-64.exe or another openjkdf2*.exe (OpenJKDF2), JediKnight.exe (the Steam release, or a patch of it), JK.EXE
// (retail).
// False when there is none of them.
bool DetectInstall(const std::wstring& jkFolder, Install& install);

// The kind of install a program belongs to, by its file name: for a JK_EXECUTABLE the player set.
InstallKind KindOf(const std::wstring& program);

// For the log.
const wchar_t* KindName(InstallKind kind);

// Whether an EXE is wrapped in Steam's DRM (SteamStub, which adds a PE section named .bind), as Steam's JediKnight.exe
// is. Such an EXE does not run where it is: it hands the start to Steam, which runs the JK install Steam registered,
// with the same switches (measured 2026-10-27). The user's community-patched JediKnight.exe has no .bind and runs
// where it is.
bool IsSteamStub(const std::string& exe);

// The folder of the JK install Steam starts: "Install Path" in HKLM\SOFTWARE\WOW6432Node\LucasArts Entertainment
// Company\JediKnight\1.0, which Steam's install script writes. Empty when there is none.
std::wstring SteamJkFolder();

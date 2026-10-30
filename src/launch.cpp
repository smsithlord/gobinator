#include "launch.h"
#include "files.h"
#include "gob.h"

#include <windows.h>
#include <shellapi.h>
#include <algorithm>

bool PlanLaunch(const Config& config, const std::wstring& jkFolder, LaunchSpec& spec, std::wstring& error)
{
	if (config.jkExecutable) {
		spec.program = FullPath(JoinPath(jkFolder, *config.jkExecutable));
		spec.kind = KindOf(spec.program);
	}
	else {
		Install install;
		if (!DetectInstall(jkFolder, install)) {
			error = L"No JK program was found in " + jkFolder + L". Gobinator looks for inject.exe (with jkgm.json), "
				L"openjkdf2-64.exe, JediKnight.exe and JK.EXE. Double-click the GOB again and choose your JK folder "
				L"when asked, or put gobinator.exe and gobinator.ini in that folder, or set JK_FOLDER or "
				L"JK_EXECUTABLE in gobinator.ini.";
			return false;
		}
		spec.program = install.program;
		spec.kind = install.kind;
		spec.detected = true;
	}
	std::wstring options = config.launchOptions.value_or(L"-devmode");
	spec.arguments = (options.empty() ? L"" : options + L" ") + L"-path " + config.modFolder.value_or(L"mod");
	spec.folder = jkFolder;
	return true;
}

std::set<std::string> KnownSwitches(const std::string& exe)
{
	std::set<std::string> switches;
	for (size_t at = exe.find("\0-", 0, 2); at != std::string::npos; at = exe.find("\0-", at + 1, 2)) {
		size_t end = exe.find('\0', at + 1);
		if (end == std::string::npos || end - at < 3 || end - at > 64)	// "-x" to a long switch
			continue;
		std::string s = exe.substr(at + 1, end - at - 1);
		if (!std::all_of(s.begin(), s.end(), [](char c) { return c > ' ' && c < 0x7F; }))
			continue;
		std::transform(s.begin(), s.end(), s.begin(), [](char c) { return (char)tolower((unsigned char)c); });
		switches.insert(s);
	}
	return switches;
}

bool AddAutostart(LaunchSpec& spec, const std::wstring& gobPath, const std::wstring& episode, std::wstring& note)
{
	if (spec.kind != InstallKind::OpenJkdf2) {
		note = L"no autostart: only OpenJKDF2 can start a level from its command line";
		return false;
	}
	std::set<std::string> known;
	{
		std::string exe;
		if (!ReadFileBytes(spec.program, exe)) {
			note = L"no autostart: " + FileName(spec.program) + L" could not be read to see which switches it knows";
			return false;
		}
		known = KnownSwitches(exe);
	}
	for (const char* s : { "-autostart", "-episode", "-map" })
		if (!known.count(s)) {
			note = L"no autostart: " + FileName(spec.program) + L" does not know -autostart, -episode and -map "
				L"(OpenJKDF2 v0.9.1 and later do)";
			return false;
		}
	// OpenJKDF2 splits its command line at spaces and has no quotes, and quits on a word it does not know.
	std::wstring name = FileName(gobPath);
	if (episode.empty() || episode.find_first_of(L" \t") != std::wstring::npos) {
		note = L"no autostart: OpenJKDF2 cannot take an episode name with a space (" + name + L"); it is first in "
			L"New Game's list instead";
		return false;
	}
	std::string episodeJk;
	if (!ReadGobEntry(gobPath, "episode.jk", episodeJk)) {
		note = L"no autostart: " + name + L" holds no episode.jk (not an episode GOB?)";
		return false;
	}
	// Single player or deathmatch, alone; anything else is first in the episode list at the menu instead.
	int type = EpisodeType(episodeJk);
	const wchar_t* mode = type == 1 ? L"-sp" : type == 2 ? L"-mp" : nullptr;
	if (!mode) {
		note = L"no autostart: " + name + (type == 8 ? L" is a Capture the Flag episode (TYPE 8)" :
			L"'s episode.jk says TYPE " + std::to_wstring(type) + L", not 1 (single player) or 2 (multiplayer)") +
			L"; it is first in the episode list instead";
		return false;
	}
	if (!known.count(type == 1 ? "-sp" : "-mp")) {
		note = L"no autostart: " + FileName(spec.program) + L" does not know " + mode;
		return false;
	}
	std::string level = FirstLevel(episodeJk);
	if (level.empty()) {
		note = L"no autostart: " + name + L"'s episode.jk names no level";
		return false;
	}

	std::wstring map(level.begin(), level.end());	// episode.jk is 8-bit text, and JK level names are ASCII
	spec.arguments += L" -autostart " + std::wstring(mode) + L" -episode " + episode + L" -map " + map;
	if (known.count("-donotresumelast"))
		spec.arguments += L" -doNotResumeLast";
	note = L"autostart: episode " + episode + L" (" + name + L"), level " + map +
		(type == 1 ? L", single player" : L", multiplayer: a game hosted on this PC");
	return true;
}

std::wstring CommandLine(const LaunchSpec& spec)
{
	std::wstring name = FileName(spec.program);
	if (name.find_first_of(L" \t") != std::wstring::npos)
		name = L"\"" + name + L"\"";
	return spec.arguments.empty() ? name : name + L" " + spec.arguments;
}

std::wstring SystemMessage(unsigned long code)
{
	wchar_t* text = nullptr;
	DWORD n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, code, 0, (LPWSTR)&text, 0, nullptr);
	std::wstring message = n ? std::wstring(text, n) : L"error " + std::to_wstring(code);
	LocalFree(text);
	while (!message.empty() && (message.back() == L'\n' || message.back() == L'\r' || message.back() == L' '))
		message.pop_back();
	return message;
}

bool ProgramExists(const LaunchSpec& spec, std::wstring& error)
{
	FileInfo info = GetFileInfo(spec.program);
	if (info.exists && !info.folder)
		return true;
	error = L"JK was not found: " + spec.program + L" does not exist. Set JK_EXECUTABLE (or JK_FOLDER) in "
		L"gobinator.ini.";
	return false;
}

bool Launch(const LaunchSpec& spec, std::wstring& error, bool* elevated)
{
	if (elevated)
		*elevated = false;
	if (!ProgramExists(spec, error))
		return false;

	// Gobinator was started by the player's double-click, so it may hand the foreground on. Without this, JK can
	// come up behind other windows and wait for a click (seen once, docs/OVERVIEW.md).
	AllowSetForegroundWindow(ASFW_ANY);

	std::wstring commandLine = CommandLine(spec);
	STARTUPINFOW startup = { sizeof(startup) };
	PROCESS_INFORMATION process = {};
	if (CreateProcessW(spec.program.c_str(), &commandLine[0], nullptr, nullptr, FALSE, 0, nullptr,
		spec.folder.c_str(), &startup, &process)) {
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
		return true;
	}

	DWORD code = GetLastError();
	if (code == ERROR_ELEVATION_REQUIRED) {
		// The program asks for admin rights: let Windows show its prompt.
		SHELLEXECUTEINFOW info = { sizeof(info) };
		info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
		info.lpFile = spec.program.c_str();
		info.lpParameters = spec.arguments.c_str();
		info.lpDirectory = spec.folder.c_str();
		info.nShow = SW_SHOWNORMAL;
		if (ShellExecuteExW(&info)) {
			if (elevated)
				*elevated = true;
			return true;
		}
		code = GetLastError();
	}
	error = L"JK could not be started (" + spec.program + L"): " + SystemMessage(code);
	return false;
}

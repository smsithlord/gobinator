#include "install.h"
#include "files.h"

#include <windows.h>

namespace
{
	bool SameName(const std::wstring& a, const std::wstring& b)
	{
		return CompareStringOrdinal(a.c_str(), (int)a.size(), b.c_str(), (int)b.size(), TRUE) == CSTR_EQUAL;
	}

	bool EndsWith(const std::wstring& s, const wchar_t* tail)
	{
		size_t n = wcslen(tail);
		return s.size() >= n && SameName(s.substr(s.size() - n), tail);
	}

	// The first file in folder matching pattern whose name ends with tail, as it is spelt on disk; empty if none.
	std::wstring Find(const std::wstring& folder, const wchar_t* pattern, const wchar_t* tail)
	{
		WIN32_FIND_DATAW data;
		HANDLE find = FindFirstFileW(LongPath(JoinPath(folder, pattern)).c_str(), &data);
		if (find == INVALID_HANDLE_VALUE)
			return L"";
		std::wstring found;
		do {
			if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && EndsWith(data.cFileName, tail)) {
				found = data.cFileName;
				break;
			}
		} while (FindNextFileW(find, &data));
		FindClose(find);
		return found;
	}

	bool Use(const std::wstring& folder, const std::wstring& name, InstallKind kind, Install& install)
	{
		if (name.empty())
			return false;
		install.kind = kind;
		install.program = FullPath(JoinPath(folder, name));
		return true;
	}
}

bool DetectInstall(const std::wstring& jkFolder, Install& install)
{
	std::wstring inject = Find(jkFolder, L"inject.exe", L"inject.exe");
	if (!inject.empty() && !Find(jkFolder, L"jkgm.json", L"jkgm.json").empty())
		return Use(jkFolder, inject, InstallKind::Remastered, install);

	std::wstring openjk = Find(jkFolder, L"openjkdf2-64.exe", L".exe");
	if (openjk.empty())
		openjk = Find(jkFolder, L"openjkdf2*.exe", L".exe");	// the .exe test skips openjkdf2-64.exe.bak and the like
	return Use(jkFolder, openjk, InstallKind::OpenJkdf2, install) ||
		Use(jkFolder, Find(jkFolder, L"JediKnight.exe", L".exe"), InstallKind::Steam, install) ||
		Use(jkFolder, Find(jkFolder, L"JK.EXE", L".exe"), InstallKind::Retail, install);
}

InstallKind KindOf(const std::wstring& program)
{
	std::wstring name = FileName(program);
	if (SameName(name, L"inject.exe"))
		return InstallKind::Remastered;
	if (name.size() >= 9 && SameName(name.substr(0, 9), L"openjkdf2"))
		return InstallKind::OpenJkdf2;
	if (SameName(name, L"JediKnight.exe"))
		return InstallKind::Steam;
	if (SameName(name, L"JK.EXE"))
		return InstallKind::Retail;
	return InstallKind::Other;
}

bool IsSteamStub(const std::string& exe)
{
	auto u16 = [&](size_t at) {
		return at + 2 <= exe.size() ? (unsigned)(unsigned char)exe[at] | (unsigned)(unsigned char)exe[at + 1] << 8 : 0u;
	};
	auto u32 = [&](size_t at) { return u16(at) | u16(at + 2) << 16; };
	if (exe.size() < 0x40 || exe.compare(0, 2, "MZ") != 0)
		return false;
	size_t pe = u32(0x3C);
	if (pe + 24 > exe.size() || exe.compare(pe, 4, std::string("PE\0\0", 4)) != 0)
		return false;
	// The COFF header: the number of sections at +6, the optional header's size at +20; the section table after it,
	// 40 bytes a section, each starting with its 8-byte name.
	size_t table = pe + 24 + u16(pe + 20);
	for (size_t i = 0, n = u16(pe + 6); i < n && table + 40 * (i + 1) <= exe.size(); i++)
		if (exe.compare(table + 40 * i, 8, std::string(".bind\0\0\0", 8)) == 0)
			return true;
	return false;
}

std::wstring SteamJkFolder()
{
	HKEY key;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\LucasArts Entertainment Company\\JediKnight\\1.0", 0,
		KEY_QUERY_VALUE | KEY_WOW64_32KEY, &key) != ERROR_SUCCESS)
		return L"";
	std::wstring folder(1024, L'\0');
	DWORD size = (DWORD)(folder.size() * sizeof(wchar_t));
	LSTATUS status = RegGetValueW(key, nullptr, L"Install Path", RRF_RT_REG_SZ, nullptr, &folder[0], &size);
	RegCloseKey(key);
	if (status != ERROR_SUCCESS)
		return L"";
	folder.resize(wcslen(folder.c_str()));
	return folder;
}

const wchar_t* KindName(InstallKind kind)
{
	switch (kind) {
	case InstallKind::Remastered: return L"JK Remastered (through JkGfxMod's inject.exe)";
	case InstallKind::OpenJkdf2: return L"OpenJKDF2";
	case InstallKind::Steam: return L"JK's Steam release (JediKnight.exe), or a patch of it";
	case InstallKind::Retail: return L"retail JK";
	default: return L"a JK program set in gobinator.ini";
	}
}

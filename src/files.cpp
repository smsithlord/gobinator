#include "files.h"

#include <windows.h>
#include <cstring>

namespace
{
	bool IsAbsolute(const std::wstring& path)
	{
		if (path.size() >= 2 && (path[0] == L'\\' || path[0] == L'/'))
			return true;	// UNC, \\?\, or rooted on the current drive
		return path.size() >= 3 && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/');
	}
}

std::wstring OwnFolder()
{
	std::wstring path(MAX_PATH, L'\0');
	for (;;) {
		DWORD n = GetModuleFileNameW(nullptr, &path[0], (DWORD)path.size());
		if (n == 0)
			return L"";
		if (n < path.size()) {
			path.resize(n);
			break;
		}
		path.resize(path.size() * 2);
	}
	size_t slash = path.find_last_of(L"\\/");
	return slash == std::wstring::npos ? path : path.substr(0, slash);
}

std::wstring JoinPath(const std::wstring& folder, const std::wstring& name)
{
	if (IsAbsolute(name) || folder.empty())
		return name;
	wchar_t last = folder.back();
	return (last == L'\\' || last == L'/') ? folder + name : folder + L"\\" + name;
}

std::wstring FullPath(const std::wstring& path)
{
	DWORD n = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
	if (n == 0)
		return path;
	std::wstring full(n, L'\0');
	n = GetFullPathNameW(path.c_str(), n, &full[0], nullptr);
	full.resize(n);
	while (full.size() > 3 && (full.back() == L'\\' || full.back() == L'/'))
		full.pop_back();
	return full;
}

std::wstring LongPath(const std::wstring& path)
{
	std::wstring full = FullPath(path);
	if (full.rfind(L"\\\\?\\", 0) == 0)
		return full;
	if (full.rfind(L"\\\\", 0) == 0)
		return L"\\\\?\\UNC\\" + full.substr(2);
	return L"\\\\?\\" + full;
}

bool ReadFileBytes(const std::wstring& path, std::string& bytes)
{
	HANDLE file = CreateFileW(LongPath(path).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
		return false;
	LARGE_INTEGER size;
	bool ok = GetFileSizeEx(file, &size) && size.QuadPart < (1 << 30);
	if (ok) {
		bytes.resize((size_t)size.QuadPart);
		DWORD read = 0;
		ok = bytes.empty() || (ReadFile(file, &bytes[0], (DWORD)bytes.size(), &read, nullptr) && read == bytes.size());
	}
	DWORD error = GetLastError();
	CloseHandle(file);
	SetLastError(error);
	return ok;
}

bool WriteFileBytes(const std::wstring& path, const std::string& bytes)
{
	std::wstring temp = path + L".tmp";
	HANDLE file = CreateFileW(LongPath(temp).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (file == INVALID_HANDLE_VALUE)
		return false;
	DWORD written = 0;
	bool ok = bytes.empty() || (WriteFile(file, bytes.data(), (DWORD)bytes.size(), &written, nullptr) &&
		written == bytes.size());
	ok = FlushFileBuffers(file) && ok;
	CloseHandle(file);
	if (ok)
		ok = MoveFileExW(LongPath(temp).c_str(), LongPath(path).c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
	if (!ok) {
		DWORD error = GetLastError();
		DeleteFileW(LongPath(temp).c_str());
		SetLastError(error);
	}
	return ok;
}

std::string ToUtf8(const std::wstring& text)
{
	if (text.empty())
		return "";
	int n = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
	std::string out(n, '\0');
	WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), &out[0], n, nullptr, nullptr);
	return out;
}

std::wstring FromUtf8(const std::string& bytes)
{
	size_t skip = bytes.rfind("\xEF\xBB\xBF", 0) == 0 ? 3 : 0;
	if (bytes.size() == skip)
		return L"";
	int n = MultiByteToWideChar(CP_UTF8, 0, bytes.data() + skip, (int)(bytes.size() - skip), nullptr, 0);
	std::wstring out(n, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, bytes.data() + skip, (int)(bytes.size() - skip), &out[0], n);
	return out;
}

FileInfo GetFileInfo(const std::wstring& path)
{
	FileInfo info;
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (!GetFileAttributesExW(LongPath(path).c_str(), GetFileExInfoStandard, &data))
		return info;
	info.exists = true;
	info.folder = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	info.size = ((unsigned long long)data.nFileSizeHigh << 32) | data.nFileSizeLow;
	info.written = ((unsigned long long)data.ftLastWriteTime.dwHighDateTime << 32) | data.ftLastWriteTime.dwLowDateTime;
	return info;
}

bool SameContent(const std::wstring& a, const std::wstring& b)
{
	FileInfo ia = GetFileInfo(a), ib = GetFileInfo(b);
	if (!ia.exists || !ib.exists || ia.folder || ib.folder || ia.size != ib.size)
		return false;
	HANDLE fa = CreateFileW(LongPath(a).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	HANDLE fb = CreateFileW(LongPath(b).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	bool same = fa != INVALID_HANDLE_VALUE && fb != INVALID_HANDLE_VALUE;
	std::string ba(1 << 20, '\0'), bb(1 << 20, '\0');
	while (same) {
		DWORD na = 0, nb = 0;
		if (!ReadFile(fa, &ba[0], (DWORD)ba.size(), &na, nullptr) || !ReadFile(fb, &bb[0], (DWORD)bb.size(), &nb, nullptr)) {
			same = false;
			break;
		}
		if (na != nb || memcmp(ba.data(), bb.data(), na) != 0)
			same = false;
		if (na == 0)
			break;
	}
	if (fa != INVALID_HANDLE_VALUE)
		CloseHandle(fa);
	if (fb != INVALID_HANDLE_VALUE)
		CloseHandle(fb);
	return same;
}

bool SameFolder(const std::wstring& a, const std::wstring& b)
{
	// CreateFileW follows a link or junction to the folder it points at.
	auto id = [](const std::wstring& path, BY_HANDLE_FILE_INFORMATION& info) {
		HANDLE h = CreateFileW(LongPath(path).c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE |
			FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
		if (h == INVALID_HANDLE_VALUE)
			return false;
		bool ok = GetFileInformationByHandle(h, &info) != 0;
		CloseHandle(h);
		return ok;
	};
	BY_HANDLE_FILE_INFORMATION ia, ib;
	if (id(a, ia) && id(b, ib))
		return ia.dwVolumeSerialNumber == ib.dwVolumeSerialNumber && ia.nFileIndexHigh == ib.nFileIndexHigh &&
			ia.nFileIndexLow == ib.nFileIndexLow;
	std::wstring fa = FullPath(a), fb = FullPath(b);
	return CompareStringOrdinal(fa.c_str(), (int)fa.size(), fb.c_str(), (int)fb.size(), TRUE) == CSTR_EQUAL;
}

std::wstring FileName(const std::wstring& path)
{
	size_t slash = path.find_last_of(L"\\/");
	return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

std::wstring ParentFolder(const std::wstring& path)
{
	size_t slash = path.find_last_of(L"\\/");
	return slash == std::wstring::npos ? L"" : path.substr(0, slash);
}

// Paths and files, in wide strings throughout, with long paths (docs/PLAN.md, part 3).
#pragma once

#include <string>

// The folder gobinator.exe is in, from GetModuleFileNameW (v1.0 used argv[0]: docs/OVERVIEW.md, defect 13).
std::wstring OwnFolder();

// name inside folder, unless name is already absolute.
std::wstring JoinPath(const std::wstring& folder, const std::wstring& name);

// The absolute, normalised path, with no trailing backslash (except a drive root).
std::wstring FullPath(const std::wstring& path);

// The \\?\ form of a path, which file calls accept past MAX_PATH.
std::wstring LongPath(const std::wstring& path);

// The whole file. False if it cannot be read; GetLastError says why.
bool ReadFileBytes(const std::wstring& path, std::string& bytes);

// Writes the whole file through a temporary file beside it, so a crash never leaves half a file.
bool WriteFileBytes(const std::wstring& path, const std::string& bytes);

std::string ToUtf8(const std::wstring& text);
std::wstring FromUtf8(const std::string& bytes);

struct FileInfo
{
	bool exists = false;
	bool folder = false;
	unsigned long long size = 0;
	unsigned long long written = 0;		// the last-write time, as a FILETIME
};
FileInfo GetFileInfo(const std::wstring& path);

// True when both files hold the same bytes.
bool SameContent(const std::wstring& a, const std::wstring& b);

// True when both paths name the same folder, however spelt and through any link or junction: compared by volume and
// file ID, or by full path when either cannot be opened.
bool SameFolder(const std::wstring& a, const std::wstring& b);

// The file name after the last slash, and the folder before it.
std::wstring FileName(const std::wstring& path);
std::wstring ParentFolder(const std::wstring& path);

#include "modfolder.h"
#include "files.h"
#include "launch.h"

#include <windows.h>

#include <cwctype>

const wchar_t* const kStateFileName = L"gobinator.state";

namespace
{
	const wchar_t* const kCopyTemp = L"gobinator-copy.tmp";	// not a .gob, so JK never loads a half-copied file
	const size_t kMaxEpisodeName = 31;						// jkRes_LoadGob and jkEpisode keep 31 characters and a NUL

	bool SameName(const std::wstring& a, const std::wstring& b)
	{
		return CompareStringOrdinal(a.c_str(), (int)a.size(), b.c_str(), (int)b.size(), TRUE) == CSTR_EQUAL;
	}

	// The record's lines, without the comments.
	std::vector<std::wstring> ReadLines(const std::wstring& folder)
	{
		std::vector<std::wstring> lines;
		std::string bytes;
		if (!ReadFileBytes(JoinPath(folder, kStateFileName), bytes))
			return lines;
		std::wstring text = FromUtf8(bytes);
		size_t start = 0;
		while (start < text.size()) {
			size_t end = text.find(L'\n', start);
			std::wstring line = text.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
			start = end == std::wstring::npos ? text.size() : end + 1;
			if (!line.empty() && line.back() == L'\r')
				line.pop_back();
			if (!line.empty() && line[0] != L'#')
				lines.push_back(line);
		}
		return lines;
	}

	bool WriteLines(const std::wstring& folder, const wchar_t* header, const std::vector<std::wstring>& lines)
	{
		std::wstring path = JoinPath(folder, kStateFileName);
		if (lines.empty()) {
			DeleteFileW(LongPath(path).c_str());
			return true;
		}
		std::wstring text = header;
		for (const auto& line : lines)
			text += line + L"\r\n";
		return WriteFileBytes(path, ToUtf8(text));
	}

	// --- The folders Gobinator made in Episode: one name a line.

	const wchar_t* const kFoldersHeader =
		L"# The empty folders Gobinator made here, each of which lists an episode first in JK. It removes them when\r\n"
		L"# you double-click another episode, or start gobinator.exe by itself; one you have put files in stays.\r\n";

	std::vector<std::wstring> ReadFolders(const std::wstring& episodeFolder)
	{
		std::vector<std::wstring> folders;
		for (const auto& line : ReadLines(episodeFolder))
			if (line.find(L'\t') == std::wstring::npos)
				folders.push_back(line);
		return folders;
	}

	// Takes out one of Gobinator's folders, if it is still empty. Either way it is no longer Gobinator's.
	void RemoveFolder(const std::wstring& episodeFolder, const std::wstring& name, Placement& placement)
	{
		std::wstring path = JoinPath(episodeFolder, name);
		if (!GetFileInfo(path).folder)
			return;
		if (RemoveDirectoryW(LongPath(path).c_str()))
			placement.removed.push_back(L"Episode\\" + name);
		else
			placement.notes.push_back(L"Episode\\" + name + L" was left where it is (" +
				SystemMessage(GetLastError()) + L"); Gobinator will not remove it.");
	}

	// --- The GOBs earlier v1.1 builds copied into the mod folder: name, size, time written.

	struct Record
	{
		std::wstring name;
		unsigned long long size = 0;
		unsigned long long written = 0;
	};

	std::vector<Record> ReadCopies(const std::wstring& folder)
	{
		std::vector<Record> records;
		for (const auto& line : ReadLines(folder)) {
			size_t tab1 = line.find(L'\t'), tab2 = line.rfind(L'\t');
			if (tab1 == std::wstring::npos || tab1 == tab2)
				continue;
			Record r;
			r.name = line.substr(0, tab1);
			r.size = wcstoull(line.c_str() + tab1 + 1, nullptr, 10);
			r.written = wcstoull(line.c_str() + tab2 + 1, nullptr, 10);
			records.push_back(r);
		}
		return records;
	}

	// Still exactly the file Gobinator copied: the same size and time written.
	bool Unchanged(const std::wstring& folder, const Record& r)
	{
		FileInfo info = GetFileInfo(JoinPath(folder, r.name));
		return info.exists && !info.folder && info.size == r.size && info.written == r.written;
	}
}

std::wstring SafeGobName(const std::wstring& fileName)
{
	// Compatibility decomposition splits an accented letter into its letter and a combining mark, which is dropped.
	std::wstring decomposed = fileName;
	int n = NormalizeString(NormalizationKD, fileName.c_str(), (int)fileName.size(), nullptr, 0);
	if (n > 0) {
		decomposed.resize(n);
		n = NormalizeString(NormalizationKD, fileName.c_str(), (int)fileName.size(), &decomposed[0], n);
		decomposed.resize(n > 0 ? n : 0);
		if (n <= 0)
			decomposed = fileName;
	}

	std::wstring out;
	for (wchar_t c : decomposed) {
		if (c >= 0x0300 && c <= 0x036F)
			continue;
		bool keep = c < 0x80 && (std::iswalnum(c) || c == L'.' || c == L'_' || c == L'-');
		out += keep ? c : L'_';
	}
	if (!out.empty() && out[0] == L'-')
		out[0] = L'_';		// a leading '-' would read as an option on JK's command line

	size_t dot = out.rfind(L'.');
	std::wstring stem = dot == std::wstring::npos ? out : out.substr(0, dot);
	std::wstring extension = dot == std::wstring::npos ? L"" : out.substr(dot);
	bool hasLetter = false;
	for (wchar_t c : stem)
		hasLetter = hasLetter || std::iswalnum(c);
	if (!hasLetter)
		stem = L"episode";
	return stem.substr(0, kMaxEpisodeName) + extension;
}

std::wstring EpisodeName(const std::wstring& gobName)
{
	return gobName.substr(0, gobName.rfind(L'.'));
}

bool CheckGobFile(const std::wstring& gobPath, std::wstring& error)
{
	std::wstring source = FullPath(gobPath);
	std::wstring sourceName = FileName(source);
	FileInfo info = GetFileInfo(source);
	if (!info.exists || info.folder) {
		error = L"The GOB was not found: " + source;
		return false;
	}
	size_t dot = sourceName.rfind(L'.');
	if (dot == std::wstring::npos || !SameName(sourceName.substr(dot), L".gob")) {
		error = sourceName + L" is not a .gob file, so Gobinator left it alone.";
		return false;
	}
	return true;
}

bool PlaceGob(const std::wstring& gobPath, const std::wstring& episodeFolder, Placement& placement,
	std::wstring& error)
{
	if (!CheckGobFile(gobPath, error))
		return false;
	std::wstring source = FullPath(gobPath);
	std::wstring sourceName = FileName(source);

	std::wstring folder = FullPath(episodeFolder);
	FileInfo folderInfo = GetFileInfo(folder);
	if (folderInfo.exists && !folderInfo.folder) {
		error = folder + L" is a file, not a folder.";
		return false;
	}
	if (!folderInfo.exists && !CreateDirectoryW(LongPath(folder).c_str(), nullptr)) {
		error = L"The folder " + folder + L" could not be created: " + SystemMessage(GetLastError());
		return false;
	}

	// A GOB that is already one of the episodes: JK loads it where it is.
	if (SameName(FullPath(ParentFolder(source)), folder)) {
		placement.name = sourceName;
		placement.notes.push_back(sourceName + L" is already in " + folder + L".");
		return true;
	}

	std::wstring target = SafeGobName(sourceName);
	if (target != sourceName)
		placement.notes.push_back(sourceName + L" is copied as " + target + L" (a name JK can open and list).");

	// A file of that name is never overwritten: if it holds the same GOB, JK loads it as it is; if not, the copy gets
	// another name. (JK lists an episode by the title inside its GOB, not the file name.)
	std::wstring stem = EpisodeName(target), extension = target.substr(stem.size());
	std::wstring name = target;
	for (int n = 2;; n++) {
		FileInfo existing = GetFileInfo(JoinPath(folder, name));
		if (!existing.exists)
			break;
		if (!existing.folder && SameContent(source, JoinPath(folder, name))) {
			placement.name = name;
			placement.notes.push_back(name + L" is already in " + folder + L" with the same content; JK loads that "
				L"one.");
			return true;
		}
		std::wstring suffix = L"_" + std::to_wstring(n);
		name = stem.substr(0, kMaxEpisodeName - suffix.size()) + suffix + extension;
	}

	// Copy to a temporary name first, so a failed copy leaves nothing behind.
	std::wstring temp = JoinPath(folder, kCopyTemp);
	if (!CopyFileW(LongPath(source).c_str(), LongPath(temp).c_str(), FALSE) ||
		!MoveFileExW(LongPath(temp).c_str(), LongPath(JoinPath(folder, name)).c_str(), 0)) {
		error = L"The GOB could not be copied into " + folder + L": " + SystemMessage(GetLastError());
		DeleteFileW(LongPath(temp).c_str());
		return false;
	}
	placement.name = name;
	placement.copied = true;
	return true;
}

bool ListFirst(const std::wstring& episodeFolder, const std::wstring& episode, Placement& placement,
	std::wstring& error)
{
	std::wstring folder = FullPath(episodeFolder);
	std::vector<std::wstring> kept;
	for (const auto& made : ReadFolders(folder)) {
		if (SameName(made, episode))
			kept.push_back(made);
		else
			RemoveFolder(folder, made, placement);
	}

	std::wstring entry = JoinPath(folder, episode);
	FileInfo info = GetFileInfo(entry);
	bool ok = true;
	if (info.exists && !info.folder) {
		error = entry + L" is a file, so Gobinator cannot make the folder of that name that lists the episode first.";
		ok = false;
	}
	else if (!info.exists) {
		if (CreateDirectoryW(LongPath(entry).c_str(), nullptr)) {
			kept.push_back(episode);
			placement.notes.push_back(L"made the empty folder Episode\\" + episode + L", which lists it first");
		}
		else {
			error = L"The folder " + entry + L", which lists the episode first, could not be made: " +
				SystemMessage(GetLastError());
			ok = false;
		}
	}
	else if (kept.empty() || !SameName(kept.back(), episode))
		placement.notes.push_back(L"Episode\\" + episode + L" is a folder of yours, which lists it first already");
	WriteLines(folder, kFoldersHeader, kept);
	return ok;
}

void UnlistAll(const std::wstring& episodeFolder, Placement& placement)
{
	std::wstring folder = FullPath(episodeFolder);
	for (const auto& made : ReadFolders(folder))
		RemoveFolder(folder, made, placement);
	WriteLines(folder, kFoldersHeader, {});
}

void RemoveCopies(const std::wstring& folder, Placement& placement)
{
	std::wstring full = FullPath(folder);
	std::vector<Record> records = ReadCopies(full);
	if (records.empty())
		return;
	std::vector<std::wstring> kept;
	for (const auto& r : records) {
		if (!Unchanged(full, r)) {
			if (GetFileInfo(JoinPath(full, r.name)).exists)
				placement.notes.push_back(r.name + L" has changed since Gobinator copied it, so it was left in " + full +
					L".");
			continue;
		}
		if (DeleteFileW(LongPath(JoinPath(full, r.name)).c_str()))
			placement.removed.push_back(r.name);
		else {
			placement.notes.push_back(L"Gobinator could not remove its earlier copy " + r.name + L": " +
				SystemMessage(GetLastError()));
			kept.push_back(r.name + L"\t" + std::to_wstring(r.size) + L"\t" + std::to_wstring(r.written));
		}
	}
	WriteLines(full, L"# The GOBs an earlier Gobinator copied into this folder: name, size, time written.\r\n", kept);
}

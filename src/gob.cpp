#include "gob.h"
#include "files.h"

#include <windows.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <vector>

namespace
{
	const unsigned kEntrySize = 136;			// int offset, uint size, char name[128]
	const unsigned kMaxEntries = 1 << 20;
	const unsigned kMaxEntryBytes = 16 << 20;	// an episode.jk is a few hundred bytes

	bool ReadAt(HANDLE file, unsigned long long offset, void* buffer, unsigned size)
	{
		LARGE_INTEGER at;
		at.QuadPart = (LONGLONG)offset;
		DWORD read = 0;
		return SetFilePointerEx(file, at, nullptr, FILE_BEGIN) && ReadFile(file, buffer, size, &read, nullptr) &&
			read == size;
	}

	std::string Normal(std::string name)
	{
		for (auto& c : name)
			c = c == '/' ? '\\' : (char)std::tolower((unsigned char)c);
		return name;
	}

	unsigned U32(const unsigned char* p)
	{
		return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24);
	}
}

GobEntry FindGobEntry(const std::wstring& gobPath, const std::string& name, std::string& bytes)
{
	HANDLE file = CreateFileW(LongPath(gobPath).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
		return GobEntry::NotGob;
	LARGE_INTEGER fileSize = {};
	GetFileSizeEx(file, &fileSize);
	unsigned long long size = (unsigned long long)fileSize.QuadPart;

	GobEntry result = GobEntry::NotGob;
	unsigned char header[12];
	if (ReadAt(file, 0, header, 12) && memcmp(header, "GOB ", 4) == 0 && U32(header + 4) == 20) {
		unsigned long long table = U32(header + 8);
		unsigned char countBytes[4];
		if (table + 4 <= size && ReadAt(file, table, countBytes, 4)) {
			unsigned count = U32(countBytes);
			if (count <= kMaxEntries && table + 4 + (unsigned long long)count * kEntrySize <= size) {
				std::vector<unsigned char> entries((size_t)count * kEntrySize);
				std::string wanted = Normal(name);
				if (count == 0)
					result = GobEntry::Missing;
				else if (ReadAt(file, table + 4, entries.data(), (unsigned)entries.size())) {
					result = GobEntry::Missing;
					for (unsigned i = 0; i < count; i++) {
						const unsigned char* e = &entries[(size_t)i * kEntrySize];
						const char* raw = (const char*)e + 8;
						if (Normal(std::string(raw, strnlen(raw, 128))) != wanted)
							continue;
						// An entry that runs past the end of the file: the GOB is damaged.
						result = GobEntry::NotGob;
						unsigned long long offset = U32(e), length = U32(e + 4);
						if (length <= kMaxEntryBytes && offset + length <= size) {
							bytes.assign((size_t)length, '\0');
							if (length == 0 || ReadAt(file, offset, &bytes[0], (unsigned)length))
								result = GobEntry::Found;
						}
						break;
					}
				}
			}
		}
	}
	CloseHandle(file);
	return result;
}

bool ReadGobEntry(const std::wstring& gobPath, const std::string& name, std::string& bytes)
{
	return FindGobEntry(gobPath, name, bytes) == GobEntry::Found;
}

int EpisodeType(const std::string& episodeJk)
{
	std::istringstream lines(episodeJk);
	std::string line;
	bool named = false;
	while (std::getline(lines, line)) {
		size_t at = line.find_first_not_of(" \t");
		if (at == std::string::npos || line[at] == '#' || line[at] == '\r' || line[at] == '\0')
			continue;
		if (!named) {
			named = true;
			continue;
		}
		// As JK does, on the whole line: "TYPE" must start it, and anything else leaves single player.
		int type = 1;
		sscanf_s(line.c_str(), "TYPE %d", &type);
		return type;
	}
	return named ? 1 : 0;
}

std::string FirstLevel(const std::string& episodeJk)
{
	std::istringstream lines(episodeJk);
	std::string line;
	while (std::getline(lines, line)) {
		std::istringstream tokens(line);
		std::string number, cd, level, type, file;
		if (!(tokens >> number >> cd >> level >> type >> file))
			continue;
		bool row = number.size() >= 2 && number.back() == ':';
		for (size_t i = 0; row && i + 1 < number.size(); i++)
			row = std::isdigit((unsigned char)number[i]) != 0;
		if (row && _stricmp(type.c_str(), "LEVEL") == 0)
			return file;
	}
	return "";
}

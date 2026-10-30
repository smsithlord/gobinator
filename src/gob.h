// Reading inside a GOB, JK's archive: "GOB " and version 20, the offset of a table of entries, and at that offset a
// count and the entries (offset, size, a 128-byte name with backslashes). The same layout as
// openjkdf2_libretro's tools/cogfactory/gob.py.
#pragma once

#include <string>

// What FindGobEntry found.
enum class GobEntry { Found, Missing, NotGob };

// Reads the bytes of the entry called name (any case, '\' or '/'). Missing when the GOB holds no such entry; NotGob
// when the file cannot be read as a GOB: not one, damaged, or not there.
GobEntry FindGobEntry(const std::wstring& gobPath, const std::string& name, std::string& bytes);

// The same, true only when the entry was found.
bool ReadGobEntry(const std::wstring& gobPath, const std::string& name, std::string& bytes);

// The episode's TYPE, as JK reads it (jkEpisode.c): the first line that is not blank or a '#' comment is the
// episode's name, and the next such line is read with "TYPE %d"; when that is not a TYPE line, JK takes the episode
// as single player. A set of bits: 1 single player, 2 deathmatch, 8 Capture the Flag (LucasArts' JK1.GOB, JK1MP.GOB
// and JK1CTF.GOB). 0 when the file has no name line.
int EpisodeType(const std::string& episodeJk);

// The file of the first LEVEL row in an episode.jk, the level an episode starts on; empty if there is none. A row is
// "lineNum: cd level TYPE FILE ..." (jkEpisode.c reads it with " %d: %d %d %s %s"), e.g. "10: 1 1 LEVEL piano.jkl".
std::string FirstLevel(const std::string& episodeJk);

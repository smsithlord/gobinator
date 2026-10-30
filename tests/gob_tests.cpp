// Unit tests for reading inside a GOB, an episode's TYPE (src/gob.cpp) and OpenJKDF2's autostart (AddAutostart in
// src/launch.cpp).
#include "check.h"
#include "../src/files.h"
#include "../src/gob.h"
#include "../src/launch.h"

#include <windows.h>
#include <string>
#include <utility>
#include <vector>

namespace
{
	std::wstring folder;

	void U32(std::string& s, unsigned v)
	{
		for (int i = 0; i < 4; i++)
			s += (char)((v >> (8 * i)) & 0xFF);
	}

	// A GOB holding these files, with its table after the header (as LucasArts' are) or at the end (as some are).
	std::wstring MakeGob(const wchar_t* name, const std::vector<std::pair<std::string, std::string>>& files,
		bool tableAtEnd)
	{
		std::string table, data;
		unsigned dataStart = tableAtEnd ? 12 : 12 + 4 + 136 * (unsigned)files.size();
		U32(table, (unsigned)files.size());
		for (const auto& f : files) {
			U32(table, dataStart + (unsigned)data.size());
			U32(table, (unsigned)f.second.size());
			std::string entryName = f.first;
			entryName.resize(128, '\0');
			table += entryName;
			data += f.second;
		}
		std::string gob = "GOB ";
		U32(gob, 20);
		U32(gob, tableAtEnd ? 12 + (unsigned)data.size() : 12);
		gob += tableAtEnd ? data + table : table + data;
		std::wstring path = JoinPath(folder, name);
		WriteFileBytes(path, gob);
		return path;
	}

	const char* const kP03 = "\"COG Factory p03 Piano\"\n\nTYPE 1\n\nSEQ 1\n\n10:\t1\t1\tLEVEL\tpiano.jkl\t0\t0\t-1\t-1\n\nend\n";
	// The headers of LucasArts' multiplayer episodes, as JK1MP.GOB and JK1CTF.GOB have them.
	const char* const kMp = "\"Jedi Training\"\r\nTYPE\t2\t# Deathmatch\r\nSEQ\t1\r\n"
		"10:\t1\t1\tLEVEL\tjkmp01.jkl\t0\t0\t-1\t-1\r\nend\r\n";
	const char* const kCtf = "\"Jedi Capture the Flag\"\r\nTYPE\t8\t# Special/CTF\r\nSEQ\t1\r\n"
		"10:\t1\t1\tLEVEL\tctf01.jkl\t0\t0\t-1\t-1\r\nend\r\n";
}

static void TestReadGobEntry()
{
	for (bool atEnd : { false, true }) {
		std::wstring gob = MakeGob(atEnd ? L"end.gob" : L"front.gob",
			{ { "episode.jk", kP03 }, { "jkl\\piano.jkl", "SECTION: JK" }, { "cog\\piano.cog", "" } }, atEnd);
		std::string bytes;
		CHECK(ReadGobEntry(gob, "episode.jk", bytes) && bytes == kP03);
		CHECK(ReadGobEntry(gob, "JKL/Piano.JKL", bytes) && bytes == "SECTION: JK");
		CHECK(ReadGobEntry(gob, "cog\\piano.cog", bytes) && bytes.empty());
		CHECK(!ReadGobEntry(gob, "missing.jk", bytes));
		CHECK(FindGobEntry(gob, "missing.jk", bytes) == GobEntry::Missing);
		CHECK(FindGobEntry(gob, "Episode.JK", bytes) == GobEntry::Found && bytes == kP03);
	}

	// A mod: a GOB with no episode.jk, or with nothing in it at all.
	std::string bytes;
	CHECK(FindGobEntry(MakeGob(L"mod.gob", { { "mat\\x.mat", "MAT" }, { "jkl\\static.jkl", "" } }, false),
		"episode.jk", bytes) == GobEntry::Missing);
	CHECK(FindGobEntry(MakeGob(L"empty.gob", {}, false), "episode.jk", bytes) == GobEntry::Missing);

	std::wstring good = MakeGob(L"good.gob", { { "episode.jk", kP03 } }, false);
	std::string raw;
	ReadFileBytes(good, raw);
	std::wstring bad = JoinPath(folder, L"bad.gob");
	WriteFileBytes(bad, "GOX " + raw.substr(4));					// not a GOB
	CHECK(!ReadGobEntry(bad, "episode.jk", bytes));
	CHECK(FindGobEntry(bad, "episode.jk", bytes) == GobEntry::NotGob);
	WriteFileBytes(bad, raw.substr(0, 8) + "\xFF\xFF\x00\x00" + raw.substr(12));	// the table past the end
	CHECK(FindGobEntry(bad, "episode.jk", bytes) == GobEntry::NotGob);
	WriteFileBytes(bad, raw.substr(0, raw.size() - 10));			// cut short: the entry runs past the end
	CHECK(FindGobEntry(bad, "episode.jk", bytes) == GobEntry::NotGob);
	WriteFileBytes(bad, "episode Episode.gob");						// a text file named .gob
	CHECK(FindGobEntry(bad, "episode.jk", bytes) == GobEntry::NotGob);
	CHECK(FindGobEntry(JoinPath(folder, L"none.gob"), "episode.jk", bytes) == GobEntry::NotGob);

	// The real test episode, when the OpenJKDF2 test install has been made.
	std::string dir = GOBINATOR_SOURCE_DIR;
	std::wstring p03 = JoinPath(std::wstring(dir.begin(), dir.end()), L"testinstall\\fixtures\\P03.GOB");
	if (GetFileInfo(p03).exists)
		CHECK(ReadGobEntry(p03, "episode.jk", bytes) && FirstLevel(bytes) == "piano.jkl");
}

static void TestFirstLevel()
{
	CHECK(FirstLevel(kP03) == "piano.jkl");
	CHECK(FirstLevel(
		"# The Dark Forces Saga\r\n"
		"\"Jedi Knight\"\r\n"
		"TYPE 1\r\n"
		"SEQ 3\r\n"
		"# line: cd level type file lightpow darkpow gotoA gotoB\r\n"
		"1: 1 1 CUT 01-02a.smk 0 0 10 -1\r\n"
		"10: 1 1 LEVEL 01narsh.jkl 0 0 20 -1\r\n"
		"20: 1 2 LEVEL 02narsh.jkl 0 0 -1 -1\r\n"
		"end\r\n") == "01narsh.jkl");
	CHECK(FirstLevel("\"no levels\"\nTYPE 1\nSEQ 0\nend\n").empty());
	CHECK(FirstLevel("").empty());
}

static void TestEpisodeType()
{
	CHECK(EpisodeType(kP03) == 1);
	CHECK(EpisodeType(kMp) == 2);
	CHECK(EpisodeType(kCtf) == 8);
	CHECK(EpisodeType("# The Dark Forces Saga\r\n\r\n  # more\r\n\"The Force Within\"\r\nTYPE  1  # single-player\r\n")
		== 1);
	CHECK(EpisodeType("\"Both\"\nTYPE 3\n") == 3);
	// As JK reads it: no TYPE line after the name, or one that does not start with TYPE, is single player.
	CHECK(EpisodeType("\"No type\"\nSEQ 1\n10: 1 1 LEVEL a.jkl 0 0 -1 -1\n") == 1);
	CHECK(EpisodeType("\"Indented\"\n  TYPE 2\n") == 1);
	CHECK(EpisodeType("\"Lower case\"\ntype 2\n") == 1);
	CHECK(EpisodeType("\"Name only\"\n") == 1);
	CHECK(EpisodeType("") == 0);
	CHECK(EpisodeType("# only a comment\r\n\r\n") == 0);
}

static void TestKnownSwitches()
{
	// As OpenJKDF2's EXE keeps them: one after another, each ending in a NUL, any case.
	const char kExe[] = "MZ\x90\0\0\0-path\0-devMode\0-autostart\0-SP\0format-map\0\0-episode\0-x\0-\0-a b\0-map";
	std::set<std::string> known = KnownSwitches(std::string(kExe, sizeof kExe - 1));
	CHECK(known.count("-path") && known.count("-devmode") && known.count("-autostart") && known.count("-sp"));
	CHECK(known.count("-episode") && known.count("-x"));
	CHECK(!known.count("-map"));		// once inside other text, once with no NUL after it
	CHECK(!known.count("-") && !known.count("-a b") && known.size() == 6);
	CHECK(KnownSwitches("").empty() && KnownSwitches(std::string("\0-", 2)).empty());
}

static void TestAddAutostart()
{
	// Fake OpenJKDF2 EXEs holding the switches they know as the real ones do.
	auto exe = [](const wchar_t* name, const char* switches, size_t n) {
		std::wstring sub = JoinPath(folder, name);
		CreateDirectoryW(sub.c_str(), nullptr);
		std::wstring path = JoinPath(sub, L"openjkdf2-64.exe");
		WriteFileBytes(path, "MZ" + std::string(switches, n) + "rest of the EXE");
		return path;
	};
	const char kUpstream[] = "\0-path\0-devMode\0-autostart\0-sp\0-mp\0-episode\0-map\0";	// v0.9.1 to v0.9.9
	std::wstring gob = MakeGob(L"Episode.gob", { { "episode.jk", kP03 } }, false);
	LaunchSpec spec{ exe(L"upstream", kUpstream, sizeof kUpstream - 1), L"-devmode -path mod", folder,
		InstallKind::OpenJkdf2 };
	std::wstring note;
	CHECK(AddAutostart(spec, gob, L"Episode", note));
	CHECK(spec.arguments == L"-devmode -path mod -autostart -sp -episode Episode -map piano.jkl");
	CHECK(note == L"autostart: episode Episode (Episode.gob), level piano.jkl, single player");

	// A multiplayer episode (TYPE 2) is hosted on this PC: -mp.
	std::wstring mp = MakeGob(L"Training.gob", { { "episode.jk", kMp } }, false);
	LaunchSpec host{ spec.program, L"-path mod", folder, InstallKind::OpenJkdf2 };
	CHECK(AddAutostart(host, mp, L"Training", note));
	CHECK(host.arguments == L"-path mod -autostart -mp -episode Training -map jkmp01.jkl");
	CHECK(note == L"autostart: episode Training (Training.gob), level jkmp01.jkl, multiplayer: a game hosted on this PC");

	// Capture the Flag (8) and any other TYPE open at the menu (docs/DECISIONS.md, 2026-10-29).
	LaunchSpec menu{ spec.program, L"-path mod", folder, InstallKind::OpenJkdf2 };
	CHECK(!AddAutostart(menu, MakeGob(L"Ctf.gob", { { "episode.jk", kCtf } }, false), L"Ctf", note));
	CHECK(note.find(L"Capture the Flag episode (TYPE 8)") != std::wstring::npos && menu.arguments == L"-path mod");
	CHECK(!AddAutostart(menu, MakeGob(L"Both.gob", { { "episode.jk", "\"Both\"\nTYPE 3\n10: 1 1 LEVEL a.jkl\n" } },
		false), L"Both", note));
	CHECK(note.find(L"TYPE 3, not 1") != std::wstring::npos && menu.arguments == L"-path mod");

	// An EXE that knows -sp but not -mp gets no autostart for a multiplayer episode.
	const char kSpOnly[] = "\0-path\0-autostart\0-sp\0-episode\0-map\0";
	LaunchSpec spOnly{ exe(L"sponly", kSpOnly, sizeof kSpOnly - 1), L"-path mod", folder, InstallKind::OpenJkdf2 };
	CHECK(!AddAutostart(spOnly, mp, L"Training", note) && spOnly.arguments == L"-path mod");
	CHECK(note.find(L"does not know -mp") != std::wstring::npos);
	CHECK(AddAutostart(spOnly, gob, L"Episode", note));

	// The user's fork knows -doNotResumeLast: without it, it resumes its last session.
	const char kFork[] = "\0-path\0-autostart\0-sp\0-doNotResumeLast\0-episode\0-map\0";
	LaunchSpec forked{ exe(L"fork", kFork, sizeof kFork - 1), L"-path mod", folder, InstallKind::OpenJkdf2 };
	CHECK(AddAutostart(forked, gob, L"Episode", note));
	CHECK(forked.arguments == L"-path mod -autostart -sp -episode Episode -map piano.jkl -doNotResumeLast");

	// An OpenJKDF2 from before v0.9.1 would quit with "Error in arguments": nothing is added.
	const char kOld[] = "\0-path\0-devMode\0-windowGUI\0";
	LaunchSpec old{ exe(L"v0.9.0", kOld, sizeof kOld - 1), L"-devmode -path mod", folder, InstallKind::OpenJkdf2 };
	CHECK(!AddAutostart(old, gob, L"Episode", note) && old.arguments == L"-devmode -path mod");
	CHECK(note.find(L"does not know -autostart") != std::wstring::npos);
	const char kPart[] = "\0-autostart\0-sp\0-episode\0";		// all four, or nothing
	LaunchSpec part{ exe(L"part", kPart, sizeof kPart - 1), L"-path mod", folder, InstallKind::OpenJkdf2 };
	CHECK(!AddAutostart(part, gob, L"Episode", note) && part.arguments == L"-path mod");
	LaunchSpec missing{ JoinPath(folder, L"none\\openjkdf2-64.exe"), L"-path mod", folder, InstallKind::OpenJkdf2 };
	CHECK(!AddAutostart(missing, gob, L"Episode", note) && missing.arguments == L"-path mod");
	CHECK(note.find(L"could not be read") != std::wstring::npos);

	// Where it does not apply, nothing changes.
	LaunchSpec retail{ JoinPath(folder, L"inject.exe"), L"-devmode -path mod", folder, InstallKind::Remastered };
	CHECK(!AddAutostart(retail, gob, L"Episode", note) && retail.arguments == L"-devmode -path mod");
	LaunchSpec s = spec;
	s.arguments = L"-path mod";
	CHECK(!AddAutostart(s, MakeGob(L"Textures.gob", { { "mat\\x.mat", "" } }, false), L"Textures", note));
	CHECK(note.find(L"no episode.jk") != std::wstring::npos && s.arguments == L"-path mod");

	// A GOB already in Episode with a space in its name: OpenJKDF2 cannot take it (it splits at spaces, no quotes).
	s.arguments = L"-path mod";
	std::wstring spaced = MakeGob(L"Dralloc Sirhc 3.gob", { { "episode.jk", kP03 } }, false);
	CHECK(!AddAutostart(s, spaced, L"Dralloc Sirhc 3", note) && s.arguments == L"-path mod");
	CHECK(note.find(L"space") != std::wstring::npos);
}

void RunGobTests()
{
	wchar_t temp[MAX_PATH];
	GetTempPathW(MAX_PATH, temp);
	CreateDirectoryW(JoinPath(temp, L"gobinator-unit").c_str(), nullptr);
	folder = JoinPath(temp, L"gobinator-unit\\gob");
	CreateDirectoryW(folder.c_str(), nullptr);

	TestReadGobEntry();
	TestFirstLevel();
	TestEpisodeType();
	TestKnownSwitches();
	TestAddAutostart();
}

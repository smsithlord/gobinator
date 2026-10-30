// Unit tests for putting a GOB where JK lists it first (src/modfolder.cpp), in throwaway folders in TEMP.
#include "check.h"
#include "../src/files.h"
#include "../src/modfolder.h"

#include <windows.h>
#include <algorithm>
#include <string>
#include <vector>

namespace
{
	std::wstring root;

	void Wipe(const std::wstring& folder)
	{
		WIN32_FIND_DATAW data;
		HANDLE find = FindFirstFileW(JoinPath(folder, L"*").c_str(), &data);
		if (find != INVALID_HANDLE_VALUE) {
			do {
				std::wstring name = data.cFileName;
				if (name == L"." || name == L"..")
					continue;
				std::wstring path = JoinPath(folder, name);
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					Wipe(path);
				else
					DeleteFileW(path.c_str());
			} while (FindNextFileW(find, &data));
			FindClose(find);
		}
		RemoveDirectoryW(folder.c_str());
	}

	// A fresh case folder holding Downloads\ and Fake JK\Episode\ (ep, with LucasArts' JK1.GOB) and Fake JK\mod\.
	struct Case
	{
		std::wstring base, downloads, jk, ep, mod;
		explicit Case(const wchar_t* name)
		{
			base = JoinPath(root, name);
			Wipe(base);
			downloads = JoinPath(base, L"Downloads");
			jk = JoinPath(base, L"Fake JK");
			ep = JoinPath(jk, L"Episode");
			mod = JoinPath(jk, L"mod");
			for (const auto& f : { base, downloads, jk, ep, mod })
				CreateDirectoryW(f.c_str(), nullptr);
			WriteFileBytes(JoinPath(ep, L"JK1.GOB"), "LucasArts' episode");
		}
	};

	std::wstring Make(const std::wstring& folder, const std::wstring& name, const std::string& content)
	{
		CreateDirectoryW(folder.c_str(), nullptr);
		std::wstring path = JoinPath(folder, name);
		WriteFileBytes(path, content);
		return path;
	}

	std::string Content(const std::wstring& path)
	{
		std::string bytes;
		return ReadFileBytes(path, bytes) ? bytes : "<missing>";
	}

	// The folder's entries, sorted, joined with spaces; folders end in '\'.
	std::wstring List(const std::wstring& folder)
	{
		std::vector<std::wstring> names;
		WIN32_FIND_DATAW data;
		HANDLE find = FindFirstFileW(JoinPath(folder, L"*").c_str(), &data);
		if (find != INVALID_HANDLE_VALUE) {
			do {
				std::wstring name = data.cFileName;
				if (name == L"." || name == L"..")
					continue;
				names.push_back(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? name + L"\\" : name);
			} while (FindNextFileW(find, &data));
			FindClose(find);
		}
		std::sort(names.begin(), names.end());
		std::wstring out;
		for (const auto& n : names)
			out += (out.empty() ? L"" : L" ") + n;
		return out;
	}

	bool Place(const std::wstring& gob, const std::wstring& folder, Placement& p)
	{
		std::wstring error;
		bool ok = PlaceGob(gob, folder, p, error);
		if (!ok)
			std::fprintf(stderr, "  (PlaceGob: %ls)\n", error.c_str());
		return ok;
	}

	bool List1st(const std::wstring& folder, const std::wstring& episode, Placement& p)
	{
		std::wstring error;
		bool ok = ListFirst(folder, episode, p, error);
		if (!ok)
			std::fprintf(stderr, "  (ListFirst: %ls)\n", error.c_str());
		return ok;
	}
}

static void TestSafeGobName()
{
	CHECK(SafeGobName(L"Episode.gob") == L"Episode.gob");
	CHECK(SafeGobName(L"JK1.GOB") == L"JK1.GOB");
	CHECK(SafeGobName(L"My Episode (v2).gob") == L"My_Episode__v2_.gob");
	CHECK(SafeGobName(L"\x00C9pisode \x00FC" L"ber.gob") == L"Episode_uber.gob");
	CHECK(SafeGobName(L"\x65E5\x672C\x8A9E.gob") == L"episode.gob");
	CHECK(SafeGobName(L"-devmode.gob") == L"_devmode.gob");
	CHECK(SafeGobName(L"A Very Long Episode Name That Goes On.gob") == L"A_Very_Long_Episode_Name_That_G.gob");	// 31

	CHECK(EpisodeName(L"Episode.gob") == L"Episode" && EpisodeName(L"Dralloc Sirhc 3.gob") == L"Dralloc Sirhc 3");
}

// The everyday case, and the v1.0 scenarios that deleted or skipped files (docs/OVERVIEW.md, A and B).
static void TestCopyIntoEpisode()
{
	Case c(L"everyday");
	Placement p;
	CHECK(Place(Make(c.downloads, L"Episode.gob", "episode one"), c.ep, p));
	CHECK(p.copied && p.name == L"Episode.gob");
	CHECK(List(c.ep) == L"Episode.gob JK1.GOB");
	CHECK(GetFileInfo(JoinPath(c.downloads, L"Episode.gob")).exists);	// copied, not moved

	// The next one is copied beside it: copies stay, they are the player's episodes now.
	Placement q;
	CHECK(Place(Make(c.downloads, L"EPISODE2.GOB", "episode two"), c.ep, q));		// upper case (B)
	CHECK(q.copied && q.name == L"EPISODE2.GOB" && q.removed.empty());
	CHECK(List(c.ep) == L"EPISODE2.GOB Episode.gob JK1.GOB");

	// The same one again: the copy there is used as it is.
	Placement r;
	CHECK(Place(JoinPath(c.downloads, L"EPISODE2.GOB"), c.ep, r));
	CHECK(!r.copied && r.name == L"EPISODE2.GOB");
	CHECK(List(c.ep) == L"EPISODE2.GOB Episode.gob JK1.GOB");
}

// v1.0 deleted a GOB double-clicked inside the folder it copies into (C). Now it is used where it is.
static void TestGobAlreadyInEpisode()
{
	Case c(L"inepisode");
	std::wstring mine = Make(c.ep, L"Dralloc Sirhc 3.gob", "player's own");
	Placement p;
	CHECK(Place(mine, c.ep, p));
	CHECK(!p.copied && p.name == L"Dralloc Sirhc 3.gob");		// its own name, spaces and all
	CHECK(Content(mine) == "player's own" && List(c.ep) == L"Dralloc Sirhc 3.gob JK1.GOB");
}

static void TestNeverOverwritesThePlayers()
{
	Case c(L"clash");
	Make(c.ep, L"Episode.gob", "player's own, different");
	Placement p;
	CHECK(Place(Make(c.downloads, L"Episode.gob", "downloaded"), c.ep, p));
	CHECK(p.copied && p.name == L"Episode_2.gob");
	CHECK(Content(JoinPath(c.ep, L"Episode.gob")) == "player's own, different");
	CHECK(Content(JoinPath(c.ep, L"Episode_2.gob")) == "downloaded");

	// Identical content: JK loads the player's file, and nothing is copied.
	Case d(L"identical");
	Make(d.ep, L"Episode.gob", "same bytes");
	Placement q;
	CHECK(Place(Make(d.downloads, L"Episode.gob", "same bytes"), d.ep, q));
	CHECK(!q.copied && q.name == L"Episode.gob" && List(d.ep) == L"Episode.gob JK1.GOB");

	// A 31-character name that clashes stays within 31 characters.
	Case e(L"long");
	Make(e.ep, L"A_Very_Long_Episode_Name_That_G.gob", "player's own");
	Placement r;
	CHECK(Place(Make(e.downloads, L"A Very Long Episode Name That Goes On.gob", "downloaded"), e.ep, r));
	CHECK(r.copied && r.name == L"A_Very_Long_Episode_Name_That_2.gob");
}

// v1.0 garbled these and copied nothing (D1, D2, E).
static void TestUnicode()
{
	Case c(L"unicode");
	std::wstring cafe = JoinPath(c.downloads, L"Caf\x00E9");
	Placement p;
	CHECK(Place(Make(cafe, L"\x00C9pisode \x00FC" L"ber.gob", "accented"), c.ep, p));
	CHECK(p.copied && p.name == L"Episode_uber.gob");
	CHECK(Content(JoinPath(c.ep, L"Episode_uber.gob")) == "accented");

	Placement q;
	CHECK(Place(Make(c.downloads, L"\x65E5\x672C\x8A9E.gob", "japanese"), c.ep, q));
	CHECK(q.copied && q.name == L"episode.gob");
}

static void TestRefusals()
{
	Case c(L"refusals");
	std::wstring before = List(c.ep);
	std::wstring error;
	Placement p;
	CHECK(!PlaceGob(Make(c.downloads, L"readme.txt", "text"), c.ep, p, error));
	CHECK(error.find(L"is not a .gob file") != std::wstring::npos);
	error.clear();
	CHECK(!PlaceGob(JoinPath(c.downloads, L"missing.gob"), c.ep, p, error));
	CHECK(error.find(L"was not found") != std::wstring::npos);
	CHECK(List(c.ep) == before);

	// A missing Episode folder is created.
	Case d(L"noepisode");
	Wipe(d.ep);
	Placement q;
	CHECK(Place(Make(d.downloads, L"Episode.gob", "one"), d.ep, q));
	CHECK(q.copied && GetFileInfo(JoinPath(d.ep, L"Episode.gob")).exists);
}

// The empty folder of the episode's name: made, recorded, swapped for the next one, and removed by gobinator.exe alone.
static void TestListFirst()
{
	Case c(L"listfirst");
	Placement p;
	CHECK(List1st(c.ep, L"Episode", p));
	CHECK(List(c.ep) == L"Episode\\ JK1.GOB gobinator.state" && p.removed.empty());
	Placement q;
	CHECK(List1st(c.ep, L"Episode", q) && q.removed.empty());			// again: nothing changes
	CHECK(List(c.ep) == L"Episode\\ JK1.GOB gobinator.state");

	// The next episode: its folder made, the earlier one removed.
	Placement r;
	CHECK(List1st(c.ep, L"Other", r));
	CHECK(r.removed.size() == 1 && r.removed[0] == L"Episode\\Episode");
	CHECK(List(c.ep) == L"JK1.GOB Other\\ gobinator.state");

	// A folder of the player's (DeaTH-WaLK's, with its readme) is used and never removed.
	Make(JoinPath(c.ep, L"DeaTH-WaLK"), L"dw_readme.txt", "the player's");
	Placement s, t;
	CHECK(List1st(c.ep, L"DeaTH-WaLK", s) && s.removed.size() == 1 && s.removed[0] == L"Episode\\Other");
	CHECK(List1st(c.ep, L"Next", t) && t.removed.empty());
	CHECK(Content(JoinPath(c.ep, L"DeaTH-WaLK\\dw_readme.txt")) == "the player's");

	// gobinator.exe alone: Gobinator's folders out, the record gone. One the player has put a file in stays.
	Make(JoinPath(c.ep, L"Next"), L"notes.txt", "the player's");
	Placement u;
	UnlistAll(c.ep, u);
	CHECK(u.removed.empty() && u.notes.size() == 1);
	CHECK(List(c.ep) == L"DeaTH-WaLK\\ JK1.GOB Next\\");
	Placement v;
	UnlistAll(c.ep, v);
	CHECK(v.removed.empty() && v.notes.empty());						// forgotten: never removed later

	Placement w, x;
	CHECK(List1st(c.ep, L"Episode", w));
	UnlistAll(c.ep, x);
	CHECK(x.removed.size() == 1 && List(c.ep) == L"DeaTH-WaLK\\ JK1.GOB Next\\");
}

// GOBs an earlier v1.1 build copied into the mod folder (with its record) come out; the player's stay.
static void TestRemoveCopies()
{
	Case c(L"cleanup");
	std::wstring old = Make(c.mod, L"bunker.gob", "an earlier build's copy");
	Make(c.mod, L"Changed.gob", "copied");
	Make(c.mod, L"keep.gob", "player's own");
	FileInfo a = GetFileInfo(old), b = GetFileInfo(JoinPath(c.mod, L"Changed.gob"));
	WriteFileBytes(JoinPath(c.mod, L"gobinator.state"), ToUtf8(L"# an earlier build's record\r\n"
		L"bunker.gob\t" + std::to_wstring(a.size) + L"\t" + std::to_wstring(a.written) + L"\r\n"
		L"Changed.gob\t" + std::to_wstring(b.size) + L"\t" + std::to_wstring(b.written) + L"\r\n"));
	WriteFileBytes(JoinPath(c.mod, L"Changed.gob"), "the player edited it");

	Placement p;
	RemoveCopies(c.mod, p);
	CHECK(p.removed.size() == 1 && p.removed[0] == L"bunker.gob");
	CHECK(List(c.mod) == L"Changed.gob keep.gob");							// the record gone too
	Placement q;
	RemoveCopies(c.mod, q);
	CHECK(q.removed.empty() && q.notes.empty());
}

void RunModFolderTests()
{
	wchar_t temp[MAX_PATH];
	GetTempPathW(MAX_PATH, temp);
	root = JoinPath(temp, L"gobinator-unit\\mod");
	CreateDirectoryW(JoinPath(temp, L"gobinator-unit").c_str(), nullptr);
	CreateDirectoryW(root.c_str(), nullptr);

	TestSafeGobName();
	TestCopyIntoEpisode();
	TestGobAlreadyInEpisode();
	TestNeverOverwritesThePlayers();
	TestUnicode();
	TestRefusals();
	TestListFirst();
	TestRemoveCopies();
}

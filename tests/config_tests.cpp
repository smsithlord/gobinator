// Unit tests for the gobinator.ini reader (src/config.cpp).
#include "check.h"
#include "../src/config.h"
#include "../src/files.h"

#include <string>

static bool HasNote(const Config& c, const wchar_t* text)
{
	for (const auto& n : c.notes)
		if (n.find(text) != std::wstring::npos)
			return true;
	return false;
}

static std::wstring SourceDir()
{
	std::string dir = GOBINATOR_SOURCE_DIR;
	return std::wstring(dir.begin(), dir.end());	// an ASCII path on the build machine
}

// The user's live ini, in v1.0's form (reference/gobinator.ini), and the two edits that wrecked it in v1.0.
static void TestUsersIni()
{
	std::string bytes;
	CHECK(ReadFileBytes(JoinPath(SourceDir(), L"reference\\gobinator.ini"), bytes));
	const std::string variants[] = {
		bytes,
		"\xEF\xBB\xBF" + bytes,		// saved as UTF-8 with a BOM (scenario F)
		[&] { std::string b = bytes; b.insert(b.find("\nMOD_FOLDER") + 1, "\r\n"); return b; }(),	// a blank line (G)
	};
	for (const auto& v : variants) {
		Config c = ParseConfig(v);
		CHECK(c.legacyForm);
		CHECK(c.errors.empty());
		CHECK(c.modFolder == L"mod");
		CHECK(c.jkExecutable == L"inject.exe");
		CHECK(c.launchOptions == L"-devmode");
		CHECK(!c.jkFolder);
		CHECK(HasNote(c, L"MOD_FOLDER_PURGE_EXCLUDE_FILE is no longer needed"));
		CHECK(!HasNote(c, L"BFP_RANDOMIZE"));		// ignored without complaint
		CHECK(!HasNote(c, L"unknown setting"));
	}
}

static void TestLegacyEmptyValue()
{
	Config c = ParseConfig("ADDITIONAL_LAUNCH_OPTIONS\n\nJK_EXECUTABLE\njk.exe\n");
	CHECK(c.legacyForm);
	CHECK(c.launchOptions == L"");
	CHECK(c.jkExecutable == L"jk.exe");
	CHECK(HasNote(c, L"has no value line"));

	c = ParseConfig("# only a key\nJK_EXECUTABLE\n");
	CHECK(!c.jkExecutable);
	CHECK(HasNote(c, L"has no value line"));
}

static void TestKeyValue()
{
	Config c = ParseConfig("JK_EXECUTABLE = inject.exe\r\nADDITIONAL_LAUNCH_OPTIONS = -devmode\r\n");
	CHECK(!c.legacyForm);
	CHECK(c.errors.empty());
	CHECK(c.jkExecutable == L"inject.exe");
	CHECK(c.launchOptions == L"-devmode");
	CHECK(!c.modFolder);
	CHECK(c.notes.empty());

	c = ParseConfig(
		"# comment\n"
		"\n"
		"; another\n"
		"[gobinator]\n"
		"  jk_folder =  \"C:\\Games\\Jedi Knight\"  \n"
		"Mod_Folder=mods\\ep1\n"
		"ADDITIONAL_LAUNCH_OPTIONS =\n");
	CHECK(c.errors.empty());
	CHECK(c.jkFolder == L"C:\\Games\\Jedi Knight");
	CHECK(c.modFolder == L"mods\\ep1");
	CHECK(c.launchOptions == L"");		// set to nothing: no options at all
	CHECK(!c.jkExecutable);

	c = ParseConfig("\xEF\xBB\xBFJK_EXECUTABLE=jk.exe\n");
	CHECK(!c.legacyForm);
	CHECK(c.jkExecutable == L"jk.exe");

	c = ParseConfig("JK_EXECUTABLE = a.exe\nJK_EXECUTABLE = b.exe\nFOO = 1\njust words\nJK_FOLDER =\n");
	CHECK(c.jkExecutable == L"b.exe");
	CHECK(!c.jkFolder);
	CHECK(HasNote(c, L"is set again"));
	CHECK(HasNote(c, L"unknown setting FOO"));
	CHECK(HasNote(c, L"has no '='"));

	c = ParseConfig("");
	CHECK(c.errors.empty());
	CHECK(!c.modFolder && !c.jkExecutable && !c.launchOptions && !c.jkFolder);
}

static void TestEncodings()
{
	Config c = ParseConfig("JK_FOLDER = D:\\\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E\n");	// UTF-8 日本語
	CHECK(c.jkFolder == L"D:\\\x65E5\x672C\x8A9E");
	CHECK(c.notes.empty());

	std::string utf16("\xFF\xFE", 2);
	for (wchar_t ch : std::wstring(L"JK_FOLDER = C:\\Caf\x00E9\r\n")) {
		utf16 += (char)(ch & 0xFF);
		utf16 += (char)(ch >> 8);
	}
	c = ParseConfig(utf16);
	CHECK(c.jkFolder == L"C:\\Caf\x00E9");

	c = ParseConfig("JK_FOLDER = C:\\Caf\xE9\n");		// é in the Windows code page: not valid UTF-8
	CHECK(HasNote(c, L"not UTF-8"));
	CHECK(c.jkFolder && c.jkFolder->rfind(L"C:\\Caf", 0) == 0 && c.jkFolder->size() == 7);
}

static void TestModFolderChecks()
{
	CHECK(ParseConfig("MOD_FOLDER = my mods\n").errors.size() == 1);
	CHECK(ParseConfig("MOD_FOLDER = C:\\mod\n").errors.size() == 1);
	CHECK(ParseConfig("MOD_FOLDER = \\mod\n").errors.size() == 1);
	CHECK(ParseConfig("MOD_FOLDER = ..\\mod\n").errors.size() == 1);
	CHECK(ParseConfig("MOD_FOLDER = mods/../..\n").errors.size() == 1);
	CHECK(ParseConfig("MOD_FOLDER = mods\\ep1\n").errors.empty());
	CHECK(ParseConfig("MOD_FOLDER = ..mod\n").errors.empty());
}

static void TestResolveJkFolder()
{
	Config c;
	CHECK(ResolveJkFolder(c, L"C:\\a\\b") == L"C:\\a\\b");
	c.jkFolder = L"..\\JK";
	CHECK(ResolveJkFolder(c, L"C:\\a\\b") == L"C:\\a\\JK");
	c.jkFolder = L"D:\\Games\\JK\\";
	CHECK(ResolveJkFolder(c, L"C:\\a\\b") == L"D:\\Games\\JK");
	c.jkFolder = L"\\\\server\\share\\JK";
	CHECK(ResolveJkFolder(c, L"C:\\a\\b") == L"\\\\server\\share\\JK");
}

// Writing JK_FOLDER into an ini of either form: the folder picker's answer (docs/DECISIONS.md, 2026-10-30).
static void TestSetIniValue()
{
	const std::wstring folder = L"D:\\Games\\Jedi Knight";

	// The packaged ini: its commented-out example becomes the setting, in place, and the rest stays as it was.
	std::string packaged;
	CHECK(ReadFileBytes(JoinPath(SourceDir(), L"package\\gobinator.ini"), packaged));
	std::string out = SetIniValue(packaged, L"JK_FOLDER", folder);
	Config c = ParseConfig(out);
	CHECK(c.jkFolder == folder);
	CHECK(c.errors.empty() && c.notes.empty());
	CHECK(out.find("# JK_FOLDER") == std::string::npos);
	CHECK(out.find("# MOD_FOLDER = mod") != std::string::npos);
	CHECK(out.rfind("\xEF\xBB\xBF", 0) == 0);

	// Set again: the setting's line is replaced, not doubled.
	std::string again = SetIniValue(out, L"JK_FOLDER", L"E:\\JK");
	CHECK(ParseConfig(again).jkFolder == L"E:\\JK");
	CHECK(again.find("Jedi Knight") == std::string::npos);

	// No such line: appended. An empty file too.
	Config a = ParseConfig(SetIniValue("# nothing set\r\n", L"JK_FOLDER", folder));
	CHECK(a.jkFolder == folder && a.notes.empty() && !a.legacyForm);
	CHECK(ParseConfig(SetIniValue("", L"JK_FOLDER", folder)).jkFolder == folder);

	// v1.0's form: the value line after the key is replaced, and the form stays.
	Config l = ParseConfig(SetIniValue("MOD_FOLDER\r\nmod\r\nJK_FOLDER\r\nC:\\old\r\nJK_EXECUTABLE\r\ninject.exe\r\n",
		L"JK_FOLDER", folder));
	CHECK(l.legacyForm && l.jkFolder == folder && l.modFolder == L"mod" && l.jkExecutable == L"inject.exe");
	// The key with no value line (another key follows): a value line is inserted.
	Config l2 = ParseConfig(SetIniValue("JK_FOLDER\r\nMOD_FOLDER\r\nmod\r\n", L"JK_FOLDER", folder));
	CHECK(l2.legacyForm && l2.jkFolder == folder && l2.modFolder == L"mod");
	// Without the key: key and value appended.
	Config l3 = ParseConfig(SetIniValue("MOD_FOLDER\r\nmod\r\n", L"JK_FOLDER", folder));
	CHECK(l3.legacyForm && l3.jkFolder == folder && l3.modFolder == L"mod");
	// The user's live ini (v1.0's form, with retired keys in it).
	std::string users;
	CHECK(ReadFileBytes(JoinPath(SourceDir(), L"reference\\gobinator.ini"), users));
	Config u = ParseConfig(SetIniValue(users, L"JK_FOLDER", folder));
	CHECK(u.legacyForm && u.jkFolder == folder && u.jkExecutable == L"inject.exe" && u.modFolder == L"mod");
	CHECK(u.errors.empty());

	// A UTF-16 file, and a folder with accents: read back the same.
	const std::wstring accents = L"D:\\Jeux\\Caf\u00e9 JK";
	const std::wstring w = L"\xFEFF# nothing set\r\n";
	std::string utf16((const char*)w.data(), w.size() * sizeof(wchar_t));
	Config e = ParseConfig(SetIniValue(utf16, L"JK_FOLDER", accents));
	CHECK(e.jkFolder == accents && e.notes.empty());
}

void RunConfigTests()
{
	TestUsersIni();
	TestLegacyEmptyValue();
	TestKeyValue();
	TestEncodings();
	TestModFolderChecks();
	TestResolveJkFolder();
	TestSetIniValue();
}

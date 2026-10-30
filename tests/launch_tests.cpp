// Unit tests for starting JK (src/launch.cpp). The last one starts the fake JK (tests/fakejk.c) for real.
#include "check.h"
#include "../src/files.h"
#include "../src/launch.h"

#include <windows.h>
#include <algorithm>
#include <cwctype>
#include <string>

static std::wstring TempFolder(const wchar_t* name)
{
	wchar_t temp[MAX_PATH];
	GetTempPathW(MAX_PATH, temp);
	CreateDirectoryW(JoinPath(temp, L"gobinator-unit").c_str(), nullptr);
	CreateDirectoryW(JoinPath(temp, L"gobinator-unit\\detect").c_str(), nullptr);
	std::wstring folder = JoinPath(temp, std::wstring(L"gobinator-unit\\detect\\") + name);
	CreateDirectoryW(folder.c_str(), nullptr);
	return folder;
}

// A throwaway JK folder holding empty files with these names.
static std::wstring FakeInstall(const wchar_t* name, std::initializer_list<const wchar_t*> files)
{
	std::wstring folder = TempFolder(name);
	for (auto f : files)
		WriteFileBytes(JoinPath(folder, f), "");
	return folder;
}

static void TestPlanLaunchFromSettings()
{
	Config c;
	c.jkExecutable = L"D:\\OpenJKDF2\\openjkdf2-64.exe";
	c.modFolder = L"mods";
	LaunchSpec s;
	std::wstring error;
	CHECK(PlanLaunch(c, L"C:\\JK", s, error));
	CHECK(s.program == L"D:\\OpenJKDF2\\openjkdf2-64.exe");
	CHECK(s.kind == InstallKind::OpenJkdf2 && !s.detected);
	CHECK(s.arguments == L"-devmode -path mods");		// -devmode when ADDITIONAL_LAUNCH_OPTIONS is not set
	CHECK(s.folder == L"C:\\JK");

	c.launchOptions = L"";
	CHECK(PlanLaunch(c, L"C:\\JK", s, error) && s.arguments == L"-path mods");
	c.jkExecutable = L"inject.exe";
	c.launchOptions = L"-windowGUI";
	CHECK(PlanLaunch(c, L"C:\\JK", s, error) && s.program == L"C:\\JK\\inject.exe");
	CHECK(s.kind == InstallKind::Remastered && s.arguments == L"-windowGUI -path mods");
}

static void TestDetectInstall()
{
	Install i;
	// The user's JK Remastered folder: JkGfxMod and the patched EXEs side by side.
	CHECK(DetectInstall(FakeInstall(L"remastered", { L"inject.exe", L"jkgm.json", L"JediKnight.EXE", L"JKR-fov115.EXE" }), i));
	CHECK(i.kind == InstallKind::Remastered && FileName(i.program) == L"inject.exe");

	// inject.exe without jkgm.json is not JkGfxMod's.
	CHECK(DetectInstall(FakeInstall(L"patched", { L"inject.exe", L"JK.EXE", L"Jedi.EXE", L"JediKnight.EXE" }), i));
	CHECK(i.kind == InstallKind::Steam && FileName(i.program) == L"JediKnight.EXE");	// spelt as on disk

	CHECK(DetectInstall(FakeInstall(L"openjkdf2", { L"openjkdf2-64.exe.bak", L"openjkdf2-64.exe", L"JK.EXE" }), i));
	CHECK(i.kind == InstallKind::OpenJkdf2 && FileName(i.program) == L"openjkdf2-64.exe");
	CHECK(DetectInstall(FakeInstall(L"openjkdf2-32", { L"openjkdf2-64.exe.bak", L"openjkdf2.exe" }), i));
	CHECK(i.kind == InstallKind::OpenJkdf2 && FileName(i.program) == L"openjkdf2.exe");

	CHECK(DetectInstall(FakeInstall(L"retail", { L"jk.exe", L"JK_.CD" }), i));
	CHECK(i.kind == InstallKind::Retail && FileName(i.program) == L"jk.exe");

	CHECK(!DetectInstall(FakeInstall(L"empty", { L"readme.txt" }), i));
	Config c;
	LaunchSpec s;
	std::wstring error;
	CHECK(!PlanLaunch(c, TempFolder(L"empty"), s, error));
	CHECK(error.find(L"No JK program was found") != std::wstring::npos);

	CHECK(PlanLaunch(c, TempFolder(L"remastered"), s, error));
	CHECK(s.detected && s.kind == InstallKind::Remastered && s.arguments == L"-devmode -path mod");
}

// A PE file's headers with sections of these names: "MZ", the PE header's offset at 0x3C, "PE\0\0", the COFF header
// (the number of sections at +6, the optional header's size at +20), the optional header, then 40 bytes a section.
static std::string FakePe(std::initializer_list<const char*> sections)
{
	std::string pe = "MZ" + std::string(0x3A, '\0') + std::string("\x80\0\0\0", 4) + std::string(0x40, '\0');
	pe += std::string("PE\0\0", 4) + std::string(20, '\0');
	pe[0x80 + 6] = (char)sections.size();
	pe[0x80 + 20] = 0x10;
	pe += std::string(0x10, '\0');
	for (auto s : sections) {
		std::string header(s);
		header.resize(40, '\0');
		pe += header;
	}
	return pe;
}

static void TestIsSteamStub()
{
	// Steam's JediKnight.EXE, and the community patch of it (2026-10-27).
	CHECK(IsSteamStub(FakePe({ ".text", ".rdata", ".data", ".idata", ".rsrc", "..idata", ".bind" })));
	CHECK(!IsSteamStub(FakePe({ ".text", ".rdata", ".data", ".idata", ".rsrc", ".text2" })));
	CHECK(!IsSteamStub(FakePe({ ".binder" })) && !IsSteamStub(FakePe({})));
	std::string cut = FakePe({ ".text", ".bind" });
	CHECK(!IsSteamStub(cut.substr(0, cut.size() - 40)));		// the table cut short before .bind
	CHECK(!IsSteamStub("") && !IsSteamStub("MZ") && !IsSteamStub(std::string(4096, 'x')));

	// The fakes the scenarios start: built from tests/fakejk.c, one with a .bind section.
	std::string fake = FAKEJK_PATH, fakeSteam = FAKESTEAMJK_PATH, bytes;
	CHECK(ReadFileBytes(std::wstring(fake.begin(), fake.end()), bytes) && !IsSteamStub(bytes));
	CHECK(ReadFileBytes(std::wstring(fakeSteam.begin(), fakeSteam.end()), bytes) && IsSteamStub(bytes));
}

static void TestSameFolder()
{
	std::wstring a = TempFolder(L"remastered"), b = TempFolder(L"retail");
	CHECK(SameFolder(a, a));
	CHECK(SameFolder(a, a + L"\\"));
	CHECK(SameFolder(a, JoinPath(b, L"..\\remastered")));
	std::wstring upper = a;
	for (auto& c : upper)
		c = towupper(c);
	CHECK(SameFolder(a, upper));
	CHECK(!SameFolder(a, b));
	CHECK(!SameFolder(a, JoinPath(a, L"no such folder")));
	CHECK(SameFolder(L"C:\\no such\\folder", L"c:\\No Such\\Folder\\"));	// by name, when neither can be opened
}

static void TestCommandLine()
{
	LaunchSpec s{ L"H:\\Star Wars Jedi Knight Remastered\\inject.exe", L"-devmode -path mod", L"" };
	CHECK(CommandLine(s) == L"inject.exe -devmode -path mod");		// what v1.0's START produced
	s.program = L"C:\\Games\\Jedi Knight.exe";
	s.arguments = L"-path mod";
	CHECK(CommandLine(s) == L"\"Jedi Knight.exe\" -path mod");
}

static std::wstring ReadUtf8(const std::wstring& path)
{
	std::string bytes;
	if (!ReadFileBytes(path, bytes))
		return L"";
	size_t skip = bytes.rfind("\xEF\xBB\xBF", 0) == 0 ? 3 : 0;
	int n = MultiByteToWideChar(CP_UTF8, 0, bytes.data() + skip, (int)(bytes.size() - skip), nullptr, 0);
	std::wstring text(n, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, bytes.data() + skip, (int)(bytes.size() - skip), &text[0], n);
	text.erase(std::remove(text.begin(), text.end(), L'\r'), text.end());		// the fake writes in text mode
	return text;
}

static void TestLaunchMissing()
{
	LaunchSpec s{ L"C:\\no such folder\\jk.exe", L"-path mod", L"C:\\no such folder" };
	std::wstring error;
	CHECK(!Launch(s, error));
	CHECK(error.find(L"was not found") != std::wstring::npos);
}

// The fake JK, copied as inject.exe into a TEMP folder with a space and an é in its name, started through Launch.
// Its log shows the command line and the working directory it got.
static void TestLaunchFake()
{
	wchar_t temp[MAX_PATH];
	GetTempPathW(MAX_PATH, temp);
	std::wstring folder = JoinPath(temp, L"gobinator-unit\\Fake JK Caf\x00E9");
	CreateDirectoryW(JoinPath(temp, L"gobinator-unit").c_str(), nullptr);
	CreateDirectoryW(folder.c_str(), nullptr);
	std::wstring log = JoinPath(folder, L"launched.txt");
	DeleteFileW(log.c_str());

	std::string fake = FAKEJK_PATH;
	CHECK(CopyFileW(std::wstring(fake.begin(), fake.end()).c_str(), JoinPath(folder, L"inject.exe").c_str(), FALSE));

	LaunchSpec s{ JoinPath(folder, L"inject.exe"), L"-devmode -path mod", folder };
	std::wstring error;
	CHECK(Launch(s, error));
	CHECK(error.empty());

	std::wstring text;
	for (int i = 0; i < 50 && text.find(L"---") == std::wstring::npos; i++) {
		Sleep(100);
		text = ReadUtf8(log);
	}
	CHECK(text.find(L"cmdline=inject.exe -devmode -path mod\n") != std::wstring::npos);
	CHECK(text.find(L"cwd=" + folder + L"\n") != std::wstring::npos);
}

void RunLaunchTests()
{
	TestPlanLaunchFromSettings();
	TestDetectInstall();
	TestIsSteamStub();
	TestSameFolder();
	TestCommandLine();
	TestLaunchMissing();
	TestLaunchFake();
}

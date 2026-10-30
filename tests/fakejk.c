// Stands in for jk.exe / inject.exe in a scratch folder: appends its command line and working directory
// to launched.txt beside itself, then exits. A GUI-subsystem app, so START opens no window.
// Built a second time as fakesteamjk.exe, with a section named .bind, the mark of Steam's DRM (IsSteamStub).
#include <windows.h>
#include <stdio.h>
#ifdef FAKE_STEAM_STUB
#pragma section(".bind", read)
__declspec(allocate(".bind")) const char steamStub[] = "not Steam's stub: a fake for Gobinator's tests";
#endif
int WINAPI wWinMain(HINSTANCE h, HINSTANCE p, PWSTR cmd, int show)
{
#ifdef FAKE_STEAM_STUB
    if (cmd == NULL)
        OutputDebugStringA(steamStub);	// keeps the linker from dropping the section
#endif
    wchar_t exe[MAX_PATH], log[MAX_PATH], cwd[MAX_PATH];
    GetModuleFileNameW(NULL, exe, MAX_PATH);
    wcscpy_s(log, MAX_PATH, exe);
    wchar_t *slash = wcsrchr(log, L'\\');
    wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - log), L"launched.txt");
    GetCurrentDirectoryW(MAX_PATH, cwd);
    FILE *f = _wfopen(log, L"a, ccs=UTF-8");
    if (f) { fwprintf(f, L"exe=%s\ncmdline=%s\ncwd=%s\n---\n", exe, GetCommandLineW(), cwd); fclose(f); }
    return 0;
}

#include "firstrun.h"
#include "resource.h"

#include <windows.h>
#include <shobjidl.h>
#include <string>

namespace
{
	bool Quiet()
	{
		return GetEnvironmentVariableW(L"GOBINATOR_NO_MESSAGES", nullptr, 0) != 0;
	}

	std::wstring Env(const wchar_t* name)
	{
		std::wstring value(32767, L'\0');
		DWORD n = GetEnvironmentVariableW(name, &value[0], (DWORD)value.size());
		if (n == 0 || n >= value.size())
			return L"";
		value.resize(n);
		return value;
	}
}

bool PickJkFolder(const std::wstring& startFolder, std::wstring& chosen)
{
	chosen.clear();
	if (Quiet()) {
		chosen = Env(L"GOBINATOR_PICK_FOLDER");
		return !chosen.empty();
	}

	HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	bool picked = false;
	IFileOpenDialog* dialog = nullptr;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
		DWORD options = 0;
		dialog->GetOptions(&options);
		dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);
		dialog->SetTitle(L"Gobinator: where is your Jedi Knight folder?");
		dialog->SetOkButtonLabel(L"Use this folder");
		if (!startFolder.empty()) {
			IShellItem* start = nullptr;
			if (SUCCEEDED(SHCreateItemFromParsingName(startFolder.c_str(), nullptr, IID_PPV_ARGS(&start)))) {
				dialog->SetFolder(start);
				start->Release();
			}
		}
		if (SUCCEEDED(dialog->Show(nullptr))) {
			IShellItem* item = nullptr;
			if (SUCCEEDED(dialog->GetResult(&item))) {
				PWSTR path = nullptr;
				if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
					chosen = path;
					CoTaskMemFree(path);
					picked = !chosen.empty();
				}
				item->Release();
			}
		}
		dialog->Release();
	}
	if (SUCCEEDED(init))
		CoUninitialize();
	return picked;
}

bool AskToPickAgain(const std::wstring& folder)
{
	if (Quiet())
		return false;
	std::wstring text = folder + L"\n\nholds no Jedi Knight program. Gobinator looks for inject.exe (with jkgm.json), "
		L"openjkdf2-64.exe, JediKnight.exe and JK.EXE: choose the folder that has one of them.\n\nChoose another folder?";
	return MessageBoxW(nullptr, text.c_str(), L"Gobinator", MB_OKCANCEL | MB_ICONWARNING | MB_SETFOREGROUND) == IDOK;
}

std::string PackagedIni()
{
	HRSRC found = FindResourceW(nullptr, MAKEINTRESOURCEW(IDR_PACKAGED_INI), RT_RCDATA);
	HGLOBAL loaded = found ? LoadResource(nullptr, found) : nullptr;
	const char* data = loaded ? (const char*)LockResource(loaded) : nullptr;
	if (!data)
		return "# gobinator.ini - Gobinator's settings. Every setting is optional.\r\n";
	return std::string(data, SizeofResource(nullptr, found));
}

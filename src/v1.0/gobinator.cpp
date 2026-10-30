/*

GOBINATOR - Episode GOB associated JK launcher. Launch into the episode GOB you want to play.

Originally made by SM Sith Lord.

Gobinator's goal is to get you directly into the world of a GOB file w/ as few clicks as possible.  And as a side effect - it also is a way to get around the episode file limit in JK's episode folder - just double click on the GOB you want to host, and it's the only one in your episode list.

Gobinator is an EXE I made that I associated with Jedi Knight: Dark Forces 2 GOB files - which are episode world files - among other things.

When you double click on a GOB file, Gobinator is executed.  It COPIES the target GOB into your JK's mod folder, and then launches JK.  The episode in your GOB is the ONLY one that appears on any episode lists.

-devmode is also turned on, so you can quickly get to the world by choosing your player, going to Singleplayer, Debug, double click on the episode, and then the level. That's 5 interactions with a menu before you are put into the world.  Not the best, but a decent start for vanilla JK.

I may add the option for a hax cog to be auto-generated when you launch into the world that teleports you to the particular location & direction (and sector) based on which screenshot you launch from - so you spawn exactly where the screenshot shows.  But for now, I'll simply take my screenshot at the default spawn position, which will be where you spawn at anyways.

*/

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <vector>
#include <locale>
#include <codecvt>
#include "derent.h"
#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <wchar.h>
using namespace std;

#ifdef _UNICODE
std::wostream& COUT = std::wcout;
#else
std::ostream& COUT = std::cout;
#endif

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

struct Option_t
{
	std::string name;
	std::string value;
};

vector<string> get_all_gobs_within_folder(string folder)
{
	vector<string> fileNames;

	DIR *dpdf;
	struct dirent *epdf;
	dpdf = opendir(folder.c_str());
	if (dpdf != NULL){
		while (epdf = readdir(dpdf)){
			fileNames.push_back(epdf->d_name);
		}
	}
	closedir(dpdf);

	vector<string> gobNames;
	string gobName;
	for (unsigned int i = 0; i < fileNames.size(); i++) {
		gobName = fileNames[i];
		auto it = gobName.find(".gob");
		if (it != std::string::npos ) {//== gobName.end()) {
			gobNames.push_back(gobName);
		}
	}

	return gobNames;
}

//bool Startup(LPCTSTR lpApplicationName, LPWSTR args)
bool Startup(string path, string app, string args)
{
	ofstream batFile("./gobinator.bat");
	string batContents = "";
	batContents += app.substr(0, 1) + ":" + "\n";
	batContents += "cd \"" + app.substr(0, app.find_last_of("/\\", app.find("\"", 1))) + "\"" + "\n";
	batContents += "START \"Launching JK...\" " + args;
	cout << batContents << "\n";
	batFile << batContents;

	batFile.close();
	system("gobinator.bat");

	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
	bool debugMode = false;
	unsigned int major = 1;
	unsigned int minor = 0;
	cout << "GOBINATOR v" << major << "." << minor << "\n";

	wchar_t szPath[MAX_PATH];
	GetModuleFileNameW(NULL, szPath, MAX_PATH);

	//wstring path = szPath;
	wstring path = argv[0];
	wstring gob = (argc > 1) ? argv[1] : L"";
	size_t found = path.find_last_of(L"/\\");
	path = path.substr(0, found+1);

	wstring config_file = path + L"gobinator.ini";

	// Load the options from a text file then save them into a vector of structs.
	std::ifstream loadFile;
	loadFile.open(config_file, std::ifstream::in);

	bool isVal = false;

	char buf[1024];
	vector<Option_t*> options;
	Option_t* pOption = NULL;

	string modPath;

	// This declares a lambda, which can be called just like a function
	auto getOption = [&](string name)
	{
		for (unsigned int i = 0; i < options.size(); i++) {
			if (options[i]->name == name) {
				return options[i];
			}
		}
		return (Option_t*)NULL;
	};

	// This declares a lambda, which can be called just like a function
	auto isExcludedFile = [&](string name)
	{
		for (unsigned int i = 0; i < options.size(); i++) {
			if (options[i]->name == "MOD_FOLDER_PURGE_EXCLUDE_FILE" && options[i]->value == name) {
				return true;
			}
		}
		return false;
	};

	// This declares a lambda, which can be called just like a function
	auto deleteFile = [&](string file)
	{
		size_t nNumChars;
		wchar_t wfile[512];
		mbstowcs_s(&nNumChars, wfile, file.c_str(), file.length() + 1);
		DeleteFile(wfile);
	};

	if (loadFile.good())
	{
		while (loadFile.good())
		{
			loadFile.getline(buf, 1024);
			if (buf[0] == '#') {
				continue;
			}
			else if (!isVal)
			{
				pOption = new Option_t;
				pOption->name = buf;
			}
			else if (pOption)
			{
				pOption->value = buf;
				options.push_back(pOption);
				pOption = NULL;
			}

			isVal = !isVal;
		}

		if (debugMode) {
			cout << "Options values are:\n";
			for (unsigned int i = 0; i < options.size(); i++) {
				pOption = options[i];
				cout << pOption->name << "\t" << pOption->value << "\n";
			}
			cout << "\n";
		}

		string modFolder = "mod";
		Option_t* pModFolder = getOption("MOD_FOLDER");
		if (pModFolder) {
			modFolder = pModFolder->value;
		}
		string trailingCharacter = "\\";

		modPath = converter.to_bytes(path) + modFolder + trailingCharacter;

		// purge all ".gob" files in the MOD_FOLDER path that are not on the EXCLUDE list.
		vector<string> gobs = get_all_gobs_within_folder(modPath);
		for (unsigned int i = 0; i < gobs.size(); i++) {
			if (isExcludedFile(gobs[i])) {
				if (debugMode) {
					cout << "EXCLUDE " << gobs[i] << "\n";
				}
				continue;
			}
			string file = modPath + gobs[i];
			if (debugMode) {
				cout << "DELETE " << gobs[i] << "\n";
			}
			deleteFile(file);
		}

		// are we given a GOB to use?

		if (debugMode) {
			system("pause");
		}

		string gobFile = converter.to_bytes(gob);
		string gobFileName = gobFile;
		found = gobFileName.find_last_of("/\\");
		gobFileName = gobFileName.substr(found + 1);

		if (gobFileName.find(".gob") != std::string::npos) {
			std::string targetPath = modPath + gobFileName;
			std::string victimPath = gobFile;

			if (debugMode) {
				cout << "TARGET: " << targetPath << "\n";
				cout << "VICTIM: " << victimPath << "\n";
			}

			size_t numChars;
			wchar_t wVictimPath[512];
			mbstowcs_s(&numChars, wVictimPath, victimPath.c_str(), victimPath.length() + 1);
			wchar_t wTargetPath[512];
			mbstowcs_s(&numChars, wTargetPath, targetPath.c_str(), targetPath.length() + 1);
			CopyFile(wVictimPath, wTargetPath, false);
		}

		if (debugMode) {
			system("pause");
		}

		if (debugMode) {
			cout << "\nLaunching Jedi Knight...\n";
		}
		// Now launch JK
		std::string name = converter.to_bytes(path);

		string jkExecutable = "jk.exe";
		Option_t* pJKExecutable = getOption("JK_EXECUTABLE");
		if (pJKExecutable) {
			jkExecutable = pJKExecutable->value;
		}

		name += jkExecutable;
		Option_t* pLaunchOptions = getOption("ADDITIONAL_LAUNCH_OPTIONS");
		string launchOptions = (pLaunchOptions) ? pLaunchOptions->value : "";

		std::string launchArgs = (launchOptions != "") ? jkExecutable + " " + launchOptions + " -path " + modFolder : jkExecutable + " -path " + modFolder;

		if (debugMode) {
			cout << "App:\t" << name << "\nArgs:\t" << launchArgs << "\n";
		}

		size_t numChars;
		wchar_t wtext[512];
		mbstowcs_s(&numChars, wtext, launchArgs.c_str(), launchArgs.length() + 1);

		std::wstring appName = converter.from_bytes(name);
		LPWSTR appArgs = wtext;

		if (debugMode) {
			cout << "App LPCTSTR:\t";
			wcout << appName;
			cout << "\nArgs LPWSTR:\t";
			wcout << appArgs;
			cout << "\n";
		}

		if (debugMode) {
			system("pause");
		}

		if (Startup(converter.to_bytes(path), name, launchArgs)) {
			cout << "Launched JK.\n";
		}
		else {
			cout << "Failed to launch JK. :(\n";
			system("pause");
		}
	}
	else {
		cout << "Could not load gobinator.ini";
		system("pause");
	}

	return 0;
}


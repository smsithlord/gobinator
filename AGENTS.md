# Gobinator

Instructions for any agent (Claude Code, Codex) working in this repo.

## What this is

Gobinator is a small Windows launcher for **Jedi Knight: Dark Forces II** (retail JK, "vanilla"). The user associates
`.gob` files with it, so double-clicking an episode GOB becomes a one-step shortcut into the game:

1. Gobinator **copies** the GOB into JK's mod folder (`mod` by default).
2. It **purges** every other `.gob` in that folder, except the ones on its exclude list, so the double-clicked episode
   is the only one in the game's episode lists. This also gets around the episode-folder limit.
3. It launches JK with `-devmode -path mod` (and any extra options you configure).

It works with plain retail JK (`jk.exe`) and with the user's **JK Remastered** install, through that package's
`inject.exe` (see below).

That is v1.0. **v1.1** (docs/DECISIONS.md, 2026-10-27) copies the GOB into JK's `Episode` folder (where it stays),
makes an empty folder of the same name beside it, which lists it first (JK lists folders before files), removes the
folder it made for the episode before, and starts JK with `-path mod`, so the player's own mods there load on top;
`gobinator.exe` with no GOB removes its folders and starts JK with the usual list. On OpenJKDF2 the episode's TYPE
picks `-autostart -sp` (single player) or `-mp` (deathmatch, hosted on this PC), anything else the menu; a GOB with no
`episode.jk` (a mod) is left where it is and JK starts at its menu (docs/DECISIONS.md, 2026-10-29).

The user wrote v1.0 in March 2022. The studio's hub (smsithlordcom) put the project on the board on 2026-10-27, at the
user's request, to pick it back up. **Start with docs/START-HERE.md.**

**JK Resurrection** (`../jkresurrection`, a separate project on the board) is the user's JK companion app. It is a mod
manager and launcher too, so it will do Gobinator's job and much more. Gobinator stays the simple, targeted tool for
players who want only this. The two projects share no code and neither depends on the other. Gobinator's source sat in
JK Resurrection's folder until 2026-10-27 only because it was written there.

## What is here

| Path | What it is |
|---|---|
| `src/` | v1.1 (docs/PLAN.md; the board's epic "Gobinator v1.1"): `main.cpp` (the run, the log, the message on failure), `config.*` (gobinator.ini, reading it and writing `JK_FOLDER` into it), `firstrun.*` (the folder picker when no JK is found, and the packaged ini), `modfolder.*` (the copy and `gobinator.state`), `launch.*` (CreateProcessW), `install.*` (which JK the folder holds), `gob.*` (reading episode.jk inside a GOB), `files.*` (paths, long paths), `version.h` (the version: the log, the EXE's properties, the zip's name), `gobinator.rc` and `resource.h` (the icon `media/gobinator.ico`, the packaged ini, the version). |
| `package/` | `gobinator.ini`, the commented ini shipped in the zip and built into the EXE (written beside it when there is none), and `make-zip.ps1`, which runs the tests and makes `dist/gobinator-<tag>.zip` (gitignored) with the EXE, the ini, `README.md` and `LICENSE` (CC0). `docs/BETA-1-TESTS.md` is the beta's test plan. |
| `media/` | The icon, the clear logo, the key art, the screenshots, and `listing/`, the profile package for the hub's Library and NavMe (its docs/PROFILES.md). `media/README.md` says what each is and how it was made; `python media/source/build.py` rebuilds the art. The art is AI-made (Nano Banana 2 and Pro, 2026-10-29). |
| `tests/capture-media-shots.ps1` | The listing's screenshots, with no window: `gobinator.exe` on COG Factory's The Crab Factory in the OpenJKDF2 test install, rendered at 1920x1080 by openjkdf2_libretro's harness. |
| `src/v1.0/` | The v1.0 source (last changed 2022-03-10), unchanged, moved here from JK Resurrection's folder. A Visual Studio console app (`_tmain`); its project file is lost, `stdafx.h` is rebuilt. `derent.h` is the `dirent` shim for MSVC (Toni Ronkko, MIT), which JK Resurrection also uses. |
| `CMakeLists.txt` | Builds both, the tests and the fake JK: `cmake -G "Visual Studio 17 2022" -A Win32 -B build`, `cmake --build build --config Release`, `ctest --test-dir build -C Release`. |
| `tests/*_tests.cpp`, `tests/check.h` | Unit tests for v1.1 (run by ctest); they start the fake JK, in TEMP. |
| `tests/run-v11-scenarios.ps1` | v1.1's acceptance scenarios: the built `gobinator.exe` in throwaway installs in TEMP, against the fake JK, with the user's real ini; PASS or FAIL per check. It sets `GOBINATOR_NO_MESSAGES`, which keeps failures in `gobinator.log` instead of a dialog; set it for any automated run. |
| `tests/run-v1-scenarios.ps1`, `tests/fakejk.c` | Runs the built v1.0 in throwaway JK-like installs in TEMP, one per scenario, against a fake JK that only logs how it was started. |
| `tests/make-openjkdf2-install.ps1` | Builds `testinstall/openjkdf2/` (gitignored, ~200 MB): upstream OpenJKDF2 v0.9.8 and JK's data, COPIED OUT of the user's MOTS install and openjkdf2_libretro's test data (both read-only), plus the test episode P03.GOB in `testinstall/fixtures/`. OpenJKDF2 writes its settings into its working folder, so it only ever runs on this copy. |
| `testinstall/openjkdf2-v099/` | A second test copy: upstream OpenJKDF2 v0.9.9 (the official `win64-debug.zip`, `gh release download v0.9.9 --repo shinyquagsire23/OpenJKDF2 --pattern win64-debug.zip`, extracted to its own folder), built with `make-openjkdf2-install.ps1 -Engine <that folder> -Name openjkdf2-v099 -ProfileFrom openjkdf2`. The profile in both copies has "Disable mission start confirmation" on; only v0.9.9 honours it under `-autostart`. |
| `tests/copy-install.ps1`, `testinstall/steam-vanilla/`, `testinstall/mots-vanilla/`, `testinstall/gog-vanilla/` | Copies of the user's vanilla Steam JK and MOTS (`H:\SteamLibrary\steamapps\common\Star Wars Jedi Knight Vanilla` and `...Mysteries of the Sith Vanilla`) and GOG JK (`H:\GOGLibrary\Star Wars Jedi Knight - Dark Forces 2 Vanilla`), all read-only, offered for tests on 2026-10-27. Retail JK keeps its settings in the registry, shared by every retail-style JK: running a copy changes them as playing does. **A copy of a Steam install cannot be run**: Steam's JediKnight.exe hands any start to Steam, which runs the registered install (the user's patched one) with the same switches; `run-desktop.ps1` refuses one, and v1.1 stops with a message. GOG's `JK.EXE` runs with admin rights, so `run-desktop.ps1` cannot close it: the user closes it. A game window needs the user's go. |
| `tests/run-openjkdf2-boot.ps1` | End to end with no window: `gobinator.exe` copies P03.GOB into the test install's `mod`, then openjkdf2_libretro's headless harness boots from that folder (`--content-path` = `-path`) and must reach `piano.jkl`; the control, with `mod` empty, must not. Starting the *desktop* OpenJKDF2 opens a game window: ask the user first. |
| `tests/run-desktop.ps1` | The real desktop check: `gobinator.exe` starts the test install's `openjkdf2-64.exe` straight into P03's level; it screenshots the window and closes it. It opens a game window: **only with the user's go**. |
| `reference/gobinator.ini` | The user's live config, copied from the JK Remastered install (2024-12-11), in v1.0's form. Its comments were the only user documentation before `README.md`. |
| `reference/gobinator.bat` | A launch script Gobinator wrote. It writes one into its working directory on every run. |

## Outside this repo (read-only)

The user's JK installs, vanilla references and play folders alike, are listed in `docs/INSTALLS.md` (2026-10-27).
The Vanilla folders are never written or run in; tests use copies in `testinstall/`.

- **The user's JK install**, `H:\SteamLibrary\steamapps\common\Star Wars Jedi Knight Remastered`. "Jedi Knight
  Remastered" (from ModDB) is **retail JK**, not an OpenJKDF2 fork: patched retail executables (`JediKnight.EXE`,
  `JKR-fov*.EXE`), `JK.dll` / `JK-Extension.dll`, plus **JkGfxMod** (jdmclark/jkgfxmod: `inject.exe`, `renderer.dll`,
  `jkgm.json`, `jkgm/`, its docs in `doc/`).
  - `inject.exe` is JkGfxMod's launcher. It starts the executable named by `"command"` in `jkgm.json` (here
    `jkr-fov115.exe`) with JkGfxMod's renderer injected, and Gobinator's arguments reach the game through it. That is
    why the user's ini sets `JK_EXECUTABLE` to `inject.exe`.
  - The installed Gobinator is there too: `gobinator.exe` (v1.0, 2022-03-10), `gobinator.ini`, `gobinator.bat`.
- **The user's own Gobinator moves to `H:\Installed Programs\JK Remastered`** (their primary single-player JK, 2026-10-27;
  docs/DECISIONS.md): the v1.1 build is placed there. Write there only with the user's go.
  - It is not one of our projects and it is the user's own game folder.
- `../jkresurrection`: JK Resurrection. Its launcher (`resurrection/MainApp.cpp`, `ActuallyLaunchJK`) grew out of
  Gobinator and has the same purge, plus a BFP character randomizer. Read it for reference only.

## Rules

- **Never run Gobinator, or anything that writes, against the user's JK install** without the user's go. The purge
  **deletes** every GOB in the mod folder that is not on the exclude list. Test against a scratch folder that mimics a
  JK install (a fake `jk.exe`, a `mod/` folder holding dummy GOBs).
- **Run v1.0 (or any build that still launches through a `.bat`) only through `tests/run-v1-scenarios.ps1`**, or copy
  its guard. v1.0 runs `system("gobinator.bat")` and `START <exe>` by bare name, so cmd *searches* for them. Claude
  Code's shell sets `NoDefaultCurrentDirectoryInExePath=1`, which makes cmd skip the scratch folder, and a 32-bit cmd's
  `system32` on PATH is really `C:\Windows\SysWOW64`, which held a `gobinator.bat` that launched the user's real JK.
  That happened on 2026-10-27. The guard puts a logging decoy first on PATH, turns the current-folder search back
  on, and refuses to run unless a 32-bit cmd resolves every name to the decoy. That bat was not stray: v1.0 writes it
  there on every double-click through the association (its working folder is System32, which Windows redirects to
  SysWOW64 for a 32-bit program, and a RUNASADMIN layer on its path gives it the rights). The association left v1.0
  and the user deleted the bat (2026-10-27); any run of v1.0 outside the guard writes it again.
- **Windows' "Open with" knows a program by its file name**: `HKCU\Software\Classes\Applications\gobinator.exe` holds
  one path, and choosing another `gobinator.exe` in "Look for another app" reuses that entry (2026-10-27: v1.0 ran
  instead of the new build). To try a build without touching the association, drag a GOB onto its EXE.
- **The `.gob` file association is the user's.** Since 2026-10-27 (the user's go) it runs
  `H:\Installed Programs\JK Remastered\gobinator.exe` (docs/DECISIONS.md). Never change it, or the registry, or
  install anything system-wide, unless the user asks.
- **Credentials:** never read, print, log or commit secrets (the user's key and login files under their home folder's `.claude\`).
- **Other folders are read-only**, the install and `../jkresurrection` included. To ask another project something,
  send it a handoff through the board.
- **Ask the user** whenever a goal or a trade-off is theirs to decide. Ask in small batches, and record each answer in
  `docs/DECISIONS.md` with its date.
- Commit focused work in this repo and push `main` to `origin`, the public repo https://github.com/smsithlord/gobinator
  (CC0, since 2026-10-30, opened with release v1.1; `main` began then as one commit). **Never push the branch
  `archive/2026-10-development`**: it is the development history before the release, kept local on purpose (28 MB of
  superseded art, and the user's working notes); commit hashes quoted in docs/ and on the board before 2026-10-30
  refer to it. A new release is `src/version.h`, `package\make-zip.ps1`, then `gh release create` with the zip; the
  user gives the go.

## The board

This project is on the SM Sith Lord board: workspace `gobinator`, prefix `gob`. The board holds its todo list, its mail
with the other projects, and the studio's knowledge base of shared techniques (`coord kb list`). Read
`G:\Documents Sym Links\GitHub\smsithlordcom\coordination\AGENTS.md` once, then run `coord ready` at the start of every
session. In PowerShell the command is `coord.cmd`; if neither works, use
`node "G:\Documents Sym Links\GitHub\smsithlordcom\tools\coord\coord.mjs"`.

# Gobinator v1.0: what it does, and what goes wrong

v1.0 is `src/gobinator.cpp` (last changed 2022-03-10). Everything below was read from that source and, where marked
**tested**, confirmed by running the rebuilt EXE (`CMakeLists.txt`) through `tests/run-v1-scenarios.ps1` on
2026-10-27. Each run is a throwaway JK-like install in TEMP, with a fake JK that only logs how it was started. The
scenario letters (A to I) are the harness's.

## Why it exists

The user associates `.gob` files with Gobinator. A double-click on an episode GOB copies it into JK's mod folder and
starts JK with `-path mod`, so the player finds that episode in the list straight away.

The mod folder is the point (the user, 2026-10-27). JK treats a `-path` folder like its Resource folder: every GOB in
it always loads, and its episode always shows in the list. The Episode folder is different: retail JK detects only
the first N GOBs in it, so episodes past the limit never appear. Putting the episode in the mod folder gets around
that limit. The purge keeps the folder down to the one episode the player asked for, plus the always-on mods on the
exclude list (BFP2X, JKE and the like).

## What it does, step by step

1. Prints `GOBINATOR v1.0`.
2. Takes its own folder from `argv[0]`. It calls `GetModuleFileNameW` but never uses the result.
3. Reads `gobinator.ini` from that folder. If the file is missing, it prints `Could not load gobinator.ini`, pauses,
   and stops. Nothing is purged or launched (**tested**, I).
4. **Purges** `<its folder>\<MOD_FOLDER>\`. Every entry whose name contains `.gob` is deleted with `DeleteFile`
   unless its name is on the exclude list exactly. Nothing goes to the Recycle Bin.
5. **Copies** the double-clicked file into the mod folder under its own name, if that name contains `.gob`. An
   existing file of that name is overwritten.
6. **Launches** JK. It writes `gobinator.bat` into its **working directory**:
   ```
   <drive of the JK EXE>:
   cd "<folder of the JK EXE>"
   START "Launching JK..." <JK_EXECUTABLE> [<ADDITIONAL_LAUNCH_OPTIONS>] -path <MOD_FOLDER>
   ```
   Then it runs that file with `system("gobinator.bat")`, prints `Launched JK.` and exits.
7. The player still has five menu steps to reach the level: choose a player, Singleplayer, Debug, double-click the
   episode, then the level (v1.0's header). `-devmode` is what puts Debug in the menu.

The EXE must sit in the JK folder: the ini is read from beside it, the mod folder is resolved from it, and
`JK_EXECUTABLE` is joined to it for the `cd`.

### The ini format

```
# a comment: any line starting with #, anywhere
MOD_FOLDER                      <- a name line
mod                             <- its value: the next line that is not a comment
JK_EXECUTABLE
inject.exe
ADDITIONAL_LAUNCH_OPTIONS
-devmode
MOD_FOLDER_PURGE_EXCLUDE_FILE   <- may repeat, once per GOB to keep
BFP2X.gob
```

| Key | Default | Use |
|---|---|---|
| `MOD_FOLDER` | `mod` | The folder under the EXE's folder that is purged and copied into, and passed as `-path`. |
| `JK_EXECUTABLE` | `jk.exe` | The EXE that is started, relative to the JK folder. The user's is `inject.exe` (JkGfxMod). |
| `ADDITIONAL_LAUNCH_OPTIONS` | none | Inserted before `-path`. The user's is `-devmode`. |
| `MOD_FOLDER_PURGE_EXCLUDE_FILE` | none | A file name the purge leaves alone. Compared exactly, case included. |

Lines alternate name, value, name, value, with `#` lines skipped. Blank lines are **not** skipped: they count as a
name or a value. Lines are cut at 1023 characters.

The user's ini also has `BFP_RANDOMIZE_CHARACTER_FILE`, `BFP_RANDOMIZE_MODEL` and `BFP_RANDOMIZE_SABER`. v1.0 never
reads them (it reads only the four keys above). They belong to JK Resurrection's launcher, which grew out of Gobinator,
and they stay there (docs/DECISIONS.md).

## The defects

Ordered by harm. **Tested** means the harness showed it; the rest are from reading the code.

| # | Defect | What happens | Evidence |
|---|---|---|---|
| 1 | **The purge deletes outright** | Every unlisted GOB in the mod folder is gone for good. No Recycle Bin, no prompt, no log. | **Tested**, A, H |
| 2 | **Double-clicking a GOB that is already in the mod folder deletes it** | The purge runs first and deletes it, then the copy has no source. The player's GOB is lost. | **Tested**, C |
| 3 | **One blank line, or a byte-order mark, wrecks the ini** | Every pair after it shifts by one line. The exclude list comes out empty, so the GOBs it protects are **deleted**. `JK_EXECUTABLE` and the options are lost too, so `jk.exe` starts instead of `inject.exe`, without `-devmode`. Notepad's "UTF-8 with BOM" is enough. | **Tested**, F (BOM), G (blank line) |
| 4 | **The exclude list is case-sensitive; Windows names are not** | `BFP2X.gob` on the list does not protect `Bfp2x.gob` on disk, which is deleted. | **Tested**, A |
| 5 | **`.gob` is matched case-sensitively, anywhere in the name** | `notes.gob.txt` and `my.goblin.cfg` are deleted. `JK1.GOB` is never purged. A double-clicked `EPISODE.GOB` is not copied, but the purge has already run, so JK starts with the episode missing. | **Tested**, A, B |
| 6 | **It runs a `gobinator.bat`, not necessarily the one it wrote** | `system("gobinator.bat")` lets cmd search for the file. When cmd skips the current folder (`NoDefaultCurrentDirectoryInExePath`, or the write failed), it runs the first one on PATH. On this machine that is `C:\Windows\SysWOW64\gobinator.bat`, a stray file the installed v1.0 left on 2026-09-04, which starts the user's real JK. That is how an unguarded test run launched the game on 2026-10-27. | **Tested**, by accident; the harness now guards against it |
| 7 | **A `.bat` is left wherever it was started** | The working directory gets a `gobinator.bat`: the folder of the double-clicked GOB (A), the mod folder (C), the JK folder (H), and once even `SysWOW64` (defect 6). Writing there needs admin rights, so that run was elevated, with system32 as its working directory. | **Tested**, A, C, H |
| 8 | **Non-ASCII names and folders fail silently** | Paths go from UTF-16 to UTF-8 and back through `mbstowcs_s` in the C locale, which garbles them. `Épisode über.gob`, `日本語.gob` and `Café\level.gob` are not copied, the purge still runs, and JK starts without the episode. | **Tested**, D1, D2, E |
| 9 | **It cannot run on a player's machine** | The installed v1.0 is a VS 2013 *Debug* build: it needs `MSVCP120D.dll` and `MSVCR120D.dll`, which only machines with VS 2013 have and which may not be redistributed. | `dumpbin /dependents` on the installed EXE |
| 10 | **It always says it launched** | `Startup` always returns true, so `Launched JK.` prints even when the `.bat` cannot be written or the EXE is not there. A missing EXE leaves only Windows' own error dialog from `START`. | Reading |
| 11 | **Paths with spaces or odd shapes break the launch line** | The `START` line is unquoted, so a `JK_EXECUTABLE` or `MOD_FOLDER` containing spaces splits. The drive line is the path's first character plus `:`, which breaks for UNC paths (`\\server\share`) and relative ones. | Reading |
| 12 | **Long paths** | Every conversion goes into a 512-character buffer, and `mbstowcs_s` ends the process when a path is longer. Before that, `CopyFile` and `DeleteFile` already fail past `MAX_PATH` (260). | Reading |
| 13 | **Its folder comes from `argv[0]`** | It works from Explorer, whose association passes the full path. Started by bare name from another folder (on PATH), it looks for the ini in the working directory instead. | Reading |
| 14 | **The same-named file is overwritten** | Double-clicking a GOB whose name matches an excluded one (say a different `JKE.gob`) replaces the player's copy. | Reading |

Observed once, not yet understood: when the real JK was started this way (defect 6, from a background process), it
needed a click on its window before it reached the main menu. Whether that also happens on a normal double-click
matters for "fewer clicks" (docs/PLAN.md).

## The test scenarios

`tests/run-v1-scenarios.ps1` builds each install fresh, with dummy GOBs in `mod/`, and a fake `jk.exe` and `inject.exe`
that log their command line and working directory. Results on 2026-10-27:

| | Scenario | Mod folder before → after | Copied | Launched |
|---|---|---|---|---|
| A | Double-click `Downloads\Episode.gob`; exclude list `keep.gob`, `BFP2X.gob` | `Bfp2x.gob JK1.GOB keep.gob my.goblin.cfg notes.gob.txt other.gob readme.txt` → `Episode.gob JK1.GOB keep.gob readme.txt` | yes | `inject.exe -devmode -path mod` in the JK folder |
| B | Double-click `EPISODE.GOB` | `keep.gob other.gob` → `keep.gob` | **no** | yes |
| C | Double-click `mod\mine.gob` itself | `keep.gob mine.gob` → `gobinator.bat keep.gob` | **no; the GOB is gone** | yes |
| D1 | `Downloads\Épisode über.gob` | `other.gob` → empty | **no** | yes |
| D2 | `Downloads\日本語.gob` | `other.gob` → empty | **no** | yes |
| E | `Downloads\Café\level.gob` | `other.gob` → empty | **no** | yes |
| F | The ini saved with a BOM; `keep.gob` excluded | `keep.gob other.gob` → `Episode.gob` (**`keep.gob` deleted**) | yes | `jk.exe -path mod` (**not** `inject.exe`, no `-devmode`) |
| G | One blank line in the ini; `keep.gob` excluded | the same as F | yes | the same as F |
| H | No GOB (the EXE started on its own) | `keep.gob other.gob` → `keep.gob` | n/a | yes |
| I | No `gobinator.ini` | unchanged | no | no; message and pause |

To rerun: build (`cmake --build build --config Release`), then `& tests\run-v1-scenarios.ps1` in PowerShell. Read the
guard at the top of the script, and the rule in AGENTS.md, before changing how it launches anything.

## Leftovers on the user's machine

- `C:\Windows\SysWOW64\gobinator.bat` (2026-09-04): it starts the real JK Remastered. It is harmless for normal
  double-clicks, but any 32-bit program that runs `gobinator.bat` by name and misses its own will find it. Deleting it
  needs admin rights, so that is the user's call.
- A `gobinator.bat` in every folder the user ever double-clicked a GOB from.

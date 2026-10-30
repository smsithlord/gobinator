# The next Gobinator (v1.1): the plan

The goal is v1.0's: a double-click on an episode GOB puts it where JK will list it, and starts the game, in one
step. The next version keeps that and removes what makes v1.0 risky (docs/OVERVIEW.md). Decided already
(docs/DECISIONS.md, 2026-10-27):

- The purge removes only the GOBs Gobinator copied in.
- `BFP_RANDOMIZE_*` stays with JK Resurrection.
- It is for other players too.
- No installer: the config sits beside the EXE, and the `.gob` association is switched by hand.
- CC0; the public repo opens with the first release.
- It supports JK Remastered, the user's community-patched JK, Steam and GOG retail, and OpenJKDF2.

This was proposed and picked on 2026-10-27; the user's picks are at the end, and the work is on the board (epic
"Gobinator v1.1").

## What a double-click does in v1.1

1. It reads `gobinator.ini` from beside the EXE. The EXE finds its own folder with `GetModuleFileNameW`, not `argv[0]`.
2. It finds the JK folder: `JK_FOLDER` if set, else the EXE's own folder (as in v1.0).
3. It finds the program to start: `JK_EXECUTABLE` if set, else the one it detects (part 5).
4. It copies the GOB into JK's `Episode` folder, unless it is there already; the copy stays, one of the player's
   episodes from then on.
5. It makes an empty folder of the episode's name beside it (`Episode\X` next to `Episode\X.gob`) if there is none,
   which lists the episode first, and removes the folder it made for the episode before (part 1, and
   docs/DECISIONS.md, 2026-10-27). With no GOB given, it only removes its folders: `gobinator.exe` alone is the
   clean-up.
6. It starts JK with `CreateProcessW`, in the JK folder, with `-path mod`: the player's own mods load on top. No
   `.bat`, no cmd, no files left behind.
7. If anything fails, it shows a message saying what failed, which file or folder, and what to do.

## 1. The purge removes only its own GOBs (decided)

**Superseded on 2026-10-27** (docs/DECISIONS.md): the GOB now goes into `Episode`, where it stays, and an empty
folder of its name lists it first; what Gobinator removes is that folder (recorded in `Episode\gobinator.state`),
never a GOB. A GOB double-clicked inside `Episode` is used where it is, and copies earlier v1.1 builds left in `mod`
are removed on every run. Nothing of the player's is ever overwritten. The text below is the mod-folder design as
first built.

- Gobinator keeps a small record in the mod folder (`gobinator.state`), so the record always describes the folder it
  sits in: each GOB it copied, with the size and time written. Next time, it removes a recorded GOB only if both
  still match. A GOB that changed is left alone and forgotten.
- A file of the same name already in the mod folder, not copied by Gobinator, is never overwritten (v1.0's defect
  14). If the content is identical, JK loads it as it is. If not, the copy gets another name (`Episode_2.gob`): JK
  lists an episode by the title inside its GOB, not by its file name. (Built 2026-10-27; the first proposal stopped
  with a message instead.)
- A GOB double-clicked inside the mod folder is neither copied nor removed (v1.0 deleted it: defect 2). With no GOB
  given, the mod folder is left as it is.
- The copy's name keeps letters, digits, `.`, `_` and `-`. Accented letters become their plain letter, and anything
  else becomes `_`, since JK opens files in the ANSI code page and OpenJKDF2's `-episode` takes no spaces (part 3).
- One episode at a time, as in v1.0. The player's own GOBs in the mod folder stay and keep loading, so their episodes
  list too. That is the trade the decision makes.
- Why `mod` and not `Resource` or OpenJKDF2's `mods\` (measured 2026-10-27, `tests/run-mount-experiment.ps1`, at the
  user's request): JK looks a file up in the `-path` folder first, then the current episode's own GOB, then
  `Resource` (where OpenJKDF2 also mounts `mods\`), and it builds its episode list from the `Episode` folder, reading
  each entry's `episode.jk` in that order. In `mod`, P03's `episode.jk` wins for the entry, so New Game starts P03 and
  Debug lists only its level. In `Resource` or `mods\`, the entry's own GOB wins: New Game starts the original
  campaign, byte-identical to no P03 at all, and P03 cannot be reached from the menus. A direct start (OpenJKDF2's
  `-autostart -episode P03`) reaches it from all three. A GOB in `Resource` or `mods\` also loads under every game
  started without Gobinator, which is what `mods\` is for.
  - The exception (the user's Steam JK lists `Resource\DuelOfFates_SE.gob`): an `Episode` entry with no `episode.jk`
    of its own, such as an empty subfolder, reads the first one in `Resource`. Measured: P03 in `resource` plus an
    empty `Episode\Gobinator` folder lists "COG Factory p03 Piano" at the top (subfolders are listed first), above
    the campaign, and choosing it plays piano.jkl as episode `Gobinator`.
  - None of the user's always-on mods (BFP2X, JKE, JKR, KCSJ, Scope, shelvapatch, war, XtremeFin, EnhancementMod,
    the HD Kyle) holds an `episode.jk`, so in `mod` they never change which episode is listed; they meet the episode
    only where both replace the same file, and the GOB first in folder order wins there.
- From v1.0: the exclude list is read and ignored. GOBs v1.0 left in a mod folder are not on the record, so they stay
  (the user's mod folder holds none today).

## 2. Launch without a `.bat` (fixes defects 6, 7, 10, 11)

`CreateProcessW` takes the full path of the EXE and a quoted command line, and starts it in the JK folder (JkGfxMod's
`inject.exe` needs that folder to find `jkgm.json`). If the start fails, Windows' reason goes into the message. To
check: whether JK still needs a click on its window before it reaches the menu (seen once, docs/OVERVIEW.md). If it
does, Gobinator hands it the foreground.

## 3. Unicode and long paths (fixes defects 8, 12, 13)

Wide strings end to end, and `\\?\` paths for copy and delete. One open question: retail JK opens its GOBs with ANSI
file calls, so a GOB named `日本語.gob` may not load even once it is copied correctly. Recommended: copy a GOB under
its own name when the name is plain ASCII, and under a safe ASCII name otherwise. To confirm on a real JK.

## 4. The config file

Recommended: `gobinator.ini` in the usual `key = value` form, with `#` or `;` comments, blank lines allowed, and a BOM
allowed. A file in v1.0's form (pairs of lines, no `=`) is still read, so the user's ini keeps working unchanged.

| Key | Default | |
|---|---|---|
| `JK_FOLDER` | the EXE's folder | New. Lets Gobinator live outside the JK folder. |
| `MOD_FOLDER` | `mod` | As in v1.0. |
| `JK_EXECUTABLE` | detected (part 5) | As in v1.0. When set, it always wins over detection. |
| `ADDITIONAL_LAUNCH_OPTIONS` | none | As in v1.0. |

The other way is to keep v1.0's form only, made tolerant (skip blank lines, strip a BOM, trim spaces). That is simpler,
but it is a form new players have never seen.

## 5. Telling the installs apart

Detection runs only when `JK_EXECUTABLE` is not set. In this order:

| Install | Recognised by | Starts | Notes |
|---|---|---|---|
| JK Remastered | `inject.exe` with `jkgm.json` beside it | `inject.exe` | JkGfxMod starts `jkgm.json`'s `"command"` (here `jkr-fov115.exe`) and passes the options on: the user's setup today. |
| OpenJKDF2 | `openjkdf2*.exe` | that EXE | Takes `-path` like retail, and can start a level directly (part 6). Its EXE names are to confirm. |
| JK's Steam release, and the user's community-patched JK | `JediKnight.exe` | `JediKnight.exe` | The Steam release's game EXE (the vanilla copy, 2026-10-27); the user's patched folder (`…\Star Wars Jedi Knight`) replaces it, and also has `JK.EXE` and `Jedi.EXE`. Steam's own EXE carries Steam's DRM (a `.bind` section) and hands every start to Steam, which runs the install Steam registered: in any other folder Gobinator stops with a message naming that folder (gob-9kg6). The patched EXE has no DRM and runs where it is. |
| Retail (CD, and GOG) | `JK.EXE` | `JK.EXE` | GOG's is DRM-free (confirmed 2026-10-27 on a copy; `JKStart.exe` is only its launcher). Windows starts it with admin rights, so Gobinator starts it through Windows' prompt (silent on the user's machine). |

`-devmode` stays the default option for retail-style installs, since v1.0 relies on the Debug menu.

## 6. Fewer clicks after launch (decided)

- **OpenJKDF2: none.** Since 2023-10 upstream OpenJKDF2 takes `-autostart -sp -episode <…> -map <…>` and goes straight
  into the level. Measured 2026-10-27 on the desktop, with General > "Disable mission start confirmation" on in the
  profile: upstream **v0.9.9 lands in the level with zero clicks** (the user kept hands off); v0.9.8 ignores that
  setting under autostart and waits for one click on the mission screen. The README says: v0.9.9 or later, with that
  option on. Gobinator reads the GOB's `episode.jk` for the first level. OpenJKDF2 quits on a switch it does not
  know, so Gobinator adds the four only when the EXE holds them (v0.9.1 and later), and `-doNotResumeLast` (the
  user's fork) the same way. GOB and episode files are simple formats
  that the `jk-content` skill already covers. This is also what makes OpenJKDF2 the place to develop and test.
- **Retail and JK Remastered: the menu steps stay.** Retail JK cannot skip player selection or any other click (the
  user). The one exception is a launch option that skips the intro video, which goes into the default options. What
  Gobinator gives retail players is the episode, easy to find in the list because it sits in the mod folder.
- v1.0's own idea, a generated COG that spawns the player where a screenshot was taken, goes to the icebox
  (START-HERE).

## 7. Messages a player can act on

No console window (a Windows-subsystem EXE). A message box appears only when something goes wrong, and it names the
file or folder and says what to do: "gobinator.ini not found beside gobinator.exe (C:\…)", "JK not found in C:\…: set
JK_FOLDER or JK_EXECUTABLE", "mod\X.gob already exists and is not one Gobinator copied". Each run also writes
`gobinator.log` beside the EXE (the last run only), for when a player asks for help. Since 2026-10-30 the two
first-run cases ask instead of failing: no `gobinator.ini` means the packaged defaults (and the file is written), and
no JK program where Gobinator looks opens Windows' folder picker, whose answer becomes `JK_FOLDER`
(docs/DECISIONS.md, `src/firstrun.cpp`).

## 8. Shipping it (decided: no installer)

A zip holding `gobinator.exe`, a commented `gobinator.ini` and a README: put both files in the JK folder, then
right-click a `.gob` → Open with → Choose another app → Gobinator, with "Always use this app" ticked. The build is
32-bit with the static runtime, so it needs no redistributable. The user switches their own association by hand.

## 9. Testing

- The scenario harness becomes v1.1's acceptance test. Each v1.0 defect gets a scenario with the expected new result,
  run against the fake JK.
- **Most development happens on OpenJKDF2** (the user). With `-autostart`, and its headless harness (the `jk-content`
  skill), a check can run end to end, from the double-click to the episode's level loading, with no one at the
  keyboard.
- Real JK is tested at milestones, or when something needs it, only with the user's go and on the vanilla copies the
  user prepares (gob-qw5x).

## The order of work

1. Config: the reader (both forms), the EXE's own folder, `JK_FOLDER`.
2. Launch through `CreateProcessW`, with real errors.
3. The purge of its own GOBs, and the Unicode copy.
4. Messages, the log, no console window.
5. The acceptance scenarios in the harness.
6. Detection: OpenJKDF2 first (the development bed), then JK Remastered and the community-patched JK; Steam and GOG
   after gob-qw5x.
7. OpenJKDF2 autostart, and an end-to-end check through OpenJKDF2's headless harness.
8. The user tries it on the real installs (needs gob-qw5x).
9. README, licence, the release zip; the public repo with the first release, on the user's go.

## The user's picks (2026-10-27)

1. **The config file:** `key = value`, still reading v1.0's form.
2. **Fewer clicks:** OpenJKDF2 autostarts. Retail keeps its menu steps, plus the intro-skip option. Development
   happens mostly on OpenJKDF2.
3. **The console window:** none; messages only on errors, and a log of the last run.
4. **Where Gobinator lives:** in the JK folder by default, with `JK_FOLDER` as an option.

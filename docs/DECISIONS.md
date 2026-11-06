# Decisions

The user's answers and rulings, newest last, each with its date.

- **2026-10-27 — Gobinator becomes a project of its own on the board** (the user, to the studio's hub). It is picked
  back up as the targeted tool ordinary players may want separately from JK Resurrection. It works with vanilla JK and
  with the JK Remastered package (through its `inject.exe`). Its v1.0 source moved here from JK Resurrection's folder.
  The next version's direction is open until the user reads `docs/PLAN.md` (docs/START-HERE.md).
- **2026-10-27 — v1.0 rebuilds with CMake** (the agent's call; START-HERE left it open). `CMakeLists.txt` builds
  `src/gobinator.cpp` **unchanged** as a 32-bit Unicode console app, the shape of the installed v1.0 (x86, console,
  `_tmain` → `wmain`). The only addition is `src/stdafx.h` (two includes). CMake rather than a `.vcxproj`: one short
  text file, readable in review, and VS 2022 opens it as a folder. One deliberate difference: the static runtime
  (`/MT`), because the installed v1.0 is a VS 2013 *Debug* build that needs `MSVCP120D.dll` / `MSVCR120D.dll`, which
  only machines with VS 2013 have. It also builds `fakejk.exe`, the stand-in JK for `tests/run-v1-scenarios.ps1`.
- **2026-10-27 — The installs it supports** (the user): JK Remastered through `inject.exe`; the user's
  community-patched JK at `H:\SteamLibrary\steamapps\common\Star Wars Jedi Knight\JediKnight.exe`; Steam retail; GOG
  retail; OpenJKDF2. Steam and GOG retail can be tested only once the user has set them up (their Steam copy is
  modded, and they do not own the GOG release yet). Once compatibility with each is established, most development can
  happen on OpenJKDF2 if prototyping is faster there. Before real work presses forward, the user prepares vanilla
  copies of the Steam and JK Remastered installs for testing: a blocker on the user.
- **2026-10-27 — The purge removes only Gobinator's own GOBs** (the user). Gobinator remembers which GOBs it copied
  into the mod folder and removes only those. The player's own GOBs are never touched, and the exclude list is no
  longer needed.
- **2026-10-27 — The `BFP_RANDOMIZE_*` options stay with JK Resurrection** (the user). Gobinator stays a one-job tool
  and ignores those keys without complaint.
- **2026-10-27 — Gobinator is for other JK players too** (the user): a public download others can install. The
  repo, licence and installer details come with the plan.
- **2026-10-27 — No installer** (the user). Gobinator stays a plain EXE with its config file beside it, as v1.0 is.
  The user switches their own `.gob` association to a new build by hand when they want to; Gobinator ships no setup
  step for it, and players associate `.gob` files the same way (Windows' "Open with").
- **2026-10-27 — Licence: CC0**, like JK Resurrection (the user). The bundled dirent shim keeps its own MIT notice.
- **2026-10-27 — The public GitHub repo opens with the first release** (the user). Until then the work stays local.
- **2026-10-27 — The plan's four picks** (the user, on docs/PLAN.md):
  1. The config is `key = value` (blank lines, comments and a BOM allowed), and v1.0's two-line form is still read.
  2. Fewer clicks: retail JK cannot skip player selection or any click, except through a launch option that skips the
     intro video. What Gobinator gives retail players is the episode easy to find in the list, from the mod folder. In
     OpenJKDF2, `-autostart` starts straight into the level, so **most development happens on OpenJKDF2** until a
     milestone, or some other reason, calls for testing in the real JK. No research item on retail clicks.
  3. No console window: a message only when something fails, and a log of the last run.
  4. Gobinator lives in the JK folder by default, and an optional `JK_FOLDER` key lets it live elsewhere.
- **2026-10-27 — The user's primary single-player JK is the JK Remastered play folder** (the user):
  `H:\Installed Programs\JK Remastered` (JK Remastered 3.3, OpenJKDF2 upgraded to v0.9.9). The user's own Gobinator
  lives there from now on: the v1.1 build and the shipped `gobinator.ini` (nothing set: Gobinator detects OpenJKDF2
  and autostarts) were placed there on 2026-10-27, not yet run. The user switches the `.gob` association to it by
  hand. The old v1.0 (in the JK Remastered v1.0 Steam folder, which the user will remove) stays the association's
  target until then.
- **2026-10-27 — A Steam JK folder that Steam does not start: warn and stop** (the user, gob-9kg6; offered: warn and
  stop, warn then start anyway, copy into Steam's folder). When the program is Steam's DRM'd EXE (a `.bind` section)
  and the JK folder is not the install Steam registered (`Install Path` in HKLM\SOFTWARE\WOW6432Node\LucasArts
  Entertainment Company\JediKnight\1.0), Gobinator copies nothing and starts nothing: the message and the log name
  the folder Steam starts and say to put Gobinator there or set `JK_FOLDER` to it. Gobinator never writes outside
  the folder the player chose.
- **2026-10-27 — Go for the desktop run on the GOG vanilla copy** (the user, gob-4xp6), hands off the game window.
- **2026-10-27 — Go to replace `gobinator.exe` in `H:\Installed Programs\JK Remastered`** with the build that has
  gob-hkq3 and gob-9kg6 (the user, gob-kzun): only that file, the ini left alone, nothing run.
- **2026-10-27 — The `.gob` association moves to v1.1** (the user: "I'm ready", after the test plan passed in the
  play folder). Windows' Open-with entry for the name `gobinator.exe`
  (`HKCU\Software\Classes\Applications\gobinator.exe\shell\open\command`, which `.gob`'s UserChoice uses) was changed,
  with the user approving the command, from `"H:\SteamLibrary\steamapps\common\Star Wars Jedi Knight
  Remastered\gobinator.exe" "%1"` (v1.0) to `"H:\Installed Programs\JK Remastered\gobinator.exe" "%1"`. Nothing else
  in the registry was touched. To undo, set that value back to the old path.
- **2026-10-27 — The episode goes into `Resource`, listed through an empty `Episode` folder; `gobinator.exe` alone
  cleans up** (the user, after `tests/run-mount-experiment.ps1`). Gobinator copies the double-clicked GOB into JK's
  `Resource` folder and makes an empty folder in `Episode` if there is none yet (any name will do), so the episode
  is listed at the top, beside the player's others; JK still starts with `-path mod`, so the player's own GOBs in
  `mod` load on top of it. The trade-offs were put to the user: the GOB loads under every start of JK until the next
  double-click, it can replace stock files for every game, and OpenJKDF2's `mods\` outranks it. The user: "Gobinator
  is about playing the GOB you launched, nothing else. So this is all fine." Running `gobinator.exe` with no GOB
  removes the GOB it copied and starts JK without it: the program is its own cleanup utility.
- **2026-10-27 — The copy keeps the GOB's own name** (the user: "we don't want to rename the gob. The only reason an
  episode gob would be in that folder is because we put it there"; the Duel of Fates GOB in the Steam JK's
  `Resource` was left from the user's test). A `!` prefix, built and tested the same day, is dropped. Measured and
  told to the user (`stockclash.py` over the Steam JK's `Episode` folder): 74 of 140 episodes ship stock-named files
  (`kyle.cog`, `00_door.cog`, `items.dat`, mats), which from `Episode` outrank `Res1hi`/`Res2`; from `Resource` only
  a name sorting before `Res1hi.gob` does, and 20 of the 37 named S-Z replace stock files (`SMHQ.gob`,
  `smretribution.gob` among them). Open: the user's view, after the evidence.
- **2026-10-27 — The episode goes into `Episode`, where it stays, listed first by an empty folder of its name**
  (the user's design, after `tests/run-episode-folder-experiment.ps1` showed that an empty `Episode\X` folder beside
  `Episode\X.gob` lists the episode first, once, and past the list's limit; it replaces the `Resource` layout above).
  A double-click copies the GOB into `Episode` if it is not there already (a copy needs no clean-up: it is one of
  the player's episodes from then on), and makes the empty folder of its name if there is none; that folder is the
  clean-up, removed on the next double-click and when `gobinator.exe` starts with no GOB. A folder of that name the
  player already has is used and never removed. JK still starts with `-path mod`. As a real episode again, its own
  versions of stock files win, and it loads only when played. The earlier build's test copy `mod\bunker.gob` is
  removed (the user: "it's a copy anyway").
- **2026-10-27 — A GOB already in `Episode` with a space in its name gets no autostart** (the user asked whether an
  MS-DOS short name could help). OpenJKDF2 splits its command line at spaces with no quotes, and the user's `G:` and
  `H:` keep no 8.3 short names (creation is off per drive, and turning it on needs admin and only covers new files).
  Such an episode is still first in New Game's list. Copies made from elsewhere get a space-free name anyway.
- **2026-10-27 — The stray `C:\Windows\SysWOW64\gobinator.bat` is gone** (the user deleted it; checked). v1.0 wrote
  it on each association run; the association now runs v1.1, which writes no `.bat`.
- **2026-10-27 — Go to replace the play folder's `gobinator.ini` with the newest `package\gobinator.ini`** (the user:
  "next time you can update the gobinator.ini to be the newest one for me"). Standing for the next session: only
  that file in `H:\Installed Programs\JK Remastered`, nothing run. The current one sets nothing, so only its comments
  change.
- **2026-10-29 — Single player or multiplayer from the GOB itself; a mod, or anything unclear, starts at the menu**
  (the user: "Is it possible for us to launch MP or SP depending on the GOB? And can we detect if the GOB is actually
  a MOD, not a level, and if we can't determine if it's an SP or MP level we should just launch to the main menu like
  normal"). On OpenJKDF2, the episode's `TYPE` (read from its `episode.jk` as JK reads it, `jkEpisode.c`) picks the
  mode: 1 (single player) adds `-autostart -sp`, 2 (deathmatch) `-autostart -mp`, which hosts a game on this PC with
  the player in it (upstream's `Main_StartupDedicated`, since 2023-10-11: the stand-in Kyle at rank 0 and the last
  Host Game settings). Any other TYPE opens at the menu, the episode first.
  - **A mod: left where it is, JK starts at its menu** (the user's pick of three; the others were copying it into
    `mod`, or a message on screen). A GOB with no `episode.jk` is a mod: it is not copied into `Episode` (where it would
    do nothing), no folder is made or removed, and `gobinator.log` says to put it in the mod folder. A `.gob` that
    cannot be read as a GOB is treated the same way.
  - **Capture the Flag (TYPE 8) opens at the menu for now** (the user's pick), until a desktop check shows that
    hosting from the command line sets CTF up right.
- **2026-10-29 — The play folder gets the multiplayer-and-mod build** (the user: "Update the one in my play folder
  now", after testing it on the v0.9.9 copy: single player, multiplayer and a mod as intended; a multiplayer episode
  spawned them as the default Kyle Katarn and Windows Firewall asked once, which they allowed; a Capture the Flag
  episode opened at OpenJKDF2's main menu, "like we wanted"). `gobinator.exe` in `H:\Installed Programs\JK
  Remastered` is the build from 0e2534b, byte for byte the one tested, and `gobinator.ini` the newest
  `package\gobinator.ini` (the standing go of 2026-10-27; it still sets nothing). Nothing was run there.
- **2026-10-30 — The logo is the terminal line, `> GOBINATOR_`, and the icon is `GOB` with a bold cursor under
  the O** (the user, from a sheet of twelve alternatives to the rusted-steel wordmark, media/README.md: "F (terminal)
  wins. And for the icon, GOB with the O having a bold underline under that letter"; then: "Clear art version can be
  the whole > GOBINATOR_ look"). Amber phosphor monospace, glowing, with scanlines inside the strokes: the clear
  logo keeps the prompt and the cursor; the icon's letters sit on a dark CRT screen with a thin bezel, so the amber
  reads on light desktops too (our choice, not asked). The key art and the screenshots stay.
- **2026-10-30 — A beta first, then a release candidate, then the release** (the user: "prepare everything we need
  for a release but do a beta build (a release candidate is what it will be once we're ready) instead of an actual
  release and I'll test the beta build (and I'll see if there are others that want to use it as well)"). The version
  is `1.1 beta 1` (`src/version.h`, shown in the log and the EXE's Properties); `package\make-zip.ps1` makes
  `dist\gobinator-1.1-beta1.zip` with the EXE, the commented ini, README.md (with a beta note at the top) and the CC0
  LICENSE, after the tests pass. The public repo still waits for the release itself (gob-63k4). The user will run
  "new user experience" tests on it (docs/BETA-1-TESTS.md).
- **2026-10-30 — The play folder gets the beta 1 build** (the user: "Deploy that new EXE to my folder"). `gobinator.exe`
  in `H:\Installed Programs\JK Remastered` is now the beta 1 EXE, byte for byte the one in the zip: the code of
  0e2534b (the build the user tested on 2026-10-29), plus the terminal icon and the version stamp. The ini there was
  already the package's. Nothing was run there.
- **2026-10-30 — Open question: a folder prompt when no JK is found** (the user, preparing "new user experience"
  tests: "when the GOB is ran without the JK to use ever being specified, what happens? Do they see a prompt like in
  the anarchy arcade/aarcadecore project or some other type of thing that asks where their JK folder to use is?").
  Today it is a message box ("No JK program was found in …"), not a prompt. Proposed to the user (gob board,
  needs-user-decision): a folder picker on that path that writes `JK_FOLDER` into the ini.
- **2026-10-30 — A folder picker when no JK is found** (the user: "I haven't tested or distributed that beta yet,
  implement this feature."; gob-86ns). When the folder `gobinator.exe` sits in, or the one `JK_FOLDER` names, holds
  no JK program (and `JK_EXECUTABLE` is not set), Windows' folder picker asks "where is your Jedi Knight folder?",
  opened on Steam's JK folder when the registry knows one. A folder with no JK program gets "choose another
  folder?"; cancelling leaves the old message and changes nothing. The answer is written as `JK_FOLDER` into
  `gobinator.ini`, which keeps its form (v1.0's included) and its other lines; a missing `gobinator.ini` is written
  from the packaged one, built into the EXE. The scenario tests answer the picker through `GOBINATOR_PICK_FOLDER`
  (under `GOBINATOR_NO_MESSAGES`). **Beta 1 was rebuilt with it before anyone had it**: the play folder and
  `dist\gobinator-1.1-beta1.zip` hold the new build, still called 1.1 beta 1. The user asked again whether the play
  folder had it (it did, checked by hash); its `gobinator.ini` was then refreshed to the day's `package\gobinator.ini`
  under the standing go of 2026-10-27 (comments only: it still sets nothing).
- **2026-10-30 — Capture the Flag stays at the menu, settled** (the user, after a beta run on StarDestroyer.gob,
  TYPE 8, opened at the menu: "Oh I didn't know it was a CTF map. That is correct behavior for CTF, so no change
  needed."). The desktop check the 2026-10-29 pick waited for is answered by OpenJKDF2's source instead: its
  command-line host (`Main_StartupDedicated`) takes the game flags from the saved Host Game settings as they stand,
  and only the Host Game screen adds the team flags, when its Team mode box is ticked; nothing keys off the episode
  being CTF. So a CTF episode started from the command line would run without teams. Gobinator keeps putting CTF
  episodes first in the list and opening the menu. gob-uw3g closed.
  - **The OpenJKDF2 fix is an upstream candidate, noted with OpenJKDF2 Libretro** (the user: "Fixing it for
    OpenJKDF2, that is something suitable for pushing upstream, so make a note of that into OpenJKDF2 Libretro's
    project (they have the project concerned with bookkeeping things suitable for upstream along with AAOpenJKDF2) and
    they will decide on it later and notify any other projects if needed"). Sent as board mail mail-1cvu, a
    suggestion to `openjkdf2libretro`: host a TYPE 8 episode with the team flags from the command line, or take a
    switch for it; Gobinator would then send `-autostart -mp` for CTF too. Nothing here waits on it.
- **2026-10-30 — Ships unsigned for now** (the user: "yes we'll ship unsigned as planned for now", asked what a
  code-signing certificate costs). The EXE imports only Windows' own DLLs (the static runtime), so nothing is bundled;
  the one thing a new user meets is SmartScreen's prompt, which the README covers. The options and prices (OV or EV
  certificates USD 220 to 700 a year; Microsoft Artifact Signing USD 9.99 a month, USA and Canada; SignPath free for
  open source, in its own name and pipeline) are a note on the hub, the knowledge-base technique "Code signing a
  Windows release", at the user's request. If testers hit the prompt, SignPath once the repo is public.
- **2026-10-30 — The last test before publishing is the Steam JK** (the user: "what's left for me to do is to test
  it with my Steam JK ... it has multiple EXEs in that folder, so seeing which one it launches will be part of the
  test ... If that test passes, then we can publish."). At their request the beta EXE was copied into
  `H:\SteamLibrary\steamapps\common\Star Wars Jedi Knight` (no ini: the first run writes it) for a drag-and-drop.
  Read-only detection there picks `JediKnight.EXE`, the community-patched one with no Steam DRM, over `JK.EXE`,
  `Jedi.EXE` and `JediKnightBAK.EXE`; retail gets no autostart, so the game should open at its menu with the episode
  first. Publishing (gob-63k4) follows a pass.
- **2026-10-30 — The Steam JK test passed, and the real-install tests are done** (the user: "I cleaned up my
  folder and tested on one that 007 level pack and it all worked good"). On the community-patched Steam JK, whose
  `Episode` holds 141 GOBs, past retail's cap of 64, 007Levels.gob (deathmatch, already in `Episode`) was used in
  place, its empty folder made, `JediKnight.EXE` picked over `JK.EXE`, `Jedi.EXE` and `JediKnightBAK.EXE`, and the
  episode headed Multiplayer > Host Game's list. The user had first looked in Singleplayer > New Game: a multiplayer
  episode is only in Host Game's list, which README and the test plan now say.
  - **Unexplained**: the first try, Monopoly.gob (Capture the Flag, TYPE 8), was in neither list. Between the two
    runs the user removed their own `Episode\DeaTH-WaLK` (a folder with a readme beside DeaTH-WaLK.gob) and
    `Episode\New folder`, so the episode's type and the other folder entries both changed. The engine code (retail's,
    decompiled in OpenJKDF2's jkEpisode.c) lists subfolders first, skips a GOB whose name matches a folder, and
    loads `<name>.gob` for a folder entry, with Host Game's list filtered to deathmatch and CTF types, so a CTF
    episode should list like a deathmatch one. A retest of Monopoly.gob in the cleaned folder would settle it.
  - GOG's real folder was not tried (optional in gob-kzun; its `JK.EXE` asks for admin rights). gob-kzun closed.
- **2026-10-30 — No release candidate: 1.1 is released after the Steam test, with a follow-up if needed** (the
  user: "We can skip release candidate for this initial release (I'll just do a followup if needed.)").
- **2026-10-30 — There is no retail intro-skip launch option; gob-rqh7 closed** (the user asked for "that retail
  compatible skip video launch option" to go into the defaults first, then agreed to close it when none was found).
  The switches compiled into retail `JK.EXE`, GOG's `JK.EXE`, Steam's vanilla and the community-patched
  `JediKnight.EXE` and JK Remastered 1.0's EXEs are the same nine (`-debug -devMode -displayConfig -dispStats
  -frameRate -noHUD -path -verbose -windowGUI`); nothing in `JK.dll`, `JK-Extension.dll`, `inject.exe`, JKR 3.3's
  `JKStart.exe`, JkGfxMod's config doc, both Remastered configuration guides or the readmes names another. Retail's
  cutscene controls are Escape and the Disable Cutscenes box in General Setup, a per-player setting Gobinator leaves
  alone. The default launch options stay `-devmode`.
- **2026-10-30 — The public repo opens from one fresh commit; the development history stays local** (the user:
  "we need to make sure nothing was leaked in it and that its commit history isn't bloated", then "the plan sounds
  good. Proceed."). The audit found no secret in any file or commit; the commit author email is the one on the
  user's other public repos; one line in AGENTS.md that named the user's home folder was reworded. The 76-commit
  history carried 28 MB of blobs no longer in the tree (25 MB of them the superseded steel logo and icon and the
  first capsules), so `main` was recreated as a single commit, "Gobinator 1.1", from the same tree, and the earlier
  history kept in the local branch `archive/2026-10-development`, never pushed. Commit hashes quoted in this file,
  the board and the media README (06746d0, f57f1dd, 0e2534b, ad273ea, 2114ee9 and others) refer to that branch.
  The internal docs (this file, docs/INSTALLS.md with the user's drive paths, the board files) are published as they
  are: not secrets, the user's call.
- **2026-10-30 — Gobinator 1.1 is out.** The public repo is https://github.com/smsithlord/gobinator (CC0, topics
  jedi-knight, dark-forces-2, openjkdf2, launcher, windows), opened with the release
  https://github.com/smsithlord/gobinator/releases/tag/v1.1, whose asset `gobinator-1.1.zip` (SHA-256
  FBA88D38AA2CAEEC491A34A9321A297E47A53F4A9779D2FEF9B7AD7ECC5F5F9E) holds `gobinator.exe` 1.1, `gobinator.ini`,
  `README.md` and `LICENSE`. The listing (`media/listing/package.json`) names the repo and the download; the hub was
  handed the package to copy. The user's play folder and Steam folder still hold the beta 1 EXE, the same code with
  the beta version stamp, until the user asks for the release EXE there. Unsigned, as decided.
- **2026-10-30 — The site follows the release: the hub refreshes the package, drops "coming soon", and lists
  Gobinator in NavMe under an Apps category** (the user: "Update our media package on the hub and ask them to push
  the update, we are released. We should also go into NavMe under an Apps category."). The hub owns its copy of the
  package and NavMe, and copies from this repo itself, so the ask went by board mail (a handoff to `smsithlordcom`,
  superseding mail-5nne): refresh `library/gobinator` from `media/listing` (the hub's copy had only the 2026-10-29
  package.json and no media), take `soon` off the registry entry and set its GitHub and download, add an Apps
  flyout to NavMe's Library menu with Gobinator in it, and publish.
  - **Done by the hub the same hour** (hub-iuuh, with screenshots): the package refreshed (the new masters kept beside
    the old as `-2` files, the hub never overwrites a master), `soon` gone, download and GitHub set, Gobinator in
    NavMe's new **Apps** flyout (between Websites and Web Demos, its icon a window frame with a download arrow) as
    its first entry, the Library window leading with Download and Source code. Live at
    https://smsithlord.com/?page=library&item=gobinator and https://smsithlord.com/?page=gobinator (checked). A
    future release: mail the hub the new download address. Which other tools join Apps is the user's call.
- **2026-10-30 — A post mortem on the 1.1 release** (the user: "Post mortem", on the board's offer after the release
  milestone). Opened on the board as `gobinator-1-1-how-it-went`, covering gob-u8hi and gob-ruma (2026-10-27 to
  2026-10-30); JK Resurrection, the hub and OpenJKDF2 Libretro are invited to answer, then it closes with what we
  take away. The lessons: keep writing each user decision down the day it is made and measuring the engine before
  building on an assumption; avoid carrying an item whose premise nobody verified, committing generated art before
  the user has picked a direction, and changing two variables between a failing test and its retest.
- **2026-11-05 — Found: SignPath's free signing does not take CC0** (the cycle 10 review of the knowledge-base
  technique). signpath.org/terms: "The project must use an OSI-approved Open Source license without commercial
  dual-licensing for all components", and the OSI never approved CC0 (Creative Commons withdrew it in 2012 over its
  patent clause; opensource.org/faq). So the fallback of 2026-10-30, "if testers hit the prompt, SignPath once the
  repo is public", needs an OSI licence first (MIT-0 or 0BSD are the closest to CC0; the dirent shim is MIT already).
  Nothing changes now: 1.1 stays unsigned and CC0. Relicensing is the user's call, if signing is ever wanted. The
  technique is corrected as r2.
- **2026-11-05 — The post mortem's answers read and replied to.** To OpenJKDF2 Libretro: the four scripts that run
  its harness and core from its build folders, what they depend on, and yes to a pinned 0.2.0 build when ojl-ilag
  tags it. To the hub: its two lessons taken (mail it the day before a release, art to the site only once picked), and
  our package keeps the plain file names. To JK Resurrection: its correction accepted (its current launcher already
  used CreateProcessW when the batch warning came).

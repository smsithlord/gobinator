# Gobinator 1.1 beta 1: what to try, and what should happen

For whoever tests the beta (2026-10-30; rebuilt the same day with the folder picker, runs 13 to 18, before anyone had
it). The zip is `gobinator-1.1-beta1.zip`: `gobinator.exe`, `gobinator.ini`, `README.md`, `LICENSE`. Each run writes
`gobinator.log` beside `gobinator.exe`; when something looks wrong, keep that file and send it with the name of the
GOB and which Jedi Knight you used.

## Setting up

1. Unzip `gobinator.exe` and `gobinator.ini` into the Jedi Knight folder (the one with `openjkdf2-64.exe`, `JK.EXE`
   or `JediKnight.exe`), or anywhere else: then the first run asks where that folder is (runs 13 to 18).
2. Either make `.gob` files open with it (README, Install step 2), or drag a `.gob` onto `gobinator.exe` each time.
   If Gobinator 1.0 was ever on this PC, **Open with** may keep starting it: put the new files where the old ones
   were.

## The runs

Each line: what to do, then what should happen. The log says what Gobinator did, in the same words.

| # | Do | Expect |
|---|---|---|
| 1 | On OpenJKDF2 v0.9.9 (with **Setup > General > Disable mission start confirmation** on), double-click a single-player episode GOB from your Downloads folder | The game opens in the episode's first level. `Episode` now holds the GOB and an empty folder of the same name. |
| 2 | Double-click the same GOB again | The same, and the log says `using`, not `copied`: nothing was copied twice. |
| 3 | Double-click a different episode | Its folder is made, the first one's is removed. In **New Game** the new episode is first. |
| 4 | Start `gobinator.exe` on its own | Gobinator's folder is removed and the game opens at its menu with the usual list. The GOBs stay in `Episode`. |
| 5 | A multiplayer (deathmatch) episode | A game hosted on your PC opens with you in it, with your last **Host Game** settings and the default Kyle. Windows Firewall asks once. |
| 6 | A Capture the Flag episode | The menu, with the episode first in the list. |
| 7 | A mod GOB (no episode in it: a saber or model pack) | Nothing is copied; the game opens at its menu; the log says it is a mod and names the `mod` folder to put it in. |
| 8 | Any file renamed to `.gob` that is not a GOB | Left alone; the menu; the log says it could not be read as a GOB. |
| 9 | A GOB whose name has spaces or accents | Copied under a plain name (`My_Episode.gob`, at most 31 characters) and started as usual. An episode **already** in `Episode` with a space in its name opens at the menu instead, first in the list. |
| 10 | OpenJKDF2 v0.9.8 or older | v0.9.1 to v0.9.8 stop for one click on the mission screen; older versions open at the menu. |
| 11 | JK Remastered 1.0 (JkGfxMod's `inject.exe`), retail or GOG JK | The game opens at its menu as usual, the episode first **in the list for its kind**: a single-player episode in Singleplayer > New Game, a deathmatch or Capture the Flag one in Multiplayer > Host Game's episode box (and not in the other). GOG's `JK.EXE` asks for administrator rights every time. Passed 2026-10-30 on the user's community-patched Steam JK with 007Levels.gob (deathmatch) in an Episode folder of 141 GOBs; `JediKnight.EXE` was picked over `JK.EXE`, `Jedi.EXE` and `JediKnightBAK.EXE`. |
| 12 | `gobinator.exe` and `gobinator.ini` in a copy of a Steam install | A message names the folder Steam really starts and says to move Gobinator there or set `JK_FOLDER`. |

## A new user who puts it anywhere

Gobinator need not sit in the Jedi Knight folder. When the folder it sits in, or the one `JK_FOLDER` names, holds no
game, it asks. Until a game is found, nothing on disk changes. These runs have had no eyes on the dialog itself yet
(the automated runs answer it through a test hook), so the look and the wording of the picker and the "choose
another folder?" box are worth a close look.

| # | Do | Expect |
|---|---|---|
| 13 | Unzip both files somewhere that is not a Jedi Knight folder (Downloads, say) and double-click a GOB | Windows' folder picker, titled "Gobinator: where is your Jedi Knight folder?", with an **Use this folder** button, opened on Steam's JK folder when this PC has one. Choose the game's folder: the GOB goes into that folder's `Episode`, the game starts, and `gobinator.ini` now reads `JK_FOLDER = <that folder>` where its commented example was. |
| 14 | The same, but choose a folder with no game in it | "<folder> holds no Jedi Knight program. Gobinator looks for inject.exe (with jkgm.json), openjkdf2-64.exe, JediKnight.exe and JK.EXE: choose the folder that has one of them. Choose another folder?" **OK** brings the picker back on that folder; **Cancel** ends as run 15 does. |
| 15 | Cancel the picker | A message box: "No JK program was found in <folder>. Gobinator looks for ... Double-click the GOB again and choose your JK folder when asked, or put gobinator.exe and gobinator.ini in that folder, or set JK_FOLDER or JK_EXECUTABLE in gobinator.ini." with the log's path under it. Nothing written, nothing changed. |
| 16 | `gobinator.exe` alone, without `gobinator.ini`, in a Jedi Knight folder | Works as usual, and writes `gobinator.ini` beside itself, every setting a comment. |
| 17 | `gobinator.exe` alone, without `gobinator.ini`, somewhere else | The picker, then a new `gobinator.ini` with the answer in it. |
| 18 | Move the game away, or point `JK_FOLDER` at a folder with no game | Asked again; the new answer replaces the old `JK_FOLDER`. |
| 19 | Windows SmartScreen on first start | "Windows protected your PC": **More info > Run anyway**. The EXE is unsigned. |

# Gobinator

Double-click a Jedi Knight: Dark Forces II episode (a `.gob` file) and play it.

Gobinator copies the episode into your game's `Episode` folder, puts it at the top of the game's episode list, and
starts Jedi Knight. On OpenJKDF2, and on Jedi Knight Remastered 3.3, which runs on it, you land straight in the
episode's first level.

No more episode limit: Jedi Knight lists at most 64 episodes from its `Episode` folder, so in a big collection the
rest never appear. The one you double-click is always first, however many you have.

It works with:

- OpenJKDF2, and Jedi Knight Remastered 3.3
- Jedi Knight Remastered 1.0 (JkGfxMod)
- the Steam, GOG and CD releases of Jedi Knight

It does not handle Mysteries of the Sith.

## Install

Download `gobinator-1.1.zip` from the [latest release](https://github.com/smsithlord/gobinator/releases/latest).

1. Unzip `gobinator.exe` and `gobinator.ini` anywhere you like. The simplest place is your Jedi Knight folder, the
   one that holds the game's EXE (`openjkdf2-64.exe`, `JK.EXE` or `JediKnight.exe`). Anywhere else, Gobinator asks
   where that folder is the first time, and remembers it (as `JK_FOLDER` in `gobinator.ini`). On Steam, the folder
   is the one Steam opens from the game's **Manage > Browse local files**.
2. Make `.gob` files open with Gobinator. Right-click any `.gob` file > **Open with** > **Choose another app**:
   - Windows 10: **More apps** > **Look for another app on this PC**, pick `gobinator.exe`, and tick **Always use
     this app**.
   - Windows 11: **Choose an app on your PC**, pick `gobinator.exe`, then **Always**.

To try it first without step 2, drag a `.gob` file onto `gobinator.exe`.

If Windows says it protected your PC the first time, click **More info** > **Run anyway**. Gobinator is a small,
unsigned program.

## Play

Double-click an episode `.gob`, wherever it is. Your Downloads folder is fine.

- **OpenJKDF2 and Jedi Knight Remastered 3.3:** the game opens in the episode's first level. For no clicks at all,
  use OpenJKDF2 v0.9.9 or later and turn on **Setup > General > Disable mission start confirmation**. Versions
  v0.9.1 to v0.9.8 can stop for one click on the mission screen; older ones open at the menu.
  - A **multiplayer** episode starts as a game hosted on your PC, with you in it. OpenJKDF2 uses your last **Host
    Game** settings for it, and its default character for you (Kyle Katarn, rank 0) rather than the one you made.
    The first time, Windows Firewall asks whether OpenJKDF2 may use the network, since it is hosting a game.
  - A **Capture the Flag** episode opens at the menu, at the top of the list.
- **Every other Jedi Knight:** the game opens at its menu as usual, and the episode is at the top of the list for
  its kind: a single-player episode in **Singleplayer > New Game**, a deathmatch or Capture the Flag one in
  **Multiplayer > Host Game**'s episode box. It is not in the other list, however many episodes you have.

**A mod is not an episode.** A GOB with no episode in it, such as a saber or model pack, is left where it is, and the
game starts at its menu as usual. To use a mod, put it in your `mod` folder yourself. Gobinator does the same with a
file it cannot read as a GOB.

The episode stays in `Episode` afterwards, as one of your episodes. What puts it first is an empty folder of the same
name that Gobinator makes beside it (`Episode\MyEpisode` next to `Episode\MyEpisode.gob`): Jedi Knight lists folders
before files. The next double-click moves that to the new episode. **To get the usual list back, start
`gobinator.exe` itself:** it removes the folder and starts the game.

**Your own mods go in `mod`.** Gobinator starts the game with `-path mod`, so whatever you keep there loads on top of
the episode, and Gobinator never touches it.

**Your own GOBs are safe.** Gobinator never deletes or overwrites a GOB. The only things it removes are the empty
folders it made itself, which it records in `Episode\gobinator.state`. A folder you have put files in stays, and so
does a folder of that name you had already.

## Settings

Every setting in `gobinator.ini` is optional. With none, Gobinator works from the Jedi Knight folder it sits in. To
use one, remove the `#` in front of it. If the file is missing, Gobinator writes a fresh one, every setting a comment.

| Setting | What it does | Default |
|---|---|---|
| `JK_FOLDER` | Where Jedi Knight is, if `gobinator.exe` is somewhere else. Gobinator asks for it when it finds no game, and writes the answer here | the folder `gobinator.exe` is in |
| `MOD_FOLDER` | Your own mods' folder, loaded on top of the episode (no spaces) | `mod` |
| `JK_EXECUTABLE` | The program to start | found in the folder |
| `ADDITIONAL_LAUNCH_OPTIONS` | Options for the game, put before `-path mod` | `-devmode` (adds **Debug** to the Singleplayer menu, to start any of the episode's levels) |

## If something goes wrong

Gobinator shows a message saying what failed and what to do. `gobinator.log`, beside `gobinator.exe`, tells what the
last run did. Send it along when you ask for help.

- **Gobinator asks where Jedi Knight is:** the folder it sits in, or the one `JK_FOLDER` names, holds no game.
  Choose the folder with the game's EXE in it (`openjkdf2-64.exe`, `JK.EXE`, `JediKnight.exe`, or JkGfxMod's
  `inject.exe`) and Gobinator remembers it. Cancel, and nothing changes.
- **You used Gobinator 1.0 before:** Windows remembers **Open with** programs by file name, so choosing the new
  `gobinator.exe` can still start the old one. Put the new `gobinator.exe` and `gobinator.ini` in place of the old
  ones instead, and copy over any setting you had changed. If the old ones sat in a different Jedi Knight folder, set
  `JK_FOLDER` in the new `gobinator.ini` to the folder you want.
- **Steam:** Steam always starts its own install of the game. If Gobinator sits in another copy, it says so and names
  the folder Steam starts: move `gobinator.exe` and `gobinator.ini` there, or set `JK_FOLDER` to it.
- **GOG:** Windows may ask for administrator rights every time. GOG's `JK.EXE` needs them, so allow it.
- **A console window opens beside OpenJKDF2:** that is OpenJKDF2's own window, and it opens however the game is
  started.
- **The copy has a different name:** a GOB whose name has spaces or accents is copied under a plain one
  (`My Episode.gob` becomes `My_Episode.gob`), at most 31 characters long. If a different GOB of that name is already
  in `Episode`, the copy gets `_2` instead. Jedi Knight lists episodes by the title inside the GOB, so you see the same
  episode either way.
- **OpenJKDF2 opens at the menu, not in the level:** a GOB already in your `Episode` folder whose name has a space
  can't be named on OpenJKDF2's command line. It is first in **New Game**'s list instead: **New Game**, **Ok**.
- **The episode is missing from the list:** a mod in `mod` that is itself an episode (it holds an `episode.jk`)
  takes over the whole list. Move it out of `mod` while you play.

## Licence

Gobinator is released under CC0 1.0: it is in the public domain (`LICENSE`). Made by SM Sith Lord: 1.0 in 2022, 1.1
in 2026. The one piece that is not: `src/v1.0/derent.h`, Toni Ronkko's `dirent` shim for MSVC (MIT licence, kept in
the file), which version 1.0 used and 1.1 does not.

## Building

Visual Studio 2022 and CMake, as a 32-bit build with the static runtime:

```
cmake -G "Visual Studio 17 2022" -A Win32 -B build
cmake --build build --config Release
ctest --test-dir build -C Release
```

`tests\run-v11-scenarios.ps1` runs the built `gobinator.exe` through its scenarios in throwaway folders, against a
fake Jedi Knight. `package\make-zip.ps1` runs both sets of tests and makes the release zip in `dist\`; the version is
`src\version.h`.

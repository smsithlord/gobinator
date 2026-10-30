# Trying v1.1 on your real installs

For the board item "Try v1.1 on the real installs". About 15 minutes. Nothing here changes your `.gob` association
until the last step, and that step is yours to take or skip.

## What you need

- Two or three episode GOBs from anywhere (a Downloads folder is ideal).
- Optional: one GOB of your own in the folder's `mod` folder, to watch it survive.

## 1. Your JK Remastered play folder (`H:\Installed Programs\JK Remastered`)

This is JK Remastered 3.3 on OpenJKDF2 v0.9.9, so Gobinator starts the episode's first level directly. Since
2026-10-27 the episode goes into `Episode`, listed first by an empty folder of its name (docs/DECISIONS.md): replace
the folder's `gobinator.exe` with the build that does that before this round. Your `.gob` association already runs
it, so a double-click is the test. Your player and "Disable mission start confirmation" are set already.

1. Double-click an episode GOB. Expect:
   - The game opens straight into the episode's first level, in front of other windows, with no click.
   - Your Remastered mods still load (the HD Kyle model, the weapon sway fix).
   - `Episode` holds the GOB (a name with spaces made plain: `My Episode.gob` becomes `My_Episode.gob`), an empty
     folder of the same name, and a small `gobinator.state`.
   - `bunker.gob` and the record in `mod` are gone: the earlier build's copy, which from `mod` would hide the
     episode. Anything of your own in `mod` stays.
   - `gobinator.log` beside the EXE says `autostart: episode ..., level ...` and shows the command line.
2. Quit the game and double-click a second GOB. Both GOBs are in `Episode`; only the second has its folder.
3. Start `gobinator.exe` itself (double-click the EXE). Expect the game at its menu with its usual list: the folder
   and `gobinator.state` are gone, the GOBs stay, and the log says `no GOB given: JK starts with its usual episode
   list`.
4. For retail-style menus: in the game, **Singleplayer > New Game** shows the double-clicked episode first.
5. One failure on purpose: add a line `JK_EXECUTABLE = nothing.exe` to `gobinator.ini`. Expect one message box that
   names the missing program, and no change in `Episode` or `mod`. Then delete that line.

## 2. Optional: the other installs

- **Your community-patched Steam JK** (`H:\SteamLibrary\steamapps\common\Star Wars Jedi Knight`): copy
  `G:\Documents Sym Links\GitHub\gobinator\build\Release\gobinator.exe` and `package\gobinator.ini` there, and run it
  as in step 2. Expect `JediKnight.EXE -devmode -path mod`, the menu, and the episode at the top of the episode list.
  Retail JK cannot skip its menus.
- **Your GOG JK** (`H:\GOGLibrary\Star Wars Jedi Knight - Dark Forces 2`): the same. `JK.EXE` needs admin rights,
  so it starts through Windows' prompt (silent on your machine), and the log says "started with admin rights". 3D
  acceleration should work here, unlike on the test copy, because GOG's compatibility settings apply to this folder.
- **A Steam folder that Steam does not start** (the Vanilla one): Gobinator now stops with a message naming the
  folder Steam starts, and copies nothing. That is expected.

## Tell the project

Anything that differed from the above, with the `gobinator.log` of that run. Also say whether you clicked anything
before the level appeared (a screenshot cannot tell), and whether the game came to the front by itself.

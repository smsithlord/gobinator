# The user's JK installs, for testing

Gathered by the user on 2026-10-27. A **Vanilla** folder is a clean reference: never run Gobinator or a game in it,
and never write there. Tests run on COPIES in `testinstall/` (`tests/copy-install.ps1`). The play folders (no
"Vanilla") are where the user plays; run nothing in them without the user's go.

## Jedi Knight: Dark Forces II

| Install | Folder | Game EXE (Gobinator detects) | Notes |
|---|---|---|---|
| Steam, vanilla | `H:\SteamLibrary\steamapps\common\Star Wars Jedi Knight Vanilla` | `JediKnight.EXE` (Steam) | DRM: any start goes through Steam, which runs the **registered** install (`...\Star Wars Jedi Knight`, the user's modded one). A copy cannot run on its own. Cleaned 2026-10-27: the user's profile (`player\`) and a modded `Resource\cog\weap_fists.cog` were moved to `H:\Installed Programs\_removed from vanilla installs (2026-10-27)\`; what is left is the 64 files of Steam's install (2022-03-09). |
| Steam, the user's | `H:\SteamLibrary\steamapps\common\Star Wars Jedi Knight` | `JediKnight.EXE` (a community patch of it) | The one Steam starts. Also has `JK.EXE`, `Jedi.EXE`. |
| GOG, vanilla | `H:\GOGLibrary\Star Wars Jedi Knight - Dark Forces 2 Vanilla` | `JK.EXE` (retail) | DRM-free. `JKStart.exe` and `GOGLauncher.exe` are launchers. The data source for the OpenJKDF2 folders. Its copy is `testinstall\gog-vanilla` (2026-10-27): there `JK.EXE` runs with admin rights (Windows asks for them; silent on this machine), and 3D acceleration fails, since GOG's compatibility layers (`16BITCOLOR WINXPSP2 HIGHDPIAWARE`) are set by path for the user's GOG folder only; the user left the copy in software mode, 1280x720 (settings are in the shared registry). |
| GOG, the user's | `H:\GOGLibrary\Star Wars Jedi Knight - Dark Forces 2` | `JK.EXE` (retail) | To play from. |
| Community, vanilla | `H:\Installed Programs\jk Vanilla` | `JK.EXE` (retail) | Community package: `JK.dll`, dgVoodoo (`DDraw.dll`, `D3DImm.dll`, `dgVoodoo.conf`), IPX networking DLLs. |
| Community, the user's | `H:\Installed Programs\jk` | `JK.EXE` (retail) | To play from. |
| JK Remastered 3.3, vanilla | `H:\Installed Programs\JK Remastered Vanilla` | `openjkdf2-64.exe` (OpenJKDF2 v0.9.1: knows -autostart, not -doNotResumeLast) | Installed 2026-10-27 on a copy of GOG vanilla JK. The installer (`H:\Jedi_Knight_Remastered_3.3`) has no options. It adds `jkgm\`, `mods\`, OpenJKDF2 and its DLLs, icons, the configuration guide and `Uninstal.exe`. |
| JK Remastered v1.0, the user's | `H:\SteamLibrary\steamapps\common\Star Wars Jedi Knight Remastered` | `inject.exe` with `jkgm.json` | JkGfxMod 0.3.0, installed 2022-03-09. The installed Gobinator v1.0 lives here; the `.gob` association pointed at it until 2026-10-27, when it moved to the v1.1 build in the JK Remastered 3.3 play folder. **To be removed by the user (2026-10-27)**: its uninstaller is the unversioned Programs and Features entry "Jedi Knight Remastered" (`Uninstal.exe` here), separate from "Jedi Knight Remastered 3.3". Do not rely on it. |
| JK Remastered 3.3, the user's (**primary single-player JK; the user's own Gobinator lives here**) | `H:\Installed Programs\JK Remastered` | `openjkdf2-64.exe` (v0.9.9; 3.3 installed v0.9.1, kept in `_OpenJKDF2 v0.9.1 backup\`) | Installed 2026-10-27 like the vanilla one, to play from; OpenJKDF2 upgraded to the official v0.9.9 at the user's request. Weapon Sway Animation Fix for 3.3 (Patrol, 2025-04-29) installed: its `mods\EnhancementMod.gob` replaces 3.3's, kept as `mods\EnhancementMod.gob.bkp`. 3.3 already installs blackhawk3d1's HD Kyle as `mods\AKyleHD2-HighRes-Tan-Shirt.gob`. Its settings are OpenJKDF2's in-game Display menu; extra mods go in its `mods\` folder. |
| OpenJKDF2, vanilla | `H:\Installed Programs\OpenJKDF2 Vanilla` | `openjkdf2-64.exe` | Built 2026-10-27: official upstream v0.9.9 (`win64-debug.zip`: the EXE, `OpenAL32.dll`, `libGameNetworkingSockets.dll`) plus GOG vanilla's `Episode`, `Resource` (with `VIDEO`) and `MUSIC`, and an empty `player\`: upstream's layout. |
| OpenJKDF2, the user's | `H:\Installed Programs\OpenJKDF2` | `openjkdf2-64.exe` | Built the same way, to play from. |

## Mysteries of the Sith

Gobinator does not handle MotS yet (`.goo` episodes; an idea in the icebox for after v1.1).

| Install | Folder | Game EXE | Notes |
|---|---|---|---|
| Steam, vanilla | `H:\SteamLibrary\steamapps\common\Jedi Knight Mysteries of the Sith Vanilla` | `JediKnightM.EXE` (Steam) | No profile or mod files. All its files are dated 2026-10-27 (copied that day), so dates cannot vouch for it; its contents match a plain Steam install. |
| Steam, the user's | `H:\SteamLibrary\steamapps\common\Jedi Knight Mysteries of the Sith` | `openjkdf2-64.exe` v0.9.8, `JediKnightM.EXE` | Holds an outdated MOTS Remastered **v4**, which the user will uninstall (2026-10-27) with its two uninstallers ("...Remastered 4.0" and "...v4 Part 2"), leaving plain Steam MotS. Steam's registered MotS install: never delete the folder. |
| GOG, vanilla | `H:\GOGLibrary\Star Wars Jedi Knight - Mysteries of the Sith Vanilla` | `JKM.EXE` | DRM-free. The data source for the OpenJKDF2 MotS folders. |
| GOG, the user's | `H:\GOGLibrary\Star Wars Jedi Knight - Mysteries of the Sith` | `JKM.EXE` | To play from. |
| Community, vanilla | `H:\Installed Programs\mots Vanilla` | `JKM.EXE`, `Sith.EXE` | Has `mod\rbots-patch.goo` (2002) and `mod.bat` (`jkm.exe -path mod`): part of the package, or added? (asked) |
| Community, the user's | `H:\Installed Programs\mots` | `JKM.EXE` | To play from. |
| MotS Remastered 5.11, vanilla | `H:\Installed Programs\MotS Remastered Vanilla` | `openjkdf2-64.exe` (v0.9.8, as installed) | Installed 2026-10-27 on a copy of GOG vanilla MotS: Part 1 (v5.11) and Part 2 (v5.0), no New Music (its MUSIC is GOG's). Adds `jkgm\`, `Mods\` (with optional mods in `Mods\disabled`), OpenJKDF2 and its DLLs, guides and two uninstallers. 4914 files. |
| MotS Remastered 5.11, the user's | `H:\Installed Programs\MotS Remastered` | `openjkdf2-64.exe` (v0.9.9; v0.9.8 kept in `_OpenJKDF2 v0.9.8 backup\`) | Installed the same way plus New Music, to play from; OpenJKDF2 upgraded to v0.9.9 at the user's request. `OPENJKMOTS_ROOT` checked unset after both installs. |
| OpenJKDF2 MotS, vanilla | `H:\Installed Programs\OpenJKDF2 MotS Vanilla` | `openjkdf2-64.exe` | Built 2026-10-27 like the JK one, from GOG MotS vanilla's data. OpenJKDF2 switches to MotS by itself when `resource\jk_.cd` holds MotS's key. |
| OpenJKDF2 MotS, the user's | `H:\Installed Programs\OpenJKDF2 MotS` | `openjkdf2-64.exe` | Built the same way, to play from. |

## Shared state to keep in mind

- The GOG-based copies carry GOG's `unins000.exe`: never run it there (it would act on GOG's own registration).
- Installing a remaster twice leaves one Programs and Features entry, pointing at the last install.

- Retail JK keeps its settings in the registry (`HKLM\SOFTWARE\WOW6432Node\LucasArts Entertainment Company\JediKnight`,
  virtualised under `HKCU\Software\Classes\VirtualStore`), shared by every retail-style JK on the machine. Steam's install
  keys there point at the user's modded Steam folder.
- Steam and GOG install compatibility databases (`.sdb`) for their EXEs; Windows rewrites a `JK.EXE`'s command line
  to its full path (harmless: the arguments are unchanged).
- OpenJKDF2 writes its settings (`registry.json`, `player\...`) into its own folder.

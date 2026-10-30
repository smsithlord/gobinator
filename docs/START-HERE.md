# Start here: picking Gobinator back up

The user's ask (2026-10-27), in short: Gobinator is a separate project the user wants to pick back up. Its goal is to
let a double-click on a GOB put it into a vanilla JK's folder so it is easy to find in the episode list, and launch the
game, all in one step. To make it work with the JK Remastered package as well as plain retail JK, it launches through
that package's `inject.exe` (AGENTS.md explains why). It is the targeted tool that ordinary players may want on its
own, apart from everything JK Resurrection does.

The v1.0 header comment states the goal: "get you directly into the world of a GOB file w/ as few clicks as possible".

**First,** put the steps below on your board list as items under one epic (`coord add`), so the studio can follow them.
Then work through them in order, asking the user as you go.

## The steps

1. **Join the board.** Run `coord join gobinator`, then commit `.coord/`. The hub already registered the workspace.
   `CLAUDE.md` already points at the skill.
2. **Build v1.0 again.** The original project file is lost.
   - Make a minimal build: CMake, or a VS 2022 project; your choice, recorded in `docs/DECISIONS.md`.
   - Change only what it takes to compile (`stdafx.h`, the `_TCHAR` entry point), so the build reproduces v1.0's
     behaviour.
   - Run it only against a scratch JK-like folder (AGENTS.md, Rules). Never against the user's install.
3. **Write `docs/OVERVIEW.md`.**
   - What v1.0 does, step by step, and the ini format (pairs of lines, `#` comments, no blank lines).
   - The risks and defects you confirm. Candidates from a first read, to check rather than trust:
     - The purge deletes the user's GOBs outright.
     - `.gob` is matched case-sensitively and anywhere in the name, so `JK1.GOB` is neither purged nor copied.
     - A `gobinator.bat` is written into the working directory, which is usually the GOB's own folder, and run through
       `system()`.
     - Paths go from UTF-8 through `mbstowcs_s` into 512-character buffers, so non-ASCII and long paths break.
     - The ini must sit next to the EXE, and the EXE in the JK folder.
     - The user's ini has `BFP_RANDOMIZE_*` options that v1.0 never reads. They belong to JK Resurrection's launcher,
       which grew out of Gobinator.
4. **Propose the next version** in `docs/PLAN.md`, and let the user pick. Topics:
   - A purge that only removes the GOBs Gobinator itself put there, and never the user's own files.
   - Launching with `CreateProcess` and the right working directory, with no stray `.bat`.
   - Unicode paths.
   - Telling the installs apart, and supporting each: retail `jk.exe`, the Steam and GOG layouts, JK Remastered through
     `inject.exe`, and whether OpenJKDF2 matters.
   - Fewer clicks after launch. Today it takes five menu steps, through Singleplayer and Debug.
   - How a player installs it and associates `.gob` files, per user, without admin.
   - Error messages a player can act on.

   Once the user has picked, put the plan on the board.
5. **Later ideas go in the icebox** (`coord icebox`), not into the plan. One is from v1.0's own header: an
   auto-generated COG that spawns the player at the exact spot of the screenshot they launched from.

## Questions for the user

Ask early, and in small batches. At least:

- Which installs must it support: the JK Remastered folder, plain Steam or GOG retail, OpenJKDF2?
- Keep the purge as it is (with its warnings), or make it safe? Which GOBs must never be touched?
- Should Gobinator take over the `BFP_RANDOMIZE_*` options, or leave them to JK Resurrection?
- Who is it for, and how do they get it: a GitHub repo, a release download, a licence?
- The installed v1.0 and the `.gob` association: when and how does a new build replace them?

Record each answer in `docs/DECISIONS.md`.

## Done when

v1.0 builds again and is described, the user has picked the next version's plan, and its steps are items on the board.

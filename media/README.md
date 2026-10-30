# Gobinator's media

Gobinator's icon, logo, key art and screenshots, and its package for the SM Sith Lord hub's Library and NavMe.

| Path | What it is |
|---|---|
| `listing/` | **The profile package** for the hub (its `docs/PROFILES.md`): `package.json` and the media under the spec's file names. The hub copies it to `library/gobinator/` and the media mirror `smsithlord.com/library/gobinator/`. |
| `listing/logo.png` | **The clear logo**: the terminal line `> GOBINATOR_` alone, amber phosphor with its glow fading to clear, 4096 x 619. |
| `listing/icon.png` | The icon, 1024 x 1024, transparent: `GOB` in the same amber phosphor on a dark CRT screen with a thin bezel, a bold cursor under the O. |
| `listing/grid-portrait.png`, `grid-landscape.png` | The capsules (1200 x 1800 and 1840 x 860): the key art with the logo, no other text. |
| `listing/menu-background.png` | NavMe's menu background and the link preview, 2560 x 1440: the key art alone, the doorway centred. |
| `listing/screenshots/` | Four real frames at 1920 x 1080 (below). |
| `gobinator.ico` | The Windows icon, 16 to 256 px. `src/gobinator.rc` builds it into `gobinator.exe`. |
| `source/` | The generations as they came back (`logo-raw-terminal.jpg`, `icon-raw-terminal.png`, `keyart.jpg`: the key art master with no text, 5504 x 3072) and `build.py`, which makes everything above from them. |
| `source/logo-round2/` | **Round two of the logo (2026-10-30)**, from which the user picked F, the terminal: `review-sheet.png` is the sheet they saw (the steel logo of round one, then twelve options, A to L); `sheet-1-raw.jpg` and `sheet-2-raw.jpg` are the two generations as they came back; `template.py` drew the templates they were given and `review.py` cuts them into `options/` and composes the review sheet (`options/` and `review-sheet-full.png` are rebuilt from the rest, and not committed). |

Rebuild: `python media/source/build.py`. It keys the wordmark by brightness (it was drawn on pure black, so its glow
fades to clear while the glyphs stay solid), keys the icon off the flat green it was drawn on and enlarges its
letters a quarter within the screen (for the small sizes), trims both, writes the icon sizes and the .ico, and
composes the capsules and the menu background.

## The screenshots

`tests/capture-media-shots.ps1` makes them, with no window, from the test install (`tests/make-openjkdf2-install.ps1`):
`gobinator.exe` is handed The Crab Factory (`P71.GOB`, from COG Factory's public release zip) in an install whose
episode folder also holds LucasArts' JK1 and 49 of COG Factory's first-generation episodes. The frames come from
OpenJKDF2's engine as the studio's libretro core, run headless at 1920 x 1080 (its harness, the `jk-content` skill),
starting from that folder as the desktop game does.

| File | Kind | What it shows |
|---|---|---|
| `01-episode-first.png` | menu | Singleplayer > New Game: THE CRAB FACTORY heads the list, ahead of The Force Within and 49 more |
| `02-straight-into-the-level.png` | gameplay | The episode started directly, as OpenJKDF2's `-autostart` starts it, two seconds in |
| `03-the-crab-factory.png` | gameplay | Twenty seconds in, nobody at the controls: the burrows' crabs close in |
| `04-usual-list.png` | menu | After `gobinator.exe` with no GOB: the list in its usual order, The Force Within first |

They are Gobinator's work as a player sees it: Gobinator itself has no window. A frame of the desktop game, of
Explorer's "Open with", or a video (the package's `video` slot) needs the user at the keyboard.

## How the art was made (AI)

Generated through OpenRouter (the `openrouter-image` skill), then keyed and composed by `build.py`.

| File | Date | Model | Cost (USD) | Prompt, in short |
|---|---|---|---|---|
| `source/keyart.jpg` | 2026-10-29 | Nano Banana 2, 4K 16:9, round one's icon as reference for palette | 0.15 | A dim riveted-steel hangar; a round blast door rolls open on a sunlit desert canyon and ruin; a lone plain figure in the doorway; no text, no symbols, no weapons |
| `source/logo-raw-terminal.jpg` | 2026-10-30 | Nano Banana Pro, 4K 21:9, option F as reference | 0.24 | Amber phosphor monospace terminal type reading exactly "> GOBINATOR_", brighter cores, a soft glow, scanlines only inside the strokes, on solid pure black with no screen or vignette |
| `source/icon-raw-terminal.png` | 2026-10-30 | Nano Banana Pro, 1K 1:1 (2K was asked), option F as reference | 0.14 | A dark charcoal CRT screen with a thin bezel; "GOB" in the same phosphor type, large; a bold amber underline cursor under the O only; scanlines, glow; on flat #00FF00 |

**Round one** (2026-10-29, USD 0.80 in all) made a rusted-steel wordmark with an amber play triangle in the first O
and a steel G firing the same arrow (Nano Banana 2 and Pro, two dropped wordmarks among them). They were the logo
and icon for a day; the sources are in git history (commit 06746d0, `media/source/logo-raw.jpg` and
`icon-raw.png`), and the round-two review sheet shows the wordmark at the top.

### Round two of the logo (2026-10-30)

The user asked for alternatives to the wordmark. Two labelled six-panel templates (`logo-round2/template.py`, 2 x 3
wide cells on neutral grey) were each filled in one generation, with the icon as a colour reference only and the
rusted-steel treatment ruled out:

| File | Model | Cost (USD) | Panels |
|---|---|---|---|
| `logo-round2/sheet-1-raw.jpg` | Nano Banana Pro, 4K 16:9 | 0.25 | A angular sci-fi, B flat modern, C stencil, D cut-out, E retro chrome 1997, F terminal. Kept the grid; all six read GOBINATOR. |
| `logo-round2/sheet-2-raw.jpg` | Nano Banana 2, 4K 16:9 | 0.15 | G roundel badge, H hazard panel, I archive label, J pixel 1997, K HUD wireframe, L engraved nameplate. Redrew the grid as four columns with two empty panels, labelled L "Concept 7", and misspelt J as GOBIN1997; the other five are usable. |

`review.py` cuts the panels out (sheet 2's edges were measured, not assumed) and lays them under round one's logo
in `review-sheet.png`. **The user picked F, the terminal** (docs/DECISIONS.md, 2026-10-30), and asked for the icon
to be `GOB` with a bold underline under the O; both were then rendered on their own (the table above) and
`build.py` rebuilt `listing/`. The sheets cost USD 0.40, the round USD 0.78 in all.

The art avoids Star Wars marks: no lightsaber, no Star Wars type, no characters. The package's `notice` covers the
game's menus in the screenshots.

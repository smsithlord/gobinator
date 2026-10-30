"""A 2x3 labelled panel template for a sheet of wordmark options, 4096x2304, panels on neutral grey.
Writes template-<name>.png and layout-<name>.json (sheet.py-compatible) for each sheet."""
import json, sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

out = Path(sys.argv[1])
W, H = 4096, 2304
TAB, GUT, HEAD = 64, 40, 150
PW = (W - 3 * GUT) // 2          # 1988
PH = 600
BOLD = ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf', 40)
BIG = ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf', 56)
SMALL = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 30)
HINT = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 34)

SHEETS = {
    'A': ('GOBINATOR - LOGO OPTIONS, SHEET 1', [
        ('A', 'ANGULAR SCI-FI', 'sharp slanted condensed sci-fi letters, gunmetal with amber edge-light'),
        ('B', 'FLAT MODERN', 'bold geometric sans, flat, no texture; amber disc O with a dark play triangle'),
        ('C', 'STENCIL', 'spray-stencilled worn yellow paint on a dark blast-door plate'),
        ('D', 'CUT-OUT', 'letters cut through thick steel, amber light pouring through'),
        ('E', 'RETRO CHROME 1997', 'mirror-chrome bevel letters, amber-red inner gradient, slight arc'),
        ('F', 'TERMINAL', 'amber phosphor monospace on a black CRT:  > GOBINATOR_'),
    ]),
    'B': ('GOBINATOR - LOGO OPTIONS, SHEET 2', [
        ('G', 'ROUNDEL BADGE', 'circular hangar-bay insignia, G and amber triangle centre, GOBINATOR on the arc'),
        ('H', 'HAZARD PANEL', 'heavy black condensed caps on amber-and-black hazard-striped steel'),
        ('I', 'ARCHIVE LABEL', 'peeling cream crate label with a thick amber bar, heavy condensed type'),
        ('J', 'PIXEL 1997', 'chunky bitmap-font letters, steel grey, amber pixel triangle O, no anti-aliasing'),
        ('K', 'HUD WIREFRAME', 'thin amber monoline letters, corner brackets, reticle ring O'),
        ('L', 'ENGRAVED NAMEPLATE', 'brushed dark bronze plate, corner screws, letters filled with amber enamel'),
    ]),
}

for name, (title, cells) in SHEETS.items():
    im = Image.new('RGB', (W, H), (22, 22, 24))
    d = ImageDraw.Draw(im)
    d.rectangle([GUT, 16, W - GUT, 16 + 100], fill=(40, 44, 52))
    tw = d.textlength(title, font=BIG)
    d.text(((W - tw) / 2, 28), title, font=BIG, fill=(235, 235, 240))
    sub = 'PAINT EACH PANEL WITH ONE FINISHED LOGO READING EXACTLY "GOBINATOR" - KEEP PANEL POSITIONS AND SIZES - LEAVE THE LABEL TABS AS THEY ARE'
    tw = d.textlength(sub, font=SMALL)
    d.text(((W - tw) / 2, 84), sub, font=SMALL, fill=(170, 180, 200))
    layout = {'name': f'logo-round2-{name}', 'sheet': [W, H], 'tab': TAB, 'gutter': GUT, 'cells': []}
    for i, (letter, label, hint) in enumerate(cells):
        col, row = i % 2, i // 2
        x = GUT + col * (PW + GUT)
        y = HEAD + TAB + row * (TAB + PH + GUT)
        d.rectangle([x, y - TAB, x + PW, y], fill=(60, 64, 74))
        d.text((x + 14, y - TAB + 10), f'{letter}   {label}', font=BOLD, fill=(255, 255, 255))
        d.rectangle([x, y, x + PW, y + PH], fill=(58, 58, 62), outline=(96, 96, 104), width=3)
        tw = d.textlength(hint, font=HINT)
        d.text((x + (PW - tw) / 2, y + PH / 2 - 17), hint, font=HINT, fill=(120, 120, 128))
        layout['cells'].append({'id': f'{letter}-{label.lower().replace(" ", "-")}', 'n': letter, 'label': label,
                                'hint': hint, 'rect': [x, y, PW, PH], 'bg': 'white', 'key': False, 'trim': False})
    im.save(out / f'template-{name}.png')
    (out / f'layout-{name}.json').write_text(json.dumps(layout, indent=1))
    print(f'template-{name}.png {W}x{H}; bottom edge {y + PH}')

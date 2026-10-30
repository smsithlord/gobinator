"""Slices the two generated sheets into their options and composes review-sheet.png (media/README.md, round two)."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
OPT = HERE / 'options'
OPT.mkdir(exist_ok=True)
BOLD = ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf', 44)
REG = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 36)
BIG = ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf', 64)

# Sheet 1 (Nano Banana Pro) came back as a clean 2 x 3 grid at exact halves and thirds.
s1 = Image.open(HERE / 'sheet-1-raw.jpg').convert('RGB')
W, H = s1.size
wide = []
for i, (letter, name) in enumerate([('A', 'Angular sci-fi'), ('B', 'Flat modern'), ('C', 'Stencil'),
                                    ('D', 'Cut-out'), ('E', 'Retro chrome 1997'), ('F', 'Terminal')]):
    c, r = i % 2, i // 2
    im = s1.crop((c * W // 2, r * H // 3, (c + 1) * W // 2, (r + 1) * H // 3))
    im.save(OPT / f'{letter}-{name.lower().replace(" ", "-")}.png')
    wide.append((letter, name, '', im))

# Sheet 2 (Nano Banana 2) redrew the grid: three panels of unequal width under four tabs in its top row, four in
# the bottom row (one empty). The edges were measured from the image; the tab rows are left out of the crops.
s2 = Image.open(HERE / 'sheet-2-raw.jpg').convert('RGB')
square = []
for (letter, name, note, box) in [('G', 'Roundel badge', '', (0, 160, 1380, 1540)),
                                  ('H', 'Hazard panel', '', (1380, 160, 3447, 1540)),
                                  ('I', 'Archive label', '', (3447, 160, 5504, 1540)),
                                  ('J', 'Pixel 1997', 'the model misspelt it; a redo is one call', (0, 1700, 1436, 3072)),
                                  ('K', 'HUD wireframe', '', (1436, 1700, 2759, 3072)),
                                  ('L', 'Engraved nameplate', 'the model labelled it "Concept 7"', (2759, 1700, 4130, 3072))]:
    im = s2.crop(box)
    im.save(OPT / f'{letter}-{name.lower().replace(" ", "-")}.png')
    square.append((letter, name, note, im))

# The current logo, on the same grey as the template, in a wide cell.
cur = Image.open(HERE.parent.parent / 'listing' / 'logo.png').convert('RGBA')
cell_w = 2752
cur_cell = Image.new('RGB', (cell_w, 1024), (58, 58, 62))
lg = cur.resize((2300, round(cur.height * 2300 / cur.width)), Image.LANCZOS)
cur_cell.paste(lg, ((cell_w - lg.width) // 2, (1024 - lg.height) // 2), lg)

OUT_W, GUT, TAB = 4096, 32, 72
PW = (OUT_W - 3 * GUT) // 2            # wide cells, two per row
SW = (OUT_W - 4 * GUT) // 3            # square cells, three per row
scale_w = PW / cell_w
wide_h = round(1024 * scale_w)
rows_h = 150 + (TAB + wide_h + GUT) * 4 + (TAB + 1400 + GUT) * 2 + 40
sheet = Image.new('RGB', (OUT_W, rows_h), (22, 22, 24))
d = ImageDraw.Draw(sheet)
d.text((GUT, 36), 'GOBINATOR  -  logo, round two  (2026-10-30)', font=BIG, fill=(235, 235, 240))
d.text((GUT, 112), 'A-F: Nano Banana Pro, 4K.  G-L: Nano Banana 2, 4K.  Top: the current logo (2026-10-29).',
       font=REG, fill=(170, 180, 200))

def place(im, x, y, w, letter, name, note, accent=(60, 64, 74)):
    h = round(im.height * w / im.width)
    d.rectangle([x, y, x + w, y + TAB], fill=accent)
    d.text((x + 18, y + 12), f'{letter}   {name}', font=BOLD, fill=(255, 255, 255))
    if note:
        tw = d.textlength(note, font=REG)
        d.text((x + w - tw - 18, y + 18), note, font=REG, fill=(255, 190, 120))
    sheet.paste(im.resize((w, h), Image.LANCZOS), (x, y + TAB))
    return y + TAB + h + GUT

y = 170
place(cur_cell, GUT, y, PW, 'CURRENT', 'rusted steel, the arrow in the O', '', accent=(90, 60, 40))
y += TAB + wide_h + GUT
for i, (letter, name, note, im) in enumerate(wide):
    x = GUT + (i % 2) * (PW + GUT)
    ny = place(im, x, y, PW, letter, name, note)
    if i % 2 == 1:
        y = ny
# Sheet 2's options, three per row at a shared height, each at its own width.
for row in (square[:3], square[3:]):
    total = sum(im.width / im.height for *_, im in row)
    h = round((OUT_W - 4 * GUT) / total)
    x = GUT
    for letter, name, note, im in row:
        w = round(im.width * h / im.height)
        place(im, x, y, w, letter, name, note)
        x += w + GUT
    y += TAB + h + GUT
sheet = sheet.crop((0, 0, OUT_W, y + 8))
sheet.save(HERE / 'review-sheet-full.png', optimize=True)
sheet.resize((2048, round(sheet.height * 2048 / OUT_W)), Image.LANCZOS).save(HERE / 'review-sheet.png', optimize=True)
print('review-sheet-full.png', sheet.size, '; review-sheet.png 2048 wide;', len(list(OPT.iterdir())), 'options')

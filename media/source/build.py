"""Builds Gobinator's art from the generations in this folder (media/README.md says how they were made).

    python media/source/build.py

The wordmark (logo-raw-terminal.jpg, amber phosphor on pure black) is keyed by brightness, so its glow fades to
clear while the glyphs stay solid; the icon (icon-raw-terminal.png, on flat #00FF00) is keyed by colour. Both are
trimmed; the icon sizes and media/gobinator.ico are written; the two capsules and the menu background are composed
from the key art (keyart.jpg). Writes into media/listing/ under the file names of the hub's docs/PROFILES.md.
"""
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

SRC = Path(__file__).resolve().parent
MEDIA = SRC.parent
OUT = MEDIA / 'listing'
OUT.mkdir(parents=True, exist_ok=True)


def key_green(im, lo=30.0, hi=225.0):
    """RGBA from an image on flat chroma green: alpha from how much green beats red and blue, then unmix and despill."""
    c = np.asarray(im.convert('RGB'), dtype=np.float32)
    r, g, b = c[..., 0], c[..., 1], c[..., 2]
    excess = g - np.maximum(r, b)
    a = np.clip(1.0 - (excess - lo) / (hi - lo), 0.0, 1.0)
    # Unmix the green: C = a*F + (1-a)*(0,255,0).
    safe = np.maximum(a, 1e-3)
    f = c.copy()
    f[..., 1] = (g - (1.0 - a) * 255.0) / safe
    f[..., 0] = r / safe
    f[..., 2] = b / safe
    f = np.clip(f, 0, 255)
    # Despill: nothing in this art is greener than its red or blue.
    f[..., 1] = np.minimum(f[..., 1], np.maximum(f[..., 0], f[..., 2]))
    a[a < 0.02] = 0.0
    out = np.dstack([f, a * 255.0]).round().astype(np.uint8)
    return Image.fromarray(out, 'RGBA')


def key_black(im, glyph=0.35, close=9, floor=0.07):
    """RGBA from emissive art on pure black: alpha is the brightness (C = a*F over black), so the glow fades to clear,
    except inside the glyphs, which are made solid so their scanlines do not show the background through them. The
    faintest haze (JPEG noise, glow pooling far from the letters) is remapped to nothing below `floor`."""
    c = np.asarray(im.convert('RGB'), dtype=np.float32) / 255.0
    lum = c.max(axis=2)
    a = np.clip((lum - floor) / (1.0 - floor), 0.0, 1.0)
    core = Image.fromarray((lum > glyph).astype(np.uint8) * 255)
    core = core.filter(ImageFilter.MaxFilter(close)).filter(ImageFilter.MinFilter(close))  # close the scanline gaps
    core = core.filter(ImageFilter.GaussianBlur(1.2))
    a = np.maximum(a, np.asarray(core, dtype=np.float32) / 255.0)
    f = np.clip(c / np.maximum(a, 1e-3)[..., None], 0.0, 1.0)
    out = np.dstack([f * 255.0, a * 255.0]).round().astype(np.uint8)
    return Image.fromarray(out, 'RGBA')


def screen_rect(em):
    """The CRT screen inside the icon's plate: from the middle, out along the middle row and column, until the
    pixels turn cool grey (the bezel) or clear. The screen is warm dark brown and the letters amber, so neither stops
    the walk."""
    c = np.asarray(em, dtype=np.int32)
    H, W = c.shape[:2]

    def bezel(p):
        r, g, b, a = p
        return a < 255 or (b > r + 6 and max(r, g, b) > 40)

    cy, cx = H // 2, W // 2
    x0 = x1 = cx
    while x0 > 0 and not bezel(c[cy, x0 - 1]):
        x0 -= 1
    while x1 < W - 1 and not bezel(c[cy, x1 + 1]):
        x1 += 1
    y0 = y1 = cy
    while y0 > 0 and not bezel(c[y0 - 1, cx]):
        y0 -= 1
    while y1 < H - 1 and not bezel(c[y1 + 1, cx]):
        y1 += 1
    return x0, y0, x1 + 1, y1 + 1


def enlarge_screen(em, factor, inset=14):
    """The icon's letters a little larger: the middle of the CRT screen is cropped by `factor` and scaled back up over
    the screen, under a feathered mask inset from its edge, so the bezel and its inner lip stay as drawn."""
    x0, y0, x1, y1 = screen_rect(em)
    iw, ih = x1 - x0, y1 - y0
    cw, ch = round(iw / factor), round(ih / factor)
    cx, cy = (x0 + x1) // 2, (y0 + y1) // 2
    crop = em.crop((cx - cw // 2, cy - ch // 2, cx - cw // 2 + cw, cy - ch // 2 + ch)).resize((iw, ih), Image.LANCZOS)
    mask = Image.new('L', (iw, ih), 0)
    ImageDraw.Draw(mask).rounded_rectangle((inset, inset, iw - inset, ih - inset), radius=round(iw * 0.08), fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(inset / 2))
    out = em.copy()
    out.paste(crop, (x0, y0), mask)
    print(f'icon screen: x {x0}-{x1}, y {y0}-{y1} of {em.width}x{em.height}')
    return out


def trim(im, pad=0, below=4):
    x0, y0, x1, y1 = im.getchannel('A').point(lambda v: 255 if v > below else 0).getbbox()
    return im.crop((max(0, x0 - pad), max(0, y0 - pad), min(im.width, x1 + pad), min(im.height, y1 + pad)))


def square(im, margin):
    side = round(max(im.size) / (1.0 - 2 * margin))
    canvas = Image.new('RGBA', (side, side), (0, 0, 0, 0))
    canvas.alpha_composite(im, ((side - im.width) // 2, (side - im.height) // 2))
    return canvas


def fit_width(im, w):
    return im.resize((w, round(im.height * w / im.width)), Image.LANCZOS)


def shadowed(logo, blur, offset, strength):
    """The logo over a soft dark shadow, on a canvas big enough for both; returns it and its padding."""
    pad = blur * 3 + max(offset)
    canvas = Image.new('RGBA', (logo.width + 2 * pad, logo.height + 2 * pad), (0, 0, 0, 0))
    sh = Image.new('RGBA', logo.size, (0, 0, 0, 255))
    sh.putalpha(logo.getchannel('A').point(lambda v: int(v * strength)))
    layer = Image.new('RGBA', canvas.size, (0, 0, 0, 0))
    layer.alpha_composite(sh, (pad + offset[0], pad + offset[1]))
    canvas.alpha_composite(layer.filter(ImageFilter.GaussianBlur(blur)))
    canvas.alpha_composite(logo, (pad, pad))
    return canvas, pad


def darken_top(im, y1, amount):
    """A black gradient over the top y1 rows, strongest at the top edge."""
    over = np.zeros((im.height, im.width, 4), dtype=np.uint8)
    ramp = np.linspace(1.0, 0.0, y1)
    over[:y1, :, 3] = (ramp[:, None] ** 1.6 * amount * 255).astype(np.uint8)
    base = im.convert('RGBA')
    base.alpha_composite(Image.fromarray(over, 'RGBA'))
    return base


def save(im, path, **kw):
    im.save(path, **kw)
    print(f'{path.relative_to(MEDIA)}: {im.width}x{im.height}, {path.stat().st_size / 1e6:.2f} MB')


# The clear logo: "> GOBINATOR_" alone, its glow fading to clear; 4096 wide.
logo_full = trim(key_black(Image.open(SRC / 'logo-raw-terminal.jpg')), pad=4, below=8)
save(fit_width(logo_full, 4096), OUT / 'logo.png', optimize=True)

# The icon: the CRT screen keyed off the green, trimmed, its letters enlarged a quarter for the small sizes, and
# centred on a square with a little air round it.
emblem = enlarge_screen(trim(key_green(Image.open(SRC / 'icon-raw-terminal.png')), pad=2), 1.25)
icon = square(emblem, 0.03).resize((1024, 1024), Image.LANCZOS)
save(icon, OUT / 'icon.png', optimize=True)

# The Windows icon. The small sizes are sharpened a little so the letters keep their edges.
sizes = [256, 128, 64, 48, 40, 32, 24, 20, 16]
frames = []
for s in sizes:
    f = icon.resize((s, s), Image.LANCZOS)
    if s <= 48:
        f = f.filter(ImageFilter.UnsharpMask(radius=1, percent=60 if s > 24 else 90, threshold=2))
    frames.append(f)
frames[0].save(MEDIA / 'gobinator.ico', format='ICO', sizes=[(s, s) for s in sizes], append_images=frames[1:])
print(f'gobinator.ico: {", ".join(map(str, sizes))} px')

key = Image.open(SRC / 'keyart.jpg').convert('RGB')
W, H = key.size
cx = W // 2  # the doorway is centred in the frame

# Menu background: the key art alone, 16:9, no logo or text.
w169 = round(H * 16 / 9)
menu = key.crop((cx - w169 // 2, 0, cx - w169 // 2 + w169, H)).resize((2560, 1440), Image.LANCZOS)
save(menu, OUT / 'menu-background.png', optimize=True)

# Portrait capsule, 2:3: the doorway and the figure, the terminal line glowing across the dark rafters above them.
wp = round(H * 2 / 3)
portrait = key.crop((cx - wp // 2, 0, cx - wp // 2 + wp, H)).resize((1200, 1800), Image.LANCZOS)
portrait = darken_top(portrait, 620, 0.6)
lg, pad = shadowed(fit_width(logo_full, 1080), blur=14, offset=(0, 0), strength=0.9)
portrait.alpha_composite(lg, ((1200 - lg.width) // 2, 150 - pad))
save(portrait.convert('RGB'), OUT / 'grid-portrait.png', optimize=True)

# Landscape capsule, 920:430: the hangar from its rafters down to the figure, the line across the dark steel above
# the door.
hl = round(W * 430 / 920)
land = key.crop((0, 0, W, hl)).resize((1840, 860), Image.LANCZOS).convert('RGBA')
land = darken_top(land, 300, 0.5)
lg, pad = shadowed(fit_width(logo_full, 1500), blur=14, offset=(0, 0), strength=0.9)
land.alpha_composite(lg, ((1840 - lg.width) // 2, 60 - pad))
save(land.convert('RGB'), OUT / 'grid-landscape.png', optimize=True)

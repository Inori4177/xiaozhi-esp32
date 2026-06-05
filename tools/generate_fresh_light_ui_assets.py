from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "main" / "boards" / "bread-compact-wifi" / "ui" / "assets" / "png"


MINT = (46, 196, 182, 255)
AQUA = (30, 175, 162, 255)
INK = (16, 40, 70, 255)
MUTED = (49, 74, 100, 255)
PEACH = (255, 212, 195, 255)
CORAL = (255, 123, 118, 255)
WHITE = (255, 255, 255, 255)
BORDER = (217, 241, 235, 255)
TRACK = (224, 228, 226, 255)
CARD = (255, 253, 249, 235)


def rgba(size, color=(0, 0, 0, 0)):
    return Image.new("RGBA", size, color)


def save(img, name):
    OUT.mkdir(parents=True, exist_ok=True)
    img.save(OUT / name)


def rounded_rect(size, radius, fill, outline=None, width=1, shadow=True):
    w, h = size
    pad = 8 if shadow else 0
    img = rgba((w + pad * 2, h + pad * 2))
    if shadow:
        sh = rgba(img.size)
        sd = ImageDraw.Draw(sh)
        sd.rounded_rectangle((pad, pad + 3, pad + w, pad + h + 3), radius, fill=(80, 150, 140, 40))
        sh = sh.filter(ImageFilter.GaussianBlur(5))
        img.alpha_composite(sh)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((pad, pad, pad + w - 1, pad + h - 1), radius, fill=fill, outline=outline, width=width)
    return img.crop((pad, pad, pad + w, pad + h))


def draw_line_icon(name, draw_fn, size=32, color=INK, accent=AQUA):
    scale = 4
    img = rgba((size * scale, size * scale))
    d = ImageDraw.Draw(img)
    draw_fn(d, scale, color, accent)
    img = img.resize((size, size), Image.Resampling.LANCZOS)
    save(img, name)


def icon_print(d, s, color, accent):
    d.rounded_rectangle((9*s, 7*s, 23*s, 15*s), 2*s, outline=color, width=2*s)
    d.rounded_rectangle((7*s, 13*s, 25*s, 23*s), 2*s, outline=accent, width=2*s)
    d.rectangle((11*s, 20*s, 21*s, 27*s), outline=color, width=2*s)
    d.line((12*s, 11*s, 20*s, 11*s), fill=color, width=1*s)


def icon_pick(d, s, color, accent):
    d.rounded_rectangle((6*s, 9*s, 26*s, 25*s), 2*s, outline=color, width=2*s)
    d.line((9*s, 9*s, 12*s, 5*s), fill=accent, width=2*s)
    d.line((12*s, 5*s, 17*s, 10*s), fill=accent, width=2*s)
    d.line((9*s, 15*s, 25*s, 15*s), fill=color, width=1*s)


def icon_settings(d, s, color, accent):
    cx = cy = 16*s
    for a in range(0, 360, 45):
        import math
        r1, r2 = 9*s, 12*s
        x1 = cx + math.cos(math.radians(a)) * r1
        y1 = cy + math.sin(math.radians(a)) * r1
        x2 = cx + math.cos(math.radians(a)) * r2
        y2 = cy + math.sin(math.radians(a)) * r2
        d.line((x1, y1, x2, y2), fill=color, width=2*s)
    d.ellipse((8*s, 8*s, 24*s, 24*s), outline=accent, width=2*s)
    d.ellipse((13*s, 13*s, 19*s, 19*s), outline=color, width=2*s)


def icon_position(d, s, color, accent):
    d.ellipse((8*s, 8*s, 24*s, 24*s), outline=color, width=2*s)
    d.line((16*s, 4*s, 16*s, 10*s), fill=accent, width=2*s)
    d.line((16*s, 22*s, 16*s, 28*s), fill=accent, width=2*s)
    d.line((4*s, 16*s, 10*s, 16*s), fill=accent, width=2*s)
    d.line((22*s, 16*s, 28*s, 16*s), fill=accent, width=2*s)
    d.ellipse((14*s, 14*s, 18*s, 18*s), fill=accent)


def icon_laser(d, s, color, accent):
    d.ellipse((13*s, 13*s, 19*s, 19*s), fill=accent)
    for a in range(0, 360, 45):
        import math
        x1 = 16*s + math.cos(math.radians(a)) * 8*s
        y1 = 16*s + math.sin(math.radians(a)) * 8*s
        x2 = 16*s + math.cos(math.radians(a)) * 13*s
        y2 = 16*s + math.sin(math.radians(a)) * 13*s
        d.line((x1, y1, x2, y2), fill=color, width=2*s)


def icon_material(d, s, color, accent):
    pts1 = [(16*s, 5*s), (26*s, 10*s), (16*s, 15*s), (6*s, 10*s)]
    pts2 = [(16*s, 12*s), (26*s, 17*s), (16*s, 22*s), (6*s, 17*s)]
    pts3 = [(16*s, 19*s), (26*s, 24*s), (16*s, 29*s), (6*s, 24*s)]
    d.line(pts1 + [pts1[0]], fill=accent, width=2*s)
    d.line(pts2 + [pts2[0]], fill=color, width=2*s)
    d.line(pts3 + [pts3[0]], fill=color, width=2*s)


def icon_speed(d, s, color, accent):
    d.arc((6*s, 8*s, 26*s, 28*s), 190, 350, fill=color, width=2*s)
    d.line((16*s, 19*s, 23*s, 12*s), fill=accent, width=2*s)
    d.ellipse((14*s, 17*s, 18*s, 21*s), fill=accent)


def icon_passes(d, s, color, accent):
    for off, c in [(4, color), (0, accent), (-4, color)]:
        pts = [(16*s, (8+off)*s), (26*s, (13+off)*s), (16*s, (18+off)*s), (6*s, (13+off)*s)]
        d.line(pts + [pts[0]], fill=c, width=2*s)


def icon_mode_line(d, s, color, accent):
    d.line((8*s, 24*s, 24*s, 8*s), fill=accent, width=2*s)
    d.ellipse((6*s, 22*s, 10*s, 26*s), outline=color, width=2*s)
    d.ellipse((22*s, 6*s, 26*s, 10*s), outline=color, width=2*s)


def icon_mode_fill(d, s, color, accent):
    for x in range(8, 25, 4):
        d.line((x*s, 24*s, (x+10)*s, 14*s), fill=color, width=2*s)
    d.rounded_rectangle((7*s, 8*s, 25*s, 26*s), 2*s, outline=accent, width=1*s)


def icon_mode_dot(d, s, color, accent):
    for y in [9, 14, 19, 24]:
        for x in [9, 14, 19, 24]:
            d.rectangle((x*s, y*s, (x+1)*s, (y+1)*s), fill=color)
    d.rounded_rectangle((7*s, 7*s, 26*s, 26*s), 2*s, outline=accent, width=1*s)


def icon_z(d, s, color, accent):
    d.line((16*s, 6*s, 16*s, 26*s), fill=color, width=2*s)
    d.line((11*s, 6*s, 21*s, 6*s), fill=accent, width=2*s)
    d.line((11*s, 26*s, 21*s, 26*s), fill=accent, width=2*s)
    d.polygon([(16*s, 4*s), (12*s, 9*s), (20*s, 9*s)], fill=color)
    d.polygon([(16*s, 28*s), (12*s, 23*s), (20*s, 23*s)], fill=color)


def icon_home(d, s, color, accent):
    d.line((8*s, 16*s, 16*s, 8*s, 24*s, 16*s), fill=accent, width=2*s)
    d.rectangle((11*s, 16*s, 21*s, 25*s), outline=color, width=2*s)
    d.line((16*s, 25*s, 16*s, 20*s), fill=color, width=2*s)


def icon_plus(d, s, color, accent):
    d.line((16*s, 8*s, 16*s, 24*s), fill=accent, width=3*s)
    d.line((8*s, 16*s, 24*s, 16*s), fill=accent, width=3*s)


def icon_minus(d, s, color, accent):
    d.line((8*s, 16*s, 24*s, 16*s), fill=color, width=3*s)


def icon_chevron_up(d, s, color, accent):
    d.polygon([(16*s, 10*s), (24*s, 21*s), (8*s, 21*s)], fill=color)


def icon_chevron_down(d, s, color, accent):
    d.polygon([(8*s, 11*s), (24*s, 11*s), (16*s, 22*s)], fill=color)


def background():
    img = rgba((480, 276), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    for y in range(276):
        t = y / 275
        r = int(217 * (1 - t) + 255 * t)
        g = int(243 * (1 - t) + 218 * t)
        b = int(234 * (1 - t) + 200 * t)
        d.line((0, y, 480, y), fill=(r, g, b, 255))
    glow = rgba((480, 276))
    gd = ImageDraw.Draw(glow)
    gd.ellipse((-80, -30, 210, 170), fill=(191, 234, 223, 92))
    gd.ellipse((250, 120, 560, 350), fill=(255, 188, 164, 86))
    gd.rounded_rectangle((128, 30, 432, 238), 42, fill=(255, 255, 255, 90))
    glow = glow.filter(ImageFilter.GaussianBlur(18))
    img.alpha_composite(glow)
    for x in range(40, 480, 40):
        d.line((x, 0, x, 276), fill=(191, 234, 223, 22))
    for y in range(28, 276, 28):
        d.line((0, y, 480, y), fill=(255, 212, 195, 24))
    save(img, "ui_bg_fresh_480x276.png")


def jog_diamond():
    size = 132
    img = rgba((size, size))
    sh = rgba((size, size))
    sd = ImageDraw.Draw(sh)
    pts = [(66, 8), (124, 66), (66, 124), (8, 66)]
    sd.polygon([(x, y + 4) for x, y in pts], fill=(80, 150, 140, 45))
    sh = sh.filter(ImageFilter.GaussianBlur(5))
    img.alpha_composite(sh)
    d = ImageDraw.Draw(img)
    d.polygon(pts, fill=(255, 253, 249, 230), outline=(217, 241, 235, 255))
    d.ellipse((44, 44, 88, 88), fill=(255, 253, 249, 255), outline=(226, 238, 242, 255), width=1)
    save(img, "ui_jog_diamond.png")


def file_thumb():
    img = rounded_rect((40, 40), 6, (210, 246, 237, 255), outline=BORDER, shadow=False)
    d = ImageDraw.Draw(img)
    d.ellipse((11, 12, 18, 19), fill=AQUA)
    d.polygon([(7, 28), (15, 19), (22, 28)], fill=MINT)
    d.polygon([(17, 28), (26, 16), (34, 28)], fill=AQUA)
    save(img, "ui_file_thumb_mountain.png")


def button_assets():
    save(rounded_rect((96, 44), 14, (50, 199, 155, 255), outline=(189, 238, 220, 255)), "ui_btn_run_96x44.png")
    save(rounded_rect((96, 44), 14, (255, 123, 90, 255), outline=(255, 212, 195, 255)), "ui_btn_pause_96x44.png")
    save(rounded_rect((220, 36), 14, (255, 103, 82, 255), outline=(255, 212, 195, 255)), "ui_btn_apply_220x36.png")
    save(rounded_rect((48, 196), 24, (255, 253, 249, 230), outline=BORDER, shadow=True), "ui_nav_pill_48x196.png")
    save(rounded_rect((92, 34), 8, CARD, outline=BORDER, shadow=True), "ui_segment_92x34.png")


def status_icons():
    draw_line_icon("ui_icon_print.png", icon_print)
    draw_line_icon("ui_icon_pick.png", icon_pick)
    draw_line_icon("ui_icon_settings.png", icon_settings)
    draw_line_icon("ui_icon_position.png", icon_position)
    draw_line_icon("ui_icon_laser.png", icon_laser)
    draw_line_icon("ui_icon_material.png", icon_material)
    draw_line_icon("ui_icon_speed.png", icon_speed)
    draw_line_icon("ui_icon_passes.png", icon_passes)
    draw_line_icon("ui_icon_mode_line.png", icon_mode_line)
    draw_line_icon("ui_icon_mode_fill.png", icon_mode_fill)
    draw_line_icon("ui_icon_mode_dot.png", icon_mode_dot)
    draw_line_icon("ui_icon_z_height.png", icon_z)
    draw_line_icon("ui_icon_home.png", icon_home)
    draw_line_icon("ui_icon_plus.png", icon_plus)
    draw_line_icon("ui_icon_minus.png", icon_minus)
    draw_line_icon("ui_icon_chevron_up.png", icon_chevron_up, color=MUTED)
    draw_line_icon("ui_icon_chevron_down.png", icon_chevron_down, color=MUTED)


def map_tile():
    img = rgba((240, 240), (247, 244, 238, 235))
    d = ImageDraw.Draw(img)
    for i in range(0, 241, 8):
        d.line((i, 0, i, 240), fill=(46, 196, 182, 24))
        d.line((0, i, 240, i), fill=(46, 196, 182, 24))
    for i in range(0, 241, 40):
        d.line((i, 0, i, 240), fill=(46, 196, 182, 60))
        d.line((0, i, 240, i), fill=(46, 196, 182, 60))
    d.line((120, 0, 120, 240), fill=(46, 196, 182, 130), width=2)
    d.line((0, 120, 240, 120), fill=(46, 196, 182, 130), width=2)
    save(img, "ui_pick_grid_240.png")


def main():
    background()
    jog_diamond()
    file_thumb()
    button_assets()
    status_icons()
    map_tile()


if __name__ == "__main__":
    main()

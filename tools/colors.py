import os
from colorsys import hsv_to_rgb

from PIL import Image

H_OFFSET = 0.06

H_MAX = (1 << 4) - 1
S_MAX = (1 << 2) - 1
V_MAX = (1 << 2) - 1

R_MAX = (1 << 5) - 1
G_MAX = (1 << 6) - 1
B_MAX = (1 << 5) - 1

values = [0.125, 0.25, 0.50, 1.0]

img = Image.new("RGB", (16, 16), "black")
pixels = img.load()

for v in range(V_MAX + 1):
    for s in range(0, S_MAX + 1):
        for h in range(H_MAX + 1):
            r, g, b = hsv_to_rgb(
                (h / (H_MAX + 1) + H_OFFSET) % 1.0,
                (s + 1) / (S_MAX + 1),
                values[v],
            )

            if s == 0 and v == 0:
                r, g, b = h / H_MAX, h / H_MAX, h / H_MAX

            r = round(r * R_MAX)
            g = round(g * G_MAX)
            b = round(b * B_MAX)

            rgb565 = (r << 11) | (g << 5) | b

            print(f"{rgb565:#0{6}x}, ", end="")
            pixels[h, (v << 2) | s] = r << 3, g << 2, b << 3

        print("")

with open("/tmp/colors.png", "wb") as fp:
    img.save(fp, bitmap_format="png")

os.system("tpix -f /tmp/colors.png")
os.unlink("/tmp/colors.png")

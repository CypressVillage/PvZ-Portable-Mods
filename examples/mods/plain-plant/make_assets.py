"""Rebuild the original, procedural example artwork; Python standard library only."""
from pathlib import Path
import struct
import zlib


def write_png(path, card=False):
    pixels = bytearray()
    for y in range(80):
        pixels.append(0)  # PNG filter: none
        for x in range(80):
            color = (0, 0, 0, 0)
            if card:
                color = (214, 232, 185, 255)
            if 17 <= x <= 63 and 28 <= y <= 68:
                color = (118 + (x % 9) * 4, 75 + (x % 9) * 2, 40, 255)
            radius = ((x - 40) / 26) ** 2 + ((y - 28) / 12) ** 2
            if radius <= 1:
                color = (206, 165, 102, 255)
                if 0.16 < radius < 0.24 or 0.57 < radius < 0.67:
                    color = (151, 105, 54, 255)
            if 27 <= x <= 31 and 43 <= y <= 49 or 49 <= x <= 53 and 43 <= y <= 49:
                color = (35, 28, 20, 255)
            pixels.extend(color)

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    path.write_bytes(b"\x89PNG\r\n\x1a\n"
                     + chunk(b"IHDR", struct.pack(">IIBBBBB", 80, 80, 8, 6, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(pixels)) + chunk(b"IEND", b""))


if __name__ == "__main__":
    root = Path(__file__).parent / "assets"
    root.mkdir(exist_ok=True)
    write_png(root / "stump.png")
    write_png(root / "stump_card.png", card=True)

"""Minimal PNG reading and writing with the Python standard library only.

Supports 8-bit RGBA and RGB images without interlacing, which is what Blender and the game write. Used by the atlas
tools so they run in Blender's bundled Python and in CI without extra packages.
"""

import struct
import zlib

_SIGNATURE = b"\x89PNG\r\n\x1a\n"


class Image:
    """An RGBA image as a flat bytearray, row-major, 4 bytes per pixel."""

    def __init__(self, width, height, pixels=None):
        self.width = width
        self.height = height
        self.pixels = pixels if pixels is not None else bytearray(width * height * 4)

    def get(self, x, y):
        i = (y * self.width + x) * 4
        return tuple(self.pixels[i:i + 4])

    def put(self, x, y, rgba):
        i = (y * self.width + x) * 4
        self.pixels[i:i + 4] = bytes(rgba)

    def blit(self, source, left, top):
        for y in range(source.height):
            start = ((top + y) * self.width + left) * 4
            row = y * source.width * 4
            self.pixels[start:start + source.width * 4] = source.pixels[row:row + source.width * 4]

    def crop(self, left, top, width, height):
        out = Image(width, height)
        for y in range(height):
            start = ((top + y) * self.width + left) * 4
            out.pixels[y * width * 4:(y + 1) * width * 4] = self.pixels[start:start + width * 4]
        return out

    def downscale(self, factor):
        """Box filter by an integer factor, with alpha-weighted colour so edges do not darken."""
        width = self.width // factor
        height = self.height // factor
        out = Image(width, height)
        count = factor * factor
        for y in range(height):
            for x in range(width):
                r = g = b = a = 0
                for dy in range(factor):
                    base = ((y * factor + dy) * self.width + x * factor) * 4
                    for dx in range(factor):
                        i = base + dx * 4
                        alpha = self.pixels[i + 3]
                        r += self.pixels[i] * alpha
                        g += self.pixels[i + 1] * alpha
                        b += self.pixels[i + 2] * alpha
                        a += alpha
                if a:
                    out.put(x, y, (r // a, g // a, b // a, a // count))
        return out


def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def read(path):
    with open(path, "rb") as handle:
        data = handle.read()
    if not data.startswith(_SIGNATURE):
        raise ValueError(f"{path}: not a PNG file")
    pos = len(_SIGNATURE)
    width = height = None
    colour = depth = interlace = None
    compressed = bytearray()
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            width, height, depth, colour, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            compressed += body
        elif kind == b"IEND":
            break
    if depth != 8 or colour not in (2, 6) or interlace != 0:
        raise ValueError(f"{path}: only 8-bit RGB or RGBA PNGs without interlacing are supported")
    channels = 4 if colour == 6 else 3
    stride = width * channels
    raw = zlib.decompress(bytes(compressed))
    rows = []
    previous = bytearray(stride)
    pos = 0
    for _ in range(height):
        kind = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        for i in range(stride):
            left = line[i - channels] if i >= channels else 0
            up = previous[i]
            corner = previous[i - channels] if i >= channels else 0
            if kind == 1:
                line[i] = (line[i] + left) & 0xFF
            elif kind == 2:
                line[i] = (line[i] + up) & 0xFF
            elif kind == 3:
                line[i] = (line[i] + ((left + up) >> 1)) & 0xFF
            elif kind == 4:
                line[i] = (line[i] + _paeth(left, up, corner)) & 0xFF
        rows.append(line)
        previous = line
    image = Image(width, height)
    for y, line in enumerate(rows):
        for x in range(width):
            src = x * channels
            alpha = line[src + 3] if channels == 4 else 255
            image.put(x, y, (line[src], line[src + 1], line[src + 2], alpha))
    return image


def write(path, image):
    raw = bytearray()
    stride = image.width * 4
    for y in range(image.height):
        raw.append(0)
        raw += image.pixels[y * stride:(y + 1) * stride]

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", image.width, image.height, 8, 6, 0, 0, 0)
    with open(path, "wb") as handle:
        handle.write(_SIGNATURE)
        handle.write(chunk(b"IHDR", header))
        handle.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        handle.write(chunk(b"IEND", b""))

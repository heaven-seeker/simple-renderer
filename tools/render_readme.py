"""Render the README gallery using the compiled renderer and local OBJ textures."""

import argparse
import os
from pathlib import Path
import struct
import subprocess
import time

from PIL import Image


ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "docs" / "images"


def convert_tga(source, destination):
    """Convert renderer output, including RLE packets spanning multiple rows."""
    # The renderer writes packets across scanlines; Pillow's TGA decoder rejects
    # these. Decode the pixel stream here, preserving its colors and orientation.
    data = source.read_bytes()
    header = struct.unpack_from("<BBBHHBHHHHBB", data)
    image_id, color_map, kind = header[:3]
    width, height, bits, descriptor = header[8:]
    channels = bits // 8
    if color_map or kind not in (2, 3, 10, 11) or channels not in (1, 3, 4):
        raise ValueError(f"Unsupported TGA format: {source}")

    position = 18 + image_id
    size = width * height * channels
    if kind in (2, 3):
        pixels = data[position:position + size]
    else:
        pixels = bytearray()
        while len(pixels) < size:
            packet = data[position]
            position += 1
            count = (packet & 127) + 1
            if packet & 128:
                pixels.extend(data[position:position + channels] * count)
                position += channels
            else:
                length = count * channels
                pixels.extend(data[position:position + length])
                position += length
    if len(pixels) != size:
        raise ValueError(f"Unexpected TGA pixel count: {source}")

    mode, raw_mode = {1: ("L", "L"), 3: ("RGB", "BGR"), 4: ("RGBA", "BGRA")}[channels]
    image = Image.frombytes(mode, (width, height), bytes(pixels), "raw", raw_mode)
    if not descriptor & 0x20:
        image = image.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    if descriptor & 0x10:
        image = image.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    image.save(destination, optimize=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("renderer", type=Path, help="Path to the compiled simplerenderer executable")
    parser.add_argument("--size", type=int, default=1536, help="Square image size (default: 1536)")
    args = parser.parse_args()
    executable = args.renderer.resolve()
    if not executable.is_file():
        parser.error(f"Renderer not found: {executable}")
    if not 1 <= args.size <= 65535:
        parser.error("--size must be between 1 and 65535")

    diablo = ["diablo3_pose/diablo3_pose.obj"]
    head = ["african_head/african_head.obj", "african_head/african_head_eye_inner.obj"]
    boggie = ["boggie/body.obj", "boggie/head.obj", "boggie/eyes.obj"]
    scenes = [
        ("diablo-tangentspace", diablo, "tangentspace", "ssao", "orange", {
            "framebuffer.tga": "diablo-tangentspace.png",
            "ssao_raw.tga": "ssao-raw.png",
            "zbuffer_image.tga": "zbuffer.png",
        }),
        ("diablo-toon", diablo, "toon", "ssao", "orange", {
            "framebuffer.tga": "diablo-toon.png",
        }),
        ("diablo-bruteforce", diablo, "tangentspace", "bruteforce", "orange", {
            "ao_buffer.tga": "ao-bruteforce.png",
        }),
        ("african-head", head, "tangentspace", "ssao", "white", {
            "framebuffer.tga": "african-head.png",
        }),
        ("boggie", boggie, "tangentspace", "ssao", "orange", {
            "framebuffer.tga": "boggie.png",
        }),
    ]

    # Fail before rendering if texture assets are missing: without them the
    # renderer silently falls back to white surfaces and flat normals.
    for model in ["floor.obj", *diablo, *head, *boggie]:
        path = ROOT / "obj" / model
        for required in (path, path.with_name(path.stem + "_diffuse.tga"),
                         path.with_name(path.stem + "_nm_tangent.tga")):
            if not required.is_file():
                parser.error(f"Model/texture missing: {required}")

    OUTPUT.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    # Rasterization opens an OpenMP region for every triangle. One thread avoids
    # that overhead for these small scenes, particularly the 1000 AO passes.
    env["OMP_NUM_THREADS"] = "1"
    for name, models, shader, ao, color, files in scenes:
        work = ROOT / "build" / "readme-renders" / name
        work.mkdir(parents=True, exist_ok=True)
        command = [str(executable), "-m", *[str(ROOT / "obj" / model) for model in models],
                   "-f", str(ROOT / "obj" / "floor.obj"), "-shader", shader,
                   "-ao", ao, "-c", color, "-w", str(args.size), "-s", str(args.size)]
        print(f"Rendering {name} ({args.size} x {args.size})...", flush=True)
        started = time.perf_counter()
        # Separate working directories keep each run's intermediate buffers.
        with (work / "render.log").open("w", encoding="utf-8") as log:
            subprocess.run(command, cwd=work, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
        for source, destination in files.items():
            convert_tga(work / source, OUTPUT / destination)
            print(f"  docs/images/{destination}", flush=True)
        print(f"  Done in {time.perf_counter() - started:.1f}s", flush=True)


if __name__ == "__main__":
    main()

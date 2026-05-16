#!/usr/bin/env python3
"""
PvZ PAK file unpacker / packer.

PAK format:
    Header (8 bytes):
        magic:  u32 LE = 0xBAC04AC0
        version: u32 LE (must be <= 0, typically 0)
    File records (repeat until FILEFLAGS_END):
        flags:      u8      (0x80 = end-of-records marker)
        name_width: u8
        name:       u8[name_width]  (\\ -> /)
        src_size:   u32 LE
        file_time:  u64 LE
    File data:
        concatenated raw file contents, in record order.

    The entire content is XOR-encrypted with 0xF7.
"""

import os
import struct
import sys
from pathlib import Path
from datetime import datetime

FILEFLAGS_END = 0x80
PAK_MAGIC = 0xBAC04AC0
XOR_KEY = 0xF7


def xor_data(data: bytearray | bytes, key: int = XOR_KEY) -> bytearray:
    d = bytearray(data)
    for i in range(len(d)):
        d[i] ^= key
    return d


# ── Unpack ──────────────────────────────────────────────────────────────

def unpack(pak_path: str, output_dir: str):
    """Extract all files from a .pak into output_dir."""
    with open(pak_path, "rb") as f:
        raw = bytearray(f.read())

    raw = xor_data(raw)
    if len(raw) < 8:
        sys.exit("PAK file too small.")

    magic = struct.unpack_from("<I", raw, 0)[0]
    version = struct.unpack_from("<I", raw, 4)[0]
    if magic != PAK_MAGIC:
        sys.exit(f"Bad magic: 0x{magic:08X}, expected 0x{PAK_MAGIC:08X}")
    if version > 0:
        sys.exit(f"Unknown version: {version}")

    files = []
    pos = 8
    while pos < len(raw):
        flags = raw[pos]; pos += 1
        if flags & FILEFLAGS_END:
            break
        name_width = raw[pos]; pos += 1
        name_bytes = raw[pos:pos + name_width]; pos += name_width
        name = name_bytes.decode("ascii").replace("\\", "/")
        src_size = struct.unpack_from("<I", raw, pos)[0]; pos += 4
        file_time = struct.unpack_from("<Q", raw, pos)[0]; pos += 8
        files.append((name, src_size, file_time))

    for name, size, ftime in files:
        out = Path(output_dir) / name
        out.parent.mkdir(parents=True, exist_ok=True)
        data = raw[pos:pos + size]
        pos += size
        with open(out, "wb") as f:
            f.write(data)
        try:
            dt = datetime.fromtimestamp(max(0, ftime))
        except (OSError, OverflowError, ValueError):
            dt = datetime.fromtimestamp(0)
        t_str = dt.strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{t_str}] {name} ({size} bytes)")

    print(f"\nExtracted {len(files)} files to {output_dir}")


# ── Pack ────────────────────────────────────────────────────────────────

def pack(input_dir: str, pak_path: str):
    """
    Pack all files in input_dir into a .pak.

    Files are included in sorted order for reproducibility.
    Symbolic links and directories are skipped.
    """
    base = Path(input_dir).resolve()
    if not base.is_dir():
        sys.exit(f"Not a directory: {input_dir}")

    # collect files
    entries = []
    for root, dirs, filenames in os.walk(base):
        for fn in filenames:
            full = Path(root) / fn
            if full.is_symlink():
                continue
            rel = full.relative_to(base).as_posix()
            size = full.stat().st_size
            mtime = int(full.stat().st_mtime)
            with open(full, "rb") as f:
                data = f.read()
            entries.append((rel, size, mtime, data))

    entries.sort(key=lambda e: e[0])

    # compute record sizes to know where data section starts
    header_size = 8
    record_sizes = []
    for name, size, ftime, _ in entries:
        # flags(1) + name_width(1) + name + src_size(4) + file_time(8)
        rec_len = 1 + 1 + len(name) + 4 + 8
        record_sizes.append(rec_len)
    # end marker: flags(1)
    total_records = sum(record_sizes) + 1
    data_offset = header_size + total_records

    # build buffer
    buf = bytearray()
    buf += struct.pack("<II", PAK_MAGIC, 0)  # magic + version

    cur_offset = 0
    for (name, size, ftime, data), rec_len in zip(entries, record_sizes):
        buf.append(0)  # flags
        name_enc = name.encode("ascii")
        buf.append(len(name_enc))
        buf += name_enc
        buf += struct.pack("<I", size)
        buf += struct.pack("<Q", ftime)
        cur_offset += size

    buf.append(FILEFLAGS_END)  # end marker

    for _, _, _, data in entries:
        buf += data

    encrypted = xor_data(buf)
    with open(pak_path, "wb") as f:
        f.write(encrypted)

    print(f"Packed {len(entries)} files -> {pak_path}")


# ── List ────────────────────────────────────────────────────────────────

def list_files(pak_path: str):
    """List contents of a .pak file without extracting."""
    with open(pak_path, "rb") as f:
        raw = bytearray(f.read())

    raw = xor_data(raw)
    if len(raw) < 8:
        sys.exit("PAK file too small.")

    magic = struct.unpack_from("<I", raw, 0)[0]
    version = struct.unpack_from("<I", raw, 4)[0]
    print(f"magic:   0x{magic:08X}")
    print(f"version: {version}")

    if magic != PAK_MAGIC:
        sys.exit(f"Bad magic: 0x{magic:08X}, expected 0x{PAK_MAGIC:08X}")

    files = []
    pos = 8
    while pos < len(raw):
        flags = raw[pos]; pos += 1
        if flags & FILEFLAGS_END:
            break
        name_width = raw[pos]; pos += 1
        name_bytes = raw[pos:pos + name_width]; pos += name_width
        name = name_bytes.decode("ascii").replace("\\", "/")
        src_size = struct.unpack_from("<I", raw, pos)[0]; pos += 4
        file_time = struct.unpack_from("<Q", raw, pos)[0]; pos += 8
        files.append((name, src_size, file_time))

    total_size = sum(s[1] for s in files)
    print(f"\n{'Name':<60} {'Size':>10} {'Time'}")
    print("-" * 95)
    for name, size, ftime in files:
        try:
            dt = datetime.fromtimestamp(max(0, ftime))
        except (OSError, OverflowError, ValueError):
            dt = datetime.fromtimestamp(0)
        t_str = dt.strftime("%Y-%m-%d %H:%M:%S")
        print(f"{name:<60} {size:>10}  {t_str}")
    print("-" * 95)
    print(f"{len(files)} files, {total_size} bytes total")


# ── CLI ─────────────────────────────────────────────────────────────────

def usage():
    print("Usage:")
    print("  python3 pak_tool.py unpack  <input.pak>  <output_dir>")
    print("  python3 pak_tool.py pack    <input_dir>   <output.pak>")
    print("  python3 pak_tool.py list    <input.pak>")
    sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        usage()

    cmd = sys.argv[1]
    if cmd == "unpack":
        if len(sys.argv) != 4:
            usage()
        unpack(sys.argv[2], sys.argv[3])
    elif cmd == "pack":
        if len(sys.argv) != 4:
            usage()
        pack(sys.argv[2], sys.argv[3])
    elif cmd == "list":
        list_files(sys.argv[2])
    else:
        usage()

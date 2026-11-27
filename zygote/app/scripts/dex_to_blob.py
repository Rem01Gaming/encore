import sys


def generate_blob(dex_path, out_path):
    with open(dex_path, "rb") as f:
        data = f.read()

    with open(out_path, "w") as f:
        f.write("#pragma once\n#include <cstdint>\n#include <cstddef>\n\n")
        f.write(f"const size_t dex_len = {len(data)};\n")
        f.write("alignas(16) const unsigned char dex_bytes[] = {\n")
        for i in range(0, len(data), 16):
            chunk = data[i:i + 16]
            f.write("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
        f.write("};\n")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: dex_to_blob.py <dex_path> <out_path>", file=sys.stderr)
        sys.exit(1)
    generate_blob(sys.argv[1], sys.argv[2])

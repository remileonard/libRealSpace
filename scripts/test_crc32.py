#!/usr/bin/env python3
# find_hash_variant.py
# Usage: prepare a file pairs.txt with lines like:
# 3E0994FF<TAB>..\..\DATA\TERRAIN\MAP.PAK
# F04CA7B4<TAB>..\..\DATA\TERRAIN\MAP2.PAK

import sys
import binascii
import struct
from typing import List, Tuple

# -----------------------------
# Utils for loading pairs file
# -----------------------------
def load_pairs(path: str) -> List[Tuple[int, str]]:
    pairs = []
    with open(path, 'r', encoding='utf-8') as f:
        for ln in f:
            ln = ln.strip()
            if not ln or ln.startswith('#'):
                continue
            parts = ln.split('\t')
            if len(parts) < 2:
                continue
            try:
                crc = int(parts[0].strip(), 16)
            except ValueError:
                continue
            name = parts[1]
            pairs.append((crc & 0xFFFFFFFF, name))
    return pairs

# -----------------------------
# Hash function implementations
# -----------------------------
# CRC generic bitwise (supports reflected or normal)
def crc32_bitwise(data: bytes, poly: int, init: int, xor_out: int, reflect: bool) -> int:
    """Compute a 32-bit CRC bitwise. poly should be the normal (unreflected) poly if reflect=False,
       or the reflected poly if reflect=True. Returns 32-bit int."""
    if reflect:
        crc = init & 0xFFFFFFFF
        for b in data:
            crc ^= b
            for _ in range(8):
                if crc & 1:
                    crc = (crc >> 1) ^ poly
                else:
                    crc >>= 1
        return crc ^ xor_out
    else:
        # non-reflected straightforward (process MSB first)
        crc = init & 0xFFFFFFFF
        topbit = 1 << 31
        mask = 0xFFFFFFFF
        for b in data:
            crc ^= (b << 24)
            for _ in range(8):
                if crc & topbit:
                    crc = ((crc << 1) ^ poly) & mask
                else:
                    crc = (crc << 1) & mask
        return crc ^ xor_out

# Adler32 wrapper
def adler32_wrap(data: bytes) -> int:
    return binascii.adler32(data) & 0xFFFFFFFF

# FNV-1 and FNV-1a 32-bit
def fnv1_32(data: bytes) -> int:
    h = 0x811c9dc5
    for c in data:
        h = (h * 0x01000193) & 0xFFFFFFFF
        h ^= c
    return h

def fnv1a_32(data: bytes) -> int:
    h = 0x811c9dc5
    for c in data:
        h ^= c
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h

# DJB2 32-bit
def djb2_32(data: bytes) -> int:
    h = 5381
    for c in data:
        h = ((h << 5) + h + c) & 0xFFFFFFFF
    return h

# stdlib CRC32 (binascii) wrapper (reflected IEEE with init=0, no xor in binascii)
def crc32_binascii(data: bytes) -> int:
    return binascii.crc32(data) & 0xFFFFFFFF

# -----------------------------
# Endianness helper
# -----------------------------
def le_swap32(v: int) -> int:
    return struct.unpack("<I", struct.pack(">I", v))[0]

# -----------------------------
# Candidate polynomials and parameter sets
# -----------------------------
# polynomials: for reflected operation we use reflected representations
POLYS = {
    "CRC32-IEEE-reflected": 0xEDB88320,     # reflected poly for IEEE (used with reflect=True)
    "CRC32C-reflected": 0x82F63B78,         # reflected poly for Castagnoli (CRC32C)
    # non-reflected polynomials (use reflect=False and non-reflected poly)
    "CRC32-IEEE": 0x04C11DB7,
    "CRC32-Koopman": 0x741B8CD7
}

# parameter combos: (init, xor_out, reflect)
PARAMS = [
    (0x00000000, 0x00000000, True),
    (0xFFFFFFFF, 0xFFFFFFFF, True),
    (0x00000000, 0xFFFFFFFF, True),
    (0xFFFFFFFF, 0x00000000, True),
    (0x00000000, 0x00000000, False),
    (0xFFFFFFFF, 0xFFFFFFFF, False),
    (0x00000000, 0xFFFFFFFF, False),
    (0xFFFFFFFF, 0x00000000, False),
]

# Candidate hash functions beyond CRC (name -> function returning 32-bit int)
OTHER_HASHES = {
    "Adler32": adler32_wrap,
    "FNV-1 32": fnv1_32,
    "FNV-1a 32": fnv1a_32,
    "DJB2 32": djb2_32,
    "CRC32_binascii": crc32_binascii,  # convenience
}

# -----------------------------
# Normalisations to test on each filename
# -----------------------------
def gen_normalisations(s: str) -> List[Tuple[str, bytes]]:
    variants = []
    # raw exact
    variants.append(("exact", s.encode('ascii', errors='ignore')))
    # lower/upper
    variants.append(("lower", s.lower().encode('ascii', errors='ignore')))
    variants.append(("upper", s.upper().encode('ascii', errors='ignore')))
    # slash variants
    variants.append(("slashes->/", s.replace("\\", "/").encode('ascii', errors='ignore')))
    variants.append(("slashes->\\\\", s.replace("/", "\\").encode('ascii', errors='ignore')))
    # without leading ..\..\ prefix if present
    if s.startswith("..\\..\\"):
        variants.append(("no ..\\\\..\\\\ prefix", s.split("..\\..\\",1)[1].encode('ascii', errors='ignore')))
    elif s.startswith("../../"):
        variants.append(("no ../../ prefix", s.split("../../",1)[1].encode('ascii', errors='ignore')))
    # with trailing null byte
    variants.append(("with_null", (s + "\x00").encode('ascii', errors='ignore')))
    # also test trimming any leading/trailing whitespace
    variants.append(("strip", s.strip().encode('ascii', errors='ignore')))
    # possible uppercase extension only
    if '.' in s:
        base, ext = s.rsplit('.',1)
        variants.append(("upper_ext", (base + "." + ext.upper()).encode('ascii', errors='ignore')))
        variants.append(("lower_ext", (base + "." + ext.lower()).encode('ascii', errors='ignore')))
    # unique-ify variants (avoid duplicates)
    seen = set()
    uniq = []
    for name, b in variants:
        key = b
        if key in seen:
            continue
        seen.add(key)
        uniq.append((name, b))
    return uniq

# -----------------------------
# Core search logic
# -----------------------------
def search_variants(pairs: List[Tuple[int,str]]):
    results = []
    for (target_crc, fname) in pairs:
        norms = gen_normalisations(fname)
        found = []
        # 1) test OTHER_HASHES directly
        for nname, b in norms:
            for hname, hfunc in OTHER_HASHES.items():
                try:
                    h = hfunc(b) & 0xFFFFFFFF
                except Exception:
                    continue
                if h == target_crc:
                    found.append((fname, nname, hname, "native", f"0x{h:08X}"))
                if le_swap32(h) == target_crc:
                    found.append((fname, nname, hname, "le_swap", f"0x{le_swap32(h):08X}"))
        # 2) test CRC polynomials + params
        for poly_name, poly_val in POLYS.items():
            for init, xor_out, reflect in PARAMS:
                # decide which implementation parameters to use:
                # - if poly_name ends with "-reflected" we should call crc32_bitwise with reflect=True and poly_val as reflected poly
                # - if poly_name is non-reflected we call with reflect=False and the non-reflected poly
                reflect_flag = reflect
                # override reflect flag for poly names that explicitly state reflected/non-reflected
                if poly_name.endswith("-reflected"):
                    reflect_flag = True
                if poly_name in ("CRC32-IEEE","CRC32-Koopman"):
                    reflect_flag = False
                for nname, b in norms:
                    try:
                        h = crc32_bitwise(b, poly_val, init, xor_out, reflect_flag) & 0xFFFFFFFF
                    except Exception:
                        continue
                    if h == target_crc:
                        found.append((fname, nname, f"{poly_name} init=0x{init:08X} xor=0x{xor_out:08X} refl={reflect_flag}", "native", f"0x{h:08X}"))
                    if le_swap32(h) == target_crc:
                        found.append((fname, nname, f"{poly_name} init=0x{init:08X} xor=0x{xor_out:08X} refl={reflect_flag}", "le_swap", f"0x{le_swap32(h):08X}"))
        results.append((target_crc, fname, found))
    return results

# -----------------------------
# Pretty print results
# -----------------------------
def print_results(results):
    any_found = False
    for target_crc, fname, found in results:
        print("="*80)
        print(f"Target CRC: 0x{target_crc:08X}   Filename: {fname}")
        if not found:
            print("  No matches found for this pair with tested algorithms/normalisations.")
        else:
            any_found = True
            for f in found:
                fname_in, norm, algo, reprtype, val = f
                print(f"  MATCH -> norm: {norm:22} algo: {algo:48} repr: {reprtype:8} value: {val}")
    if not any_found:
        print("="*80)
        print("No matches across all pairs with the tested candidate algos/params.")

# -----------------------------
# Main
# -----------------------------
def main():
    if len(sys.argv) < 2:
        print("Usage: python find_hash_variant.py pairs.txt")
        return
    pairs_file = sys.argv[1]
    pairs = load_pairs(pairs_file)
    if not pairs:
        print("No valid pairs found in", pairs_file)
        return
    print(f"Loaded {len(pairs)} pairs from {pairs_file}")
    results = search_variants(pairs)
    print_results(results)

if __name__ == "__main__":
    main()

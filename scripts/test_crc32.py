#!/usr/bin/env python3
"""
Script pour identifier l'algorithme CRC/Hash utilisé par Wing Commander III
pour les archives XTRE.

Usage: python find_wc3_crc.py
"""

import struct
from typing import List, Tuple, Callable

# Paires connues : (CRC, chemin de fichier)
KNOWN_PAIRS = [
    (0x3E0994FF, r"..\..\DATA\TERRAIN\MAP.PAK"),
    (0xF04CA7B4, r"..\..\DATA\TERRAIN\MAP2.PAK"),
]

# ============================================================================
# Implémentations CRC
# ============================================================================

def crc32_bitwise(data: bytes, poly: int, init: int, xor_out: int, refin: bool, refout: bool) -> int:
    """
    Implémentation CRC32 bit-par-bit configurable.
    
    Args:
        data: Données à hasher
        poly: Polynôme (normal ou réfléchi selon refin)
        init: Valeur initiale
        xor_out: XOR final
        refin: Réflexion des bits d'entrée
        refout: Réflexion du résultat avant xor_out
    """
    def reflect_byte(val: int) -> int:
        result = 0
        for i in range(8):
            if val & (1 << i):
                result |= 1 << (7 - i)
        return result
    
    def reflect_32(val: int) -> int:
        result = 0
        for i in range(32):
            if val & (1 << i):
                result |= 1 << (31 - i)
        return result
    
    crc = init & 0xFFFFFFFF
    
    if refin:
        # Algorithme réfléchi (LSB first)
        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 1:
                    crc = (crc >> 1) ^ poly
                else:
                    crc >>= 1
    else:
        # Algorithme normal (MSB first)
        topbit = 1 << 31
        for byte in data:
            crc ^= (byte << 24)
            for _ in range(8):
                if crc & topbit:
                    crc = ((crc << 1) ^ poly) & 0xFFFFFFFF
                else:
                    crc = (crc << 1) & 0xFFFFFFFF
    
    if refout and not refin:
        crc = reflect_32(crc)
    
    return (crc ^ xor_out) & 0xFFFFFFFF

def crc32_table(data: bytes, poly: int, init: int, xor_out: int, refin: bool, refout: bool) -> int:
    """Version optimisée avec table de lookup."""
    # Générer la table
    table = []
    for i in range(256):
        if refin:
            crc = i
            for _ in range(8):
                if crc & 1:
                    crc = (crc >> 1) ^ poly
                else:
                    crc >>= 1
            table.append(crc)
        else:
            crc = i << 24
            for _ in range(8):
                if crc & 0x80000000:
                    crc = ((crc << 1) ^ poly) & 0xFFFFFFFF
                else:
                    crc = (crc << 1) & 0xFFFFFFFF
            table.append(crc)
    
    # Calculer le CRC
    crc = init & 0xFFFFFFFF
    
    if refin:
        for byte in data:
            crc = table[(crc ^ byte) & 0xFF] ^ (crc >> 8)
    else:
        for byte in data:
            crc = table[((crc >> 24) ^ byte) & 0xFF] ^ ((crc << 8) & 0xFFFFFFFF)
    
    if refout and not refin:
        def reflect_32(val: int) -> int:
            result = 0
            for i in range(32):
                if val & (1 << i):
                    result |= 1 << (31 - i)
            return result
        crc = reflect_32(crc)
    
    return (crc ^ xor_out) & 0xFFFFFFFF

# ============================================================================
# Configurations CRC courantes
# ============================================================================

CRC_CONFIGS = {
    # Configurations standards
    "CRC-32 (IEEE 802.3)": (0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, True, True),
    "CRC-32 (IEEE) reflected poly": (0xEDB88320, 0xFFFFFFFF, 0xFFFFFFFF, True, True),
    "CRC-32/BZIP2": (0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, False, False),
    "CRC-32C (Castagnoli)": (0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, True, True),
    "CRC-32C reflected poly": (0x82F63B78, 0xFFFFFFFF, 0xFFFFFFFF, True, True),
    "CRC-32/MPEG-2": (0x04C11DB7, 0xFFFFFFFF, 0x00000000, False, False),
    "CRC-32/POSIX": (0x04C11DB7, 0x00000000, 0xFFFFFFFF, False, False),
    "CRC-32Q": (0x814141AB, 0x00000000, 0x00000000, False, False),
    
    # Variantes Wing Commander (hypothèses basées sur les jeux Origin des années 90)
    "WC-variant-1": (0x04C11DB7, 0x00000000, 0x00000000, False, False),
    "WC-variant-2": (0xEDB88320, 0x00000000, 0x00000000, True, False),
    "WC-variant-3": (0x04C11DB7, 0xFFFFFFFF, 0x00000000, False, False),
    "WC-variant-4": (0xEDB88320, 0xFFFFFFFF, 0x00000000, True, False),
    "WC-variant-5": (0x04C11DB7, 0x00000000, 0xFFFFFFFF, False, True),
    "WC-variant-6": (0xEDB88320, 0x00000000, 0xFFFFFFFF, True, True),
    
    # Polynômes moins courants
    "CRC-32/XFER": (0x000000AF, 0x00000000, 0x00000000, False, False),
    "CRC-32D": (0xA833982B, 0xFFFFFFFF, 0xFFFFFFFF, True, True),
}

# ============================================================================
# Normalisations de chaînes
# ============================================================================

def generate_string_variants(path: str) -> List[Tuple[str, bytes]]:
    """Génère toutes les variantes possibles de normalisation de chaîne."""
    variants = []
    
    # 1. Tel quel
    variants.append(("original", path.encode('ascii', errors='ignore')))
    
    # 2. Casse
    variants.append(("uppercase", path.upper().encode('ascii', errors='ignore')))
    variants.append(("lowercase", path.lower().encode('ascii', errors='ignore')))
    
    # 3. Séparateurs
    variants.append(("forward_slash", path.replace("\\", "/").encode('ascii', errors='ignore')))
    variants.append(("double_backslash", path.replace("\\", "\\\\").encode('ascii', errors='ignore')))
    
    # 4. Sans préfixe relatif
    if path.startswith("..\\..\\"):
        variants.append(("no_prefix", path[6:].encode('ascii', errors='ignore')))
    elif path.startswith("../../"):
        variants.append(("no_prefix_fwd", path[6:].encode('ascii', errors='ignore')))
    
    # 5. Avec terminateur null
    variants.append(("null_terminated", (path + "\x00").encode('ascii', errors='ignore')))
    
    # 6. Sans espaces
    variants.append(("no_spaces", path.replace(" ", "").encode('ascii', errors='ignore')))
    
    # 7. Juste le nom de fichier (sans chemin)
    if "\\" in path:
        filename = path.split("\\")[-1]
        variants.append(("filename_only", filename.encode('ascii', errors='ignore')))
    elif "/" in path:
        filename = path.split("/")[-1]
        variants.append(("filename_only_fwd", filename.encode('ascii', errors='ignore')))
    
    # 8. Extension en majuscules uniquement
    if "." in path:
        base, ext = path.rsplit(".", 1)
        variants.append(("ext_upper", f"{base}.{ext.upper()}".encode('ascii', errors='ignore')))
    
    # Dédupliquer
    seen = set()
    unique = []
    for name, data in variants:
        if data not in seen:
            seen.add(data)
            unique.append((name, data))
    
    return unique

# ============================================================================
# Transformations de sortie
# ============================================================================

def apply_output_transforms(value: int) -> List[Tuple[str, int]]:
    """Applique différentes transformations sur la valeur de sortie."""
    transforms = []
    
    # 1. Tel quel
    transforms.append(("native", value))
    
    # 2. Byte swap
    transforms.append(("byte_swap", struct.unpack("<I", struct.pack(">I", value))[0]))
    
    # 3. Complément
    transforms.append(("complement", (~value) & 0xFFFFFFFF))
    
    # 4. Byte swap du complément
    comp = (~value) & 0xFFFFFFFF
    transforms.append(("byte_swap_complement", struct.unpack("<I", struct.pack(">I", comp))[0]))
    
    return transforms

# ============================================================================
# Recherche exhaustive
# ============================================================================

def search_crc_algorithm():
    """Recherche l'algorithme CRC correspondant aux paires connues."""
    print("=" * 80)
    print("RECHERCHE DE L'ALGORITHME CRC WING COMMANDER III")
    print("=" * 80)
    print()
    
    matches = []
    
    for config_name, (poly, init, xor_out, refin, refout) in CRC_CONFIGS.items():
        for target_crc, filepath in KNOWN_PAIRS:
            variants = generate_string_variants(filepath)
            
            for variant_name, data in variants:
                # Calculer le CRC
                try:
                    crc = crc32_bitwise(data, poly, init, xor_out, refin, refout)
                    
                    # Tester les transformations de sortie
                    transforms = apply_output_transforms(crc)
                    
                    for transform_name, transformed_crc in transforms:
                        if transformed_crc == target_crc:
                            matches.append({
                                'config': config_name,
                                'poly': f"0x{poly:08X}",
                                'init': f"0x{init:08X}",
                                'xor_out': f"0x{xor_out:08X}",
                                'refin': refin,
                                'refout': refout,
                                'filepath': filepath,
                                'variant': variant_name,
                                'transform': transform_name,
                                'target': f"0x{target_crc:08X}",
                                'data': data.decode('ascii', errors='replace')
                            })
                except Exception as e:
                    continue
    
    # Afficher les résultats
    if matches:
        print(f"✓ TROUVÉ {len(matches)} CORRESPONDANCE(S) !\n")
        
        for i, match in enumerate(matches, 1):
            print(f"Match #{i}:")
            print(f"  Configuration: {match['config']}")
            print(f"  Polynôme:     {match['poly']}")
            print(f"  Init:         {match['init']}")
            print(f"  XOR out:      {match['xor_out']}")
            print(f"  RefIn:        {match['refin']}")
            print(f"  RefOut:       {match['refout']}")
            print(f"  Fichier:      {match['filepath']}")
            print(f"  Variante:     {match['variant']}")
            print(f"  Transform:    {match['transform']}")
            print(f"  Données:      {repr(match['data'])}")
            print(f"  Target CRC:   {match['target']}")
            print()
        
        # Vérifier si une config marche pour toutes les paires
        config_counts = {}
        for match in matches:
            key = (match['config'], match['variant'], match['transform'])
            config_counts[key] = config_counts.get(key, 0) + 1
        
        complete_matches = [k for k, v in config_counts.items() if v == len(KNOWN_PAIRS)]
        
        if complete_matches:
            print("=" * 80)
            print("✓✓✓ ALGORITHME IDENTIFIÉ ✓✓✓")
            print("=" * 80)
            for config, variant, transform in complete_matches:
                print(f"\nConfiguration: {config}")
                print(f"Normalisation: {variant}")
                print(f"Transformation: {transform}")
                
                # Afficher les détails
                example = next(m for m in matches if m['config'] == config and 
                             m['variant'] == variant and m['transform'] == transform)
                print(f"\nParamètres:")
                print(f"  Polynôme:  {example['poly']}")
                print(f"  Init:      {example['init']}")
                print(f"  XOR out:   {example['xor_out']}")
                print(f"  RefIn:     {example['refin']}")
                print(f"  RefOut:    {example['refout']}")
        else:
            print("⚠ Aucune configuration ne correspond à toutes les paires.")
            print("  Il faut peut-être plus d'exemples ou tester d'autres variantes.")
    else:
        print("✗ Aucune correspondance trouvée avec les configurations testées.")
        print("\nSuggestions:")
        print("  1. Vérifier que les valeurs CRC sont correctes")
        print("  2. Ajouter plus de paires connues")
        print("  3. Le CRC pourrait utiliser un polynôme personnalisé")

# ============================================================================
# Main
# ============================================================================

if __name__ == "__main__":
    search_crc_algorithm()
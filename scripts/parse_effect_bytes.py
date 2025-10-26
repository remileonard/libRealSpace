MISN_EFFECT = [
    0x0A, 0x39, 0x1B, 0x0D, 0x29, 0x0F, 0x04, 0x1F, 0x04, 0x06, 0x6E, 0x08,
    0x0A, 0x58, 0x02, 0x18, 0x11, 0x00, 0xBE, 0x00, 0x02, 0x00, 0x0B, 0x04,
    0x1F, 0x04, 0x18, 0x02, 0x0A, 0xB9, 0x00, 0x00, 0xBF, 0x00, 0x28, 0x04,
    0x0D, 0x20, 0x0A, 0x63, 0x08, 0x23, 0x56, 0x02, 0x12, 0x11, 0x0F, 0x04,
    0x1F, 0x04, 0x0A, 0x62, 0x08, 0x23, 0x56, 0x07, 0x12, 0x10, 0x0F, 0x04,
    0x1F, 0x04, 0x0A, 0x61, 0x08, 0x23, 0x56, 0x06, 0x12, 0x0F, 0x0F, 0x04,
    0x1F, 0x04, 0x0A, 0x67, 0x08, 0x23, 0x56, 0x05, 0x12, 0x0E, 0x0F, 0x04,
    0x1F, 0x04, 0x0A, 0x5F, 0x08, 0x23, 0x56, 0x08, 0x12, 0x0D, 0x0F, 0x04,
    0x1F, 0x04, 0x0A, 0x5E, 0x08, 0x23, 0x56, 0x04, 0x12, 0x0C, 0x0F, 0x04,
    0x1F, 0x04, 0x0A, 0x5D, 0x08, 0x23, 0x56, 0x03, 0x12, 0x0D, 0x0F, 0x04,
    0x1F, 0x04, 0x0A, 0x66, 0x08, 0x23, 0x56, 0x02, 0x12, 0x0A, 0x0F, 0x04,
    0x1F, 0x04, 0x23, 0x56, 0x01, 0x12, 0x09, 0x0F, 0x04, 0x1F, 0x04, 0x07,
    0xD0, 0x07, 0x07, 0xD1, 0x07, 0x07, 0xD2, 0x07, 0x07, 0xE8, 0x03, 0x06,
    0x71, 0x08, 0x07, 0xB8, 0x08, 0x0A, 0x59, 0x02, 0x00, 0xB9, 0x00, 0x1F,
    0x04, 0x0D, 0x01, 0x0C, 0x00, 0x0A, 0x70, 0x08, 0x0A, 0x59, 0x02, 0x07,
    0x70, 0x08, 0x06, 0x6E, 0x08, 0x07, 0x59, 0x02, 0x1F, 0x04, 0x1F, 0x04,
    0x0A, 0x6F, 0x08, 0x0A, 0x59, 0x02, 0x06, 0x70, 0x08, 0x07, 0x6F, 0x08,
    0x07, 0x59, 0x02, 0x1F, 0x04, 0x1F, 0x04, 0x0A, 0x6E, 0x08, 0x0A, 0x59,
    0x02, 0x06, 0x6F, 0x08, 0x07, 0x6E, 0x08, 0x07, 0x59, 0x02, 0x1F, 0x04,
    0x1F, 0x04, 0x0A, 0x97, 0x08, 0x0A, 0x24, 0x09, 0x00, 0x34, 0x01, 0x07,
    0x24, 0x09, 0x02, 0x00, 0x1F, 0x04, 0x0A, 0x25, 0x09, 0x00, 0x35, 0x01,
    0x07, 0x25, 0x09, 0x02, 0x00, 0x1F, 0x04, 0x0A, 0x26, 0x09, 0x00, 0x36,
    0x01, 0x07, 0x26, 0x09, 0x02, 0x00, 0x1F, 0x04, 0x0A, 0x27, 0x09, 0x00,
    0x37, 0x01, 0x07, 0x27, 0x09, 0x02, 0x00, 0x1F, 0x04, 0x0A, 0x28, 0x09,
    0x00, 0x38, 0x01, 0x07, 0x28, 0x09, 0x02, 0x00, 0x1F, 0x04, 0x0A, 0x29,
    0x09, 0x00, 0x39, 0x01, 0x07, 0x29, 0x09, 0x02, 0x00, 0x1F, 0x04, 0x1F,
    0x04, 0x07, 0x97, 0x08, 0x06, 0x5C, 0x08, 0x06, 0x34, 0x08, 0x23, 0x51,
    0x07, 0x23, 0x52, 0x0B, 0x23, 0x53, 0x29, 0x0E, 0x04
]

# Définition des opcodes avec leur nombre de paramètres
OPCODE = {
    0x00: ("PLAY CONV", 1),
    0x01: ("PLAY SCENE", 1),
    0x02: ("PLAY MIDGAME", 1),
    0x03: ("FLY MISSION AND GOTO NEXT MISSION", 1),
    0x06: ("SET TRUE", 1),
    0x07: ("SET FALSE", 1),
    0x08: ("PLAY SHOT", 1),
    0x09: ("IF NOT", 1),
    0x0A: ("IF", 1),
    0x0C: ("PLAY FLY MISSION", 1),
    0x0D: ("SET NEXT MISSION", 1),
    0x0F: ("GOTO NEXT MISSION", 1),
    0x10: ("SAVE GAME", 1),
    0x11: ("LOAD GAME", 1),
    0x12: ("PLAY SHOW MAP", 1),
    0x13: ("LOOK AT KILLBOARD", 1),
    0x14: ("IF MISSION ACCEPTED", 1),
    0x15: ("IF MISSION REFUSED", 1),
    0x17: ("VIEW CATALOG", 1),
    0x19: ("LOOK AT LEDGER", 1),
    0x1E: ("ELSE", 1),
    0x1F: ("ENDIF", 1),  # Plus de paramètre, le 0x04 est un terminateur
    0x20: ("IF MISSION SUCCESSFULL", 1),
    0x22: ("APPLY CHANGES", 1),
    0x23: ("VIEW CATALOG", 1),
}

TERMINATOR = 0x04

def analyze_control_bytes_around_sequence(effect_bytes):
    """Analyse les octets de contrôle autour de la séquence obligatoire"""
    print("\n=== ANALYSE DES OCTETS DE CONTRÔLE ===\n")
    print("Règle stricte : [OPCODE] [VALEUR] uniquement\n")
    
    # Trouver la séquence obligatoire
    target_pos = 0xA9
    
    print(f"Séquence obligatoire à 0x{target_pos:04X} :")
    print(f"  0x{target_pos:04X}: 0x0D (SET NEXT MISSION)")
    print(f"  0x{target_pos+1:04X}: 0x01 (valeur)")
    print(f"  0x{target_pos+2:04X}: 0x0C (PLAY FLY MISSION)")
    print(f"  0x{target_pos+3:04X}: 0x00 (valeur)")
    
    print(f"\n=== ANALYSE DES 15 OCTETS PRÉCÉDENTS ===\n")
    
    start = target_pos - 15
    for i in range(start, target_pos):
        byte = effect_bytes[i]
        
        # Classification
        classification = []
        if byte in OPCODE:
            opcode_name, params = OPCODE[byte]
            classification.append(f"OPCODE: {opcode_name}")
        if byte == 0x04:
            classification.append("TERMINATEUR")
        if byte == 0x08 and i > 0 and effect_bytes[i-1] in [0x59, 0x6E, 0x70, 0x6F, 0x63, 0x62, 0x61, 0x67, 0x5F, 0x5E, 0x5D, 0x66, 0x71, 0xB8, 0x97, 0x5C, 0x34]:
            classification.append("MARQUEUR APRÈS ADRESSE ?")
        
        # Vérifier si c'est la valeur d'un opcode précédent
        if i > 0 and effect_bytes[i-1] in OPCODE:
            prev_opcode = effect_bytes[i-1]
            _, param_count = OPCODE[prev_opcode]
            if param_count > 0:
                classification.append("VALEUR DE L'OPCODE PRÉCÉDENT")
        
        class_str = " | ".join(classification) if classification else "CONTRÔLE ?"
        print(f"  [0x{i:04X}] 0x{byte:02X}  {class_str}")
    
    print(f"\n{'='*60}")
    print(f"  [0x{target_pos:04X}] 0x0D (SET NEXT MISSION)")
    print(f"  [0x{target_pos+1:04X}] 0x01 (valeur)")
    print(f"{'='*60}\n")

def find_control_byte_patterns(effect_bytes):
    """Trouve les patterns d'octets de contrôle"""
    print("\n=== RECHERCHE DE PATTERNS DE CONTRÔLE ===\n")
    
    # Hypothèse : les octets de contrôle apparaissent avant les OPCODE
    # Format possible : [0x??] [0x08] [OPCODE] [VALEUR]
    
    patterns = {}
    
    i = 0
    while i < len(effect_bytes) - 3:
        byte1 = effect_bytes[i]
        byte2 = effect_bytes[i + 1]
        byte3 = effect_bytes[i + 2]
        
        # Pattern : [NON-OPCODE] 0x08 [OPCODE]
        if byte2 == 0x08 and byte3 in OPCODE and byte1 not in OPCODE and byte1 != 0x04:
            pattern = (byte1, byte2)
            if pattern not in patterns:
                patterns[pattern] = []
            patterns[pattern].append((i, byte3))
        
        i += 1
    
    print("Patterns trouvés : [BYTE] 0x08 [OPCODE]\n")
    for (byte1, byte2), occurrences in sorted(patterns.items(), key=lambda x: len(x[1]), reverse=True):
        if len(occurrences) >= 2:
            print(f"  Pattern [0x{byte1:02X}] [0x08] avant OPCODE : {len(occurrences)} fois")
            for pos, opcode in occurrences[:3]:
                opcode_name = OPCODE[opcode][0]
                print(f"      [0x{pos:04X}] 0x{byte1:02X} 0x08 → 0x{opcode:02X} ({opcode_name})")

def test_hypothesis_control_format(effect_bytes):
    """Teste l'hypothèse : [ADDR] 0x08 [OPCODE] [VALEUR]"""
    print("\n=== TEST HYPOTHÈSE : [ADDR] 0x08 [OPCODE] [VALEUR] ===\n")
    
    target_pos = 0xA9
    
    print("Hypothèse : 0x08 est un marqueur qui précède certains opcodes")
    print("            et indique qu'une adresse le précède\n")
    
    # Analyser la zone 0xA0-0xAD
    print("Zone autour de la séquence obligatoire :\n")
    
    i = 0xA0
    while i < 0xAD:
        byte = effect_bytes[i]
        
        # Si on trouve 0x08 suivi d'un opcode
        if byte == 0x08 and i + 1 < len(effect_bytes) and effect_bytes[i + 1] in OPCODE:
            addr = effect_bytes[i - 1] if i > 0 else None
            opcode = effect_bytes[i + 1]
            opcode_name, param_count = OPCODE[opcode]
            value = effect_bytes[i + 2] if i + 2 < len(effect_bytes) and param_count > 0 else None
            
            print(f"[0x{i-1:04X}] 0x{addr:02X} ← ADRESSE")
            print(f"[0x{i:04X}] 0x08 ← MARQUEUR")
            print(f"[0x{i+1:04X}] 0x{opcode:02X} ← {opcode_name}")
            if value is not None:
                print(f"[0x{i+2:04X}] 0x{value:02X} ← VALEUR\n")
            i += 3 if value is not None else 2
        elif byte == 0x04:
            print(f"[0x{i:04X}] 0x04 ← TERMINATEUR\n")
            i += 1
        elif byte in OPCODE:
            opcode_name, param_count = OPCODE[byte]
            print(f"[0x{i:04X}] 0x{byte:02X} ← {opcode_name}")
            if param_count > 0 and i + 1 < len(effect_bytes):
                value = effect_bytes[i + 1]
                print(f"[0x{i+1:04X}] 0x{value:02X} ← VALEUR\n")
                i += 2
            else:
                print()
                i += 1
        else:
            print(f"[0x{i:04X}] 0x{byte:02X} ← ???")
            i += 1
OPCODES_WITH_ADDRESS_PREFIX = {0x0A, 0x09, 0x06, 0x07}  # IF, IF NOT, SET TRUE, SET FALSE

def parse_with_address_prefix(effect_bytes):
    """Parser final gérant le préfixe [ADDR] 0x08 pour certains opcodes"""
    i = 0
    parsed_instructions = []
    
    while i < len(effect_bytes):
        byte = effect_bytes[i]
        
        # Cas 1 : Terminateur
        if byte == TERMINATOR:
            parsed_instructions.append(f"[0x{i:04X}] TERMINATEUR")
            i += 1
            continue
        
        # Cas 2 : Pattern [ADDR] 0x08 [OPCODE] [VALEUR]
        if i + 3 < len(effect_bytes) and effect_bytes[i + 1] == 0x08:
            next_opcode = effect_bytes[i + 2]
            if next_opcode in OPCODES_WITH_ADDRESS_PREFIX:
                addr = byte
                opcode = next_opcode
                value = effect_bytes[i + 3]
                opcode_name = OPCODE[opcode][0]
                
                parsed_instructions.append(
                    f"[0x{i:04X}] {opcode_name} (@0x{addr:02X}, 0x{value:02X})"
                )
                i += 4
                continue
        
        # Cas 3 : Opcode standard [OPCODE] [VALEUR]
        if byte in OPCODE:
            opcode_name, param_count = OPCODE[byte]
            
            if param_count > 0 and i + 1 < len(effect_bytes):
                value = effect_bytes[i + 1]
                parsed_instructions.append(
                    f"[0x{i:04X}] {opcode_name} (0x{value:02X})"
                )
                i += 2
            else:
                parsed_instructions.append(
                    f"[0x{i:04X}] {opcode_name}"
                )
                i += 1
            continue
        
        # Cas 4 : Octet inconnu
        parsed_instructions.append(f"[0x{i:04X}] [INCONNU 0x{byte:02X}]")
        i += 1
    
    return parsed_instructions

def verify_mandatory_sequence_final(effect_bytes):
    """Vérification finale de la séquence obligatoire"""
    parsed = parse_with_address_prefix(effect_bytes)
    
    print("\n=== VÉRIFICATION FINALE ===\n")
    
    # Trouver la séquence
    found = False
    for i, inst in enumerate(parsed):
        if "SET NEXT MISSION" in inst and "0x01" in inst:
            print(f"✅ Séquence obligatoire trouvée :")
            print(f"   {inst}")
            if i + 1 < len(parsed):
                print(f"   {parsed[i + 1]}")
            
            # Afficher le contexte (5 instructions avant)
            print(f"\n📋 Contexte (5 instructions avant) :")
            for j in range(max(0, i - 5), i):
                print(f"   {parsed[j]}")
            
            found = True
            break
    
    if not found:
        print("❌ Séquence obligatoire non trouvée")
    
    # Vérifier qu'il n'y a pas de PLAY CONV
    conv_count = sum(1 for inst in parsed if "PLAY CONV" in inst and "INCONNU" not in inst)
    print(f"\n📊 Nombre de PLAY CONV détectés : {conv_count}")
    
    if conv_count == 0:
        print("✅ Aucun PLAY CONV (cohérent avec l'intro)")
    else:
        print(f"⚠️  {conv_count} PLAY CONV détectés (à investiguer)")
        for inst in parsed:
            if "PLAY CONV" in inst and "INCONNU" not in inst:
                print(f"   {inst}")

def analyze_0x1F_usage(effect_bytes):
    """Analyse comment 0x1F (ENDIF) est utilisé"""
    print("\n=== ANALYSE DE 0x1F (ENDIF) ===\n")
    
    # Trouver toutes les occurrences de 0x1F
    endif_positions = [i for i, b in enumerate(effect_bytes) if b == 0x1F]
    
    print(f"Trouvé {len(endif_positions)} occurrences de ENDIF (0x1F)\n")
    print("Règle : ENDIF prend 1 paramètre (ignoré) pour uniformité\n")
    
    for pos in endif_positions:
        # Le paramètre devrait toujours être 0x04 (terminateur)
        param = effect_bytes[pos + 1] if pos + 1 < len(effect_bytes) else None
        
        print(f"[0x{pos:04X}] 0x1F (ENDIF)")
        if param is not None:
            if param == 0x04:
                print(f"         → paramètre = 0x04 (TERMINATEUR) ✓")
            else:
                print(f"         → paramètre = 0x{param:02X} (BIZARRE !)")
        print()

def reparse_strict_format(effect_bytes):
    """Re-parse avec format strict : TOUS les opcodes ont 1 paramètre"""
    print("\n=== PARSING STRICT : [OPCODE] [PARAM] pour TOUS ===\n")
    
    target_pos = 0xA9
    start = 0x95
    end = 0xB0
    
    print("Zone autour de la séquence obligatoire :\n")
    
    i = start
    while i < end and i < len(effect_bytes):
        byte = effect_bytes[i]
        
        if byte in OPCODE:
            opcode_name, _ = OPCODE[byte]
            param = effect_bytes[i + 1] if i + 1 < len(effect_bytes) else None
            
            if param is not None:
                # Marquer si c'est la séquence obligatoire
                marker = "  <<<" if i == target_pos else ""
                print(f"[0x{i:04X}] {opcode_name} (0x{param:02X}){marker}")
                i += 2
            else:
                print(f"[0x{i:04X}] {opcode_name} [FIN DE DONNÉES]")
                break
        else:
            print(f"[0x{i:04X}] [INCONNU 0x{byte:02X}]")
            i += 1

def analyze_0x08_pattern_deeper(effect_bytes):
    """Analyse approfondie du pattern 0x08"""
    print("\n=== ANALYSE APPROFONDIE DU PATTERN 0x08 ===\n")
    
    # Trouver tous les 0x08
    positions_08 = [i for i, b in enumerate(effect_bytes) if b == 0x08]
    
    print(f"Trouvé {len(positions_08)} occurrences de 0x08\n")
    
    # Classifier les contextes
    contexts = {
        "après_opcode": [],
        "avant_opcode": [],
        "isolé": []
    }
    
    for pos in positions_08:
        before = effect_bytes[pos - 1] if pos > 0 else None
        after = effect_bytes[pos + 1] if pos + 1 < len(effect_bytes) else None
        
        if before in OPCODE:
            contexts["après_opcode"].append((pos, before, after))
        elif after in OPCODE:
            contexts["avant_opcode"].append((pos, before, after))
        else:
            contexts["isolé"].append((pos, before, after))
    
    print(f"Contextes de 0x08 :\n")
    print(f"  - Après un OPCODE : {len(contexts['après_opcode'])} fois")
    print(f"  - Avant un OPCODE : {len(contexts['avant_opcode'])} fois")
    print(f"  - Isolé : {len(contexts['isolé'])} fois\n")
    
    # Analyser les cas "après opcode"
    if contexts["après_opcode"]:
        print("Cas : 0x08 APRÈS un OPCODE (donc = paramètre) :\n")
        for pos, opcode, after in contexts["après_opcode"][:5]:
            opcode_name = OPCODE[opcode][0]
            after_str = f"0x{after:02X}" if after is not None else "FIN"
            print(f"  [0x{pos-1:04X}] {opcode_name} → [0x{pos:04X}] 0x08 (param) → {after_str}")
    
    print("\n💡 HYPOTHÈSE : 0x08 est un PARAMÈTRE valide pour certains opcodes")
    print("   (pas un marqueur de contrôle)")

if __name__ == "__main__":
    analyze_0x1F_usage(MISN_EFFECT)
    reparse_strict_format(MISN_EFFECT)
    analyze_0x08_pattern_deeper(MISN_EFFECT)
# STRIKE.EXE — Chargement des données & modèle de données

Ce document décrit **comment le jeu lit ses fichiers de données** (la pile de
chargement, de l'accès fichier DOS brut jusqu'aux chargeurs d'assets typés) et
**le modèle de données** qui en résulte (format conteneur IFF/RealSpace, structures
produites, graphe d'objets à l'exécution).

Source : lecture des segments seg184-198 (pile I/O), seg082-084/seg104 (modèles),
seg455-457/seg450/seg454 (missions/textes), ovr228/ovr241-242/ovr265-269/ovr288-289
(dégâts, polices, config, ressources, textures), seg066/seg442 (palette),
seg073/ovr290-299 (terrain), seg079/seg432-453 (débris, composants joueur).

Statut : les **couches 0-3 (I/O + navigation IFF) sont lues en détail** ; la
**couche 4 (chargeurs d'assets) est cartographiée par ses tags de chunk et ses
appels**, plusieurs gros chargeurs restant `⚠️ NON DÉTAILLÉE` ligne à ligne
(voir § « Zones non lues » en fin de document).

> **Convention d'axes (cf. `CLAUDE.md`)** : les vecteurs position/vitesse sont
> `3× i32` en virgule fixe 24.8 (`val / 256` = pieds). La **3ᵉ composante (le
> dernier i32, offset le plus élevé) est l'altitude**. Côté ASM/jeu = **Z-up**,
> composantes `(X, Y, Z)` avec **Z = altitude** ; libRealSpace = **Y-up**,
> `(X, Y, Z)` avec **Y = altitude** → le portage **échange les composantes 1 et 2**.
> Les tables ci-dessous nomment donc la 3ᵉ composante `Z` (= altitude).

---

## 1. Architecture générale : 5 couches

```
┌─ Couche 4 : CHARGEURS D'ASSETS (par type)
│    IFF_LoadModelMain, JDYN_LoadChunk, MissionScenario_*, MissionLoader_*,
│    MissionRecord_*, TextureLoader_*, Font_*, Config_*, SaveGame_*, Terrain_*…
│
├─ Couche 3 : FORMAT IFF  (seg193-197)
│    ResourceRecord  = navigateur FORM/CAT/CHUNK + PACK indexé
│    IndexedRecordReader + LZW_Decompress = packs compressés
│
├─ Couche 2 : StreamReader  (seg190-192, 195)
│    lecteur tamponné + lecture/écriture TYPÉE par handle (u8/u16/u32/bytes/cstring)
│
├─ Couche 1 : FileStream  (seg189, 192)
│    classe C++ « fichier » : Open/Read/Write/Seek/Close/GetSize (vtable)
│
└─ Couche 0 : E/S DOS brute  (seg184-187)
     INT 21h 3Ch/3Dh/3Eh/3Fh/40h/42h + délégation « gros fichier » → mémoire paginée
```

Toutes les ressources passent par ce tuyau. En amont de presque tous les chargeurs :
**`Path_ResolveDataFile`** (`sub_2B06C`, seg057) — si le nom contient déjà un `\`,
il est pris tel quel ; sinon il devient `..\..\DATA\<sous-dossier>\<nom>.<ext>`.
Sous-dossiers observés : `OBJECTS`, `MISSIONS`, `COCKPITS`, plus les `.shp` d'écrans
d'options. Chaînes de chemin confirmées (seg339) : `..\..\data\cockpits\`,
`..\..\DATA\`, `..\..\data\airdens.tbl`, fichier de config `sc.cfg`.

---

## 2. Couche 0 — E/S fichier DOS

| Fonction | Adr. | Rôle |
|---|---|---|
| `File_CreateWithRetry_630BA` | seg184 | `INT 21h/3Ch` CREAT, boucle de retry via `off_70FCE` |
| `File_OpenWithRetry_63434` | seg186 | `INT 21h/3Dh` OPEN (mode al = 0 lecture / 1 écriture / 2 r+w) |
| `File_CloseWithRetry_63463` | seg186 | `INT 21h/3Eh` |
| `File_SeekWithRetry_63482` | seg187 | `INT 21h/42h` LSEEK |
| `File_ReadOrDelegateToPaged_630E7` | seg185 | `INT 21h/3Fh` par blocs de `0xFFF0` ; **si l'offset dépasse `word_709D6`, délègue à `File_ReadViaPagedMemory_63294`** |
| `File_WriteOrDelegateToPaged_631E3` | seg185 | idem en écriture (`INT 21h/40h`) → `File_WriteViaPagedMemory_63362` |
| `File_ReadViaPagedMemory_63294` | seg185 | lit dans de la mémoire **paginée/EMS** (`int 15h/1209h`, sig `"sm"/"SM"`, `PagedMemory_ResolveAddress_5BBCC`) — permet de charger des ressources > 640 Ko |

Globales : `dword_70FB6` (octets transférés), `word_70FBA` (code erreur),
`dword_709D8` / `word_709D6` (bornes conventionnel ↔ paginé).

---

## 3. Couche 1 — `FileStream` (classe C++, seg189/192)

Objet de **0x1C octets**, alloué par `FileStream_Construct_635AE`.

| Offset | Type | Champ |
|---|---|---|
| `+0x00` | vtable | tags `3F42h` puis `3F22h` |
| `+0x02` | word | handle DOS |
| `+0x04` | dword | pointeur buffer de lecture |
| `+0x08` | dword | taille buffer |
| `+0x0C` | dword | base buffer |
| `+0x10` | dword | position du buffer dans le fichier |
| `+0x14` | dword | octets valides dans le buffer |
| `+0x18` | dword | **position courante** |

Méthodes vtable (seg192) : `Open` (`sub_64413`), `Read` (`sub_64353` → couche 0),
`Write` (`loc_64393`), `Seek` (`loc_643D7`), `Close` (`loc_64472`),
`GetSize` (`loc_64497`, 3× LSEEK : pos courante / fin / retour).
Chaînes de debug confirmées (seg339) : `Filename=%s`, `Length=%ld`,
`Position=%ld`, `Mode=%s` (CREATE / READ / APPEND).

---

## 4. Couche 2 — `StreamReader` (seg190-191, 195)

Lecteur tamponné générique **par-dessus n'importe quel objet flux** (FileStream,
bloc mémoire, sous-flux IFF). Cœur de toute lecture typée du jeu —
`StreamReader_ReadTyped_63FA1` est référencée dans des dizaines de segments
(`Handle_ReadWordField_63511`, `Handle_ReadByteField_63534`, seg188, IA, texte…).

### 4.1 Disposition mémoire (dérivée du code, `si` = base)

| Offset | Type | Champ | Sens |
|---|---|---|---|
| `+0x00` | vtable | tags `3FB3h`/`39E3h`/`3F42h` | classe |
| `+0x0A` | buffer inline | destination des lectures/parses formatés | |
| `+0x4C` | byte | mode | `1` = mode texte/ligne (déclenche `ParseFormattedValue`) |
| `+0x4D` | dword | **curseur logique** | position de lecture courante |
| `+0x51` | dword | **base physique** | offset ajouté pour convertir logique → sous-flux |
| `+0x55` | dword | **longueur valide / capacité** | test EOF : `+0x4D >= +0x55` |
| `+0x59` | word | pointeur flux sous-jacent | vtable : `+4` Read, `+0xC` Seek/Tell, `+0x18` GetSize |
| `+0x5B` | word | flag « tout en mémoire » | si ≠ 0, pas de refill depuis un flux |
| `+0x61` | word | capacité allouée du buffer | `Method_GrowBuffer_6552E` la double en boucle (tag `5C44h`) |

### 4.2 Fonctions clés

| Fonction | Rôle |
|---|---|
| `StreamReader_ReadTyped_63FA1` ⭐ | `read(self, dst, len, pos=-1)` — `pos = -1` → curseur courant ; convertit logique → physique, appelle `Read` du sous-flux, avance `+0x4D` ; refill/EOF via `RefillBuffer_63EE5` / `CheckBufferValid_63DDB` ; erreur → `ReportError_6404E` (tag `1911h`) |
| `StreamReader_WriteTyped_6568F` ⭐ | pendant en écriture ; débordement → `WriteWithGrowth_65795` (agrandissement auto du tampon) |
| `ConstructVariantA/B/C_63A39/63A88/63AD3` | 3 constructeurs (bindé fichier / mémoire / dispatch) |
| `ConstructAndBind_63B23` | construit et lie un sous-flux à un intervalle `[base, base+len]` d'un flux parent (utilisé pour lire un chunk IFF comme un flux autonome) |
| `ParseFormattedString_64217`, `ParseFormattedValue_6410E` | quasi-`sscanf` typé sur le flux (mode `+0x4C == 1`) |
| `RegistryAdd/Remove_640B0/640D4` (seg191) | **table globale de 8 flux ouverts** (`word_7107C` = compteur) |
| `ReleaseBuffer_65643` | libère buffer + ferme sous-flux + free objet |

---

## 5. Couche 3 — Format **IFF** : `ResourceRecord` (seg193-194)

Navigateur **IFF complet** (FORM / CAT / CHUNK / PACK). Il **étend** `StreamReader`
(mêmes champs `0x4C-0x5B`) et ajoute une machine à pile de conteneurs à partir de
`+0x65`. Confirmé littéralement par les chaînes seg339 : `Form=%4s`, `Chunk=%4s`,
`NumPacks=%ld`, `CurPack=%ld`, `PackSize=%ld`, `PackBase=%ld`, `PackLoc=%ld`,
extension `.iff`.

### 5.1 Disposition mémoire (extension au-dessus de StreamReader)

| Offset | Type | Champ |
|---|---|---|
| `+0x65` | dword | **curseur de lecture structurée** (avancé de 1/2/4 après chaque champ) |
| `+0x69` | dword | position sauvegardée (début du chunk courant) |
| `+0x6D` | 4 o | **FourCC courant** (tag lu) |
| `+0x71` | byte | flag |
| `+0x72` | dword | **taille du chunk courant** — lue **BIG-ENDIAN** (`xchg ah,al` + swap des 2 mots) |
| `+0x76` | dword | fin du chunk courant = `+0x65 + taille + (taille & 1)` → **padding pair IFF** |
| `+0x7A` | word | profondeur d'imbrication (nb de frames conteneurs ouverts) |
| `+0x7C` | word | pointeur **frame conteneur courant** (liste chaînée de structs de 0x13 o) |
| `+0x7E`/`+0x82` | dword | stride / compte d'un tableau à pas fixe |
| `+0x86`/`+0x88` | word/dword | signet du 1er form-type |
| `+0x8C`/`+0x90` | dword | base + nombre d'éléments d'un tableau de références |
| `+0x94`/`+0x96` | word | dims 2D (w × h) pour tableaux indexés |
| `+0x98` | dword | base d'un tableau à pas fixe |
| `+0x9C`/`+0xA0`/`+0xA4` | dword | offset / taille / fin de l'élément de PACK courant |

**Frame conteneur (0x13 octets, alloué par `sub_658(0x13)`)** :
`+0x00` lien parent (ancien `+0x7C`) · `+0x02` offset du corps (utilisé avec `+0xC`
= saut de l'en-tête FORM : 4 tag + 4 taille + 4 form-type) · `+0x06` FourCC du
form-type (4 o, copié via `sub_2B6D`) · `+0x0F` **offset de fin du conteneur**
(borne de tous les scans du niveau).

### 5.2 Primitives de navigation

| Fonction | Rôle | pseudo-API |
|---|---|---|
| `ResourceRecord_ReadDwordAdvance_64B9B` | lit le FourCC (4 o → `+0x6D`), avance de 4 | `read_tag()` |
| `ResourceRecord_ReadHeaderFields_64698` | lit la taille BE (`+0x72`), avance de 4 | `read_size_be()` |
| `ResourceRecord_ComputeAlignedBounds_64BC1` | header complet + calcule `+0x76` avec padding | `read_chunk_header()` |
| `ResourceRecord_ComputeRecordBounds_646DB` | header + exige tag `FORM` (`0x4D524F46`) + lit le form-type | `expect_form()` |
| `ResourceRecord_ComputeRecordBoundsB_6470F` | idem pour `CAT ` (`0x20544143`) | `expect_cat()` |
| `ResourceRecord_SeekToStart_64BF8` | recale `+0x65` au début du corps du conteneur (ou 0) | `rewind()` |
| `ResourceRecord_SeekAndRead_64743` ⚠️ | **scan forward des chunks du niveau courant jusqu'au tag demandé** ; skip = `+0x65 = +0x76` ; stop à `frame[+0xF]` | `seek_chunk(tag)` |
| `ResourceRecord_SeekAndReadB_647B2` ⚠️ | idem mais sur des **FORM** (wrapper), et **push un frame** si trouvé | `enter_form(type)` |
| `ResourceRecord_SeekAndReadC_6482B` | idem pour `CAT ` | `enter_cat(type)` |
| `ResourceRecord_AllocateAndDecode_64C21` / `…B_64CA3` | **push frame conteneur** (alloc 0x13, chaîne, copie form-type + fin, `+0x7A++`) | `push_frame()` |
| `ResourceRecord_ReleaseAndClear_64D25` | **pop frame** (`+0x65 = frame[+0xF]`, unlink, free, `+0x7A--`) | `pop_frame()` |
| `ResourceRecord_Helper_645A5` | pop **tous** les frames, `+0x65 = 0` | `rewind_all()` |
| `ResourceRecord_InitAndValidate_645CD` | ouvre + valide le 1er FORM d'un fichier | `open_root()` |
| `ResourceRecord_AdvanceByArrayCount_649C1` | avance de `count × 4` — saut d'un tableau de références | |

### 5.3 Lecture de champs (au-dessus de `StreamReader_ReadTyped`)

| Fonction | Lit |
|---|---|
| `ResourceRecord_ReadFinalField_64B51` | **u8** (avance +1) |
| `ResourceRecord_ReadFieldGroupB_64A54` | **u16** LE (avance +2) |
| `ResourceRecord_ReadFieldGroupC_64A7E` | **u32** LE (avance +4) |
| `ResourceRecord_ReadDwordField_64AAE` | u32 **sans avancer** (peek) |
| `ResourceRecord_ReadFieldGroupA_64A19` | **N octets** dans un buffer (avance +N) |
| `ResourceRecord_ReadViaHelper_64AD9` | **chaîne C** (boucle u8 jusqu'à 0), retourne la longueur |
| `ResourceRecord_ReadPayload_64DB3` | lit le **reste du chunk** dans un buffer, puis `+0x65 = +0x76` |
| `ResourceRecord_ReadStructured_648E3` | **accès indexé à un PACK** : `+0x65 = base + idx*4` → lit un offset dword → seek → lit la taille → renseigne `+0x9C/+0xA0/+0xA4` |

**Endianness** : les **tailles de chunk sont big-endian** (IFF standard) ; les
**champs internes sont little-endian** (`ReadFieldGroupB/C` sans swap).

Contrepartie en écriture (seg194) : `WriteField_64E1D`, `WriteFieldGroupA/B/C/D`,
`ReadAndWriteBridge` — le format est **relisible/réinscriptible** (utilisé par
`sc.cfg`, l'éditeur de scénario `MissionScenario_ResolveAndBindExpressions_A8F22`,
`TextRenderer_SaveResourceFile`).

### 5.4 Packs compressés — `IndexedRecordReader` (seg196) + `LZW` (seg197)

- `IndexedRecordReader` : classe distincte (champs `+0x5D` index courant,
  `+0x5F` total, `+0x61`/`+0x65` position, `+0x75` cache) qui itère une **table
  d'index** d'enregistrements de taille variable.
- `IndexedRecordReader_ReadIndexTable_65B73` charge la table et **appelle
  `LZW_Decompress_66068`** → certains packs sont **LZW** : code 9 → 12 bits
  (`word_710F6 = 9`), table 512 entrées (`word_710F8 = 0x200`), 1er code libre 258
  (`word_710FC = 0x102` = 256 symboles + clear `0x100` + EOD `0x101`) — paramétrage
  type GIF. `LZW_ReadCode_65FCE` = extraction bit-à-bit (`word_7110D/0F/71111`).
- `IndexedRecordReader_Method_ComputeCount_65B26` : nb d'entrées =
  `taille_table / 4 − 1`.

Consommateurs : `MissionRecord_LoadEntityDatabase`, `TextCycler`, `TerrainSector`,
`MissionText`, `AudioQueue`, `IndexedRecordReader_ScanAndReadTyped_99120`.

---

## 6. Couche 4 — Chargeurs d'assets par type

### 6.1 Modèles 3D — `IFF_LoadModelMain` (`sub_3B876`, seg083, 1699 L)

**Fabrique polymorphe d'objets du monde.** Fichier `OBJECTS\<nom>.IFF` :

1. Détecte VESA (`INT 15h`), résout le chemin, ouvre le flux.
2. Vérifie le **magic `REAL`** (premier form-type), `push_frame`.
3. `enter_form('OBJT')`.
4. **Probe séquentiel** d'un sous-FORM parmi ~25 types → sélectionne la sous-classe
   C++, alloue la struct, pose le tag vtable (`1B6Fh` → patché `2578h`/`252Ch`/…),
   init les champs communs (`+0x2B` / `+0x2F` dword = 0, `+0x33` byte = 0), puis
   délègue à un chargeur d'overlay spécifique (stub VROOMM).

| FourCC | Taille struct | Type d'objet |
|---|---|---|
| `BOBJ` | 0x35 | basic object |
| `ORNT` | 0x36 | objet orienté (+ chunk `ANGL` via `IFF_LoadAngleParam` → angle `+0x35`) |
| `TRCR` | 0x82 | tracer / traçante |
| `AFTB` | 0x36 | afterburner (tuyère) |
| `MOBL` | 0x37 | objet mobile |
| `OMOB` | 0x46 | objet mobile orienté |
| `GUID` | 0x4A | missile guidé |
| `JETP` | 0x6A | jet / avion |
| `XMIT` | 0x36 | émetteur |
| `WEAP` | 0x5E | arme |
| `MISS` | 0x65 | missile |
| `PODR` | 0x6A | pod (nacelle) |
| `DURD` | 0x67 | Durandal (anti-piste) |
| `DECY` | 0x3B | leurre (decoy) |
| `SWPN` | 0x51 | arme secondaire |
| `GRND` | 0x46 | objet au sol |
| `RNWY` | 0x36 | piste |
| `EXPL` | 0x3E | explosion |
| `DEBR` | 0x3B | débris |
| + `BOMB`, `ARMG`, … | | (branches supplémentaires non détaillées) |

Chunk transverse **`INFO`** (`IFF_LoadInfoChunk`, seg104) : `+0x37` (w) =
**rayon de collision** (défaut `0x14` = 20), `+0x39` (b) flag, `+0x3A` (b) flag2.

Cache & instanciation : `Debris_FindModelByID_38A79` / `Debris_LoadAndInstantiate_38B70`
(seg079) — chargement paresseux d'un modèle par identifiant, réutilisé pour tout
objet. Pièces d'objet / hardpoints confirmés (seg339) : `FUSELAGE`, `LWING`,
`RWING`, `LOWEJECT`, `CHAFF`, `FLARE`, `HARDPT#`, `POD`, `GUN`, `BIGSPHER`.

### 6.2 Modèle de vol / dynamique — `JDYN_LoadChunk` (`sub_3A49C`, seg082, 577 L)

**Relecture détaillée (session dédiée).** `JDYN_LoadChunk` est d'abord un
**constructeur C++**, pas un chargeur de données : le chargement des champs est
délégué à un overlay du cluster `PlayerComponent` (seg447-449).

Séquence :

1. `enter_form('DYNM')` (`ResourceRecord_SeekAndReadB_647B2`).
2. `seek_chunk` d'**un seul** des sous-chunks (une dynamique = un type par
   véhicule/arme) :

   | FourCC (disque) | Taille struct | Constructeur de branche | Modèle |
   |---|---|---|---|
   | `JDYN` | **0xC5 (197 o)** | `sub_6CACA`/`6CB0A`/`6CB73` + `sub_6CBCE` | **modèle de vol avion complet** |
   | `MISS` | 0x3C | `sub_6C553` | dynamique de missile |
   | `GBMB` | 0x49 | `sub_6C4A2` | balistique bombe / canon |
   | `DURD` | 0x47 | `sub_6C4D9` | dynamique Durandal |
   | `AGRV` | 0x3B | `sub_6CB2D` | gravité / aéro simplifiée |
   | `SURF` | 0x29 | `sub_6C625` | surfaces de contrôle |
   | `PARA` | 0x5B | `sub_6CD04` | paramètres (parachute ?) |

3. Pour la branche `JDYN` : `malloc(0xC5)` puis mise en place de **pointeurs
   intérieurs** vers deux **sous-objets embarqués** :
   - `[si+0x00]`, `[si+0x1E]`, `[si+0x24]` → `si+0x8E` (sous-objet **A**, ~0x23 o,
     vtable @ `+0x8E`)
   - `[si+0x10]`, `[si+0x22]` → `si+0xB1` (sous-objet **B**, ~0x14 o, 1er mot @
     `+0xB1` = back-ptr vers A, tag `+0xB3` = `21CEh`, 3 dwords `+0xB5/+0xB9/+0xBD`
     = 0)
   - `[si+0x8C]` (mot) = delta d'ajusteur d'héritage multiple, remis à 0 après
     construction.
4. Construction en cascade des sous-objets A puis B via les stubs VROOMM
   `sub_6CACA` → `sub_A4A88`, `sub_6CB0A` → `sub_A4B10`, `sub_6CB73` →
   `sub_A52FD` (constructeurs de variantes `PlayerComponent`, seg447-448). Les
   `mov word ptr [si+X], NNNNh` (223Ah, 21CEh, 20AEh, 212Eh, 228Ah, 2312h…) ne
   sont **pas des patchs de coefficients** : ce sont les **tags de vtable et
   pointeurs de membre** réécrits à chaque étape de la construction multi-base
   (motif compilateur Borland C++ pour héritage multiple).
5. `sub_6CBCE(resourceRecord, si)` → `sub_A5CEA`
   (`PlayerComponent_LoadFieldsWrapper_A5CEA`, seg449) →
   `PlayerComponent_ConstructAndLoadMassive_A5EB4` →
   **`PlayerComponent_LoadFieldsMassive_A5D18`** : c'est **le vrai lecteur du
   chunk `JDYN`**.

**Découverte structurelle** : le cluster « PlayerComponent » (seg432-453, ~150
fonctions), documenté jusqu'ici comme « composants de dommages/état du joueur »,
**est le lecteur générique de paramètres de véhicule/dynamique** — partagé par le
modèle de vol (`JDYN`), les dynamiques d'armes (`MISS`/`GBMB`/`DURD`) et les
composants d'avion.

#### Hiérarchie IFF de la dynamique d'un avion (`JETP`) — **fait vérifié côté données**

Le FORM `JETP` contient un **FORM `DYNM` externe** qui regroupe **5 chunks
frères** (pas 3 — `ATMO` et `STBL` s'intercalent), lus en séquence par le
cluster seg447-449 ; codes d'erreur **consécutifs `A001`→`A005`** :

```
JETP  (FORM, struct 0x6A)
 └─ DYNM  (FORM externe)
     ├─ DYNM  (chunk)  → u32            = MASSE (kg, 24.8)                       [A001]  sub_A4A20
     ├─ ATMO  (chunk)  → u32 (+ plus)   = ATMOSPHÈRE (id modèle → dword_72A0A)   [A002]  sub_A4BE1
     ├─ THRS  (chunk)  → u32 + u8×3     = POUSSÉE (N, 24.8) + 3 params moteur    [A003]  sub_A5224
     ├─ STBL  (chunk)  → u32            = coeff de STABILITÉ / autorité pitch    [A004]  sub_A52B8
     │                                    → jdyn[0x12] (échelle q' de l'asservissement)
     └─ JDYN  (chunk)  → 28 champs (73o)= enveloppe de vol                       [A005]  sub_A5D18
```

*(FourCC disque = octets `ATMO` / `STBL` ; l'ASM les pousse « à l'envers »
`'OMTA'` / `4C425453h`.)*

| Chunk | Lecteur | Disque | → destination | Err. |
|---|---|---|---|---|
| `DYNM` (masse) | `sub_A4A20` (seg447) | `u32` | sous-obj masse `+0x04` (kg, 24.8) | `A001` |
| `ATMO` | `sub_A4BE1` (seg448) | `u32` (+ bloc si `dword_72A0A≠0`) | sous-obj `+0x10` ; **`dword_72A0A` = id atmosphère global** (posé par le 1ᵉʳ avion, partagé) | `A002` |
| `THRS` | `sub_A5224` (seg448) | `u32`, `u8`, `u8`, `u8` | **sous-objet embarqué à `jdyn+0x22`** : `+0x0A`→`jdyn[0x2C]`, `+0x0E`→`jdyn[0x30]`, `+0x0F`→`jdyn[0x31]`, `+0x10`→`jdyn[0x32]` | `A003` |
| `STBL` | `sub_A52B8` (seg449) | `u32` | **`jdyn[0x12]`** | `A004` |
| `JDYN` | `sub_A5D18` (seg449) | 73 o | `jdyn[0x33..+0x8B]` etc. | `A005` |

- **Sous-objet masse** (~0x14 o) : `sub_A4A88` met à zéro `+0x02` (flag),
  `+0x08/0C/10` (vecteur — CG / inertie diagonale, à confirmer) ; puis `+0x04` = masse.
- **THRS — les 3 `u8` identifiés** (via `Aero_ComputeCoeffSaturating`, voir « Poussée » plus bas) :
  `jdyn[0x30]` = **fraction de poussée MIL/sèche** (part de la poussée AF max) ;
  `jdyn[0x31]` = **fraction de poussée à la vitesse de référence** (lapse RAM) ;
  `jdyn[0x32]` = **échelle de la vitesse de coupure** (poussée → 0).
  `jdyn[0x2C]` = poussée AF max (N, 24.8).

#### Graphe d'objets à l'exécution (vtables)

Deux allocations distinctes reliées par pointeur, calquées sur l'IFF :

| Objet runtime | Taille | vptr (offset *near* dans seg339) | Construit par |
|---|---|--:|---|
| wrapper **`JETP`** | `0x6A` | `[+0]` = **`0x24C8`** | `IFF_LoadModelMain` (`sub_3B876`) via `sub_5C6F3(tag 0x5C44, 0x6A, …)` ; fixups `1B6F→2578→252C→24F0→24DC→24C8` |
| objet **`DYNM`/`JDYN`** | `0xC5` | `[+2]` = **`0x228A`** ; `[[+0]]` (sous-obj A) = `0x2312` ; `[[+0x10]]+2` = `0x2362` | `JDYN_LoadChunkAndConstruct_3A49C` (`enter_form('DYNM')`, `malloc 0xC5`) ; fixups terminant à `228A` (strike.asm ≈ 116908) |

- Base de segment `seg339` = linéaire **`0x6D070`** (vérifié : `0x228A` → vtable
  linéaire `0x6F2FA`).
- `PhysicsTicks` / `Aero_SumLinearForces` opèrent sur l'objet **0xC5**
  (`si = [playerCtx+0x0B]`), via `[si+2]` = `0x228A`.
- **Point ouvert** : le slot `+0x3C` de la vtable `0x228A` (diviseur de masse
  appelé par `Aero_SumLinearForces`) et le slot `+0x34` (tête de `PhysicsTicks`)
  pointent tous deux, en lecture statique, vers des **stubs no-op partagés**
  (`loc_4692C` / `loc_4691A`, 8 o, `mov ax,[bp+6] / retf`). Voir « Le diviseur de
  masse `m` » plus bas.

#### Layout du chunk `JDYN` (lu par `JDYN_ReadChunkFields_A5D18`, seg449)

`seek_chunk('JDYN')` puis **28 champs séquentiels**, little-endian, écrits à des
offsets non contigus de la struct 0xC5. **Payload disque = 73 octets (`0x49`)**.

| # | Off. disque | Taille | → struct | Défaut (`A5620`) |
|--:|:--:|:--:|:--:|:--:|
| 1 | `0x00` | u32 | `+0x69` (copié en `+0x6D` par `A5CEA`) | — |
| 2–7 | `0x04`–`0x1B` | u32×6 | `+0x33`, `+0x37`, `+0x3B`, `+0x3F`, `+0x43`, `+0x47` | — |
| 8 | `0x1C` | u32 | `+0x71` | — |
| 9–11 | `0x20`–`0x22` | u8×3 | `+0x4B`, `+0x4C`, `+0x4D` | — |
| 12–13 | `0x23`, `0x27` | u32×2 | `+0x4E`, `+0x52` | — |
| 14–16 | `0x2B`–`0x2D` | u8×3 | `+0x56`, `+0x57`, `+0x58` | — |
| 17–19 | `0x2E`, `0x32`, `0x36` | u32×3 | `+0x59`, `+0x5D`, `+0x61` | — |
| 20–22 | `0x3A`–`0x3C` | u8×3 | `+0x65`, `+0x66`, `+0x67` | — |
| 23–25 | `0x3D`, `0x3F`, `0x41` | u16×3 | `+0x80`, `+0x82`, `+0x84` | 500 / 100 / 231 |
| 26 | `0x43` | u32 | `+0x86` | 11005 (`0x2ADD`) |
| 27–28 | `0x47`, `0x48` | u8×2 | `+0x8A`, `+0x8B` | 3 / 2 |

Chunk absent → erreur `0xA005`. Avant lecture, `PlayerComponent_ResetVisualStateDefaults_A5620`
pose les défauts (colonne ci-dessus) + `[si+0x68]=0xFF`, `[si+0x7C]=0`, `flags`
`[si+0x75]` = bit4|bit7, et remet **7 gains globaux** (`dword_72A14/18/1C/20/24/28/2C`) à `0x100` (1.0 en 8.8).

#### Sémantique des champs (tracée seg103, avec sites de lecture)

| struct | rôle | preuve |
|---|---|---|
| `+0x69` | **capacité carburant** (24.8) — *fait vérifié côté données* | copié en `+0x6D`, jamais lu en calcul |
| `+0x6D` | **carburant courant** (24.8, copie runtime de `+0x69`) | `sub_49242` L2560 `sub [si+6Dh],eax` ; clamp `≥0` L2554/L5118 ; `≤0` → `[si+0x28]=0` (**flameout**) L5121 |
| `+0x33` | **coefficient de consommation carburant** | `sub_49242` L2543-2549 : `burn = (0xA00·throttle) × [+0x33] >> 8` puis `[+0x6D] -= burn` |
| `+0x37` | **traînée aérofreins sortis** | `Aero_ComputeDragFromFlags75` L146 : `if flags_75 bit0: drag += [+0x37]` |
| `+0x3B` | **traînée train sorti** | id. L157 : `if flags_75 bit2: drag += [+0x3B]` |
| `+0x3F` | coeff. de traînée **au sol** (roulage/réaction sol) | `Aero_ComputeDragWithFeedback` L724, gaté par `[A+0x20]` (flag « au sol ») |
| `+0x43` | id. + extra si aérofrein sorti | id. L736 (`+ flags_75 bit0`) |
| `+0x47` | **limite symétrique ±** d'un delta rate-limité | `Aero_ComputeControlFlags75Bit5C` L1855-1867 (clamp) |
| `+0x4B` | **α de décrochage / Cl max** (u8) — borne `α_eff` à `±(0x4B<<8)` ; dépassement (diff≥10, joueur) → alerte `flags_75` bit6 + portance annulée | `Aero_ComputeLiftAndSideForce` L348 |
| `+0x4C` | **angle de calage d'aile / α de portance nulle** (u8, non signé, `×256`) — **toujours** ajouté à α. *Pas un trim pilote* (jeu en accès direct sur les axes) : constante de cellule | `Aero_ComputeAoAWithTrim` L219 |
| `+0x4D` | **incrément de portance des volets** (u8) — ajouté si `flags_75` bit1 (volets sortis) | id. L230 |
| `+0x4E` | borne ± d'enveloppe **contact-sol** | bloc « au sol » L2213/2268 |
| `+0x52` | distance/vitesse max pour état « au sol » | id. L2252 (`if [+0x52] ≤ dist → bail`) |
| `+0x56`/`+0x57`/`+0x58` | bornes d'attitude 3 axes (i8, `<<8`) pour « au sol » | id. L2221/2231/2245 |
| `+0x59` | **altitude plafond de l'effet de sol** | `Aero_ApplyGroundEffect` L1546 (`if alt < [+0x59]` → `-0x1400`) |
| `+0x61` | **gain de portance / d'agilité de la cellule** (aussi utilisé ÷4 pour la force latérale) | `Aero_ComputeLiftAndSideForce` L343 |
| `+0x65` | coeff./borne × gain global `dword_72A1C` | `Aero_ComputeControlFlags75Bit5B` L1457 |
| `+0x66` | valeur de reset de l'accumulateur `[jdyn+0x1A]` × gain `dword_72A18` | `Aero_ResetAccumulatorFlags75Bit5` L1972 |
| `+0x67` | → `[jdyn+0x78]` (échelle 8-bit) | `Aero_ComputeControlFlags75Bit5B` L1158 |
| `+0x71` | **taux de rotation/cap max** (intégré par `dt` `dword_70458`) | `JDYN_TickSubcalcB` L3441 |
| `+0x80/82/84/86/8A/8B` | **rôle non tracé.** Pas de consommateur repéré par recherche de motif `[reg+off]` dans seg101-103 / seg109 (recherche **non exhaustive** — un accès via pointeur résolu, `es:[bx+…]` ou base+offset passerait au travers). Écrits par le loader `A5D18`, les défauts `A5620`, et une copie de variante `PlayerComponent` (seg449:807). Défauts 500/100/231/11005/3/2 — plausibles pour des params d'enveloppe (`500`≈Vne, `100`≈échelle, `3`/`2`≈enums). | — |
| `+0x5D` | **coeff. de traînée induite** — module la traînée par les angles d'écoulement au carré : `mag = q·(jdyn[+0x5D]·α_eff² + (jdyn[+0x5D]>>3)·β² + drag_flags75)` | `Aero_ComputeDragWithFeedback` `sub_48400` L633-637 (`>>3`) et L678-693 |

**Champs hors chunk `JDYN`, sur la même struct 0xC5 :**
- `+0x12` = **coeff de stabilité / autorité de tangage** — lu du **chunk `STBL`**
  (`sub_A52B8`, u32) ; sert d'échelle dans l'asservissement d'attitude
  (`q' = q · jdyn[0x12] / 100`, `Aero_ComputeForcesMain`).
- `+0x22..+0x32` = **sous-objet `THRS` embarqué** : `+0x2C` = poussée AF max (N,
  24.8) ; `+0x30/+0x31/+0x32` = les 3 `u8` du chunk `THRS` (fraction MIL /
  fraction à Vref / échelle Vcut — voir « Poussée »).
- `+0x28` = poussée moteur **courante** (sortie de `Aero_ThrottleThrustCurve`,
  mise à 0 au flameout).
- `+0x1A` = accumulateur ; `+0x78` = copie mise à l'échelle de `+0x67`.
- `[jdyn+0]` → sous-objet A ; `[A+0x20]` = flag « au sol ».

#### Découverte structurelle

La struct JDYN **n'est pas la table des coefficients aéro principaux**. Le cœur
(pression dynamique / angles d'écoulement / asservissement) est dans **seg102**
(`Aero_ComputeForcesMain` + `Aero_FlowAngle_AoA/Sideslip` +
`Aero_DynamicPressure`) et résout son coefficient de portance/traînée vs
vitesse-air via **`Aero_ResolveAeroCoeffTable` (`sub_46DA3`)** — une **table 1-D
à interpolation linéaire**. seg102 ne lit aucun champ JDYN par offset (il passe
par `[A+2]` = objet monde).

**La portance est explicite** — `Aero_ComputeLiftAndSideForce` (`sub_4812B`,
ex-`DetectControlSaturation`) : `L = k · α_eff · q · n̂_corps` avec
`α_eff = α + calage_aile(0x4C) [+ volets(0x4D)]`, `k = jdyn[0x61]·dword_72A24`,
`q ∝ v²`. Voir le bloc « Portance » plus bas.

L'essentiel des 28 champs JDYN est l'**enveloppe de la cellule** (consommée par
seg103) : carburant + consommation, traînées additionnelles (aérofrein/train),
**calage d'aile / volets** (`+0x4C`/`+0x4D`), **α de décrochage** (`+0x4B`),
limites de débattement/taux, plafond d'effet de sol, taux de rotation max, gain
de portance/agilité (`+0x61`), coeff. de réponse angulaire (`+0x5D`), et
**détection « au sol »** (`+0x4E/52/56/57/58` + train sorti → `[A+0x20]=1` →
active les coeffs de réaction sol `+0x3F/+0x43`). **6 champs (`+0x80..+0x8B`) :
rôle non tracé** (aucun consommateur repéré, recherche non exhaustive).

#### Le tick physique — **`PhysicsTicks` (seg103, ~0x4A85B)** = le tick de l'avion

**Tranché** (via les vtables). `JDYN_LoadChunk` installe pour la classe `JDYN`
des vtables aux offsets seg339 `0x228A`/`0x2312`/`0x2362` ; leur slot « Update »
pointe vers un symbole nommé littéralement **`PhysicsTicks`**
(`seg339:off_6F35E dd PhysicsTicks`), appelé par **4 thunks d'ajusteur `this`**
dans seg082. `FlightPhysics_TickCandidate_4F4EE` (seg109) est dans une **autre
vtable** (seg339 ~`0x1CFE`), utilisée par **3 classes `DYNM` plus simples**
(missile/bombe — `MISS`/`GBMB`/…). Tout en **virgule fixe 24.8**,
`dt` = `dword_70458`.

`PhysicsTicks(si = struct JDYN 0xC5 = [playerCtx+0x0B], arg_2)` :

```
1. Terrain_QueryAltitudeAt([[si]+2]+0x12, word_70474)      détection sol
   → [A+0x20] (flag « au sol ») posé/effacé selon difficulté word_70466
2. FlightControl_InvalidateAllCachesGlobal(si)              reset des caches aéro
3. MANETTE DES GAZ → POUSSÉE
   cran es:[obj2+0x1E] (0-10 : MIL 0-5 / AFT 1-5), rate-limité (× dword_72A2C·10)
   → Aero_ComputeCoeffSaturating(si+0x22, cran) → [si+0x28] = poussée courante
     (× gain dword_72A2C)
4. CONSOMMATION CARBURANT
   facteur 0x33 (51) si MIL (cran ≤ 5) sinon 0x4C (76) en AFT (post-comb. ≈ 1,5×)
   burn = base · dword_72A14 · [si+0x33](SFC) · cran · dt
   [si+0x6D] -= burn ;  flameout ([si+0x28] = 0) si [si+0x6D] ≤ 0
5. ÉTAT volets/aérofrein/train  ← bits du sous-objet contrôle es:[obj2+0x1C/0x1D]
   + Roster("FLAPS"/"LANDGEAR") ; agit sur flags_75 [si+0x75]
6. vitesse = |A.velocity([A+8/+C/+10])|                     auto-comportements bas niveau
7. SI [si+0x68] ≠ 0xFF ET !flags_75.bit5 :
      Guidance_HomingVelocityUpdate(si, arg_2)     ← AUTOPILOTE / homing (cinématique)
   SINON (vol manuel) :
      Aero_SumLinearForces_48639(si) → [A+0x14/+0x18/+0x1C] = ACCÉLÉRATION LINÉAIRE (net des forces)
      Aero_ControlOrchestrator(si, arg_2) → vecteur moment  (roulis/tangage/lacet + effet de sol, sat.)
      Physics_IntegrateSecondaryPosition(moment, si)        intègre l'ORIENTATION (si+4)
      [drag sol : si [A+0x20] et |A.vitesse| < 0x2800 → décél. roulage sur [si+0x0C]]
      Physics_IntegratePosition([si]=A, [si]+0x14)
          A.vitesse [A+8/+C/+10] += accél [A+0x14..] · dt  (transform corps→monde via [[A+2]+0x70])
      [contrainte sol : si [A+0x20] → projette A.vitesse hors du plan sol,
          vitesse verticale bornée ≥ 0, deadband |v|<5 → arrêt, limite taux sur [si+4]]
```

**La position monde n'est PAS intégrée dans `PhysicsTicks`** — il intègre accél→vitesse.
La position (`X/Y/Z` 24.8) est intégrée par la méthode vtable en tête de tick
(`[si+2] → [bx+0x34]`, sur l'objet monde), avant le calcul des forces (ordre
« intègre-puis-calcule »).

**Sous-objet A** (`[si]` → `si+0x8E`) — layout confirmé par `PhysicsTicks` :
`+0x02` ptr membre (→ objet monde `word_70474`) · `+0x08/+0x0C/+0x10` **vecteur vitesse**
(24.8) · `+0x14/+0x18/+0x1C` **vecteur accélération** (net des forces) · `+0x20` flag
« au sol ». Sur la struct JDYN : `[si+0x0C]` = décélération de roulage sol ;
`[si+0x68]` (défaut `0xFF` de `A5620`) = cible d'autopilote (`0xFF` = vol manuel).

##### Repères (à garder en tête)

- **Monde** : `[A+0x08]=X`, `[A+0x0C]=Y` (plan horizontal), `[A+0x10]=Z=altitude`
  (Z-up). Position + vitesse y sont stockées.
- **Corps** : sortie de `Physics_ResolveWindVectorCached` (transforme la vitesse
  monde→corps via `[[A+2]]→[+0x6C]`, magic `0x2EEB`). Axes
  **`c0` = envergure (aile droite)**, **`c1` = longitudinal (nez, axe poussée)**,
  **`c2` = normale (portance, à travers la verrière)**.
- Tout le bilan de forces est fait **en repère corps**, puis
  `Physics_IntegratePosition` le repasse **corps→monde** (`[[A+2]]→[+0x70]`)
  avant d'intégrer dans la vitesse monde.
- **Angle d'attaque** `α = Aero_FlowAngle_AoA` `= −Cα·v_corps.c2/|v|` ;
  **dérapage** `β = Aero_FlowAngle_Sideslip` `= −Cα·v_corps.c0/|v|`.
  **`Cα = dword_707B2 = 0x394B = 57.29 = 180/π`** (init `loc_54E83`,
  xref `seg339:off_71F14`) → **α et β sont en DEGRÉS** (`α ≈ −asin(v_corps.c2/|v|)`
  en degrés). Le clip de contrôle à `0x3800 = 56.0` est donc **56°**.
  Autres constantes de la même routine : `dword_707B6 = 4`, `dword_707BA = 1`.
- **Pas de trim pilote** : le jeu est en accès direct sur les axes
  (clavier/joystick). `[ctrl+0x1F]` (tangage brut) → consigne d'angle
  `jdyn[0x16]` directement, sans intégrateur de trim.

##### Équation de force linéaire — `Aero_SumLinearForces_48639` (ex-`…ControlFlags75Bit5A`, seg103)

`si` = struct JDYN ; sortie = vecteur accélération 3× i32 (24.8) **en repère
corps**, écrit par `PhysicsTicks` dans `[A+0x14/+0x18/+0x1C]`. Corps lu ligne à
ligne (seg103 L874-1117) :

```
SI flags_75.bit5 :  thrust = satur = 0  →  accel = grav seule   (physique « gelée »)
SINON :
  thrust = FlightControl_CacheTripleParam(&si[0x22])   → (0, si[0x28], 0)   poussée pure sur c1 (nez)
  satur  = Aero_ComputeLiftAndSideForce(si)            → PORTANCE + force latérale  (cf. bloc « Portance »)
  aero   = Aero_ComputeDragWithFeedback(si)            → traînée le long de la trajectoire (cf. bloc « Traînée »)
  m      = call [ [si+2] + 0x3C ] (this=si)  >> 8      scalaire 24.8 ≈ MASSE / inertie (virtuel JDYN)
  grav   = FlightControl_ResolveParamCached(&si[0x1E])  via vtable[0x6C] (magic 0x2F1A) — gravité résolue en repère corps

  F      = thrust>>8  +  satur  +  aero                somme par composante (var_40/3C/38)
  accel  = (F << 8) / m           (idiv, par composante)
  [si+0x7C] = −accel.c2 / dword_6FFD7                  FACTEUR DE CHARGE (G-mètre, axe normal corps) — pas l'accél verticale monde
  accel += grav                                        gravité ajoutée APRÈS division par m
```

**Poussée = `(0, si[0x28], 0)`** — `FlightControl_CacheTripleParam` (seg102
L1929) ne remplit que `c1` avec `si[0x28]` (magnitude calculée par
`PhysicsTicks`). Poussée **purement selon l'axe nez**.

##### Bloc « Portance » — `Aero_ComputeLiftAndSideForce` (`sub_4812B`, seg103 L261-593)

Terme `satur`. Nom hérité (`DetectControlSaturation`) trompeur : c'est **le
générateur de portance + force latérale**, en repère corps.

```
α_eff  = Aero_ComputeAoAWithTrim(si)  =  α + si[0x4C] (calage d'aile, TOUJOURS) + si[0x4D] (volets, si flags_75.bit1)
         borné à ±(si[0x4B]<<8)  = α de décrochage / Cl max
β      = Aero_FlowAngle_Sideslip(si)
q      = Aero_DynamicPressure(&si[0x10])              = ½·(v·v)·coeff(vitesse-air)   [table lerp, cf. bloc « q »]

k_lift = si[0x61] · dword_72A24 · α_eff · q
Portance     = k_lift · normalize( 0 , −v_corps.c2 , v_corps.c1 )     ⟂ vitesse, plan longitudinal-vertical
                                                                       ≈ (0,0,+1) en palier → PORTANCE ≈ k_lift sur c2

k_side = (si[0x61]>>2) · β · q
ForceLatérale = k_side · normalize( v_corps.c1 , −v_corps.c0 , 0 )     ⟂ vitesse, plan horizontal (force de virage)

sortie = Portance + ForceLatérale
```

Effet de bord : si `|α_eff| > si[0x4B]` **et** difficulté `word_70466 > 10`
**et** joueur → `flags_75.bit6` (alerte) + **portance mise à zéro** (départ).
`normalize` = `Vector_Normalize3D` (norme 24.8 → 0x100), **pas une rotation**.

**Pourquoi ça reproduit le comportement observé :**
- `si[0x4D]` (volets) augmente `α_eff` d'un cran fixe → **portance en plus sans
  toucher au manche ni changer l'assiette** (`normalize(0,−v.c2,v.c1)` vaut
  `(0,0,1)` même à `v.c2 = 0`).
- `k_lift ∝ q ∝ v²` → **ralentir fait chuter la portance** → `Portance/m < g` →
  descente à assiette constante.
- Le **roulis n'entre pas** dans la magnitude (seulement `α` et `q`). Il agit via
  la transformation corps→monde : à `φ` de gîte, composante verticale monde
  `= L·cos φ` → à 90° elle s'annule, toute la portance devient centripète →
  descente inévitable sans augmenter `α`.

##### Bloc « Traînée » — `Aero_ComputeDragWithFeedback` (`sub_48400`, seg103 L606-853)

Terme `aero`. Ex-`ComputeMomentsWithFeedback` : produit un **vecteur de traînée**
(pas des moments), en repère corps.

```
q      = Aero_DynamicPressure(&si[0x10])
α_eff  = Aero_ComputeAoAWithTrim(si)   ;   β = Aero_FlowAngle_Sideslip(si)
drag75 = Aero_ComputeDragFromFlags75(si) = ( 0x100 + si[0x37]·bit0(aérofrein) + si[0x3B]·bit2(train) )
                                           · [ [si+0x10] + 0x10 ]
mag    = q · [ si[0x5D]·α_eff²  +  (si[0x5D]>>3)·β²  +  drag75 ]           (traînée induite + parasite)
SI au sol (A[0x20]≠0) :
  mag += ( si[0x3F]·256 + si[0x43]·256·bit0 ) · (|v_corps.c1|<256 ? ·v_corps.c1 : 1) >> 8    réaction sol
dir    = Vector_Normalize3D( −v_corps )         unitaire opposé au flux, en repère corps
sortie = dir · mag   → écrit dans [si+0x10]+4..+0xC
```

- `si[0x5D]` = coeff de réponse angulaire / amortissement ; `si[0x37]`/`si[0x3B]`
  = traînée aérofrein/train ; `si[0x3F]`/`si[0x43]` = réaction sol (au sol seul).
- La traînée s'oppose à la trajectoire ; sa composante `c2` (quand `α ≠ 0`)
  apporte une portance secondaire, mais **la portance principale vient du bloc
  ci-dessus**.

##### Bloc « q » — `Aero_DynamicPressure` (`sub_46D13`) + `Atmosphere_AirDensityAtAltitude` (`sub_46DA3`, seg102)

`Atmosphere_AirDensityAtAltitude` (ex-`ResolveNamedParam`/`ResolveAeroCoeffTable`)
= **`ρ(altitude)` par interpolation linéaire dans `AIRDENS.TBL`** :

```
tab  = Memory_TypedAllocDispatchB_5C832(tag 0x5C44, dword_72A0A, byte_72A0E)   → AIRDENS.TBL
h    = [ [A+2] + 0x1A ]          ALTITUDE (24.8 m), bornée [0, 0x77EC00] = [0, 30702 m]
i    = h >> 16                   index = altitude_m / 256  (une entrée par bande de 256 m)
f    = h − (i<<16)               fraction dans la bande
y    = lerp( tab[i], tab[i+1], f / 0x10000 )   avec tab[k] ré-échelonné (<<8 puis >>10 ⇒ ÷4)
```

**`AIRDENS.TBL`** = `..\..\DATA\AIRDENS.TBL`, **121 × u32 LE, décroissant de 1322
(niveau mer) à 28 (~31 km)**. C'est une **densité d'air ρ(h)**, pas un Cd/Cl.
Colle très bien une exponentielle : `ρ ≈ 1.225 · exp(−h / 8000)` (hauteur d'échelle
≈ 8 km ; erreur < 1 % sur toute la table). Conversion : `ρ_SI(h) = 1.225 ·
tab_lerp(h) / 1322`.

`Aero_DynamicPressure` (seg102 L500) l'emballe : **`q = ½ · ρ(h) · (v·v)`** (vraie
pression dynamique), borné ≥ 0, cache frame (`dword_6FFB7`). C'est l'échelle de
force aéro maîtresse — multiplie portance, traînée **et** forces de contrôle.

La sélection de table passe par un registre de handlers typés
(`Memory_TypedAllocDispatchB_5C832`, `word_5C38[subkey=2]`, `vtable[0x10]`) keyé
par `dword_72A0A` = id d'atmosphère du chunk `ATMO` (global mission ; `0` par
défaut → `AIRDENS.TBL` standard).

##### Poussée — courbe manette (`Aero_ComputeCoeffSaturating`, `sub_4730F`, seg102 L1411-1644)

`PhysicsTicks` appelle `Aero_ComputeCoeffSaturating(&jdyn[0x22], cran)` →
`jdyn[0x28]` = poussée courante (N, 24.8). Sous-objet `THRS` embarqué à
`jdyn+0x22` ; `cran` ∈ [0,10] (MIL 0–5, AF 6–10 ; `cran = −1` → traité comme 0).

```
poussée = jdyn[0x2C] · fThrottle(cran) · fLapse(V)          (borné ≥ 0)

fThrottle(cran) :   cran ≤ 5 (MIL) :  (cran/5) · jdyn[0x30]
                    cran > 5 (AF)  :  jdyn[0x30] + (1 − jdyn[0x30]) · (cran−5)/5
   ⇒ 0 à cran 0 ,  jdyn[0x30] à cran 5 (MIL 100%) ,  1.0 à cran 10 (pleine PC)

fLapse(h) :   h ≤ href :  1 − (h/href) · (1 − jdyn[0x31])
              h > href :  jdyn[0x31] − jdyn[0x31] · (h−href)/(hcut − href)
   ⇒ 1.0 au niveau mer ,  jdyn[0x31] à href ,  → 0 vers hcut
   href = 11000 m (0x2AF800 en 24.8) , hcut = jdyn[0x32]·100 m  (défaut 231 → 23100 m)
   h = [[[jdyn+0x24]+2]+0x1A] = ALTITUDE (même champ que la densité d'air ci-dessus)
```

C'est un **lapse de poussée avec l'ALTITUDE** (moins d'air en altitude), pas avec
la vitesse. `jdyn[0x2C]` = poussée PC max (N), `jdyn[0x30]` = fraction MIL,
`jdyn[0x31]` = fraction de poussée à 11 000 m, `jdyn[0x32]` = altitude de coupure
÷100. **Les gains globaux `dword_72A14..72A2C` valent tous `1.0`** (`0x100`,
`sub_A5620`) — ignorables.

##### `Aero_ControlOrchestrator` (`sub_48FC2`, seg103 L2056-2142) : moments de contrôle

Assemble le **vecteur de moment** passé à `Physics_IntegrateSecondaryPosition`
(→ orientation `[si+4]`). Deux pré-appels à effet de bord :
`Aero_ComputeControlFlags75Bit5B` (calcule la **consigne d'angle de tangage
`si[0x16]`** depuis le manche pilote `[ctrl+0x1F]`, le gain `si[0x61]`, le coeff
`si[0x65]·dword_72A1C`, le plafond d'effet de sol `si[0x59]`, le calage d'aile
`si[0x4C]` et les volets `si[0x4D]`) puis `Aero_ResetAccumulatorFlags75Bit5`.

| Composante sortie | Producteur | Contenu |
|---|---|---|
| `[out+0]` | `Aero_ApplyGroundEffect` → `Aero_ComputeForcesMain` c0 | + correction effet de sol `−0x1400` si bas & au sol |
| `[out+4]` | `Aero_ComputeControlFlags75Bit5C` | table de params indexée `si[0x77]` |
| `[out+8]` | `Aero_ExtractRollMoment` → `Aero_ComputeForcesMain` c2 | — |

**`Aero_ComputeForcesMain` (`sub_4791E`, seg102 L2176-2824)** = solveur
d'asservissement d'attitude (loi en racine carrée), mis en cache frame
dans `dword_6FFE8/EC/F0` :

```
q'      = Aero_DynamicPressure(&si[0x10]) · si[0x12] / 100
α, β    = Aero_FlowAngle_AoA(si), Aero_FlowAngle_Sideslip(si)   angles d'écoulement actuels
          bornés ±0x5A00 (±90°) quand le flux d'air est inversé (v_corps.c2 < 0)
errα    = si[0x16] − α    (consigne tangage − incidence actuelle)  ; errβ = si[0x1A] − β
mα      = ±2·√(q'·|errα|)                                 loi de rappel en racine
          borné à ±(errα · dword_70454)                   → rate-limit (amorti critique, sans dépassement)
          = 0  si |errα| ≥ 0x3800 (56°)                   décrochage de contrôle → aucune autorité
incr    = (mα − si[4]) · dt   borné ±3·q'   (+ garde-fous anti-dépassement)
Utility_Helper_55E65(tag 0x2F38)  →  stocke le triplet (mα, 0, mβ)
```

##### Constante de gravité `g` — **trouvée**

`dword_6FFD7` (utilisée par `Aero_SumLinearForces` pour le G-mètre et par le
terme `grav`) est initialisée dans `loc_47E74` (routine de reset de l'état
aéro, xref `seg339:off_71EF0`) :

```
; strike.asm:144603
mov  dword ptr [bp-1Ch], 0FFFFF634h      ; = −2508
mov  eax, [bp-1Ch]
mov  dword_6FFD7, eax
```

`−2508` en virgule fixe 24.8 = **−9.797 ≈ −9.8 m/s²** → **le moteur est en
SI** : masse en **kg**, poussée `THRS` en **N**, positions en **m**, vitesses
en **m/s**, `dt` en **s**, le tout ×256 (24.8). Corollaires :
- `grav` = `worldToBody( (0, 0, −9.8) )` — `FlightControl_ResolveParamCached`
  pré-remplit `(0, 0, dword_6FFD7)` (Z = altitude monde) puis `vtable[0x6C]`
  magic `0x2F1A` le tourne en repère corps.
- `jdyn[0x7C] = −accel_corps.c2 / dword_6FFD7` = **facteur de charge en G**
  (division par g) — confirme l'interprétation « G-mètre ».

##### Le diviseur de masse `m` — trace bloquée en statique

`m` vient d'un **appel virtuel `[[si+2] + 0x3C]`** (this = `si` = objet 0xC5),
scalaire 24.8 lu ensuite dans le paramètre de sortie (`>>8`).

- Le constructeur `JDYN_LoadChunkAndConstruct_3A49C` (`enter_form('DYNM')`,
  `malloc(0xC5)`) écrit les vptr finaux : **`[si+2] = 0x228A`** (objet
  principal), `[[si]] = 0x2312` (sous-objet A / dynamique), `[[si+0x10]]+2 =
  0x2362`. (strike.asm ~116908.)
- Base seg339 = `0x6D070` (vérifié). `seg339:0x228A` = vtable linéaire
  `0x6F2FA` ; slot `+0x3C` = `off_6F336 → loc_4692C` = **stub partagé de 8
  octets** (`mov ax,[bp+6] / retf`, strike.asm:141797) qui **n'écrit rien**
  dans le paramètre de sortie. Le slot `+0x34` (appelé en tête de
  `PhysicsTicks`) = `loc_4691A`, même stub.
- Or `Aero_SumLinearForces` lit ce paramètre de sortie (`var_4`, `[bp-4]`,
  **non initialisé** avant l'appel dans la lecture statique) et s'en sert
  comme diviseur pour les 3 composantes. **Contradiction** : soit l'avion
  reçoit à l'exécution un `[si+2]` différent de `0x228A` (probable — Rémi
  confirme qu'il existe une **classe wrapper `DYNM`** au niveau de la
  structure IFF `JETP → FORM DYNM → {DYNM, THRS, JDYN}` ; le wrapper `JETP`
  lui-même fait 0x6A o, vtable `0x24C8`, alloué par `IFF_LoadModelMain` via
  `sub_5C6F3(0x5C44, 0x6A, …)`), soit la sémantique de retour par buffer
  échappe à l'analyse statique.
- **À confirmer par trace d'exécution** (DOSBox : point d'arrêt dans
  `sub_48639` sur le `call [bx+3Ch]`, lire `bx` et la valeur écrite dans
  `var_4`).
- Conceptuellement, la masse divisée reste **le chunk `DYNM`** :
  `AircraftDynamics_ReadMassChunk_A4A20` (seg447) — FourCC `'DYNM'` →
  `ResourceRecord_ReadFieldGroupC_64A7E` (u32) → `[massSubobj+0x04]` (24.8,
  kg) ; erreur `0xA001` si absent.

##### Chunk `THRS` et champs restants

`AircraftDynamics_ReadThrustChunk_A5224` (seg449, → `sub_6CB41` VROOMM) lit le FourCC **`'THRS'`** :

| Lecture | Primitive | Destination sous-objet | Rôle |
|---|---|---|---|
| u32 | `sub_64A7E` (`ReadFieldGroupC`) | `+0x0A` | **poussée max (24.8, N)** |
| u8 | `sub_64B51` | `+0x0E` | param 1 (non tracé) |
| u8 | `sub_64B51` | `+0x0F` | param 2 (non tracé) |
| u8 | `sub_64B51` | `+0x10` | param 3 (non tracé) |

Il y a donc **3** octets de paramètre après la poussée (pas 2). Ces trois
octets alimentent la courbe manette→poussée
(`Aero_ComputeCoeffSaturating(&jdyn[0x22], cran)` dans `PhysicsTicks` →
`jdyn[0x28]`) ; le détail de leur usage n'est pas tracé.

`JDYN +0x80..+0x8B` (u16 défauts 500 / 100 / 231, u32 11005, u8 3 / 2) :
toujours non tracé.

##### Modèle de vol linéaire — synthèse

Toutes les forces sont **en repère corps** (c0 envergure, c1 nez, c2 normale) ;
`Physics_IntegratePosition` repasse corps→monde avant d'intégrer.

```
tick(struct JDYN) :
  [si+2]→vtable[0x34](si)                           intégration position monde (« intègre-puis-calcule »)
  accel_corps = (poussée + portance+latéral + traînée) / masse + gravité_corps    Aero_SumLinearForces_48639
    poussée   = (0, jdyn[0x28], 0)                          sur c1 (nez)
    portance  = k·α_eff·q · n̂(0,−v.c2,v.c1)  ≈ sur c2       Aero_ComputeLiftAndSideForce
                α_eff = α + calage_aile(0x4C) [+ volets(0x4D)] ,  borné ±(0x4B) = décrochage
                k     = jdyn[0x61]·dword_72A24
    latéral   = (jdyn[0x61]>>2)·β·q · n̂(v.c1,−v.c0,0)  ≈ sur c0    (force de virage)
    traînée   = −v̂_corps · [ q·(0x5D·α_eff² + (0x5D>>3)·β² + drag_flags75) (+ réaction sol) ]   Aero_ComputeDragWithFeedback
    q         = ½·(v·v)·coeff(vitesse-air)                  [table lerp Aero_ResolveAeroCoeffTable]
    gravité   = FlightControl_ResolveParamCached(si+0x1E)   résolue en repère corps, ajoutée APRÈS ÷ masse
  vitesse_monde += transform_corps→monde(accel_corps) · dt  Physics_IntegratePosition([A], [A]+0x14)
  si au sol : projeter la vitesse hors du plan sol, v_alt ≥ 0, deadband, drag roulage
  orientation : Aero_ControlOrchestrator → moments (mα, ?, mβ) → Physics_IntegrateSecondaryPosition
    consigne α  si[0x16] ← manche [ctrl+0x1F]  (Aero_ComputeControlFlags75Bit5B, PAS de trim)
    mα = ±2·√(q'·|consigne − α|), rate-limité ±err·dt, nul si |err| ≥ 56°
```

**Accélération verticale (altitude, monde Z)** = composante Z de
`transform_corps→monde(portance + latéral + traînée + poussée)/masse` **+**
gravité. En palier ailes à plat, `portance` (sur c2 corps) ≈ verticale monde ;
avec de la gîte `φ`, seule `portance·cos φ` reste verticale. `jdyn[0x7C]`
(`= −accel_corps.c2 / dword_6FFD7`) est le **facteur de charge (G-mètre)**, pas
cette accélération monde.

`FlightPhysics_TickCandidate_4F4EE` (seg109), pour référence — tick corps rigide
compact des 3 classes simples : `ResetAccumulators` → linéaire
(`ComputeWeightedCoeff` + `ComputeWindComponent` → ÷ masse `vtable[0x10]` →
`IntegratePosition`) → angulaire (`Aero_ComputeForcesMain` → `Aero_FlowAngle_*`
→ `IntegrateSecondaryPosition`). Ce tick divise par la masse via **`vtable[0x10]`**,
pas `vtable[0x3C]` : offsets de slot différents entre `DYNM` simple et `JDYN`.

#### Entrées joueur → état → physique

`Player_MainUpdate_13100` (seg011) est **l'étage d'entrée** (ni tick physique ni
rendu). Contexte joueur (`arg_0`) :

| Offset `playerCtx` | Type | Rôle |
|---|---|---|
| `+0x02` | far ptr | objet à vtable (`call [bx+0x3C]`…) |
| `+0x07` | far ptr | **sous-objet contrôle/HUD** : flags commande `+0x1B/1C/1D`, **`+0x1F` = commande tangage**, **`+0x23` = commande roulis** (24.8), `+0x27` probable lacet |
| `+0x0B` | **near ptr → struct JDYN 0xC5** | prouvé : `[+0x75]` = `flags_75`, `[[+0]]` → sous-objet A, `[A+0x20]` = flag « au sol » lu par la physique |
| `+0x0D` | dword | testé |

| Entrée | Touche(s) | Écrit | Chemin |
|---|---|---|---|
| Tangage | joystick **ou ↑/↓** | `[playerCtx+7 +0x1F]`, `÷3` | `Input_GetCalibratedYValue_67499` → `word_72E6A` |
| Roulis | joystick **ou ←/→** | `[playerCtx+7 +0x23]`, `÷0x0A` | `Input_GetCalibratedXValue_67481` → `word_72E68` |
| Volets | `F` (toggle) | `flags_75` bit1 (`[jdyn+0x75]`) | dispatcher clavier `Player_MainUpdate` (msg « Flaps ON/OFF ») |
| Aérofrein | `B` (toggle) | `flags_75` bit0 | id. (« Brakes ON/OFF ») |
| Train | `L` (toggle) | `flags_75` bit2 (interlock `[A+0x20]`) | id. (`and…0FBh` / `or…4`) |
| Poussée | `+`/`-`, `1`…`0` (**MIL 0-5 / AFT 6-10**, chaînes seg339) | `cran` → `Aero_ThrottleThrustCurve` (`sub_4730F`) : `jdyn[0x2C]·fThrottle(cran)·fLapse(V)` → **`[jdyn+0x28]`** (0 au flameout) | dispatcher clavier **hors `Player_MainUpdate`** — non tracé |
| **Vue cockpit** | **souris** | `word_72E68`/`72E6A` + `word_71200 = 1` | `Mouse_EventCallback_68109` (seg210) → chemin `Cockpit_ProcessInputFrame` / `Mouse_InitAndRead` |

**Exclusion mutuelle souris / pilotage** : `word_72E68`/`72E6A` est une paire
partagée gardée par `word_71200`. En vol (`== 0`) elle porte les axes
joystick/curseur ; `Input_GetCalibrated{X,Y}Value` **plante volontairement**
(`Runtime_FatalErrorHandler`, tag 4100h) si `word_71200 != 0` (mode souris/vue).
La souris ne pilote **pas** — elle contrôle la vue 3D du cockpit.

Restes non tracés : (1) où `[jdyn+0x28]` (poussée) devient une force propulsive
dans l'intégrateur linéaire (seul consommateur repéré : seg103:5172) ; (2) le
dispatcher clavier de la manette des gaz (= consommateur de la file d'événements
clavier) ; (3) la conversion touche-curseur → axe virtuel dans `word_72E68/6A`
(candidats `Input_ReadCombinedAxes` / `Input_ReadAxis2Deadzone`, seg049) ;
(4) l'emplacement de la position/géométrie (sur `playerCtx` ? sur l'objet
`JETP` 0x6A ?) — non visible depuis l'étage d'entrée.

### 6.3 Textures — `TextureLoader_*` (ovr288-289)

`TextureLoader_OpenAndLoadFull_98A60` / `OpenAndDecode_9800A` /
`OpenAndLoadSubrecord_98670` : `Path_ResolveDataFile` → StreamReader →
`ResourceRecord_AllocateAndDecode` → champs mixtes (`ReadFieldGroupB/C`) +
`Fmemcpy` du bloc pixels. `SelectMipLevel_980F0` = choix du niveau de mipmap
(`String_FromPtrLen` + `Texture_SelectMipLevel`). Familles
`LoadWithDwordFields*` / `LoadMixedFields*` = variantes selon le format d'en-tête.

### 6.4 Polices — `Font_*` (ovr241-242)

`Font_OpenDefinitionFile_7FD64` (chemin + `InitAndValidate`) →
`Font_LoadDefinitionFromIFF_7E1B4` (482 L, en-tête police) →
`Font_LoadGlyphTable_7FF90` (table de glyphes) →
`GlyphObject_LoadMetricsFromIFF_7DE90` / `…B_7FB10` (métriques par glyphe,
`GlyphObject_ComputeMetrics_60456`, `GlyphObject_AllocateBuffer_604C9`).
Police par défaut : `SM-FONT` (chargée au boot, cf. `TextRenderer_Main`).
`Lexer_LoadScriptFile_7F90F` = chargement d'un fichier script résolu par chemin
(parseur PROF/`_AI_`).

### 6.5 Palette — seg066 / seg442

- `Palette_ParseBlock_2E84E` : lit type + taille depuis un flux, alloue
  `taille × 3` (RGB), `memcpy` — bloc palette d'un fichier image (LBM/PCX).
- `VGA_LoadPalette_2E801` : DAC VGA ports `0x3C8`/`0x3C9`, sync retrace
  (`word_70EA0`).
- seg442 (`PaletteScreen_*`) : écran d'édition de palette — DAC via `INT 10h`
  (1010h/1012h/1015h/1017h), tampon de travail 768 o
  (`Palette_GetOrLoadWorkingBuffer_A1550`), dégradés/courbes personnalisés
  (`PaletteScreen_BuildCustomGradient_A1626` : 22 entrées + 23 points de courbe).

### 6.6 Missions — 3 niveaux imbriqués

#### a) Scénario — `MissionScenario_*` (seg456-457) — **relecture détaillée**

Le « scénario » (SCEN) est le sous-système **seg456-457** : il lit une mission au
format FORM `MISN`, câble chaque champ dans le **graphe de nœuds de la VM
d'expression** (seg114) via `ExprSlot` (seg457), et persiste la progression de
campagne par deux **fichiers-pont IFF** : `OP2MISN` (entrée) et `MISN2OP` (sortie).

##### Chaîne d'appel

```
Campagne  ──►  MissionScenario_LoadFromIFF_A8C44   (lit  DATA\OP2MISN.IFF, FORM INVN)
                     │
                     ├─ chunks pilote/roster (voir plus bas)
                     ├─►  MissionScenario_LoadMainRecord_A8331  (lit  MISSIONS\<name>.IFF, FORM MISN)
                     │         └─ 17 chunks  →  sous-objets de la struct MissionScenario
                     │            + câblage Expr VM (nœud PLAYER  si+0x3A)
                     └─ bind des enregistrements PLYR  →  nœuds participants

Fin de mission  ──►  MissionScenario_ResolveAndBindExpressions_A8F22
                          └─ ÉCRIT  DATA\MISN2OP.IFF  (FORM INVN : INFO/STAT/JETS/KILL/EXIT/DIED/SCOR)
```

Chemins : `Path_ResolveDataFile` → `MISSIONS\<name>.IFF` pour la mission ;
`Path_BuildComposite(prefix unk_70720, "OP2MISN"/"MISN2OP", ".IFF")` pour les ponts.
`unk_6E4D9` (nom pilote), buffers `AIRHEAD` / `SUTEK` = **contenus de chaîne par
défaut** (labels IDA), pas des noms de fichiers.

##### `MISSIONS\<name>.IFF` — FORM `MISN` (par `LoadMainRecord_A8331`)

Chunks lus **dans cet ordre**, chacun rangé dans un champ / sous-objet de la struct
`MissionScenario` (base `si`) ; le span de chaque chunk est enregistré (`var_AB`)
pour la passe de liaison `ExprSlot` :

| # | FourCC | Lecteur | Cible (struct) | Contenu |
|---|---|---|---|---|
| 1 | `VERS` | `ReadFieldGroupB` u16 | (local) | version — **fatale si < 0x500** (erreur `3692h`) |
| 2 | `NAME` | `ReadFieldGroupA` N≤16 o | `si+0x09` (char[]) | nom de mission (défaut « UNK ») |
| 3 | `TUNE` | `ReadFinalField` u8 | `byte_706A2` | piste musicale |
| 4 | `WRLD` | `enter_form` | — | sous-FORM monde (**obligatoire**, erreur `9015h`) |
| 4a | ↳ `FILE` | `ReadFieldGroupA` 8 o | (local) → `sub_6CDCD` | nom du fichier monde à charger |
| 4b | ↳ (sinon) | `sub_6CDC8` | — | monde inline |
| 5 | `CAST` | `sub_6CF0D` | `si+0x2E` | acteurs / personnages |
| 6 | `AREA` | `sub_6CED6` | `si+0x1A` | zones géographiques |
| 7 | `SPOT` | `sub_6CEE5` | `si+0x1E` | points nommés |
| 8 | `NUMS` | `sub_6CEF4` | `si+0x24` | variables numériques de mission |
| 9 | `MSGS` | `sub_6CF03` | `si+0x28` | messages / dialogues |
| 10 | `FLAG` | `sub_6CE9F` | `si+0x48` | flags d'événement de mission |
| 11 | `PROG` | `sub_6CEB3` (assert `size` pair) | `si+0x40` | **programme scripté** (bytecode Expr VM) |
| 12 | `PART` | `sub_6CF35` → `A9CC3` | `si+0x34` | participants / factions |
| 13 | `TEAM` | count = `size/2` (≤8) ; `ReadDwordFieldB` | `si+0x84` count, `si+0x74` tableau | équipes (dwords) |
| 14 | `PLAY` | `enter_form('PLAY')` → `sub_6CF58` → `MissionPlay_LoadScenes_AA595(si+0x3A)` → `pop` | `si+0x3A` | **la « pièce » : FORM contenant N chunks `SCNE`** (voir plus bas) |
| 15 | `LOAD` | `MissionScenario_LoadFieldGroup_A8225` | — | emport (loadout) |
| 16 | `HOME` | `ReadFieldGroupB` u16 → `GeomNode_SumAndCount_53034(si+0x1A)` → `si+0x58` ; puis 3× `ReadFieldGroupC` u32 → `si+0x5C/0x60/0x64` | `si+0x58`, `si+0x5C/60/64` | base de départ : index dans la liste `AREA` + **vecteur position** (X, Y, Z ; Z = altitude, fixed-point) |

> **Modèle « théâtre »** : la narration scriptée d'une mission est structurée comme
> une pièce de théâtre. `CAST` = distribution (personnages), `PLAY` = la pièce
> (script), qui contient une suite de `SCNE` = **scènes**. `PROG` = programme /
> didascalies. Ce ne sont pas des données « joueur ».

Puis câblage Expr sur `si+0x3A` (l'objet PLAY) : `Expr_Node_Accessor_532AD(si+0x3A)`
→ nœud `si+0x4E` (erreur `9019h` si nul) ; `Expr_Node_RecomputeFieldA_531CD` ;
`ConvertFieldValue_A8B8F` → objet résolu `si+0x54` (far ptr) ; on pose sur cet
objet les flags `+0x39 |= 0x10|0x20`, on lit son id `+0x52`, `SetReference16`
(weak ptr, tag `5236h`). Si `[si+0x50]≠0 && [si+0x58]==0`, la position `HOME`
(`si+0x5C/60/64`) est **recalculée** via `Expr_Node_StoreResultVector_52C9C`.
Enfin `Expr_Node_RecomputeAllFields_532EA(si+0x3A)` (évalue toutes les scènes) et
`Lexer_CountTokensAndFormat(si+0x68)`.

##### `PLAY` → chunks `SCNE` (scènes) — `MissionPlay_LoadScenes_AA595` (seg457)

`enter_form('PLAY')` puis **2 passes** sur les sous-chunks :

1. **Collecte** : parcourt les chunks du niveau ; pour chaque tag == `SCNE`
   (`[rr+0x6D] == 0x454E4353`, octets disque `S C N E`), mémorise l'offset de
   début dans un tableau local (max 0x20 scènes).
2. **Allocation + parsing** : alloue `si+0x3A[+2]` = tableau de `N` **records de
   0x27 (39) octets** (`si+0x3A[+0]` = compteur), puis pour chaque scène :
   `seek` à son offset, `ComputeAlignedBounds` (relit l'en-tête), puis
   `MissionScene_ParseRecord_AA3A5(dst = base + i*0x27, ctx, rr)`.

**Chunk `SCNE` sur disque** = en-tête fixe de **24 octets** + `(taille − 24) / 2`
mots u16 en queue (liste d'unités, voir plus bas).

Layout confirmé (croisé avec le code + `MISSIONS\MISN-1A.IFF`) :

| Off. disque | Taille | Champ | Résolution moteur | → record (0x27 o) |
|---|---|---|---|---|
| `0x00` | u8 | `is_active` | copié tel quel | `+0x00` |
| `0x01` | u16 | `area_id` (index dans `AREA`, `-1` = scène globale) | `GeomNode_SumAndCount_53034(ctx+0x1A, area_id)` | `+0x04` (handle 32 b) |
| `0x03` | i16 | `on_is_activated` (ref `PROG`, `-1` = aucun) | `Expr_LookupNamedValue_51E4A(ctx+0x40, …)` | `+0x08` (far ptr) |
| `0x05` | i16 | `on_leaving` | idem | `+0x0C` |
| `0x07` | i16 | `on_mission_update` | idem | `+0x10` |
| `0x09` | u8 | `has_own_coord` (1 = position explicite ci-dessous ; 0 = déclenche sur le volume de l'aire) | — | `+0x18` |
| `0x0A` | u16 | `heading` — **candidat : cap en degrés** (0–359). Dans MISN-1A : `180` pour les scènes ancrées à une aire, `0` pour celles à `pos` explicite. Rôle non tracé dans le code ; à confirmer sur d'autres missions (valeurs ≠ 0/180 attendues si c'est bien un cap). | — | `+0x19` |
| `0x0C` | i32 | `pos_x` — fixed-point ÷256 | — | `+0x1B` |
| `0x10` | i32 | `pos_y` — fixed-point ÷256 | — | `+0x1F` |
| `0x14` | i32 | `pos_z` — fixed-point ÷256 (**Z = altitude**, pieds) | — | `+0x23` |
| `0x18…` | u16 × `(taille−24)/2` | **liste d'unités** de la scène | `malloc` + copie brute | `+0x14` count, `+0x16` ptr |

Le record en mémoire ajoute `+0x02` = back-ptr vers l'objet `PLAY` (contexte, = `arg`).

##### La liste d'unités (queue du `SCNE`)

Chaque valeur `u16` (octet haut = 0) est un **index dans le chunk `PART`, dans son
ordre de stockage** — **ni un id d'entité, ni un index `CAST`, sans offset**.
`PART` est une liste de records de taille variable, chacun commençant par
`<u16 entity_id> <char[8] type>` ; **l'ordre de stockage ≠ l'ordre des
`entity_id`**. Résolution : `slot → PART[slot].entity_id → CAST[entity_id]` (nom).

`CAST` (chunk séparé, records de 9 o, noms `PLAYER`/`STERN`/`NULL`/…) n'est **pas**
indexé par les scènes : il sert vraisemblablement à associer des scripts / rôles
d'IA aux entités par nom symbolique (`NULL` = entité sans rôle scénarisé, ex. un
repère de piste `RWYEXT18`).

##### Exemple vérifié — `MISSIONS\MISN-1A.IFF` (« Mauritania »)

`PART` (ordre de stockage) → `entity_id` : `[0, 1, 8, 2, 5, 6, 7, 3, 4]`
(types : F-16des, F-16des, C130des, STRIBASE, MIG21, MIG21, MIG21, RWYEXT18, RWYEXT18).

| SCNE | `area_id` → zone | `prog` (act / leav / upd) | `pos` ÷256 | queue (slots `PART`) → unités |
|--:|---|---|---|---|
| 0 | `-1` (globale) | –, –, **1** | — | `[0,3,1]` → PLAYER, STRIBASE, STERN |
| 1 | 0 Strike Base | – | (-70762, 126343, **3580 ft**) | `[7]` → RWYEXT18 (runway) |
| 2 | 1 Restricted Airspace | – | — | `[4,5,6]` → MiG-21R ×3 |
| 3 | 3 Nouakchott | – | (-130000, -56500, **1000 ft**) | `[]` |
| 4 | 2 Cargo Destination | – | — | `[8]` → RWYEXT18 (runway) |
| 5 | 4 Rendezvous | **13**, –, – | — | `[]` (le C-130 est activé ici par `PROG` 13) |

Le C-130 (entity 8) n'apparaît dans aucune queue de `SCNE` → il est spawné/activé
par script.

##### Struct `MissionScenario` (offsets confirmés)

| Offset | Champ |
|---|---|
| `+0x09` | nom de mission (char[≤16], « UNK ») |
| `+0x1A` | liste `AREA` |
| `+0x1E` | liste `SPOT` |
| `+0x24` | `NUMS` (variables numériques) |
| `+0x28` | `MSGS` (messages / dialogues) |
| `+0x2E` | `CAST` (distribution / personnages) |
| `+0x34` | `PART` (participants / factions) |
| `+0x3A` | objet `PLAY` (la pièce) : `[+0]` = nb de scènes, `[+2]` = tableau de records `SCNE` (0x27 o chacun), `[+4]` = contexte |
| `+0x40` | `PROG` (programme / didascalies) |
| `+0x48` | `FLAG` (flags d'événement) |
| `+0x4E` | handle de nœud Expr (accès à l'objet `PLAY` `+0x3A`) |
| `+0x50` | flag « HOME depuis nœud » |
| `+0x54` | far ptr vers l'objet de départ joueur résolu |
| `+0x58` | index `AREA` de la base (`0` = résoudre via nœud) |
| `+0x5C` / `+0x60` / `+0x64` | vecteur position `HOME` (X, Y, Z ; Z = altitude, u32) |
| `+0x68` | scratch tokens / formatage (Lexer) |
| `+0x74` | tableau `TEAM` (dwords, ≤8) |
| `+0x84` | nombre d'équipes (word) |
| `+0xA1` | (par MISN2OP) 8× u16 de stats par équipe |
| `+0xB1` | tableau d'objets joueur `PLYR` (lié par OP2MISN) |

##### `DATA\OP2MISN.IFF` — FORM `INVN` (par `LoadFromIFF_A8C44`) — pont campagne → mission

| FourCC | Lecteur | Cible | Contenu |
|---|---|---|---|
| `INFO` | `String_ConstructFromPtrLen(si)` | `si+0x00` | description |
| `NAME` | `ReadFieldGroupA` N≤16 | `unk_6E4D9` | nom du pilote |
| `SNAM` | `ReadFieldGroupA` N≤16 | buffer « AIRHEAD » | escadrille / nom court |
| `CALL` | `ReadFieldGroupA` N≤16 | buffer « SUTEK » | indicatif |
| `TEAM` | boucle ≤3 × `String_ConstructFromPtrLen(si+0x86 + i*9)` | `si+0x86` | jusqu'à 3 noms d'équipe (9 o) |

Puis `LoadMainRecord_A8331(si)` (charge la mission), `ResolveFieldEntry_A8B37` →
compte de participants, puis boucle de liaison des enregistrements `PLYR` (chunk
lu via `enter_form('PLYR')` + `sub_6CF2B`/`sub_6CF21`, tableau `si+0xB1`) sur les
nœuds participants, clé = id objet `+0x52`. Erreur `901Ah` si `INVN` absent.

##### `DATA\MISN2OP.IFF` — FORM `INVN` (par `ResolveAndBindExpressions_A8F22`, **écriture**)

Utilise les primitives **d'écriture** (`WriteFieldGroupA/B`, `WriteTyped`,
`AllocateAndDecodeVariant`). Persiste le résultat de mission :

| FourCC | Contenu écrit |
|---|---|
| `INFO` | 8 o depuis `si` |
| `STAT` | 8× u16 depuis `si+0xA1` (stats par équipe) |
| `JETS` | par participant : marqueur si nœud `+0x39` bit posé **et** type `+0x00` == `"F-16"` (`0x36312D46`) |
| `KILL` | par participant : u16 `+0x3E`, u16 `+0x40`, descripteur 8 o via `Expr_LookupBuiltinFuncDesc_52C5F` |
| `EXIT` | 1 o depuis `byte_706AF` |
| `DIED` | par équipe : résolution via `Expr_Node_LookupAndConvert_52D69(si+0x34)`, test du bit `+0x39` |
| `SCOR` | score (suite non détaillée) |

##### Liaison Expr

`ExprSlot_*` (seg457, 46 fonctions) = slots de valeurs nommées du scénario :
`ExprSlot_LoadAndResolveNames_A9E3C` résout les noms via
`Expr_LookupNamedValue_51E4A` (seg114) ; `ExprSlot_ConstructMain_A974A` construit
l'ensemble. `MissionScenario_RecomputeFields_A9382` /
`QueryAndResolve_A8A87` réévaluent les champs à l'exécution
(`Expr_Node_RecomputeFieldC_53211`, `Expr_LookupTableEntry85B_52D45`).
`MissionScenario_DecodeFieldA/B` (`A9B14`/`A9BD8`) décodent des champs
compressés/codés (`sub_2B6D`).

**Terminologie « théâtre »** : le chunk de scène s'appelle `SCNE` (octets disque
`S C N E`, `0x454E4353`), contenu dans le FORM `PLAY` (la pièce), lui-même dans le
FORM `MISN`. Les fonctions `ExprSlot_*` de seg457 nommées ainsi dans une session
précédente sont en fait les **lecteurs des listes de la pièce** :
`sub_6CF0D→A9956` (CAST), `sub_6CED6→A974A` (AREA), `sub_6CEE5→AA6C9` (SPOT),
`sub_6CEF4→AA1BE` (NUMS), `sub_6CF03→AA10D` (MSGS), `sub_6CE9F→A9A89` (FLAG),
`sub_6CEB3→AA31D` (PROG), `sub_6CF35→AA23D` (PART), `sub_6CF58→AA595` (PLAY/SCNE).

#### b) Monde / carte — chaîne de chargement depuis le chunk `WRLD`

**Chaîne exacte** (mission → carte) :

```
MissionScenario_LoadMainRecord_A8331 (seg456)
  chunk MISN/WRLD  →  enter_form('WRLD')  →  seek_chunk('FILE')
     FILE présent  →  read 8 o (nom, ex. "MAURITAN")  →  sub_6CDCD
                                                          └─(stub VROOMM)→ MissionLoader_OpenAndDispatch_A75D0 (sub_A75D0, seg455)
     FILE absent   →  sub_6CDC8 ─(stub)→ MissionLoader_LoadEntitiesMain_A767F  (monde inline)
```

**`MissionLoader_OpenAndDispatch_A75D0` = la fonction appelée sur le fichier du
chunk `WRLD`** :
1. `StreamReader_ConstructVariantA`
2. `Path_ResolveDataFile(path, "MISSIONS", <nom FILE>, ".IFF")` → **`..\..\DATA\MISSIONS\<nom>.IFF`**
   (le fichier monde est dans le même dossier que la mission)
3. `ResourceRecord_InitAndValidate(&sr, path, 'WRLD')` → ouvre + **exige FORM racine `WRLD`**
4. `MissionLoader_LoadEntitiesMain_A767F(&sr)` → parse le contenu
5. cleanup (`rewind_all`, `ReleaseBuffer`)

**`MissionLoader_LoadEntitiesMain_A767F`** (239 L, `sub_A767F`) = le parseur du FORM
`WRLD`. Garde `byte_70446` (chargé une fois) ; initialise les mêmes globales que
l'écran de mission-planning seg112 (`byte_70447/70471`, `dword_70468`,
`byte_7046C/7046D`, `word_7046E`). Chunks (vérifié sur `MISSIONS\MAURITAN.IFF`) :

| FourCC | MAURITAN | rôle |
|---|---|---|
| `INFO` | `0x0320` (u16) | métadonnées / compteur |
| `PALT` | `"PALETTE"` | → `DATA\PALETTE.IFF` |
| `TERA` | **`"mauritan"`** | **nom des données de terrain** → `DATA\<nom>.PAK` (voir §6.8) |
| `SKYS` | `"SUN"` + couleur/dir/intensité | ciel / soleil |
| `GLNT` | 7 dwords + table d'index palette + entrées de fondu (270 o) | carte de reflets spéculaires (glint soleil sur eau/métal) |
| `SMOK` | `"SMOKESET"` | jeu d'effets de fumée |
| `LGHT` | 14 o (`03 00 64 00 05 00 f6 ff e7 ff 64 00 01 00`) | paramètres d'éclairage |
| `FORM CAMR` | 882 o : `STRT`/`CHAS`/`CKPT`/`VICT`/`TARG`/`WEAP`/`ROTA` + records `COMP` (STARTCAM/TAKEOFF/LANDING/AUTOPILT) | **caméras scriptées** — via `MissionLoader_LoadChunkA_A7C8E` → `Cinematic_LoadCameraDef` (`sub_23E7D`, seg041) |
| (autres missions) `HORZ` | 2× u32 → `dword_704DD/704E1` | horizon |
| (autres) `WTCH` | u16 → `WeakRef_TableConstruct` | table d'entités « watch » |
| (autres) `SCAL` | u32 → `dword_7044C` ; calcule `dword_70448 = 256²/scale` | échelle du monde |

Sous-loaders appelés en séquence (chacun cherche ses propres chunks) :
`LoadChunkA_A7C8E` (→ `Cinematic_LoadCameraDef`), `LoadWaypoint_A78AD`,
`LoadFormation_A78F5` (via `IndexedRecordReader`), `SpawnEntity_A7B1E`,
`LoadPosition_A7A10`, `LoadChunkB_A7CA1`. `SpawnEntity` instancie un objet du monde
(`sub_38B70`, motif de spawn seg109), lit 3 champs u32, l'enregistre dans le
conteneur global (`sub_21F8D`, tag `59C3h`).

#### b-bis) Caméras scriptées — `FORM CAMR` (dans `WRLD`)

Chargé une seule fois par monde (`byte_70446`) par `MissionLoader_LoadChunkA_A7C8E`
→ **`Cinematic_LoadCameraDef` (`sub_23E7D`, seg041, 338 L)**, appelé
`Cinematic_LoadCameraDef(listeManager = 0x59CD, resourceRecord)`.
`ResourceRecord_SeekAndReadB('CAMR')` ; si absent → retour immédiat.

**Layout d'un sous-chunk `CAMR`** (décodé sur `MISSIONS\MAURITAN.IFF`, offset
fichier `0x18E`, `FORM CAMR` = 882 o) — en-tête commun aux caméras simples :

```
+0x00  char[8]   nom interne   "CHASECAM"/"COCKPIT"/"VICTIM"/"AUTOTRAC"/"WEAPON"/"ROTATCAM"
+0x08  u16       flags = 0     (CHASECAM : 14 o à 0 ici — slot look-at secondaire vide)
+0x0A  char[8]   sujet         entité porteuse = "PLAYER"
       (CKPT)  +0x12 char[8]   nom d'art cockpit = "F16-CKPT"
+..    u32       farClip = 50000 (0xC350)   — constant, pieds
+..    u16       fov     = 40 (0x28)        — constant
+..    u8/u16    nearClip (contient 0x0A = 10)
+..    u32       0
+..    u16,u16   viewport = 319, 199 (0x13F, 0xC7) = rect de rendu (w-1, h-1)
+..    i32[]     (VICT / WEAP)  table d'offsets — FIXED-POINT 24.8 (val/256 = pieds)
```

Table d'offsets `WEAPON` (÷256, pieds) : `-10, 20, -30, -8, 1.5, [153], -4, [307],
0, -6, 10, 2.5, 20, 5, [153], 2.5, 6, 0.5, [-153], -0.5, 102` — vecteurs de
keyframe/boîte relatifs au sujet ; les valeurs entre `[]` (octet bas non nul, ex.
`0x99`=153, `0x66`=102) sont de petits params séparés (angles / marqueurs), **pas**
du 24.8.

**`COMP`** (STARTCAM / TAKEOFF / LANDING / AUTOPILT) : même en-tête + `u8` compte +
`char[8]` sujet, puis un **script de keyframes à longueur variable** (opcodes 1 o,
petits int signés = pitch/yaw/durée, offsets `i32/256`), terminé par l'octet
**`0xFE`** suivi d'un `char[8]` = vue de reprise après la séquence (`"COCKPIT"`).
Ex. `LANDING` : `-7680, 5120, 30, -650, 375` (÷256) parmi ses keyframes.

**Tags reconnus** (`record[+0x6D]`, dword LE) → code de type (`var_4`) :
`STRT`→(string, nom du jeu de caméras, stocké `liste+6`), `CAMR`→0, `ATCH`→1,
`CHAS`→3, `CKPT`→4 (+ `stub 6B120` → objet ; `word_6E4B2 = obj[+0x5C]`), `CONT`→6,
`VICT`→7, `ROTA`→8, `TARG`→9, `WEAP`→0x0B, `COMP`→0x13 ; inconnu → 0x14 (ignoré).

**Ce que `Cinematic_LoadCameraDef` stocke** : pour chaque tag reconnu, une entrée de
registre (26 o, type `0x119C`) ajoutée en queue de la liste `0x59CD`
(`LinkedListB_InsertAtTail_5F57F`) via `Kneeboard_EntryConstruct{Text,Obj}` :

| off | rôle |
|---|---|
| `+0x04` u8 | code de type de caméra (table ci-dessus) |
| `+0x05` 8 o | ID/clé = **8 premiers octets du payload** (`COMP` : le nom `"TAKEOFF\0"` etc. — d'où la sélection par nom) ; `CKPT` : `obj[+0x53]`,`obj[+0x57]` (ID composite 2×u32) |
| `+0x0E` u16 | réf objet (`CKPT` seul, sinon 0) |
| `+0x10` far ptr | **payload brut du chunk**, copié verbatim (`ResourceRecord_ReadFieldGroupA`, longueur `record[+0x72]`) |
| `+0x16` u32 | longueur du payload |

Le loader **ne décode pas** les paramètres — seuls les 8 octets d'ID et le tag sont
lus. Le payload (offsets 24.8, script `COMP`) est consommé à l'activation.

**Objet caméra runtime — `word_72A8F`** (caméra active courante ;
`DATA XREF: Player_MainUpdate`). Layout partiel vérifié (`HUD_RenderSymbologyMain`/
`…Alt` seg088, `Debris_SpawnOrchestrator` seg079) :

| off | type | rôle |
|---|---|---|
| `+0x14` / `+0x18` / `+0x1C` | i32 | **position X / Y / Z** monde (Z = altitude), **fixed-point 24.8** (÷256 = pieds) |
| `+0x11` | u8 | code d'état (via `[vtable+0x14]` ; `4` = mode spécial) |
| `+0x50` | far ptr | **vtable** — `[0x00]` tick/avance keyframe, `[0x04]` stop précédente, `[0x10]` start (parse le script), `[0x14]` get state |
| `+0x5C` | ptr | sous-objet (cible secondaire) |
| `+0x60` | dword | param (`Kneeboard_ProcessSelected`) |

Preuve 24.8 : partout `delta = entité[+0x12/+0x16/+0x1A] − caméra[+0x14/+0x18/+0x1C]`
→ `Math_VectorLength3D_Raw_5828E` → **`sar eax, 8`**. Les entités portent leur
position en `+0x12/+0x16/+0x1A`, la caméra a 2 octets de plus avant.

**Chaîne d'activation** (tracée) : `Kneeboard_SelectByID(0x59CD, "<nom>")`
(`sub_23C4F`) sélectionne par nom 8 c —
`"TAKEOFF"` ← état de vol seg009 (`entité[+0x11] == word_722E6`),
`"AUTOPILT"` ← `UIScript_ParseAndEvaluate_7A054` (ovr233) puis
`Camera_DetachParent(word_72A8F)` + reset `word_7046E`/`dword_70468`/`byte_7046C`,
par state-code ← `Player_MainUpdate` (`Kneeboard_SelectByStateCode(0x59CD, 4)`),
par script mission ← `Expr_VM_OpcodeHelperA_53504` (opcode VM `+0xA38` :
`Kneeboard_FindByID` + `Kneeboard_ApplySelection`).
`Kneeboard_ApplySelection` (`sub_23CCC`) pose `manager[0x12]` = entrée active,
lance `Cockpit_PanAzimuthUpdate` → `Camera_PanTransitionController` (pan doux, seuil
`0x100`), puis pilote l'entrée via sa vtable `+0x50`.

#### Interpréteur du script `COMP` — `AI_ManeuverSolutionMain_781D0`

Le byte-code `COMP` est exécuté par **`AI_ManeuverSolutionMain_781D0`** (ovr232,
2373 L) = slot `[vtable+0x10]` (« start/solve ») de l'objet caméra. **Découverte
structurelle** : c'est la **même classe et le même solveur que les manœuvres de
combat de l'IA** — vtable commune `off_6D510` (seg339:0x460), constructeur
`AIManeuver_LoadFormationDataB_77FCA` (stub `6AC3E`), en-tête parsé par
`AIManeuver_Helper_780A5`. (Réponse partielle à la question ouverte #3.)

**Chargement.** `Kneeboard_RenderEntry` (case `0x13`) mappe le payload brut, appelle
`sub_77FCA` (alloc 0x102 o, `[obj+0x50]=0x460`, en-tête 0x1C → `sub_780A5`), puis
pose `[obj+0x80]` = `payload+0x1C` (début script), `[obj+0x86]` = `len-0x1C`.

**Boucle.** Curseur far ptr `[obj+0x8A:0x8C]` (init depuis `[obj+0x80]`). Lit 1
octet d'opcode → switch 30 cas (valeurs `word_79C6F`, sauts `off_79CAB`) → exécute →
opcode suivant, jusqu'à `0xFE`. Opérandes `i16`/`i32` LE **signés**. **Convention** :
les opérandes de position/vecteur sont des **entiers** dans le fichier ; le moteur
fait `shl …,8` pour sa maths interne 24.8 (noté « `<<8` »). `raw` = stocké tel quel.

| op | taille | opérandes | effet |
|----|--------|-----------|-------|
| `01` | 5 | `i32 a` | `obj+A0=a` (raw) ; mode `obj+FE=1` |
| `02` | 13 | `i32 x,y,z` | **position absolue** → `obj+14/18/1C` (`<<8`) |
| `03` | 1 | — | reconstruit matrice d'orientation (`Matrix_BuildFromAngle`, `obj+20`) |
| `04` | 11 | `i32 a ; i16 x,y,z` | `obj+A0=a` (raw) ; `obj+E4/E8/EC` (`<<8`) ; mode=4 |
| `05` | 7 | `i16 x,y,z` | `obj+D8/DC/E0` (`<<8`) |
| `06` | 13 | `i32 a,b,c` | `obj+C0/C4/C8` (raw) |
| `07` | 9 | `char[8] nom` | **lie l'entité de référence** `obj+92` par nom (ex. `"PLAYER"`) |
| `08` | 5 | `i32 a` | `obj+A0=a` (raw) ; mode=8 |
| `09` | 5 | `i32 a` | `obj+BC=a` (raw) |
| `0A` | 13 | `i32 x,y,z` | `obj+B0/B4/B8` (`<<8`) |
| `0B` | 5 | `i32 a` | `obj+A0=a` (raw) ; mode=0x0B |
| `0C` | 5 | `i32 a` | `obj+BC=a` (raw) |
| `0D` | 13+N | `i32 p,q,r` puis octets jusqu'à `0x00` | `obj+A4/A0/A8` puis division 24.8 → `obj+A8/AC` |
| `0E` | 13 | `i32 x,y,z` | **position relative à l'entité liée** : `<<8`, tourné par l'orientation de `obj+92` + position de `obj+92` → `obj+14/18/1C` |
| `0F` | 5 | `i16 h ; i16 k` | `obj+F0=h` (raw) ; `obj+F2=k` (`<<8`) ; angles d'approche → `obj+F6/FA` |
| `10` | 1 | — | si `obj+92` : `AI_ComputeGeometryHelper(obj+20, entité)` |
| `11` | 7 | `i16 a,b,c` | axes d'orientation (`Matrix_BuildAxisZ/X/Y`, `<<8`) + `Matrix_ApplyToVectorY` |
| `12` | 13 | `i32 x,y,z` | `obj+94/98/9C` (raw) |
| `13` | 1 | — | `obj+FF=1 ; obj+100=0` |
| `14` | 5 | `i32 s` | `obj+C0/C4/C8 = lignes_orientation(obj+2C) × s` (24.8) |
| `15` | 13 | `i32 x,y,z` | `obj+CC/D0/D4` (raw) |
| `16` | 5 | `i32 s` | `obj+CC/D0/D4 = lignes_orientation × s` (24.8) |
| `17` | 5 | `i32 d` | `obj+C0/C4/C8 = (chaque `<<8`) / d` (division 24.8) |
| `18` | 1 | — | si `obj+92` : `entité->vtable[0x4C]` → `obj+C0/C4/C8` (vitesse entité) |
| `19` | 1 | — | si `obj+92` : tourne `obj+C0` par l'orientation de l'entité |
| `1A` | 1 | — | si `obj+92` : tourne `obj+CC` par l'orientation de l'entité |
| `1B` | 1 | — | `byte_70471=0 ; obj+101=1` (utilisé par `STARTCAM`) |
| `1C` | 1 | — | si `obj+92` : `obj+BC = ‖pos_entité − obj+14‖` (distance) |
| `1D` | 17 | `i32 a,x,y,z` | `obj+A0/E4/E8/EC` (raw) ; si `obj+92` : `obj+B0/B4/B8 = pos_entité − obj+14` ; mode=0x1D |
| `FE` | 9 | `char[8] vue` | **FIN** ; `word_72A8C = Kneeboard_FindByID(0x59CD, vue)` → enchaîne sur la caméra nommée (`"COCKPIT"`) |

Opcode inconnu → ignoré (avance de 1).

**Champs de l'objet manœuvre/caméra** (0x102 o) :

| off | rôle |
|---|---|
| `+0x14/18/1C` | position monde (24.8) — **sortie** |
| `+0x20` | matrice d'orientation 3×3 (lignes `+0x20`, `+0x2C`, `+0x38`) |
| `+0x50` / `+0x53` / `+0x5C` | vtable (0x460) / nom / sous-objet viewport (0x44C) |
| `+0x80/84/86` | script : base / kind / longueur |
| `+0x8A:8C` | **curseur vivant** (far ptr, persiste entre frames) |
| `+0x8E` | compteur de temps écoulé (`+= dword_70458` / frame) |
| `+0x92` | entité liée (weak ref ; opcode `07`) |
| `+0x94/98/9C` | offset d'ancrage dans le repère entité (opcode `12`) |
| `+0xA0` | compteur du segment (`−= dword_70458` / frame sauf mode 1) |
| `+0xA4/A8/AC` | compteur / accumulateur / taux (opcode `0D`) |
| `+0xB0/B4/B8` | position courante **relative à l'entité liée** |
| `+0xBC` | distance de recul le long de l'axe avant |
| `+0xC0/C4/C8` | vitesse linéaire (→ position via `dword_7045E`) |
| `+0xCC/D0/D4` | accélération linéaire (→ vitesse via `dword_70458`) |
| `+0xD8/DC/E0` | taux de rotation (→ orientation via `dword_70458`) |
| `+0xE4/E8/EC` | vecteur cible du segment (opcodes `04`/`1D` ; orient. cible mode 4) |
| `+0xF0/F2/F6/FA` | angle d'approche : param / restant / accum Z / accum X (opcode `0F`) |
| `+0xFE` | **mode** : `0xFF` = exécuter des opcodes ; `1/4/8/0xB/0x1D` = mouvement actif |
| `+0xFF` / `+0x100` | recalcul d'ancrage depuis `+0x94` / activation du chemin ancrage-visée |
| `+0x101` | « interruptible » (opcode `1B` ; avec `byte_722D1==1` → fin forcée) |

**Globales** (calculées seg112 depuis `dword_70454`, défaut `0x1900`) :
`dword_70458 = (0x100<<8)/dword_70454` = **pas de temps** par frame (accel/rotation,
~10 au défaut) ; `dword_7045E` = pas pour la vitesse linéaire (dérivé, un peu plus
petit) ; `byte_722D1` = « sauter la cinématique » ; `word_72A8C` = caméra suivante
en file (posée par `FE`).

**Appel par frame `AI_ManeuverSolutionMain(obj)` → 1 = en cours, 0 = terminé :**

1. Abort : `byte_722D1==1 && obj+0x101!=0` → mode = `0xFF`.
2. Dispatch sur `obj+0xFE` (`word_79CE7` : 1/4/0xB/0x1D) :
   - **1 — maintien temporisé** : fini quand `obj+0x8E ≥ obj+0xA0` → mode `0xFF`.
   - **4 — rotation vers orientation cible** (`obj+0xE4..`) en `obj+0xA0` pas : erreur
     d'angle via `AI_ApplyAngleBetweenVectors`/`AI_ComputeGeometrySolution`/
     `AI_ApplyApproachAngles` ; si `< 0x200` (2.0) → snap (`WorldObject_BuildOrientationMatrix`),
     mode `0xFF` ; sinon tourne de `erreur/A0 · dword_70458`.
   - **8** — pas de handler ; intégrateur générique seul.
   - **0xB — visée/poursuite d'un point** : `dir = obj+0xB0.. − pos` ;
     `ligne_orient(obj+0x2C) ← dir` (+ `dir/A0`/frame) ; `Targeting_LineOfSightCheck` ;
     `Matrix_ApplyToVectorY`.
   - **0x1D — déplacement sur segment** vers `rotate(obj+0xE4.., orient_entité)` :
     `pas = ((cible − obj+0xB0..)<<8)/obj+0xA0 · dword_70458` ; `obj+0xB0.. += pas` ;
     `pos_monde = obj+0xB0.. + pos_entité`. Snap + mode `0xFF` si `obj+0xA0 ≤ 0` ou
     pas d'entité.
   - `0xFF`/autre → étape 3.
3. `mode == 0xFF` → **boucle opcodes** jusqu'à un opcode posant un mode ≠ `0xFF`, ou
   `0xFE` (pose `word_72A8C`, renvoie **0**).
   `mode != 0xFF` → **intégrateur générique** (`loc_796B4`) :
   - `pos += vitesse(C0..) · dword_7045E`
   - `vitesse += accel(CC..) · dword_70458`
   - orientation ← reconstruite depuis `rot_rate(D8..) · dword_70458`
   - si `obj+0x100` : `obj+0xB0.. = rotate(obj+0x94.., orient_entité) + pos_entité`
   - `0D` : `obj+0xA8 += obj+0xAC·dt ; d = obj+0xA8·dword_7045E ; obj+0xA4 −= d ; obj+0xBC −= d`
   - `0F` : consomme `obj+0xF2`, accumule `obj+0xF6/FA`, reconstruit les axes
   - **`pos = obj+0xB0.. − ligne_orient(obj+0x2C..) · obj+0xBC`** (recul de `BC` derrière l'ancrage)
   - sinon (`obj+0x100==0`, `obj+0xFF`, entité) : `UI_ApplyLineOfSightAndTransform` → vise l'entité
   - `obj+0xA0 −= dword_70458` (sauf mode 1) ; `obj+0x8E += dword_70458`
   - `Camera_ComputeViewMatrix(obj)` ; renvoie **1**

**Cycle de vie d'un segment** : les opcodes remplissent des champs **et** arment un
mouvement (mode + cible + `obj+0xA0`). Le moteur exécute ce mode ~`obj+0xA0 /
dword_70458` frames ; le compteur expire → mode `0xFF` → reprise de la boucle
opcodes au curseur → segment `1D` suivant / `FE`.

**Structure typique d'un script** (confirmée sur `MAURITAN.IFF`) :
`[07 "PLAYER"]` (lie le sujet) `[02|0E x y z]` (position abs/relative)
`[05|0A|11 …]` (vecteurs cible/vitesse/orientation) `[01|04|08|0B|1D …]` (arme un
segment) … `[FE "COCKPIT"]` (fin + vue suivante). `STARTCAM` commence par `[1B]`.

**`STARTCAM` décodé et vérifié** (11 instr., dump du port libRealSpace, plan
d'ouverture de mission — la caméra balaie horizontalement autour de l'avion puis
plonge vers le cockpit) :

| # | opcode | opérandes | rôle |
|---|--------|-----------|------|
| 0 | `1B` START_FLAG | — | marque « interruptible » (`obj+0x101`) |
| 1 | `07` BIND_ENTITY | `"PLAYER"` | lie l'avion joueur (`obj+0x92`) |
| 2 | `0E` SET_POS_REL_ENTITY | `0, 2048, 384` | position = pos_avion + rot(cap)·(2048 devant, 384 au-dessus) → 3/4 avant surélevé |
| 3 | `10` COMPUTE_GEOM | — | **orientation caméra `obj+0x20` ← matrice d'orientation de l'avion** (copie via `AI_ComputeGeometryHelper_56E29`) |
| 4 | `11` SET_ORIENT_AXES | `-30, 0, 180` | **compose** (pas remplace) sur `obj+0x20` : pitch −30° (plongée) + lacet +180° → la caméra fait face à l'avion. Les angles sont **relatifs au cap semé en #3**. |
| 5 | `09` SET_DIST | `160` | `obj+0xBC = 160` ; active le chemin ancrage+recul (`obj+0x100`). Pas d'opcode `12` → offset d'ancrage nul → ancre = position avion → caméra en **orbite rayon 160**. |
| 6 | `0F` ARM_APPROACH_ANGLE | `20, 180` | consomme +180° de lacet à ~20/tick pendant le hold = **le balayage rotatif horizontal** |
| 7 | `01` ARM_HOLD | `1536` | durée du balayage (`obj+0xA0`, mode 1) |
| 8 | `0D` DIV_SETUP | `120, 1280, 0` | flag 0 → `div_a8 = 120·256/1280 = 24` ; le décompte de l'intégrateur **fait décroître `obj+0xBC` (dist) 160 → ~47** = **la rentrée / chute vers le cockpit** |
| 9 | `01` ARM_HOLD | `2816` | durée de la rentrée |
| 10 | `FE` END | `"COCKPIT"` | fin → enchaîne sur la caméra `COCKPIT` |

Points confirmés par ce script : (a) l'opcode `10` sème l'orientation caméra
depuis l'entité liée, et l'opcode `11` **compose** son delta par-dessus (les
`Matrix_BuildAxisZ/X/Y_572BC/56EC3/570C5` lisent-recombinent-réécrivent
`obj+0x20`, ils ne l'écrasent pas) → les angles d'orientation d'un script sont
**relatifs au cap de l'entité**, pas absolus ; (b) `SET_DIST` sans `SET_ANCHOR_OFS`
(opcode `12`) → ancrage sur la position même de l'entité, caméra en orbite à
distance `obj+0xBC` ; (c) `ARM_APPROACH_ANGLE` = balayage angulaire progressif ;
(d) `DIV_SETUP` (flag 0) = rampe linéaire qui grignote simultanément `obj+0xA4` et
`obj+0xBC` (ici : rétrécit le rayon d'orbite = mouvement de rentrée).

**vtable** `off_6D510` : `[+0x00]` tick `sub_7818E` ; `[+0x0C]` `AI_ManeuverCleanup_79CFF`
(libère `+0x92` + le buffer script) ; `[+0x10]` = `AI_ManeuverSolutionMain_781D0` ;
`[+0x14]` get-state `sub_78184` ; `[+0x1C]` get-entité `sub_79D7A`.

**Caméras simples procédurales** : `CHAS`/`TARG`/`ROTA` n'ont pas de coord
explicite — `Camera_ChaseComputeMain` (`sub_3D57E`, seg085),
`Camera_ComputeMountedPosition` (`sub_3D31D`) calculent la position par formule
depuis la cible + `dword_7045E` + globales `dword_707F8/FC/800`. Le chunk ne fournit
alors que `{sujet, farClip 50000, fov 40, viewport}`.

#### c) Base d'entités — `MissionRecord_*` (ovr239-257)

`MissionRecord_LoadEntityDatabase_7B035` (**3103 L, plus grosse fonction du
binaire**) : `Path_ResolveDataFile` + `IndexedRecordReader` (variantes A/C) —
charge en masse une base d'entités depuis un fichier indexé (potentiellement LZW).
Tags ASCII vus : `EJECT`, chunk générique **`DATA`** (`0x41544144`, cf.
`PlayerComponent_LoadDataChunk_A6CA0`). `MissionRecord_LoadFullDatabase_86B53`
(816 L, ovr249) = variante `Path_ResolveDataFile` + `InitAndValidate` + lecture
massive de champs. `MissionRecord_LoadAndBuildWidgetTree_7D31A` (1081 L, ovr240) =
même motif + construction de l'arbre de widgets (`UIScreen_BuildWidgetTree_53A94`,
seg114). Les ~50 `MissionRecord_LoadWithDwordFields*` / `LoadFieldGroupMix*` sont
des lecteurs de sous-enregistrements selon la disposition de champs.

### 6.7 Composants d'avion & dégâts

- `AircraftDamageModel_LoadComponents_73B4F` (440 L, ovr228) : StreamReader + IFF,
  **boucle ×8** (`ReleaseComponentArray_742FC`) = les 8 composants confirmés
  (seg339) : `RUDDER`, `ELEVATOR`, `AILERON`, `LWING`, `RWING`, `ENGINE`,
  `LANDGEAR`, `FLAPS`. `LoadFromIFF_73FB4` (293 L) = ~15 champs u32.
- `PlayerComponent_Load*` (seg432-453, ~40 fonctions) : chargement IFF des
  composants d'état / visuels du joueur (`LoadFromResourceRecord_9DF63` appelle
  même `IFF_LoadModelMain`). `LoadFieldsMassive_A5D18` = ~30 champs
  matériau/visuels. `LoadDataChunk_A6CA0` : chunk **`DATA`** + 3 champs u32.
- `AircraftComponent_Load*` (ovr272-285, ~14 fonctions) : sous-enregistrements de
  composants (variantes par disposition de champs — `LoadWithDwordFields`,
  `LoadFieldSequence`, `LoadFieldGroupPair`, …).

### 6.8 Terrain / carte — `DATA\<nom>.PAK` + seg071-077 + ovr290-299

Le chunk `WRLD/TERA` donne un **nom** (ex. `"mauritan"`). Les données de terrain
sont dans **`DATA\<nom>.PAK`** — une **archive PAK** :

```
PAK :  dd  taille_fichier
       dd  (offset | 0xE0000000) × N      ; table des bornes des N sous-ressources
       <données>                          ; la 1re sous-ressource commence à l'offset[0]
```

(Le tag `0xE000` dans les offsets est le même marqueur « far segment » que les
tables de stubs VROOMM.) Vérifié sur `MAURITAN.PAK` (1 044 396 o, 7 sous-ressources ;
`dd[0]` = taille du fichier).

**Sous-ressource `[0]` = `FORM TERA`** (IFF imbriqué), descripteur :

| chunk | MAURITAN | rôle |
|---|---|---|
| `VERS` | `0x0002` | version |
| `INFO` | `01 00 | c0 5d 00 00` (word 1, dword `0x5DC0`) | métadonnées |
| `FORM BLOX` | | référence les 3 LOD d'élévation + les triangles + les objets : |
| ↳ `ELEV` (138 o) | `0F 00 | E8 03 | 08 00 00 00 | 70 17 00 00 | 12 00 | 14 00 | 20 4e 00 00` + `"MAURITAN.ELV\0"` + `"MAURITAN.AVG\0"` | dims `0x12×0x14` (18×20) |
| ↳ `ATRI` (53 o) | `12 00 | 20 4e 00 00 | 01 00 | 28 23 00 00 | …0… | 20 4e 00 00 | …0… | "MAURITAN.TRI\0"` | overlays (voir §6.8bis) |
| ↳ `OBJS` (27 o) | `12 00 | 20 4e 00 00 | 28 23 00 00 | 20 4e 00 00` + `"MAURITAN.OBJ\0"` | grille d'objets |
| `FORM TXMS` (240 o) | `INFO` (`FF 00 00 00`) + `MAPS` (noms de textures sol par LOD : `BEACH`/`MDBEACH`/`LOBEACH`, `GRS2RTH`/`MDGR2B`…) | set de textures de terrain |

**Sous-ressources `[1..6]` du PAK — vérifié sur `MAURITAN.PAK`** :

| # | nom | taille | format |
|---|---|---|---|
| `[1]` | `.ELV` | 778 900 o | **PAK imbriqué de 324 cellules (grille 18×18)** ; chaque cellule = 2400 o = **20×20 sommets × 6 o** (`u16 a, u16 h (hauteur), u16 c`) |
| `[2]` | `.MED` | 195 700 o | idem, LOD moyen |
| `[3]` | `.LOW` | 49 900 o | idem, LOD bas |
| `[4]` | `.AVG` | 15 196 o | PAK imbriqué de 324 cellules (quasi toutes « vides » → valeur moyenne/couleur uniforme par cellule) |
| `[5]` | `.TRI` | 3 480 o | **PAK imbriqué de 324 cellules** ; cellules non vides = régions de géométrie de piste/aire, parsées par `sub_99120` (§6.8bis) |
| `[6]` | `.OBJ` | 648 o | **tableau plat de 324 `u16`** (1 par cellule 18×18), champ lisse ~0→6510 — candidat : altitude/couleur de base par cellule |

**PAK imbriqué** : `dd taille` puis `dd (offset|0xE0000000) × 324` ; les cellules
vides pointent toutes vers le même offset (fin de table). 18×18 = 324 = grille de
cellules ; 20×20 = sommets par cellule.

Parseurs (cluster `Terrain_*`) :
- `Terrain_LoadHeightmapFromIFF_998C0` (ovr292) = **lecteur du chunk `ATRI`** :
  `seek_chunk('ATRI')` → `ReadFieldGroupA(rr, hdr, 0x35)` (en-tête 53 o) →
  `sprintf(nameBuf, hdr+0x28)` (le nom `.TRI`) →
  `Terrain_HeightmapAllocate_33346(terrain, rr, hdr[0]=dim, nameBuf)`.
- `Terrain_HeightmapAllocate_33346` (seg072) : alloue une grille `dim×dim` de `u16`
  (`terrain[+0x03]`, `dim` = `terrain[+0x07]`, `dim²` = `terrain[+0x09]`) ; **résout
  l'extension du nom** (`.ELV`→1, `.MED`→2, `.LOW`→3, `.AVG`→4, `.TRI`→5) →
  `IndexedRecordReader.AdvanceIndex(rr+0x208, idx)` → positionne sur le blob ;
  contrôle de compression `[rr+0x275] & 0xC0 == 0xC0` ; puis
  `StreamReader_ConstructAndBind_63B23(terrain+0x0B, rr+0x208, pos, pos+len)` →
  **sous-flux borné sur le blob, rangé dans `terrain[+0x0B]`**. `terrain[0]=1`.
  Variantes `…B_99CEF` (ovr294, avec StreamReader), `…C_9AC10` (ovr297).
- **`ovr299` (`Terrain_BuildAndLoadTilePath_9AECE` + voisins)** : lecteurs qui
  consomment `terrain[+0x0B]` (très nombreux `add ax, 0Bh`) — parsing des
  cellules ; non tracé ligne à ligne.
- `Terrain_SectorLoadOnDemand_33607` (seg073) : streaming par secteur.
- `DATA\MAPDATA.PAK` (325 Ko, même format PAK) : données de carte globales.
- Chaînes seg339 : `MAPGRI-I.CPP`, « No mem for vertices and triangles »,
  « Terrain Vertex overrun - mem trashed! ».

#### 6.8bis Overlays `.TRI` — géométrie des pistes/aires d'aérodrome (vérifié via `sub_99120`)

**Chaîne** : chunk `ATRI` (`FORM TERA`/`BLOX`) → en-tête 53 o lu par
`Terrain_LoadATRIChunk_998C0` (ovr292) → nom `<théâtre>.TRI` → sous-ressource `[5]`
du `<théâtre>.PAK`, elle-même un **PAK imbriqué de 324 cellules** (grille 18×18).
Une cellule non vide est un **blob de géométrie** parsé par
**`TRI_ParseOverlayGeometry_99120`** (`sub_99120`, ovr291, 599 L), appelé **via le
stub `sub_6BD30` depuis `Model_RenderMain` (`sub_31E87`, seg070)** — chemin de
rendu de modèle, **pas** le pipeline terrain. Plusieurs cellules pointent vers la
même « région » (données de géométrie partagées, réinstanciées à des positions
monde différentes).

**Structure d'une région, dans l'ordre du flux** (offsets vérifiés ligne à ligne
sur `sub_99120`) :

| # | section | taille | contenu |
|---|---|---|---|
| 0 | en-tête | 8 o | `u16 numvertice`, `u16 nbpoly`, `u16 param2`, `u16 param3` (4× u16 LE) |
| 1 | sommets | `numvertice × 13` o | `i32 X` @+0, `i32 Y` @+4, `i32 Z` @+8 (Z = altitude), `u8 pad` @+0xC |
| 2 | polygones | `nbpoly × 16` o | `u8 flag0` @+0, `u16 idx0` @+1, `u16 idx1` @+3, `u16 idx2` @+5, `u8 type` @+7, `u16 color` @+8, `UV uv[3]` @+0xA (3× `{u8 u; u8 v;}`) |
| 3 | table param3 | `param3 × 2` o | suite de runs `[count, polyIdx_0 … polyIdx_{count-1}]` ; `0xFFFF` = « liste vide » |
| 4 | grille | `param2² × 2` o | chaque case = index (en mots) dans la table param3 |

- **Coordonnées sommet = fixed-point 24.8** : `valeur / 256` = pieds monde (même
  système que `SCNE` / `JDYN` / `HOME`). Ordre **X, Y, Z** (Z = altitude, 3ᵉ composante).
  `pad` @+0xC : per-vertex, valeurs observées `0` / `0x4C` / `0xB1` — rôle non figé
  (corrélé texturé/bord).
- **Polygone** : `stride 16 o`, **toujours 3 indices** — le format Strike Commander
  n'a **pas** de mix triangle/quad (le pointeur source avance de `0x10` sans
  condition ; le mix quad est une évolution *Pacific Strike*). Les indices sont
  résolus `× 30` (stride du record de sortie) au moment du rendu.
- **`type`** @+7 : recopié en tête du record de sortie (`+0x00`). Sur `ALASKA.TRI` :
  région 0 → `type=6` partout ; régions 1‑2 → `type=6` sur les faces à-plat,
  `type=255` sur les faces texturées. Hypothèse de travail : sélecteur de mode de
  rendu (texturé / à-plat).
- **`color`** @+8 : index (palette, ou texture via lookup `word_7254E`/`word_72550`
  selon la plage) ; `0x0E ≤ color ≤ 0x11` → face **double-face**. `color == 0xFF`
  ⟺ `uv == (0xFF,0xFF)` (100 % corrélé) = face à-plat, sans texture.
- **`uv`** : entiers, plage observée `0..125` (`0xFF` = sentinelle). `125`/`128` ≈
  carreau plein.

**Table param3 + grille** (queue de `sub_99120`, `loc_99519`..`loc_9961A`) :

- La **table param3** (`param3` mots) est lue telle quelle et **conservée**
  (`[di+1Ch]`).
- La **grille** (`param2²` mots) est lue dans un buffer **temporaire**. Pour chaque
  case `c` : `g = grille[c]` ; si `param3Table[g] == 0xFFFF` → case vide (pointeur
  `NULL`) ; sinon la case donne `count = param3Table[g]` puis les `count` indices
  de polygone `param3Table[g+1 … g+count]`. Résultat : `param2²` pointeurs vers
  l'intérieur de la table param3 (`[di+18h]`) ; le buffer grille brut est libéré.
- **Rôle** : index spatial de culling/LOD. `param2 = 1` → 1 case listant tous les
  polys → dessiner `0 … nbpoly-1` dans l'ordre. `param2 > 1` → dessiner l'union des
  listes des cases visibles.
- La position du flux est sauvée à l'entrée (`var_4`) et restaurée en sortie
  (`StreamReader_SeekVariant`) : chaque entrée `.TRI` se parse indépendamment ; ne
  pas lire ces deux tables ne décale rien.

**Vérifié sur `ALASKA.TRI`** (15 196 o, standalone) : PAK imbriqué de 324 entrées,
**3 régions distinctes** de 4 632 o, chacune `numvertice=192 nbpoly=118 param2=1
param3=119`, `param3Table = [118, 0, 1, 2, … , 117]`, `grille = [0]`. Y = 2521 /
1576 / 456 ft → 3 aérodromes ; géométrie **strictement identique** aux 3 (même
modèle de piste réinstancié), seul l'octet `type` diffère. Le parseur générique
`scratchpad/tri_parse_general.py` consomme les 3 régions exactement.

> **Point ouvert** — un passage antérieur avait lu `MAURITAN.TRI` comme une liste
> de placement d'objets (runs `[u16 count]` + records ~70 o portant des **noms de
> modèles** ASCII : `HANGB22`, `TOWA24`, `OILOF23`, `OFFA10`, `FAC14`, variantes
> `…D`). Cette lecture n'a **jamais été rattachée à un parseur** et est
> **contredite** par `sub_99120` + la confirmation côté données (les pistes de la
> carte sortent bien de ce chemin). À réconcilier : soit l'observation portait sur
> une autre ressource, soit le placement des bâtiments d'aérodrome passe par un
> chunk distinct (candidat : `OBJS` → `<théâtre>.OBJ`), soit format spécifique à un
> théâtre. Les bâtiments eux-mêmes restent des `OBJECTS\*.IFF` (§6.1).

### 6.9 Config & sauvegarde

- `Config_LoadOrCreate_26F2B` (seg047) + `Config_LoadAndApplySettings_8EEBC`
  (610 L, ovr265) : fichier **`sc.cfg`** (confirmé seg339) via StreamReader typé ;
  `Config_ReadCalibration_26F7E` = calibration joystick selon le mode
  (`byte_722D0/D1/D3`). `Config_ReadSettingsField_8FBE7` /
  `WriteSettingsField_8FB73` = champ typé r/w.
- `SaveGame_ReadFile_A1173` / `WriteFile_A10C0` (seg441) : bascule mode vidéo →
  `File_Open/Create` → `File_Read/WriteOrDelegateToPaged` (support gros fichier via
  mémoire paginée) → `Close`. **Pas de format IFF** : blob binaire direct.
- `Cockpit_LoadAndInitFull_90D20` (1390 L, ovr267) : `COCKPITS\*` + `EMS_MapPage`
  (le cockpit vit en mémoire paginée) + `UI_ApplyEncodingHelper` + StreamReader.
  Écrans d'options `.shp` (seg339) : `joycalib`, `mouse`, `flitopt`, `mainopt`,
  `detlopt`, `restart`, `gameopt`, `cockopt`, `camropt`, `audiopt`.

### 6.10 Textes / dialogues / audio

- `MissionText_LoadAndFormatFromIFF_A636C` (392 L, seg450) : IFF compressé →
  `TextLabel_LoadFieldsExtended_A61D0` + mesure multi-ligne
  (`Text_ComputeMaxLineWidth_5E6BF` / `…Height_5E706`) + `ReadPayload`.
- `MissionText_LoadSubtitleRecord_87A60` (ovr251), `…B_94710` (ovr270),
  `MissionText_DrawTypewriterSynced_975CC` (ovr287) : sous-titres synchronisés
  audio/vidéo (`AV_SyncComputeOffset`, `Text_TypewriterDrawChar`).
- `TextCycler_*` (seg454) : file de dialogues via `IndexedRecordReader`, sélection
  aléatoire (`TextCycler_LoadFromIndexedReader_A713E`).
- `AudioQueue_*` (seg458-461) : `IndexedRecordReader` + `StreamReader` pour des
  clips audio / messages indexés (voix « Betty », chatter radio). Pilotes
  `roland` / `adlib` / `pas`, fichiers `.adl` / `.rol`, « No mem for XMIDI state
  table. » (seg339).

### 6.11 Divers

- `HUDSymbol_LoadConfigFromIFF_85F65` (ovr247) : config de symbole HUD (IFF,
  `ReadFieldGroupA` ×2).
- `UIWidget_LoadDefinitionFromIFF_8D66C` (394 L, ovr258) /
  `UIWidget_LoadFieldsFromIFF_8DB90` (ovr259) : définitions de widgets (IFF,
  `ReadFinalField` répété).
- `ResourceFile_LoadTypeA…G` (ovr266-269) : **7 grosses fonctions quasi
  identiques** (470–1400 L), même ouverture (`Path_ResolveDataFile` +
  `StreamReader_ConstructVariantA` + `GetCapacity`) — à quels 7 types de fichiers
  elles correspondent reste à établir.

---

## 7. Modèle de données consolidé

### 7.1 Le conteneur générique (format RealSpace = IFF)

```
Fichier .IFF
 └─ FORM <MAGIC>              REAL  → modèles 3D d'objets
                             MISN  → scénario de mission
                             WRLD  → monde / entités de mission
                             DYNM  → modèle dynamique (vol / arme)
     ├─ CHUNK <tag> [taille BIG-ENDIAN] [payload padé sur 2 octets]
     │        └─ champs internes little-endian : u8 / u16 / u32 / N octets / chaîne C
     ├─ FORM <sous-type>      ← push frame conteneur, récursif
     │   └─ …
     ├─ CAT  <type>           ← concaténation d'éléments
     └─ PACK                  ← table d'index [offset u32 × N]  →  enregistrements
                                 de taille variable (option compression LZW 9→12 bits)
```

- **Tailles de chunk : big-endian.** Champs internes : little-endian.
- Navigation = **machine à pile de frames** ; chaque scan (`seek_chunk`) est borné
  par la fin du frame courant (`frame[+0xF]`).
- Le **même format** sert : modèles 3D, modèle de vol, missions, mondes, polices,
  textures, secteurs de terrain, définitions de widgets, textes, config `sc.cfg`.
- `sc.cfg` et l'éditeur de scénario utilisent la **contrepartie en écriture**
  (`WriteTyped` / `WriteField*`).
- La **sauvegarde de partie est hors format** (blob binaire brut).

### 7.2 Graphe d'objets à l'exécution

```
                    Path_ResolveDataFile ( ..\..\DATA\<dir>\<name>.<ext> )
                                     │
        ┌────────────────────────────┼─────────────────────────────┐
        ▼                            ▼                             ▼
  OBJECTS\*.IFF                 MISSIONS\*                    COCKPITS\* , sc.cfg , *.sav
  IFF_LoadModelMain      MissionLoader / MissionScenario / MissionRecord
        │                            │
        ▼                            ▼
  WorldObjectModel          MissionScenario ──(champs numériques)──► Expr VM / Nodes (seg114)
   ├─ vtable/type (BOBJ … DEBR)  ├─ MISN: VERS/NAME/CAST/TEAM/PLYR/LOAD/FLAG/PROG/STAT…
   ├─ INFO: rayon de collision   └─ WRLD: INFO/TERA/PALT/LGHT/SKYS/HORZ/SMOK/SCAL
   ├─ ANGL: angle d'attache            │
   ├─ DYNM/JDYN ──► FlightModel        ├── entités ──► MissionLoader_SpawnEntity
   │     (197 o, fixed 24.8)           │      ├─ position   (LoadPosition)
   ├─ géométrie 3D (chunks OBJT)       │      ├─ waypoints   (LoadWaypoint)
   ├─ textures ──► TextureLoader       │      └─ formations  (LoadFormation, IndexedRecordReader)
   │     (mipmaps)                     │
   └─ AircraftDamageModel : 8 comp.    └── base d'entités ──► MissionRecord_LoadEntityDatabase
      RUDDER/ELEVATOR/AILERON/               (IndexedRecordReader + LZW, chunk DATA)
      LWING/RWING/ENGINE/LANDGEAR/FLAPS
```

### 7.3 Correspondance fichier → format → structure (résumé)

| Asset | Chemin | Format | Structure résultante |
|---|---|---|---|
| Modèle 3D (avion, arme, débris, sol…) | `OBJECTS\*.IFF` | FORM `REAL`/`OBJT` + sous-FORM typé | `WorldObjectModel` 0x35–0x82 selon type + `INFO` (rayon) |
| Modèle de vol | dans le `.IFF` modèle | FORM `DYNM` → chunk `JDYN` | 197 o, fixed-point 24.8 |
| Dynamique d'arme | idem | `DYNM` → `MISS`/`GBMB`/`DURD`/`AGRV`/`SURF`/`PARA` | 0x29–0x5B |
| Dégâts avion | `.IFF` | boucle ×8 chunks | 8 × composant de dommage |
| Texture | `DATA\…` (chemin résolu) | IFF + `Fmemcpy` bloc pixels, mipmaps | buffer + en-tête |
| Police | `DATA\…` (`SM-FONT`) | IFF (def + table glyphes + métriques) | FontObject + GlyphObject[] |
| Mission (scénario) | `MISSIONS\*` | FORM `MISN` + chunks | MissionScenario + ExprSlots (graphe VM) |
| Monde / entités | `MISSIONS\*` | FORM `WRLD` + chunks | entités spawnées dans le conteneur global |
| Base d'entités | chemin résolu | `IndexedRecordReader` (+ LZW), chunk `DATA` | table d'enregistrements typés |
| Secteur de terrain | chemin construit | IFF heightmap + tuiles indexées | grille w×h de mots 16-bit + tuiles |
| Config | `sc.cfg` | StreamReader typé (lecture + écriture) | struct de réglages |
| Sauvegarde | `*.sav` | blob binaire (paginé si volumineux) | image mémoire directe |
| Cockpit | `COCKPITS\*` | IFF + mémoire paginée (EMS) | struct cockpit en pages de 16 Ko |
| Dialogues / voix | chemin résolu | `IndexedRecordReader` (+ LZW) | TextCycler / AudioQueue |
| Écran d'options | `*.shp` | (format SHP, non analysé) | — |

---

## 8. Confirmations issues des chaînes de debug (seg339)

- Format IFF littéral : `Form=%4s`, `Chunk=%4s`, `NumPacks=%ld`, `CurPack=%ld`,
  `PackSize=%ld`, `PackBase=%ld`, `PackLoc=%ld`, extension `.iff`.
- FileStream : `Filename=%s`, `Length=%ld`, `Position=%ld`, `Mode=%s`
  (CREATE / READ / APPEND).
- Composants de dommages : `RUDDER`, `ELEVATOR`, `AILERON`, `LWING`, `RWING`,
  `ENGINE`, `LANDGEAR`, `FLAPS`.
- Pièces d'objet 3D : `FUSELAGE`, `LWING`, `RWING`, `LOWEJECT`, `CHAFF`, `FLARE`,
  `HARDPT#`, `POD`, `GUN`, `BIGSPHER`.
- Armement : AIM-9M/9J, AIM-120, AGM-65D, LAU3, MK20, MK82, Durandal, GBU-15,
  Vulcan (codes SWDM, SWDJ, MVRK, LAU3, MK20, MK82, DRDL, GB15, A120, 20MM).
- Chemins : `..\..\DATA\`, `..\..\data\cockpits\`, `..\..\data\airdens.tbl`,
  `sc.cfg`.
- Terrain : `MAPGRI-I.CPP`, « No mem for vertices and triangles », « Terrain
  Vertex overrun - mem trashed! », « No mem for fade palettes. ».

---

## 9. Zones non lues ligne à ligne (impact sur le modèle)

1. **Modèle de vol de l'avion** (§6.2) — **largement décodé.** Établis :
   hiérarchie IFF `JETP → FORM DYNM → {DYNM masse, ATMO atmosphère, THRS
   poussée+3 u8, STBL coeff stabilité, JDYN 28 champs}` (5 chunks, err `A001`→`A005`) ;
   layout des 28 champs `JDYN` + sémantique de ~24 (carburant `+0x69`/`+0x6D`,
   conso `+0x33`, calage d'aile/volets `+0x4C/4D`, α de décrochage `+0x4B`,
   traînées `+0x37/3B`, traînée induite `+0x5D`, gain de portance `+0x61`, taux
   max `+0x71`, détection sol `+0x4E/52/56/57/58`, effet de sol `+0x59`,
   `jdyn[0x12]`=STBL, `jdyn[0x2C/30/31/32]`=THRS…) ; tick **`PhysicsTicks`**
   (classe `JDYN`) vs `FlightPhysics_TickCandidate_4F4EE` (classes `DYNM`
   simples) ; **bilan de forces en repère corps** :
   `accel = (poussée(0,jdyn[0x28],0) + portance + latéral + traînée)/masse + gravité`,
   puis transform corps→monde (`Physics_IntegratePosition`) ;
   **portance explicite** `Aero_ComputeLiftAndSideForce` = `k·α_eff·q·n̂` (α, β en
   **degrés**, `Cα = 180/π` ; `q = ½·ρ(h)·v²`) ; **courbe manette→poussée**
   `Aero_ThrottleThrustCurve` (piecewise MIL/AF + lapse vitesse) ; asservissement
   d'attitude `±2·√(q'·err)`, `q' = q·jdyn[0x12]/100` ; `g = −9.8 m/s²`
   (`dword_6FFD7`) ; gains globaux `dword_72A14..2C = 1.0` ; carte des entrées
   (accès direct sur les axes, pas de trim ; `flags_75` bits 0/1/2).
   **Restent** : ligne exacte des overrides `vtable[+0x3C]` (masse ≈ chunk `DYNM`)
   / `vtable[+0x34]` (intégration position) dans la vtable `JDYN` primaire ; les
   **valeurs de la table `Cd/Cl vs vitesse-air`** (ressource indexée par
   atmosphère `dword_72A0A`, hors binaire) ; le bloc supplémentaire du chunk
   `ATMO` quand `dword_72A0A≠0` ; `JDYN +0x80..+0x8B` ;
   `Aero_ComputeControlFlags75Bit5C` (composante `c1` du moment) ; le dispatcher
   clavier de la manette des gaz ; layout complet des sous-objets A/B.
2. **Sous-chunks de `OBJT`** — la géométrie 3D elle-même (sommets / faces / LOD /
   sous-objets attachés) est lue dans les branches de `IFF_LoadModelMain` non
   remontées jusqu'au champ.
3. **`MissionScenario`** — chunks `MISN` mappés, `SCNE` décodé et vérifié sur
   MISN-1A (§6.6a : layout disque, queue = index `PART` en ordre de stockage,
   3 handlers `PROG` on_activated/on_leaving/on_update) ; **restent** : le format
   interne de `AREA`/`SPOT`/`PART`/`CAST`/`NUMS`/`MSGS`/`FLAG`/`PLYR` (fonctions
   `A9956`/`A974A`/`AA6C9`/… seg457), le contenu du chunk `PROG` (bytecode Expr VM),
   le rôle exact de `SCNE.heading` (`+0x0A`, candidat cap), et
   `MissionRecord_LoadEntityDatabase_7B035` (3103 L, base d'entités indexée).
4. **`ResourceFile_LoadTypeA…G`** (ovr266-269, 7 fonctions 470–1400 L quasi
   identiques) — identifier les 7 types de fichiers correspondants.
5. **LZW** — la table de dictionnaire n'a pas été tracée (seul le paramétrage
   9 → 12 bits, clear `0x100`, EOD `0x101` est confirmé).
6. **Format `.shp`** des écrans d'options — non analysé.
7. **En-têtes exacts** des formats texture et police (nombre et ordre des champs).
8. **Cluster `PlayerComponent` (seg432-453)** — confirmé comme lecteur générique
   de paramètres de véhicule/dynamique (§6.2) ; les ~40 variantes de
   constructeurs/lecteurs de champs restent à cartographier une par une.

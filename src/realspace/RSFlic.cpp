//
//  RSFlic.cpp
//  libRealSpace
//
//  Animation d'introduction de la version CD-ROM (DATA\FLIC\THEFLIC.FLX), décodée comme MENU.EXE.
//  Format : strike_commander_re/analysis/menu_cd/FLX_FORMAT.md ; adresses = MENU.EXE du CD-ROM.
//
#include "RSFlic.h"

#include <cstdio>
#include <cstring>

#include "../commons/ByteStream.h"

RSFlic::RSFlic() {
}

RSFlic::~RSFlic() {
}

// En-tête de 128 octets : nombre d'enregistrements (u32 en 84), taille du fichier (u32 en 92) ; puis la
// table des enregistrements, (position u32, taille u32) chacun (lecteur 10576 : `shl ax, 3`).
bool RSFlic::InitFromRAM(uint8_t *data, size_t size) {
    this->records.clear();
    this->next = 0;
    this->frameCount = 0;
    this->paletteChanged = false;
    memset(this->screen, 0, sizeof(this->screen));
    if (data == nullptr || size < 128) {
        return false;
    }
    ByteStream stream;
    stream.Set(data, size);
    stream.MoveForward(84);
    uint32_t count = stream.ReadUInt32LE();
    if (128 + (size_t)count * 8 > size) {
        printf("RSFlic: table des enregistrements hors du fichier (%u)\n", count);
        return false;
    }
    stream.MoveForward(128 - stream.GetCurrentPosition());
    for (uint32_t i = 0; i < count; i++) {
        uint32_t offset = stream.ReadUInt32LE();
        uint32_t length = stream.ReadUInt32LE();
        if (length == 0 || (size_t)offset + length > size) {
            printf("RSFlic: enregistrement %u hors du fichier\n", i);
            return false;
        }
        FlicRecord rec;
        rec.data = data + offset;
        rec.size = length;
        this->records.push_back(rec);
    }
    return true;
}

size_t RSFlic::GetNumRecords(void) const {
    return this->records.size();
}

// Boucle de 61691 : un enregistrement de type 0 annonce une palette (l'enregistrement suivant), un
// enregistrement de type 1 est une image (60188).
bool RSFlic::DecodeNextFrame(void) {
    this->paletteChanged = false;
    while (this->next < this->records.size()) {
        const FlicRecord &rec = this->records[this->next];
        this->next++;
        if (rec.data[0] == 0) {
            if (this->next >= this->records.size()) {
                return false;
            }
            this->ApplyPalette(this->records[this->next]);
            this->next++;
            this->paletteChanged = true;
            continue;
        }
        this->DrawFrame(rec);
        this->frameCount++;
        return true;
    }
    return false;
}

// Palette au format des palettes du jeu : premier indice et nombre de couleurs (u16), puis RVB sur
// 6 bits. Puis les bandes du haut et du bas (lignes 0 à 23 et 176 à 199) prennent la couleur la plus
// proche du noir (62966 à 63031 : recherche 33267, puis remplissage des deux rectangles).
void RSFlic::ApplyPalette(const FlicRecord &rec) {
    ByteStream stream;
    stream.Set(rec.data, rec.size);
    this->palette.ReadPatch(&stream);
    uint8_t dark = this->DarkestColor();
    memset(this->screen, dark, FLC_WIDTH * 24);
    memset(this->screen + FLC_WIDTH * 176, dark, FLC_WIDTH * 24);
}

uint8_t RSFlic::DarkestColor(void) const {
    int best = 0;
    int bestDistance = 1 << 30;
    for (int i = 0; i < 256; i++) {
        const Texel &c = this->palette.colors[i];
        int distance = c.r * c.r + c.g * c.g + c.b * c.b;
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    return (uint8_t)best;
}

// En-tête d'image (16 octets) : octet 1 = compression (0 aucune, 1 LZW du jeu), octet 2 = affichage
// (0 lignes, 1 copie, 2 blocs de 4 × 4), octet 3 = première ligne (24 : image de 320 × 152 entre les
// bandes ; 0 : plein écran).
void RSFlic::DrawFrame(const FlicRecord &rec) {
    if (rec.size < 16) {
        return;
    }
    uint8_t compression = rec.data[1];
    uint8_t display = rec.data[2];
    int firstRow = rec.data[3];
    uint8_t *body = rec.data + 16;
    size_t bodySize = rec.size - 16;
    if (compression == 1) {
        size_t uncompressed = 0;
        body = this->lzw.DecodeLZW(body, bodySize, uncompressed);
        bodySize = uncompressed;
    } else if (compression != 0) {
        printf("RSFlic: compression %d non traitée\n", compression);
        return;
    }
    if (firstRow * 2 >= FLC_HEIGHT) {
        return;
    }
    if (display == 0) {
        this->DrawRows(body, bodySize);
    } else if (display == 1) {
        if (rec.data[4] != 0) {
            printf("RSFlic: affichage 1 avec doublement des pixels non traité\n");
            return;
        }
        this->CopyRows(body, bodySize, firstRow);
    } else if (display == 2) {
        this->DecodeBlocks(body, bodySize, firstRow);
    }
}

// Affichage 1 (60875) : l'image décompressée recouvre les lignes firstRow à 199 - firstRow.
void RSFlic::CopyRows(const uint8_t *src, size_t size, int firstRow) {
    size_t count = (size_t)FLC_WIDTH * (FLC_HEIGHT - 2 * firstRow);
    if (count > size) {
        count = size;
    }
    memcpy(this->screen + FLC_WIDTH * firstRow, src, count);
}

// Affichage 2 (15350) : blocs de 4 × 4 pixels, 80 par ligne de blocs, (200 - 2 × firstRow) / 4 lignes
// de blocs. Le 1er octet du flux est l'octet d'échappement : rencontré au début d'un bloc, il fait
// basculer entre blocs codés et blocs bruts. Bloc codé : deux couleurs c0, c1 ; c0 = c1 → bloc uni ;
// sinon un masque de 16 bits (octet bas : lignes 0 et 1, octet haut : lignes 2 et 3, bit de poids fort
// d'abord ; 1 = c1). Bloc brut : 16 pixels, ligne par ligne.
void RSFlic::DecodeBlocks(uint8_t *src, size_t size, int firstRow) {
    if (size < 1) {
        return;
    }
    ByteStream stream(src, size);
    uint8_t escape = stream.ReadByte();
    bool coded = true;
    int blockRows = (FLC_HEIGHT - 2 * firstRow) / 4;
    for (int by = 0; by < blockRows; by++) {
        for (int bx = 0; bx < 80; bx++) {
            if (stream.IsEndOfStream()) {
                return;
            }
            if (stream.CurrentByte() == escape) {
                coded = !coded;
                stream.ReadByte();
            }
            size_t remaining = stream.GetSize() - stream.GetCurrentPosition();
            uint8_t *dst = this->screen + (firstRow + 4 * by) * FLC_WIDTH + 4 * bx;
            if (!coded) {
                if (remaining < 16) {
                    return;
                }
                for (int y = 0; y < 4; y++) {
                    stream.ReadBytes(dst + y * FLC_WIDTH, 4);
                }
                continue;
            }
            if (remaining < 2) {
                return;
            }
            if (stream.PeekByte(0) == stream.PeekByte(1)) {
                uint8_t c0 = stream.ReadByte();
                stream.ReadByte();
                for (int y = 0; y < 4; y++) {
                    memset(dst + y * FLC_WIDTH, c0, 4);
                }
                continue;
            }
            if (remaining < 4) {
                return;
            }
            uint8_t c0 = stream.ReadByte();
            uint8_t c1 = stream.ReadByte();
            uint16_t mask = stream.ReadUShort();
            for (int y = 0; y < 4; y++) {
                int shift = (y < 2) ? 7 - 4 * y : 15 - 4 * (y - 2);
                for (int x = 0; x < 4; x++) {
                    int bit = (mask >> (shift - x)) & 1;
                    dst[y * FLC_WIDTH + x] = bit ? c1 : c0;
                }
            }
        }
    }
}

// Affichage 0 (65708) : image par lignes dessinée en (0, 0) par-dessus l'image précédente. En-tête :
// drapeaux u16, largeur, hauteur, x, y (i16) ; puis un décalage u16 par ligne, compté depuis sa propre
// position. Une ligne : paires (pixels sautés, compteur) jusqu'à épuiser la largeur. Drapeau 1 :
// compteur impair = suite de compteur / 2 pixels d'une couleur (octet suivant), pair = compteur / 2
// pixels bruts ; sans le drapeau : compteur pixels bruts.
void RSFlic::DrawRows(uint8_t *src, size_t size) {
    if (size < 10) {
        return;
    }
    ByteStream stream(src, size);
    uint16_t flags = stream.ReadUShort();
    int16_t w = stream.ReadShort();
    int16_t h = stream.ReadShort();
    int16_t x0 = stream.ReadShort();
    int16_t y0 = stream.ReadShort();
    if ((size_t)10 + 2 * (size_t)h > size) {
        return;
    }
    for (int row = 0; row < h; row++) {
        size_t entry = stream.GetCurrentPosition();
        size_t p = entry + stream.ReadUShort();
        int y = row - y0;
        int x = -x0;
        int left = w;
        while (p < size) {
            int skip = src[p];
            p++;
            left -= skip;
            if (left <= 0) {
                break;
            }
            x += skip;
            if (p >= size) {
                return;
            }
            int count = src[p];
            p++;
            int n = (flags & 1) ? count >> 1 : count;
            bool run = (flags & 1) && (count & 1);
            for (int k = 0; k < n; k++) {
                uint8_t color;
                if (run) {
                    color = src[p];
                } else {
                    if (p + k >= size) {
                        return;
                    }
                    color = src[p + k];
                }
                int px = x + k;
                if (px >= 0 && px < FLC_WIDTH && y >= 0 && y < FLC_HEIGHT) {
                    this->screen[y * FLC_WIDTH + px] = color;
                }
            }
            p += run ? 1 : n;
            x += n;
            left -= n;
            if (left <= 0) {
                break;
            }
        }
    }
}

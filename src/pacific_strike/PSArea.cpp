#include "PSArea.h"

void PSArea::ParseTriFile(PakEntry *entry) {
    if (entry->size <= 0) {
        return;
    }
    AreaOverlay overlay;
    size_t read = 0;
    ByteStream stream(entry->data, entry->size);

    // En-tête : 5 mots, le 3e n'est pas lu par le jeu.
    int numvertice = stream.ReadUShort();
    int nbpoly = stream.ReadUShort();
    int unread_word = stream.ReadUShort();
    int param2 = stream.ReadUShort();
    int param3 = stream.ReadUShort();
    read += 10;
    overlay.param2 = (uint16_t)param2;
    overlay.param3 = (uint16_t)param3;
    printf("PSArea: region de %zu octets, %d sommets, %d faces, mot non lu %d, grille %d, table des listes %d\n", entry->size, numvertice, nbpoly, unread_word, param2, param3);

    // Sommets : 13 octets, comme Strike Commander.
    AoVPoints *vertices = new AoVPoints[numvertice];
    overlay.lx = 0;
    overlay.ly = 0;
    overlay.hx = 0;
    overlay.hy = 0;
    for (int i = 0; i < numvertice; i++) {
        AoVPoints *v = &vertices[i];
        v->x = stream.ReadFixedFloatLE() * BLOCK_COORD_SCALE;
        v->z = -stream.ReadFixedFloatLE() * BLOCK_COORD_SCALE;
        v->y = stream.ReadFixedFloatLE() * HEIGH_MAP_SCALE;
        v->pad = stream.ReadByte();
        read += 13;
        if (i == 0) {
            overlay.lx = v->x;
            overlay.hx = v->x;
            overlay.ly = v->z;
            overlay.hy = v->z;
        }
        overlay.lx = v->x < overlay.lx ? v->x : overlay.lx;
        overlay.ly = v->z < overlay.ly ? v->z : overlay.ly;
        overlay.hx = v->x > overlay.hx ? v->x : overlay.hx;
        overlay.hy = v->z > overlay.hy ? v->z : overlay.hy;
        overlay.verticesVec.push_back(*v);
    }
    overlay.vertices = vertices;
    if (numvertice > 0) {
        printf("PSArea: premier sommet (%.1f, %.1f, %.1f), emprise X %.1f a %.1f, Z %.1f a %.1f\n", vertices[0].x, vertices[0].y, vertices[0].z, overlay.lx, overlay.hx, overlay.ly, overlay.hy);
    }

    // Faces : 28 octets, toujours un triangle.
    overlay.nbTriangles = 0;
    for (int i = 0; i < nbpoly; i++) {
        AreaOverlayTriangles aot;
        aot.flag0 = stream.ReadByte();
        aot.verticesIdx[0] = stream.ReadUShort();
        aot.verticesIdx[1] = stream.ReadUShort();
        aot.verticesIdx[2] = stream.ReadUShort();
        // Octets 7 à 18 : non lus par le jeu.
        stream.MoveForward(12);
        aot.type = stream.ReadByte();
        // Numéro de matériau.
        aot.color = stream.ReadUShort();
        for (int k = 0; k < 3; k++) {
            aot.uv[k].u = stream.ReadByte();
            aot.uv[k].v = stream.ReadByte();
        }
        read += 28;
        for (int k = 0; k < 3; k++) {
            if (aot.verticesIdx[k] >= numvertice) {
                printf("PSArea: face %d, sommet %u hors des %d sommets\n", i, aot.verticesIdx[k], numvertice);
                aot.verticesIdx[k] = 0;
            }
        }
        printf("PSArea: face %d, groupe %d, sommets %u %u %u, type %d, materiau %d\n", i, aot.flag0, aot.verticesIdx[0], aot.verticesIdx[1], aot.verticesIdx[2], aot.type, aot.color);
        overlay.trianles[overlay.nbTriangles++] = aot;
    }

    overlay.param3Table.reserve(param3);
    for (int i = 0; i < param3; i++) {
        overlay.param3Table.push_back(stream.ReadUShort());
        read += 2;
    }

    int ncell = param2 * param2;
    overlay.grid.reserve(ncell);
    for (int i = 0; i < ncell; i++) {
        overlay.grid.push_back(stream.ReadUShort());
        read += 2;
    }

    std::vector<char> seen(nbpoly, 0);
    for (int c = 0; c < ncell; c++) {
        uint16_t g = overlay.grid[c];
        if (g >= overlay.param3Table.size()) {
            continue;
        }
        if (overlay.param3Table[g] == 0xFFFF) {
            continue;
        }
        uint16_t count = overlay.param3Table[g];
        for (uint16_t k = 0; k < count && (size_t)(g + 1 + k) < overlay.param3Table.size(); k++) {
            uint16_t idx = overlay.param3Table[g + 1 + k];
            if (idx < nbpoly && !seen[idx]) {
                seen[idx] = 1;
                overlay.drawList.push_back(idx);
            }
        }
    }
    if (overlay.drawList.empty()) {
        for (int i = 0; i < nbpoly; i++) {
            overlay.drawList.push_back((uint16_t)i);
        }
    }
    if (read != entry->size) {
        printf("PSArea: region lue sur %zu octets, l'entree en fait %zu\n", read, entry->size);
    }
    objectOverlay.push_back(overlay);
}

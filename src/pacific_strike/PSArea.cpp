#include "PSArea.h"
#include "PSEntity.h"

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

// Pont d'un porte-avions actif sous le point : le point est ramené dans le repère du navire (position et cap de son
// PART, modèle : x = longueur, z = largeur) et comparé à la boîte du modèle ; hauteur = navire + hauteur du chunk CARD
// (Aircraft_ContactWithObject_3CA69 pose l'avion à cette hauteur). Le point d'origine du navire lui-même n'est pas
// sur son pont : c'est la question que pose le placement du navire.
bool PSArea::carrierDeckAt(float x, float z, float &deck) {
    bool found = false;
    for (auto carrier : this->carriers) {
        if (!carrier->is_active || carrier->is_destroyed) {
            continue;
        }
        float dx = x - carrier->object->position.x;
        float dz = z - carrier->object->position.z;
        if (fabsf(dx) < 1.0f && fabsf(dz) < 1.0f) {
            continue;
        }
        PSEntity *entity = static_cast<PSEntity *>(carrier->object->entity);
        float angle = (360.0f - (float)carrier->object->azymuth + 90.0f) * (float)M_PI / 180.0f;
        float local_x = dx * cosf(angle) - dz * sinf(angle);
        float local_z = dx * sinf(angle) + dz * cosf(angle);
        BoudingBox *box = entity->GetBoudingBpx();
        if (local_x < box->min.x || local_x > box->max.x || local_z < box->min.z || local_z > box->max.z) {
            continue;
        }
        float height = carrier->object->position.y + entity->deck_height;
        if (!found || height > deck) {
            deck = height;
        }
        found = true;
    }
    return found;
}

float PSArea::getY(float x, float z) {
    float ground = RSArea::getY(x, z);
    float deck = 0.0f;
    if (this->carrierDeckAt(x, z, deck) && deck > ground) {
        return deck;
    }
    return ground;
}

// sommet (x, y) d'un bloc, pris dans le bloc voisin à droite ou en bas quand il déborde ; nul hors de la carte
MapVertex *PSArea::blockVertex(size_t lod, int block, int x, int y, int side) {
    if (x >= side) {
        if (block % BLOCK_PER_MAP_SIDE == BLOCK_PER_MAP_SIDE - 1) {
            return nullptr;
        }
        block += 1;
        x -= side;
    }
    if (y >= side) {
        if (block / BLOCK_PER_MAP_SIDE == BLOCK_PER_MAP_SIDE - 1) {
            return nullptr;
        }
        block += BLOCK_PER_MAP_SIDE;
        y -= side;
    }
    return this->GetAreaBlockByID(lod, block)->GetVertice(x, y);
}

// Render_TerrainPipelineMain_3362C : un triangle dont le sommet le plus haut est de couleur 0 est de la mer
bool PSArea::seaTriangle(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2) {
    MapVertex *highest = tri0;
    if ((uint16_t)tri1->height >= (uint16_t)highest->height) {
        highest = tri1;
    }
    if ((uint16_t)tri2->height >= (uint16_t)highest->height) {
        highest = tri2;
    }
    return highest->type == 0;
}

// Render_WaterOrSkyPlane_2CDA0 : u, v = position / 16 en points de l'image, fixes sur le monde (asm c0 = x, c1 = −z) ;
// water1 (128 points) revient tous les 2048 m
static const float SEA_TILE_METERS = 2048.0f;
// image de la mer : water1 répétée 4 x 4 (8192 m), pour qu'un triangle de 4000 m ne sorte jamais de l'image
static const int SEA_TILES = 4;

// Render_SkyAndSeaBackground_2D6FC : water1 = place 7 de TXM\ACC_LIST.IFF, entrée 7 d'ACCPACK.PAK (128 x 128)
uint16_t PSArea::waterImage() {
    if (this->water_image_id != 0xFF) {
        return this->water_image_id;
    }
    if (this->textures.size() < 2) {
        return 0xFF;
    }
    RSImage *water = this->textures[1]->GetImageById(7);
    if (water == nullptr || water->width < 2) {
        return 0xFF;
    }
    int width = (int)water->width;
    int height = (int)water->height;
    std::vector<uint8_t> pixels((size_t)(width * SEA_TILES) * (size_t)(height * SEA_TILES));
    for (int y = 0; y < height * SEA_TILES; y++) {
        for (int x = 0; x < width * SEA_TILES; x++) {
            pixels[(size_t)y * (size_t)(width * SEA_TILES) + (size_t)x] = water->data[(y % height) * width + (x % width)];
        }
    }
    RSImage *sea = new RSImage();
    sea->Create("SEA", (uint32_t)(width * SEA_TILES), (uint32_t)(height * SEA_TILES), 1);
    sea->UpdateContent(pixels.data());
    size_t place = this->textures[1]->AddImage(sea);
    this->water_image_id = (uint16_t)(this->textures[0]->GetNumImages() + place);
    printf("PSArea: image de mer %d (water1 %d x %d répétée %d x %d)\n", this->water_image_id, width, height, SEA_TILES, SEA_TILES);
    return this->water_image_id;
}

// u, v du triangle dans l'image de mer, à partir de la répétition de 2048 m qui contient son coin le plus petit
void PSArea::seaUV(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2, float uv[3][2]) {
    MapVertex *corners[3];
    corners[0] = tri0;
    corners[1] = tri1;
    corners[2] = tri2;
    float min_u = tri0->v.x;
    float min_v = -tri0->v.z;
    for (int i = 1; i < 3; i++) {
        min_u = (std::min)(min_u, corners[i]->v.x);
        min_v = (std::min)(min_v, -corners[i]->v.z);
    }
    float origin_u = floorf(min_u / SEA_TILE_METERS) * SEA_TILE_METERS;
    float origin_v = floorf(min_v / SEA_TILE_METERS) * SEA_TILE_METERS;
    float extent = SEA_TILE_METERS * (float)SEA_TILES;
    for (int i = 0; i < 3; i++) {
        uv[i][0] = (corners[i]->v.x - origin_u) / extent;
        uv[i][1] = (-corners[i]->v.z - origin_v) / extent;
    }
}

// Render_TerrainPipelineMain_3362C : la mer n'est pas du terrain, elle prend ici l'image de mer avec ses u, v du monde
void PSArea::ParseBlocks(size_t lod, PakEntry *entry, size_t verticePerBlock) {
    RSArea::ParseBlocks(lod, entry, verticePerBlock);
    uint16_t water = this->waterImage();
    if (water == 0xFF) {
        return;
    }
    int side = (int)verticePerBlock;
    int sea_triangles = 0;
    for (int block = 0; block < BLOCKS_PER_MAP; block++) {
        for (int y = 0; y < side; y++) {
            for (int x = 0; x < side; x++) {
                MapVertex *current = this->blockVertex(lod, block, x, y, side);
                MapVertex *right = this->blockVertex(lod, block, x + 1, y, side);
                MapVertex *bottom_right = this->blockVertex(lod, block, x + 1, y + 1, side);
                MapVertex *bottom = this->blockVertex(lod, block, x, y + 1, side);
                if (right == nullptr || bottom_right == nullptr || bottom == nullptr) {
                    continue;
                }
                if (current->lowerImageID == 0xFF && this->seaTriangle(current, bottom_right, bottom)) {
                    current->lowerImageID = water;
                    this->seaUV(current, bottom_right, bottom, current->lowerUV);
                    sea_triangles++;
                }
                if (current->upperImageID == 0xFF && this->seaTriangle(current, right, bottom_right)) {
                    current->upperImageID = water;
                    this->seaUV(current, right, bottom_right, current->upperUV);
                    sea_triangles++;
                }
            }
        }
    }
    printf("PSArea: niveau %zu, %d triangles de mer avec l'image %d\n", lod, sea_triangles, water);
}

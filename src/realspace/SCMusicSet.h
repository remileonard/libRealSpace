//
//  SCMusicSet.h
//  libRealSpace
//
//  Donnees d'un jeu de musique de Strike Commander (ex. COMBAT.ADL + COMBAT.DAT) et
//  bibliotheque de timbres (STRIKE.AD). Independant du chargeur d'archives : RSMusic remplit
//  ces structures avec PakArchive.
//
//  .DAT, enregistrement 0 (analysis/MUSIC_SYSTEM.md 2.3) :
//    u8 N ; N x (A, B) ; matrice N x N (0xFF = aucune) ; u8 E ; E x ([L] puis L+1 octets)
//  .ADL (verifie sur le vrai COMBAT.ADL) :
//    fichier : 1 entree = le jeu ; jeu : [0] archive des pistes de liaison, [1..N] pistes
//  Bibliotheque (Music_LoadTimbreFromLibrary_5A577) : entrees de 6 octets
//    (patch, banque, offset u32), fin sur banque 0xFF ; a l'offset : u16 longueur puis donnees.
//
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

struct SCMusicSet {
    int trackCount = 0;             // word_7084C
    std::vector<uint8_t> phraseLen; // TrackDescriptor +0xA (A) / +0xB (B)
    std::vector<uint8_t> phraseLast;
    std::vector<uint8_t> matrix;                   // [courante * N + demandee]
    std::vector<std::vector<uint8_t>> linkEntries; // L+1 octets, indexes par la position
    std::vector<std::vector<uint8_t>> tracks;      // pistes principales (XMIDI)
    std::vector<std::vector<uint8_t>> linkTracks;  // pistes de liaison (XMIDI), word_7084E

    // Analyse l'enregistrement 0 du .DAT. Renvoie false si le format est inattendu.
    bool parseDat(const uint8_t *rec, size_t n);
};

class SCTimbreLibrary {
public:
    void set(const uint8_t *data, size_t size) {
        lib = data;
        libSize = size;
    }
    bool loaded() const {
        return lib != nullptr;
    }
    // Pointeur sur le timbre (commencant par sa longueur u16), ou nullptr.
    const uint8_t *find(int bank, int patch) const;
    const uint8_t *data() const {
        return lib;
    }
    size_t size() const {
        return libSize;
    }

private:
    const uint8_t *lib = nullptr;
    size_t libSize = 0;
};

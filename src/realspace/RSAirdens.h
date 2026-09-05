//
//  RSAirdens.h
//  libRealSpace
//
//  Table de densite de l'air du jeu (fichier DATA\AIRDENS.TBL).
//  Format : suite de u32 LE, un echantillon par bande de 256 m d'altitude
//  (index = altitude_m / 256), valeurs decroissantes (~1322 au niveau mer
//  jusqu'a ~28 vers 30 km). Chargee brute puis stockee en float.
//  Ref : strike_commander_re/analysis/PHYSICS.md §6.
//
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class RSAirdens {
public:
    RSAirdens();
    ~RSAirdens();

    // Prend uniquement un tableau binaire (comme les autres classes RS).
    void initFromRam(uint8_t *data, size_t size);

    bool isLoaded() const { return !this->table.empty(); }

    // Valeur brute de la table interpolee lineairement entre deux bandes de 256 m.
    float rawAt(float altitude_m) const;

    // Densite en kg/m3, normalisee sur la valeur niveau-mer de la table
    // (table[0] -> 1.225 kg/m3). Repli exponentiel si la table n'est pas chargee.
    float densityAt(float altitude_m) const;

private:
    static constexpr float kBandMeters = 256.0f;
    static constexpr float kSeaLevelDensity = 1.225f;

    std::vector<float> table;
};

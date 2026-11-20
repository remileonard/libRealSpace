//
//  TerrainDiag.h - les types de terrain et les textures d'une zone (RSArea), en images.
//
#pragma once

#include <string>
#include <vector>

class RSArea;
struct PaletteRange;
struct VGAPalette;

// Écrit dans dir (qui doit exister), au LVL0, le nord en haut : terrain.ppm, types.ppm, textures.ppm, tex_<id>.ppm (les images utilisées),
// all_<id>.ppm (toutes les textures du jeu), types.txt et textures.txt (les comptes). false (et pourquoi) en cas d'erreur.
bool terrainDiagnostic(RSArea *area, const VGAPalette &palette, const std::vector<PaletteRange> &ranges, const std::string &dir, std::string &error);

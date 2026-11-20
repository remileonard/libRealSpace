//
//  r4objviewer - les objets, les missions et les mondes de Strike Commander et Tactical Operations, lus par libRealSpace et dessinés par R4nkine.
//  r4objviewer [-data DIR] [-tre FICHIER.TRE ...] [-object NOM] [-mission NOM] [-frames N] [-shot F.bmp]
//
//  -data     le dossier des TRE et des terrains .PAK (par défaut le dossier courant)
//  -tre      les TRE à ouvrir (par défaut, chaque jeu présent seul : BIGTRE.TRE Strike Commander, TOBIGTRE.TRE Tactical Operations ; sinon OBJECTS.TRE + MISC.TRE)
//  -game G   le jeu au départ : sc ou to (par défaut, celui de -mission)
//  -object   l'objet montré au départ (F-16DES, A-10...)
//  -mission  la mission ou le monde montré au départ (MISN-A1, TURKEY...)
//  -focus    avec -mission : la caméra sur le premier objet de ce nom (ZSU-23...)
//  -graph    le graphe de la scène ouvert au départ
//  -distance la distance de la caméra au départ
//  -flat     le rendu plat, objets et terrain (object_gouraud faux, terrain_flat)
//  -gouraud  l'éclairage des objets calculé aux sommets, comme le jeu
//  -lit      le terrain éclairé par le soleil (ses rampes, ses normales)
//  -sun C E  le soleil : son cap et son élévation, en degrés
//  -shadows  les ombres portées du soleil
//  -treeshadows avec -shadows : les arbres aussi projettent leur ombre
//  -nolift   les objets de la carte et de la mission laissés à la hauteur des données, au lieu d'être posés sur le terrain
//  -nofog    sans le brouillard, qui commence 120 km et finit 450 km au-delà de la cible
//  -at X,Y   avec -mission : la caméra sur ce point de la carte (axes de R4nkine)
//  -upscale N les textures agrandies une fois : 0 non, 1 2xSaI, 2 xBR 2x, 3 xBR 4x
//  -mipmaps  les textures avec leurs mipmaps
//  -aniso N  le filtrage anisotrope (1, 2, 4, 8, 16)
//  -woods N  les arbres : 0 sur le vert et un bruit, 1 sur les bois peints dans les textures
//  -forests N la densité des forêts : 0 aucune, 1 clairsemées, 2 normales (par défaut), 3 denses
//  -diag DIR avec -mission : les types de terrain et les textures de la carte, en images dans DIR
//
//  Souris : bouton gauche tourner, molette zoom, bouton droit déplacer. Clavier : flèches haut et bas, objet précédent ou suivant ; Échap, quitter.
//
#include "../../r4bridge/R4Data.h"
#include "engine/Config.hpp"
#include "realspace/AssetManager.h"
#include "realspace/RSEntity.h"
#include "realspace/RSPalette.h"
#include "realspace/TreArchive.h"

#include "../../r4bridge/R4Bridge.h"
#include "TerrainDiag.h"
#include "WorldScene.h"

#include "r4/axes.h"
#include "r4/gpu/device.h"
#include "r4/render/orbit.h"
#include "r4/render/postprocess.h"
#include "r4/render/renderer.h"
#include "r4/scene/node.h"
#include "r4/ui/controls.h"
#include "r4/ui/ui.h"

#include <SDL.h>
#include <SDL_opengl.h> // glReadPixels, pour -shot

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

std::string upperOf(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

struct Window {
    SDL_Window *window = nullptr;
    SDL_GLContext context = nullptr;
    int width = 0; // le framebuffer, en pixels
    int height = 0;
    int pointWidth = 0; // la fenêtre, en points (la souris)
    int pointHeight = 0;

    bool open(const char *title, int w, int h) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            return false;
        }
#ifdef __APPLE__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        if (!window || !(context = SDL_GL_CreateContext(window))) {
            return false;
        }
        SDL_GL_SetSwapInterval(1);
        return true;
    }
    void sizes() {
        SDL_GL_GetDrawableSize(window, &width, &height);
        SDL_GetWindowSize(window, &pointWidth, &pointHeight);
    }
    ~Window() {
        if (context) {
            SDL_GL_DeleteContext(context);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
    }
};

struct ObjectEntry {
    std::string label; // F-16DES
    TreEntry *entry;
};

// les entrées ...\OBJECTS\*.IFF des own premiers TRE ouverts : ceux du jeu, les suivants ne font que compléter
std::vector<ObjectEntry> listObjects(AssetManager &assets, size_t own) {
    std::vector<ObjectEntry> list;
    for (size_t t = 0; t < assets.tres.size() && t < own; t++) {
        for (TreEntry *e : assets.tres[t]->entries) {
            std::string name = e->name;
            std::string upper = name;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            size_t slash = upper.rfind('\\');
            if (upper.find("\\OBJECTS\\") == std::string::npos || upper.size() < 4 || upper.compare(upper.size() - 4, 4, ".IFF") != 0) {
                continue;
            }
            list.push_back({name.substr(slash + 1, name.size() - slash - 5), e});
        }
    }
    std::sort(list.begin(), list.end(), [](const ObjectEntry &a, const ObjectEntry &b) {
        return a.label < b.label;
    });
    return list;
}

std::string fmt(const char *f, ...) {
    char buf[256];
    va_list args;
    va_start(args, f);
    std::vsnprintf(buf, sizeof buf, f, args);
    va_end(args);
    return buf;
}

// une courbe des 240 dernières images, à l'échelle de son pic
void plot(r4::ui::Context &gui, const std::deque<float> &values, float minScale, r4::ui::Color color, const char *name) {
    float buf[240] = {};
    int n = (int)std::min<size_t>(values.size(), 240);
    float sum = 0;
    float peak = 0;
    for (int i = 0; i < n; i++) {
        float v = values[values.size() - n + i];
        buf[240 - n + i] = v;
        sum += v;
        peak = std::max(peak, v);
    }
    float scale = minScale;
    while (scale < peak * 1.1f) {
        scale *= 2;
    }
    gui.plot(buf, 240, scale, color, fmt("%s %.2f ms", name, n ? sum / n : 0.0f).c_str());
}

// une capture du framebuffer, en BMP (les tests)
void screenshot(const char *path, int w, int h) {
    std::vector<uint8_t> rgba((size_t)w * h * 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ABGR8888);
    for (int y = 0; y < h; y++) {
        std::memcpy((uint8_t *)s->pixels + (size_t)y * s->pitch, &rgba[(size_t)(h - 1 - y) * w * 4], (size_t)w * 4);
    }
    SDL_SaveBMP(s, path);
    SDL_FreeSurface(s);
}

struct GraphView {
    std::map<const r4::scene::Node *, bool> open;      // the open nodes
    std::map<r4::scene::Node *, uint32_t> hiddenMasks; // the hidden nodes, their old mask
    const r4::scene::Node *selected = nullptr;
    r4::Mat4 selectedParent{1.0f}; // the world matrix of the parent of the selection
    void reset() {
        open.clear();
        hiddenMasks.clear();
        selected = nullptr;
        selectedParent = r4::Mat4(1.0f);
    }
};

// the box of a node in the space of its parent: the boxes of its meshes, the spheres of its instancers
r4::Box boxOf(r4::scene::Node &n) {
    r4::Box box;
    if (n.type() == r4::scene::NodeType::INSTANCER) {
        const r4::Sphere &s = n.bounds();
        if (!s.empty()) {
            box.extend(s.center - r4::Vec3(s.radius));
            box.extend(s.center + r4::Vec3(s.radius));
        }
        return box;
    }
    r4::Box local;
    if (n.type() == r4::scene::NodeType::GEODE || n.type() == r4::scene::NodeType::BILLBOARD) {
        for (const r4::scene::Drawable &d : static_cast<r4::scene::Geode &>(n).drawables()) {
            if (d.mesh && !d.mesh->box.empty()) {
                local.extend(d.mesh->box);
            }
        }
    }
    for (const r4::scene::NodePtr &c : n.children()) {
        r4::Box child = boxOf(*c);
        if (!child.empty()) {
            local.extend(child);
        }
    }
    if (local.empty()) {
        return box;
    }
    for (int k = 0; k < 8; k++) {
        r4::Vec3 corner{k & 1 ? local.max.x : local.min.x, k & 2 ? local.max.y : local.min.y, k & 4 ? local.max.z : local.min.z};
        box.extend(r4::transformPoint(n.matrix(), corner));
    }
    return box;
}

// the culling sphere (a circle about each axis, of radius 1) and the bounding box (the unit cube) of the selected node
std::shared_ptr<r4::scene::Group> makeSelectionMarkers(r4::render::Renderer &renderer, std::shared_ptr<r4::scene::Geode> &sphere, std::shared_ptr<r4::scene::Geode> &box) {
    r4::assets::Material material;
    material.name = "selection";
    material.shading = r4::assets::Shading::UNLIT;
    material.vertexColors = true;
    material.castShadows = false;
    auto line = [](r4::assets::Mesh &m, r4::Vec3 a, r4::Vec3 b, uint32_t color) {
        for (r4::Vec3 p : {a, b}) {
            r4::assets::Vertex v;
            v.position = p;
            v.color = color;
            m.vertices.push_back(v);
            m.indices.push_back((uint32_t)m.vertices.size() - 1);
        }
    };
    r4::assets::Mesh circles;
    circles.name = "culling sphere";
    circles.primitive = r4::assets::Primitive::LINES;
    const int n = 64;
    const float turn = 2 * std::acos(-1.0f);
    for (int axis = 0; axis < 3; axis++) {
        for (int i = 0; i < n; i++) {
            auto at = [&](int k) {
                float c = std::cos(turn * k / n);
                float s = std::sin(turn * k / n);
                return axis == 0 ? r4::Vec3{0, c, s} : axis == 1 ? r4::Vec3{c, 0, s} : r4::Vec3{c, s, 0};
            };
            line(circles, at(i), at(i + 1), 0xff20e0ffu);
        }
    }
    circles.computeBounds();
    r4::assets::Mesh cube;
    cube.name = "bounding box";
    cube.primitive = r4::assets::Primitive::LINES;
    for (int k = 0; k < 8; k++) {
        for (int axis = 0; axis < 3; axis++) {
            int other = k | 1 << axis;
            if (other != k) {
                r4::Vec3 a{(float)(k & 1), (float)(k >> 1 & 1), (float)(k >> 2 & 1)};
                r4::Vec3 b{(float)(other & 1), (float)(other >> 1 & 1), (float)(other >> 2 & 1)};
                line(cube, a, b, 0xffff40ffu);
            }
        }
    }
    cube.computeBounds();
    sphere = r4::scene::make<r4::scene::Geode>();
    sphere->setName("culling sphere");
    sphere->addDrawable({renderer.uploadMesh(circles), renderer.createMaterial(material)});
    box = r4::scene::make<r4::scene::Geode>();
    box->setName("bounding box");
    box->addDrawable({renderer.uploadMesh(cube), renderer.createMaterial(material)});
    auto group = r4::scene::make<r4::scene::Group>();
    group->setName("selection");
    group->addChild(sphere);
    group->addChild(box);
    return group;
}

const char *typeTag(r4::scene::NodeType t) {
    switch (t) {
    case r4::scene::NodeType::GROUP:
        return "GRP";
    case r4::scene::NodeType::GEODE:
        return "GEO";
    case r4::scene::NodeType::SWITCH:
        return "SW";
    case r4::scene::NodeType::LOD:
        return "LOD";
    case r4::scene::NodeType::BILLBOARD:
        return "BB";
    case r4::scene::NodeType::INSTANCER:
        return "INST";
    }
    return "?";
}
r4::ui::Color typeColor(r4::scene::NodeType t) {
    switch (t) {
    case r4::scene::NodeType::GEODE:
        return r4::ui::Color::rgb(0.55f, 0.85f, 0.55f);
    case r4::scene::NodeType::SWITCH:
        return r4::ui::Color::rgb(0.95f, 0.75f, 0.4f);
    case r4::scene::NodeType::LOD:
        return r4::ui::Color::rgb(0.6f, 0.75f, 1.0f);
    default:
        return r4::ui::Color::rgb(0.85f, 0.85f, 0.85f);
    }
}

// la lampe montre ou cache le nœud, un clic sur son nom le choisit ; rend le nœud cliqué
r4::scene::Node *graphRows(r4::ui::Context &gui, GraphView &view, r4::scene::Node &n, const r4::Mat4 &parent, int depth, r4::Vec3 &center, float &radius) {
    r4::scene::Node *clicked = nullptr;
    if (gui.remainingHeight() < 22) {
        return nullptr;
    }
    auto it = view.open.find(&n);
    if (it == view.open.end()) {
        it = view.open.emplace(&n, depth < 2).first;
    }
    bool open = it->second;
    bool visible = view.hiddenMasks.count(&n) == 0;
    std::string label = n.name().empty() ? std::string("(") + typeTag(n.type()) + ")" : n.name();
    if (!n.children().empty()) {
        label += fmt("  [%zu]", n.children().size());
    }
    r4::ui::Context::TreeRow row;
    row.id = &n;
    row.depth = depth;
    row.label = label.c_str();
    row.tag = typeColor(n.type());
    row.tagText = typeTag(n.type());
    row.hasChildren = !n.children().empty();
    row.selected = view.selected == &n;
    bool wasVisible = visible;
    if (gui.treeRow(row, open, visible)) {
        view.selected = &n;
        view.selectedParent = parent;
        clicked = &n;
        center = r4::transformPoint(parent, n.bounds().center);
        radius = n.bounds().radius;
    }
    it->second = open;
    if (visible != wasVisible) {
        if (!visible) {
            view.hiddenMasks[&n] = n.mask();
            n.setMask(0);
        } else {
            n.setMask(view.hiddenMasks[&n]);
            view.hiddenMasks.erase(&n);
        }
    }
    if (open) {
        r4::Mat4 world = parent * n.matrix();
        for (const auto &c : n.children()) {
            if (r4::scene::Node *k = graphRows(gui, view, *c, world, depth + 1, center, radius)) {
                clicked = k;
            }
        }
    }
    return clicked;
}

} // namespace

int main(int argc, char **argv) {
    std::string dataDir = ".";
    std::string startObject = "F-16DES";
    std::string startMission;
    std::string focus;
    std::string shot;
    std::string diagDir;
    std::vector<std::string> tres;
    int frames = -1;
    bool openTextures = false;
    bool graphAtStart = false;
    float startDistance = 0;
    float startAt[2] = {0, 0};
    bool hasStartAt = false;
    bool flatAtStart = false;
    bool gouraudAtStart = false;
    bool litTerrainAtStart = false;
    bool shadowsAtStart = false;
    bool fogAtStart = true;
    bool onGroundAtStart = true;
    bool treeShadowsAtStart = false;
    int forestsAtStart = 2;
    std::string gameAtStart;
    int woodsAtStart = 0;
    int upscaleAtStart = 0;
    bool mipmapsAtStart = false;
    float anisoAtStart = 1;
    int reliefAtStart = 2;
    float sunAtStart[2] = {1000, 1000};
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-data" && i + 1 < argc) {
            dataDir = argv[++i];
        } else if (a == "-tre" && i + 1 < argc) {
            tres.push_back(argv[++i]);
        } else if (a == "-object" && i + 1 < argc) {
            startObject = argv[++i];
        } else if (a == "-mission" && i + 1 < argc) {
            startMission = argv[++i];
        } else if (a == "-focus" && i + 1 < argc) {
            focus = argv[++i];
        } else if (a == "-frames" && i + 1 < argc) {
            frames = std::atoi(argv[++i]);
        } else if (a == "-shot" && i + 1 < argc) {
            shot = argv[++i];
        } else if (a == "-textures") {
            openTextures = true;
        } else if (a == "-graph") {
            graphAtStart = true;
        } else if (a == "-lit") {
            litTerrainAtStart = true;
        } else if (a == "-relief" && i + 1 < argc) {
            reliefAtStart = std::atoi(argv[++i]);
        } else if (a == "-sun" && i + 2 < argc) {
            sunAtStart[0] = (float)std::atof(argv[++i]);
            sunAtStart[1] = (float)std::atof(argv[++i]);
        } else if (a == "-flat") {
            flatAtStart = true;
        } else if (a == "-gouraud") {
            gouraudAtStart = true;
        } else if (a == "-shadows") {
            shadowsAtStart = true;
        } else if (a == "-nofog") {
            fogAtStart = false;
        } else if (a == "-treeshadows") {
            treeShadowsAtStart = true;
        } else if (a == "-nolift") {
            onGroundAtStart = false;
        } else if (a == "-at" && i + 1 < argc) {
            hasStartAt = std::sscanf(argv[++i], "%f,%f", &startAt[0], &startAt[1]) == 2;
        } else if (a == "-upscale" && i + 1 < argc) {
            upscaleAtStart = std::clamp(std::atoi(argv[++i]), 0, 3);
        } else if (a == "-mipmaps") {
            mipmapsAtStart = true;
        } else if (a == "-aniso" && i + 1 < argc) {
            anisoAtStart = std::max(1.0f, (float)std::atof(argv[++i]));
        } else if (a == "-woods" && i + 1 < argc) {
            woodsAtStart = std::clamp(std::atoi(argv[++i]), 0, 1);
        } else if (a == "-game" && i + 1 < argc) {
            gameAtStart = argv[++i];
        } else if (a == "-forests" && i + 1 < argc) {
            forestsAtStart = std::clamp(std::atoi(argv[++i]), 0, 3);
        } else if (a == "-diag" && i + 1 < argc) {
            diagDir = argv[++i];
        } else if (a == "-distance" && i + 1 < argc) {
            startDistance = (float)std::atof(argv[++i]);
        }
    }

    r4bridge::textureQuality().upscale = (r4::assets::Upscale)upscaleAtStart;
    r4bridge::textureQuality().mipmaps = mipmapsAtStart;

    Config &config = Config::getInstance();
    AssetManager &assets = AssetManager::getInstance();
    if (dataDir.back() != '/' && dataDir.back() != '\\') {
        dataDir += '/';
    }
    assets.SetBase(dataDir.c_str());
    for (const std::string &ini : {dataDir + "config.ini", dataDir + "assets/config.ini", std::string("./assets/config.ini")}) {
        if (SDL_RWops *f = SDL_RWFromFile(ini.c_str(), "rb")) {
            SDL_RWclose(f);
            config.load(ini);
            std::printf("r4objviewer: configuration %s\n", ini.c_str());
            break;
        }
    }
    if (flatAtStart) {
        config.setBool("Game", "object_gouraud", false);
        config.setBool("Game", "terrain_flat", true);
    }
    struct Game {
        std::string name;
        std::vector<std::string> tres;
        size_t own = 1000;
    };
    std::vector<Game> games;
    auto exists = [&](const std::string &file) {
        SDL_RWops *f = SDL_RWFromFile((dataDir + file).c_str(), "rb");
        if (f) {
            SDL_RWclose(f);
        }
        return f != nullptr;
    };
    if (!tres.empty()) {
        games.push_back({"Custom", tres});
    } else {
        if (exists("BIGTRE.TRE")) {
            Game sc{"Strike Commander", {"BIGTRE.TRE"}};
            if (exists("TEXTURES.TRE")) {
                sc.tres.push_back("TEXTURES.TRE");
            } else if (exists("TOBIGTRE.TRE")) {
                sc.tres.push_back("TOBIGTRE.TRE");
            }
            sc.own = 1;
            games.push_back(sc);
        }
        if (exists("TOBIGTRE.TRE")) {
            games.push_back({"Tactical Operations", {"TOBIGTRE.TRE"}});
        }
        if (games.empty()) {
            games.push_back({"Strike Commander", {"OBJECTS.TRE", "MISC.TRE"}});
        }
    }
    int game = 0;
    for (size_t i = 0; i < games.size(); i++) {
        if (!gameAtStart.empty() && (upperOf(games[i].name).find(upperOf(gameAtStart)) != std::string::npos || (gameAtStart == "sc" && games[i].name == "Strike Commander") || (gameAtStart == "to" && games[i].name == "Tactical Operations"))) {
            game = (int)i;
        }
    }
    bool lookForMission = !startMission.empty() && gameAtStart.empty();

    Window win;
    if (!win.open("r4objviewer - libRealSpace + R4nkine", 1400, 860)) {
        std::fprintf(stderr, "r4objviewer: %s\n", SDL_GetError());
        return 1;
    }
    std::string error;
    std::unique_ptr<r4::gpu::Device> device = r4::gpu::createGLDevice(
        [](const char *n) {
            return SDL_GL_GetProcAddress(n);
        },
        &error);
    if (!device) {
        std::fprintf(stderr, "r4objviewer: %s\n", error.c_str());
        return 1;
    }
    std::printf("r4objviewer: %s\n", device->name());

    for (int tried = 0;;) {
        assets.init(games[game].tres);
        std::printf("r4objviewer: %s (%s)\n", games[game].name.c_str(), games[game].tres[0].c_str());

        TreEntry *paletteEntry = assets.GetEntryByName("..\\..\\DATA\\PALETTE\\PALETTE.IFF");
        if (!paletteEntry) {
            std::fprintf(stderr, "r4objviewer: pas de PALETTE.IFF dans les TRE de %s\n", dataDir.c_str());
            return 1;
        }
        RSPalette palette;
        palette.initFromFileRam(paletteEntry->data, paletteEntry->size);
        std::vector<ObjectEntry> objects = listObjects(assets, games[game].own);
        if (objects.empty()) {
            std::fprintf(stderr, "r4objviewer: aucun objet (OBJECTS\\*.IFF) dans les TRE\n");
            return 1;
        }
        std::printf("r4objviewer: %zu objets\n", objects.size());
        assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
        assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
        assets.texture_root_path = "..\\..\\DATA\\TXM\\";
        assets.txm_filename = assets.texture_root_path + "TXMPACK.PAK";
        assets.acc_filename = assets.texture_root_path + "ACCPACK.PAK";
        std::vector<WorldEntry> worlds = listWorlds(assets, dataDir, games[game].own);
        std::printf("r4objviewer: %zu missions ou mondes avec leur terrain\n", worlds.size());
        if (lookForMission) {
            bool found = false;
            for (const WorldEntry &w : worlds) {
                found |= w.label == startMission || w.label == startMission + " (world)";
            }
            if (!found && ++tried < (int)games.size()) {
                game = (game + 1) % (int)games.size();
                continue;
            }
            lookForMission = false;
        }
        int switchTo = -1;

        {
            r4::render::Renderer renderer(*device);
            renderer.options().anisotropy = anisoAtStart;
            r4::render::PostProcess post(*device);
            r4::ui::Context gui(*device);
            r4::render::Environment &env = renderer.environment();
            env.clearColor = {0.30f, 0.42f, 0.60f, 1};
            env.lightDirection = r4::normalize(r4::Vec3{0.3f, 0.5f, -0.8f});
            r4::Vec3 sky{env.clearColor.x, env.clearColor.y, env.clearColor.z};

            int current = -1;
            for (size_t i = 0; i < objects.size(); i++) {
                if (objects[i].label == startObject) {
                    current = (int)i;
                }
            }
            if (current < 0) {
                std::fprintf(stderr, "r4objviewer: pas d'objet %s, le premier est montre\n", startObject.c_str());
                current = 0;
            }
            std::unique_ptr<RSEntity> entity;
            std::shared_ptr<r4::assets::Model> model;
            r4::scene::NodePtr root;
            std::shared_ptr<r4::scene::Geode> selectionSphere;
            std::shared_ptr<r4::scene::Geode> selectionBox;
            std::shared_ptr<r4::scene::Group> selection = makeSelectionMarkers(renderer, selectionSphere, selectionBox);
            selection->setMask(0);
            std::shared_ptr<r4::scene::Group> top = r4::scene::make<r4::scene::Group>();
            top->setName("view");
            r4::Sphere bounds;
            r4bridge::Lighting lighting;
            struct Thumb {
                std::string name;
                int width;
                int height;
                int frames;
                r4::gpu::TextureHandle texture;
            };
            std::vector<Thumb> thumbs;
            size_t modelVertices = 0;
            size_t modelTriangles = 0;
            double loadMs = 0;
            float listScroll = 0;
            int listRows = 20;

            auto load = [&](int index) {
                const ObjectEntry &o = objects[index];
                Uint64 t0 = SDL_GetPerformanceCounter();
                entity = std::make_unique<RSEntity>();
                entity->InitFromRAM(o.entry->data, o.entry->size, o.label);
                model = r4bridge::toModel(entity.get(), *palette.GetColorPalette(), palette.ranges, lighting);
                auto group = r4::scene::make<r4::scene::Group>();
                if (model) {
                    group->addChild(renderer.upload(*model));
                }
                root = group;
                loadMs = (SDL_GetPerformanceCounter() - t0) * 1000.0 / SDL_GetPerformanceFrequency();
                bounds = root->bounds();
                if (bounds.radius <= 0) {
                    bounds.radius = 1;
                }
                modelVertices = modelTriangles = 0;
                if (model) {
                    for (const r4::assets::Mesh &m : model->meshes) {
                        modelVertices += m.vertices.size();
                        modelTriangles += m.indices.size() / 3;
                    }
                }
                for (Thumb &t : thumbs) {
                    device->destroyTexture(t.texture);
                }
                thumbs.clear();
                const VGAPalette &pal = *palette.GetColorPalette();
                for (RSImage *img : entity->images) {
                    if (!img || !img->data || !img->width || !img->height) {
                        continue;
                    }
                    r4::assets::Image im;
                    im.width = (int)img->width;
                    im.height = (int)img->height;
                    im.channels = 4;
                    im.clampS = im.clampT = true;
                    im.pixels.resize((size_t)im.width * im.height * 4);
                    for (size_t p = 0; p < (size_t)im.width * im.height; p++) {
                        const Texel &c = pal.colors[img->data[p]];
                        uint8_t *d = &im.pixels[p * 4];
                        d[0] = c.r;
                        d[1] = c.g;
                        d[2] = c.b;
                        d[3] = img->data[p] == r4bridge::PALETTE_CLEAR ? 0 : 255;
                    }
                    thumbs.push_back({img->name, im.width, im.height, img->nbframes, renderer.uploadImage(im, false)});
                }
                if (index < (int)listScroll || index >= (int)listScroll + listRows) {
                    listScroll = (float)std::max(0, index - listRows / 2);
                }
            };
            load(current);

            WorldScene world(assets, palette, renderer);
            int mode = 0;
            int currentWorld = -1;
            int terrainLod = 0;
            bool showTerrain = true;
            bool showMapObjects = true;
            bool showParts = true;
            bool showOverlays = true;
            bool onGround = onGroundAtStart;
            bool showForests = true;
            bool showBuildings = true;
            int forests = forestsAtStart;
            const float kForestDensity[] = {0, 0.4f, 1, 2.5f};
            world.setForestDensity(kForestDensity[forests]);
            int woods = woodsAtStart;
            world.setForestMode((r4bridge::ForestMode)woods);
            std::vector<bool> showScenes;
            GraphView graph;
            bool showGraph = graphAtStart;
            std::string worldError;
            const r4::Vec3 objectLight = env.lightDirection;
            float sunHeading = r4::axes::headingOf(-objectLight);
            float sunElevation = r4::axes::pitchOf(-objectLight);
            auto sunTowards = [&]() {
                float h = r4::radians(sunHeading);
                float el = r4::radians(sunElevation);
                return r4::axes::kRight * (-std::sin(h) * std::cos(el)) + r4::axes::kForward * (std::cos(h) * std::cos(el)) + r4::axes::kUp * std::sin(el);
            };
            auto sunFrom = [&](r4::Vec3 towards) {
                sunHeading = r4::axes::headingOf(towards);
                sunElevation = r4::axes::pitchOf(towards);
            };
            bool dynamicTerrainLight = litTerrainAtStart;
            int reliefChoice = std::clamp(reliefAtStart, 0, 4);
            auto reliefOf = [&]() {
                return (float)(1 << reliefChoice);
            };
            bool followGround = true;
            float worldScroll = 0;
            auto loadWorld = [&](int index) {
                currentWorld = index;
                worldError.clear();
                if (!world.load(worlds[index], worldError)) {
                    std::fprintf(stderr, "r4objviewer: %s\n", worldError.c_str());
                }
                world.showTerrain(showTerrain);
                world.showMapObjects(showMapObjects);
                world.showParts(showParts);
                world.showOverlays(showOverlays);
                world.setOnGround(onGround);
                world.showForests(showForests);
                world.showBuildings(showBuildings);
                if (world.hasLight) {
                    sunFrom(world.lightTowards);
                }
                if (world.terrainLightDynamic() != dynamicTerrainLight || world.terrainRelief() != reliefOf()) {
                    world.setTerrainLight(dynamicTerrainLight, reliefOf());
                }
                showScenes.assign(world.scenes().size() + 1, true);
                showScenes.back() = false;
                graph.reset();
                std::printf("r4objviewer: %s : %zu triangles de terrain, %zu objets de carte, %zu parts, %zu bâtiments, %zu arbres, %.0f ms\n", worlds[index].label.c_str(), world.terrainTriangles, world.mapObjects().size(), world.parts().size(), world.buildings, world.forestTrees, world.loadMs);
                if (index < (int)worldScroll || index >= (int)worldScroll + listRows) {
                    worldScroll = (float)std::max(0, index - listRows / 2);
                }
            };

            r4::render::Orbit orbit;
            orbit.setAngles(-140, 20, 0);
            r4::Vec3 target = bounds.center;
            float distance = bounds.radius * 3;
            auto select = [&](int index) {
                graph.reset();
                load(current = index);
                target = bounds.center;
                distance = bounds.radius * 3;
            };
            auto selectWorld = [&](int index) {
                loadWorld(index);
                if (world.parts().empty()) {
                    r4::Sphere b = world.mapBounds();
                    target = b.center;
                    distance = b.radius * 1.6f;
                } else {
                    target = world.start;
                    distance = 3000;
                }
                orbit.setAngles(-140, 25, 0);
            };
            if (!startMission.empty()) {
                for (size_t i = 0; i < worlds.size(); i++) {
                    if (worlds[i].label == startMission || worlds[i].label == startMission + " (world)") {
                        currentWorld = (int)i;
                    }
                }
                if (currentWorld >= 0) {
                    mode = 1;
                    selectWorld(currentWorld);
                    if (!diagDir.empty()) {
                        std::string error;
                        if (terrainDiagnostic(world.area(), *palette.GetColorPalette(), palette.ranges, diagDir, error)) {
                            std::printf("r4objviewer: diagnostic du terrain dans %s\n", diagDir.c_str());
                        } else {
                            std::fprintf(stderr, "r4objviewer: diagnostic : %s\n", error.c_str());
                        }
                    }
                    bool found = false;
                    for (const auto *list : {&world.parts(), &world.mapObjects()}) {
                        for (const WorldScene::Item &it : *list) {
                            if (!found && !focus.empty() && it.name.compare(0, focus.size(), focus) == 0) {
                                target = it.position;
                                distance = std::max(it.radius * 4, 60.0f);
                                found = true;
                            }
                        }
                    }
                } else {
                    std::fprintf(stderr, "r4objviewer: pas de mission %s avec son terrain\n", startMission.c_str());
                }
            }
            if (hasStartAt && mode == 1) {
                target = {startAt[0], startAt[1], 0};
                float ground;
                if (world.groundHeight(target, ground)) {
                    target.z = ground;
                }
            }
            if (startDistance > 0) {
                distance = startDistance;
            }
            if (sunAtStart[0] < 999) {
                sunHeading = sunAtStart[0];
                sunElevation = sunAtStart[1];
            }
            r4::render::Camera camera;

            r4::ui::Input input;
            bool quit = false;
            bool sceneDrag = false;
            bool wireframe = false;
            bool openRender = false;
            bool openPost = false;
            bool openPerf = true;
            bool openObject = true;
            bool vsync = true;
            bool flat = !config.getBool("Game", "object_gouraud", true) || config.getBool("Game", "terrain_flat", false);
            bool gouraudByVertex = gouraudAtStart;
            renderer.options().rampGouraud = gouraudByVertex;
            renderer.options().flatShading = flat;
            bool openWorld = true;
            bool openParts = false;
            bool openMapObjects = false;
            bool openScenes = true;
            bool openSun = false;
            bool openNav = true;
            bool shadows = shadowsAtStart;
            bool treeShadows = treeShadowsAtStart;
            bool fog = fogAtStart;
            float fogStartKm = 120;
            float fogEndKm = 450;
            int shadowRange = 0;
            const float kShadowRanges[] = {0, 500, 2000, 10000, 50000};
            std::deque<float> frameMs;
            std::deque<float> cullMs;
            std::deque<float> drawMs;
            Uint64 lastFrame = SDL_GetPerformanceCounter();
            const float leftWidth = 230;
            const float rightWidth = 310;
            r4::Vec2 doubleClick{-1, -1};
            int lastVx = 0;
            int lastVw = 1;
            while (!quit) {
                input.pressed = input.released = false;
                input.wheel = 0;
                SDL_Event e;
                while (SDL_PollEvent(&e)) {
                    bool guiMouse = gui.wantsMouse() && !sceneDrag;
                    if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                        quit = true;
                    }
                    if (e.type == SDL_KEYDOWN && mode == 0 && (e.key.keysym.sym == SDLK_DOWN || e.key.keysym.sym == SDLK_UP)) {
                        int step = e.key.keysym.sym == SDLK_DOWN ? 1 : -1;
                        int n = (int)objects.size();
                        select((current + step + n) % n);
                    }
                    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && e.button.clicks == 2 && !guiMouse && mode == 1 && currentWorld >= 0) {
                        doubleClick = {(float)e.button.x, (float)e.button.y};
                    }
                    if (e.type == SDL_MOUSEMOTION) {
                        input.mouseX = (float)e.motion.x;
                        input.mouseY = (float)e.motion.y;
                        if (sceneDrag && (e.motion.state & SDL_BUTTON_LMASK)) {
                            orbit.turn(e.motion.xrel * 0.4f, e.motion.yrel * 0.4f);
                        }
                        if (sceneDrag && (e.motion.state & SDL_BUTTON_RMASK)) {
                            float k = distance * 0.0015f;
                            target += orbit.right() * (-e.motion.xrel * k) + orbit.up() * (e.motion.yrel * k);
                        }
                    } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
                        bool down = e.type == SDL_MOUSEBUTTONDOWN;
                        if (down && !guiMouse) {
                            sceneDrag = true;
                        }
                        if (!sceneDrag) {
                            (e.button.button == SDL_BUTTON_LEFT ? input.left : input.right) = down;
                            (down ? input.pressed : input.released) = true;
                        }
                        if (!down && !(SDL_GetMouseState(nullptr, nullptr) & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK))) {
                            sceneDrag = false;
                        }
                    } else if (e.type == SDL_MOUSEWHEEL) {
                        if (guiMouse) {
                            input.wheel += (float)e.wheel.y;
                        } else {
                            distance *= std::exp(-e.wheel.y * 0.12f);
                        }
                    }
                }
                win.sizes();
                if (frames >= 0 && frames-- == 0) {
                    quit = true;
                }
                float frameSeconds = frameMs.empty() ? 1.0f / 60 : std::min(0.1f, frameMs.back() / 1000.0f);

                if (mode == 1 && currentWorld >= 0) {
                    const Uint8 *keys = SDL_GetKeyboardState(nullptr);
                    float fwd = (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN] ? 1.0f : 0.0f);
                    float side = (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT] ? 1.0f : 0.0f);
                    float turn = (keys[SDL_SCANCODE_Q] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_E] ? 1.0f : 0.0f);
                    float zoom = (keys[SDL_SCANCODE_F] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_R] ? 1.0f : 0.0f);
                    bool fast = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
                    if (fwd != 0 || side != 0) {
                        r4::Vec3 f = orbit.forward();
                        r4::Vec3 r = orbit.right();
                        f -= r4::axes::kUp * r4::axes::heightOf(f);
                        r -= r4::axes::kUp * r4::axes::heightOf(r);
                        if (r4::length(f) > 1e-4f) {
                            f = r4::normalize(f);
                        }
                        if (r4::length(r) > 1e-4f) {
                            r = r4::normalize(r);
                        }
                        float speed = std::max(50.0f, distance) * (fast ? 3.0f : 0.8f);
                        target += (f * fwd + r * side) * (speed * frameSeconds);
                    }
                    if (turn != 0) {
                        float h;
                        float p;
                        float rl;
                        orbit.angles(h, p, rl);
                        orbit.setAngles(h + turn * 60 * frameSeconds, p, rl);
                    }
                    if (zoom != 0) {
                        distance *= std::exp(zoom * frameSeconds * (fast ? 2.5f : 1.2f));
                    }
                    if (doubleClick.x >= 0) {
                        float pixel = win.pointWidth > 0 ? (float)win.width / win.pointWidth : 1;
                        float nx = (doubleClick.x * pixel - lastVx) / lastVw * 2 - 1;
                        float ny = 1 - doubleClick.y * pixel / std::max(1, win.height) * 2;
                        r4::Mat4 inv = r4::inverse(camera.projection * camera.view);
                        r4::Vec4 a = inv * r4::Vec4(nx, ny, -1, 1);
                        r4::Vec4 b = inv * r4::Vec4(nx, ny, 1, 1);
                        r4::Vec3 from = r4::Vec3(a) / a.w;
                        r4::Vec3 dir = r4::normalize(r4::Vec3(b) / b.w - r4::Vec3(a) / a.w);
                        float step = std::max(5.0f, distance / 200);
                        float t0 = 0;
                        float ground = 0;
                        bool hit = false;
                        for (float t = 0; t < 600000 && !hit; t += step) {
                            r4::Vec3 p = from + dir * t;
                            if (world.groundHeight(p, ground) && r4::axes::heightOf(p) <= ground) {
                                float lo = t0;
                                float hi = t;
                                for (int k = 0; k < 24; k++) {
                                    float mid = (lo + hi) / 2;
                                    r4::Vec3 q = from + dir * mid;
                                    float g = 0;
                                    (world.groundHeight(q, g) && r4::axes::heightOf(q) <= g ? hi : lo) = mid;
                                }
                                target = from + dir * hi;
                                hit = true;
                            }
                            t0 = t;
                            step *= 1.01f;
                        }
                        doubleClick = {-1, -1};
                    }
                    float ground = 0;
                    if (followGround && world.groundHeight(target, ground)) {
                        target = target - r4::axes::kUp * r4::axes::heightOf(target) + r4::axes::kUp * ground;
                    }
                }

                Uint64 now = SDL_GetPerformanceCounter();
                auto push = [](std::deque<float> &d, double v) {
                    d.push_back((float)v);
                    if (d.size() > 240) {
                        d.pop_front();
                    }
                };
                push(frameMs, (now - lastFrame) * 1000.0 / SDL_GetPerformanceFrequency());
                lastFrame = now;
                const r4::render::RenderStats &st = renderer.stats();
                push(cullMs, st.cullMs);
                push(drawMs, st.drawMs);

                float eyeDistance = r4::length(orbit.eye() - bounds.center);
                int shownLod = 0;
                if (model && model->nodes.size() > 1 && model->nodes[1].kind == r4::assets::NodeKind::LOD) {
                    const std::vector<float> &r = model->nodes[1].lodRanges;
                    float d = eyeDistance * renderer.options().lodScale;
                    for (size_t i = 0; i + 1 < r.size(); i++) {
                        if (d >= r[i] && d < r[i + 1]) {
                            shownLod = (int)i;
                        }
                    }
                }

                gui.beginFrame(win.pointWidth, win.pointHeight, input, win.pointWidth > 0 ? (float)win.width / win.pointWidth : 1);

                gui.beginPanel("objects", {0, 0, leftWidth, (float)win.pointHeight}, "r4objviewer");
                if (games.size() > 1) {
                    std::vector<std::string> names;
                    for (const Game &g : games) {
                        names.push_back(g.name);
                    }
                    int chosen = game;
                    if (gui.menu("Game", chosen, names) && chosen != game) {
                        switchTo = chosen;
                        quit = true;
                    }
                }
                if (gui.radio("Mode", mode, {"Objects", "Missions"}) && mode == 1 && currentWorld < 0 && !worlds.empty()) {
                    selectWorld(0);
                }
                if (mode == 1) {
                    if (worlds.empty()) {
                        gui.message("No terrain (.PAK) in the data\nfolder for these missions.");
                    } else {
                        gui.message(fmt("%d / %d   %s", currentWorld + 1, (int)worlds.size(), currentWorld >= 0 ? worlds[currentWorld].label.c_str() : "-").c_str());
                        listRows = std::max(4, (int)(gui.remainingHeight() / 21.0f) - 1);
                        int maxFirst = std::max(0, (int)worlds.size() - listRows);
                        worldScroll = std::clamp(worldScroll - input.wheel * 3, 0.0f, (float)maxFirst);
                        int first = (int)worldScroll;
                        for (int i = first; i < std::min((int)worlds.size(), first + listRows); i++) {
                            r4::ui::Context::TreeRow row;
                            row.id = &worlds[i];
                            row.label = worlds[i].label.c_str();
                            row.selected = i == currentWorld;
                            bool open = false;
                            bool visible = true;
                            if (gui.treeRow(row, open, visible) && i != currentWorld) {
                                selectWorld(i);
                            }
                        }
                    }
                    gui.endPanel();
                } else {
                    gui.message(fmt("%d / %d   %s", current + 1, (int)objects.size(), objects[current].label.c_str()).c_str());
                    gui.columns(2);
                    if (gui.button("< Previous")) {
                        select((current + (int)objects.size() - 1) % (int)objects.size());
                    }
                    if (gui.button("Next >")) {
                        select((current + 1) % (int)objects.size());
                    }
                    gui.columns(1);
                    {
                        listRows = std::max(4, (int)(gui.remainingHeight() / 21.0f) - 1);
                        int maxFirst = std::max(0, (int)objects.size() - listRows);
                        listScroll = std::clamp(listScroll - input.wheel * 3, 0.0f, (float)maxFirst);
                        int first = (int)listScroll;
                        for (int i = first; i < std::min((int)objects.size(), first + listRows); i++) {
                            r4::ui::Context::TreeRow row;
                            row.id = &objects[i];
                            row.label = objects[i].label.c_str();
                            row.selected = i == current;
                            bool open = false;
                            bool visible = true;
                            if (gui.treeRow(row, open, visible) && i != current) {
                                select(i);
                            }
                        }
                        gui.label(fmt("%d-%d of %d, wheel to scroll", first + 1, std::min((int)objects.size(), first + listRows), (int)objects.size()).c_str(), r4::ui::Font::SMALL);
                    }
                    gui.endPanel();
                }

                gui.beginPanel("view", {(float)win.pointWidth - rightWidth, 0, rightWidth, (float)win.pointHeight}, "r4objviewer");
                if (gui.section("Scene Graph", showGraph)) {
                    r4::scene::Node &shown = mode == 1 && currentWorld >= 0 ? *world.root() : *root;
                    r4::Vec3 center;
                    float radius = 0;
                    if (graphRows(gui, graph, shown, r4::Mat4(1.0f), 0, center, radius)) {
                        target = center;
                        distance = std::max(radius * 3, 60.0f);
                    }
                }
                if (gui.section("Render", openRender)) {
                    gui.columns(2);
                    gui.toggle("Wireframe", wireframe);
                    gui.toggle("Textures", renderer.options().textures);
                    gui.columns(1);
                    if (gui.toggle("Flat Shading", flat)) {
                        renderer.options().flatShading = flat;
                        config.setBool("Game", "terrain_flat", flat);
                        if (mode == 1 && currentWorld >= 0) {
                            loadWorld(currentWorld);
                        }
                    }
                    if (gui.toggle("Gouraud by Vertex (as the game)", gouraudByVertex)) {
                        renderer.options().rampGouraud = gouraudByVertex;
                    }
                    r4bridge::TextureQuality &tq = r4bridge::textureQuality();
                    int up = (int)tq.upscale;
                    bool remake = gui.menu("Texture Upscale", up, {"None (as the game)", "2xSaI", "xBR 2x", "xBR 4x"});
                    tq.upscale = (r4::assets::Upscale)up;
                    remake |= gui.toggle("Mipmaps", tq.mipmaps);
                    const float kAniso[] = {1, 2, 4, 8, 16};
                    int an = 0;
                    while (an < 4 && kAniso[an] < renderer.options().anisotropy) {
                        an++;
                    }
                    bool anisoOk = device->limits().maxAnisotropy > 1;
                    if (gui.menu("Anisotropic Filter", an, {"Off", "2x", "4x", "8x", "16x"}, anisoOk)) {
                        renderer.options().anisotropy = kAniso[an];
                        remake = true;
                    }
                    if (remake) {
                        if (mode == 1 && currentWorld >= 0) {
                            loadWorld(currentWorld);
                        } else if (current >= 0) {
                            load(current);
                        }
                    }
                    gui.columns(2);
                    gui.columns(1);
                    gui.slider("LOD Scale", renderer.options().lodScale, 0.05f, 4, "%.2f", true);
                    bool relit = gui.slider("Ambient", lighting.ambient, 0, 1);
                    relit |= gui.slider("Light", lighting.light, 0, 1);
                    if (relit) {
                        load(current);
                    }
                    if (gui.color("Sky", sky)) {
                        env.clearColor = {sky, 1};
                    }
                    gui.toggle("Fog", fog);
                    gui.slider("Fog Start (km)", fogStartKm, 10, 600, "%.0f");
                    gui.slider("Fog End (km)", fogEndKm, 20, 1200, "%.0f");
                    float heading;
                    float pitch;
                    float roll;
                    orbit.angles(heading, pitch, roll);
                    bool turned = gui.slider("Yaw (Heading)", heading, -180, 180, "%.0f");
                    turned |= gui.slider("Pitch", pitch, -89, 89, "%.0f");
                    turned |= gui.slider("Roll", roll, -180, 180, "%.0f");
                    if (turned) {
                        orbit.setAngles(heading, pitch, roll);
                    }
                    gui.slider("Distance", distance, bounds.radius * 0.2f, bounds.radius * 400, "%.0f", true);
                    if (gui.button("Reset View")) {
                        orbit.setAngles(-140, 20, 0);
                        target = bounds.center;
                        distance = bounds.radius * 3;
                    }
                }
                if (gui.section("Post Process", openPost)) {
                    r4::ui::postProcessControls(gui, post.settings);
                }
                if (gui.section("Performance", openPerf)) {
                    if (gui.toggle("Vertical Sync", vsync)) {
                        SDL_GL_SetSwapInterval(vsync ? 1 : 0);
                    }
                    float avg = 0;
                    for (float v : frameMs) {
                        avg += v;
                    }
                    avg = frameMs.empty() ? 0 : avg / frameMs.size();
                    gui.message(fmt("%.1f fps   %.2f ms per frame", avg > 0 ? 1000 / avg : 0, avg).c_str());
                    plot(gui, frameMs, 16.667f, r4::ui::Color::rgb(0.4f, 1, 0.4f), "frame");
                    plot(gui, cullMs, 0.25f, r4::ui::Color::rgb(0.4f, 0.8f, 1), "cull");
                    plot(gui, drawMs, 0.5f, r4::ui::Color::rgb(1, 0.6f, 0.3f), "draw");
                    gui.table({{"Draw calls", fmt("%u", st.gpu.drawCalls)}, {"Triangles drawn", fmt("%u", st.gpu.triangles)}, {"Pipeline / texture changes", fmt("%u / %u", st.gpu.pipelineChanges, st.gpu.textureChanges)}, {"Items opaque / transparent", fmt("%u / %u", st.opaqueItems, st.transparentItems)}, {"Shadow casters / CPU", fmt("%u / %.2f ms", st.shadowCasters, st.shadowMs)}, {"Nodes visited / culled", fmt("%u / %u", st.nodesVisited, st.nodesCulled)}});
                }
                if (gui.section("Sun", openSun)) {
                    gui.slider("Sun Heading", sunHeading, -180, 180, "%.0f");
                    gui.slider("Sun Elevation", sunElevation, -10, 90, "%.0f");
                    if (mode == 1 && currentWorld >= 0) {
                        gui.columns(2);
                        if (gui.button("World's Light", world.hasLight)) {
                            sunFrom(world.lightTowards);
                        }
                        if (gui.toggle("Lit Terrain", dynamicTerrainLight)) {
                            world.setTerrainLight(dynamicTerrainLight, reliefOf());
                        }
                        gui.columns(1);
                        if (dynamicTerrainLight && gui.menu("Relief of the Light", reliefChoice, {"x1 (as it is)", "x2", "x4", "x8", "x16"})) {
                            world.setTerrainLight(true, reliefOf());
                        }
                        gui.label(dynamicTerrainLight ? "terrain: its ramps lit by the sun" : "terrain: the light of the data (RSArea)", r4::ui::Font::SMALL);
                    }
                    gui.toggle("Cast Shadows", shadows);
                    if (shadows) {
                        gui.menu("Shadow Distance", shadowRange, {"Auto (the view)", "500", "2 000", "10 000", "50 000"});
                        gui.toggle("Tree Shadows", treeShadows);
                        gui.slider("Shadow on Unlit", renderer.options().shadowStrength, 0, 1, "%.2f");
                    }
                }
                if (mode == 1 && currentWorld >= 0 && gui.section("Navigation", openNav)) {
                    gui.message("W A S D / Z Q S D, arrows: move\nQ E / A E: turn   R F: in, out\nShift: faster   double click: go there");
                    gui.toggle("Follow the Ground", followGround);
                    gui.columns(2);
                    if (gui.button("Whole Map")) {
                        r4::Sphere b = world.mapBounds();
                        target = b.center;
                        distance = b.radius * 1.6f;
                    }
                    if (gui.button("Top View")) {
                        float h;
                        float p;
                        float rl;
                        orbit.angles(h, p, rl);
                        orbit.setAngles(h, 89, 0);
                    }
                    gui.columns(1);
                    for (const WorldScene::Item &a : world.areas()) {
                        if (gui.button(a.name.c_str())) {
                            target = a.position;
                            distance = std::max(a.radius * 2.5f, 1500.0f);
                        }
                    }
                }
                if (mode == 1 && currentWorld >= 0) {
                    if (gui.section("World", openWorld)) {
                        gui.columns(2);
                        if (gui.toggle("Terrain", showTerrain)) {
                            world.showTerrain(showTerrain);
                        }
                        if (gui.toggle("Runways", showOverlays)) {
                            world.showOverlays(showOverlays);
                        }
                        if (gui.toggle("Map Objects", showMapObjects)) {
                            world.showMapObjects(showMapObjects);
                        }
                        if (gui.toggle("Mission Parts", showParts)) {
                            world.showParts(showParts);
                        }
                        if (gui.toggle("Forests", showForests)) {
                            world.showForests(showForests);
                        }
                        if (gui.toggle("Buildings", showBuildings)) {
                            world.showBuildings(showBuildings);
                        }
                        if (gui.toggle("On Ground", onGround)) {
                            world.setOnGround(onGround);
                        }
                        gui.columns(1);
                        if (gui.menu("Forest Source", woods, {"Green Terrain + Noise", "Woods of the Textures"})) {
                            world.setForestMode((r4bridge::ForestMode)woods);
                        }
                        if (gui.menu("Forest Density", forests, {"None", "Sparse", "Normal", "Dense"})) {
                            world.setForestDensity(kForestDensity[forests]);
                        }
                        if (gui.menu("Terrain Detail", terrainLod, {"By Distance (LOD)", "Fine (400 per block)", "Medium (100)", "Coarse (25)"})) {
                            world.setTerrainLod(terrainLod);
                        }
                        size_t missing = 0;
                        for (const auto *list : {&world.parts(), &world.mapObjects()}) {
                            for (const WorldScene::Item &it : *list) {
                                missing += it.found ? 0 : 1;
                            }
                        }
                        gui.table({{"World", world.worldName}, {"Terrain", world.terrainName}, {"Terrain blocks", fmt("%zu (a LOD each)", world.terrainBlocks)}, {"Terrain triangles (fine + skirts)", fmt("%zu", world.terrainTriangles)}, {"Terrain textures", fmt("%zu", world.terrainTextures)}, {"Runway triangles", fmt("%zu", world.overlayTriangles)}, {"Buildings", fmt("%zu", world.buildings)}, {"Trees (chunks of 2 km)", fmt("%zu (%zu)", world.forestTrees, world.forestChunks)}, {"Map objects", fmt("%zu", world.mapObjects().size())}, {"Mission parts", fmt("%zu", world.parts().size())}, {"Objects not in the TRE", fmt("%zu", missing)}, {"Light ambient / direct", fmt("%.2f / %.2f", world.lighting.ambient, world.lighting.light)}, {"Load + convert", fmt("%.0f ms", world.loadMs)}});
                        if (!worldError.empty()) {
                            gui.message(worldError.c_str());
                        }
                    }
                    auto itemList = [&](const std::vector<WorldScene::Item> &items) {
                        for (const WorldScene::Item &it : items) {
                            r4::ui::Context::TreeRow row;
                            row.id = &it;
                            std::string label = it.scene >= 0 ? fmt("%s  (scene %d)", it.name.c_str(), it.scene) : it.scene == -1 ? it.name + "  (no scene)" : it.name;
                            row.label = label.c_str();
                            row.tagText = it.found ? "" : "?";
                            bool open = false;
                            bool visible = true;
                            if (gui.treeRow(row, open, visible)) {
                                target = it.position;
                                distance = std::max(it.radius * 4, 60.0f);
                            }
                        }
                    };
                    if (!world.scenes().empty() && gui.section(fmt("Scenes (%zu)", world.scenes().size()).c_str(), openScenes)) {
                        for (size_t sc = 0; sc <= world.scenes().size(); sc++) {
                            bool last = sc == world.scenes().size();
                            std::string label = last ? fmt("No scene (%zu)", world.partsWithoutScene) : fmt("%s (%zu)", world.scenes()[sc].label.c_str(), world.scenes()[sc].parts);
                            bool on = showScenes[sc];
                            if (gui.toggle(label.c_str(), on)) {
                                showScenes[sc] = on;
                                world.showScene(last ? -1 : (int)sc, on);
                            }
                        }
                    }
                    if (!world.parts().empty() && gui.section(fmt("Parts (%zu)", world.parts().size()).c_str(), openParts)) {
                        itemList(world.parts());
                    }
                    if (gui.section(fmt("Map Objects (%zu)", world.mapObjects().size()).c_str(), openMapObjects)) {
                        itemList(world.mapObjects());
                    }
                }
                if (mode == 0 && gui.section("Object", openObject)) {
                    const ObjectEntry &o = objects[current];
                    std::vector<std::pair<std::string, std::string>> rows = {{"File", fmt("%s, %u bytes", o.label.c_str(), (unsigned)o.entry->size)}, {"Vertices", fmt("%zu", entity->vertices.size())}, {"Triangles / quads", fmt("%zu / %zu", entity->triangles.size(), entity->quads.size())}, {"Textured tris / quads", fmt("%zu / %zu", entity->uvs.size(), entity->qmapuvs.size())}, {"Textures", fmt("%zu", entity->images.size())}, {"Radius", fmt("%.1f", bounds.radius)}, {"Load + convert", fmt("%.2f ms", loadMs)}};
                    for (size_t i = 0; i < entity->lods.size(); i++) {
                        rows.push_back({fmt("LOD %zu%s", i, (int)i == shownLod ? " (shown)" : ""), fmt("from %d, %d faces", (int)entity->lods[i].dist, entity->lods[i].numTriangles)});
                    }
                    rows.push_back({"Eye distance x scale", fmt("%.0f", eyeDistance * renderer.options().lodScale)});
                    if (model) {
                        rows.push_back({"R4nkine meshes", fmt("%zu (%zu materials)", model->meshes.size(), model->materials.size())});
                        rows.push_back({"R4nkine vertices", fmt("%zu (%zu triangles)", modelVertices, modelTriangles)});
                    }
                    gui.table(rows);
                }
                if (mode == 0 && gui.section(fmt("Textures (%zu)", thumbs.size()).c_str(), openTextures)) {
                    for (const Thumb &t : thumbs) {
                        std::string frames = t.frames > 1 ? fmt(", %d frames", t.frames) : "";
                        gui.label(fmt("%s  %dx%d%s", t.name.c_str(), t.width, t.height, frames.c_str()).c_str(), r4::ui::Font::SMALL);
                        gui.image(t.texture, (float)t.width / t.height, 80);
                    }
                }
                gui.endPanel();

                gui.endFrame();

                float pixel = win.pointWidth > 0 ? (float)win.width / win.pointWidth : 1;
                int vx = (int)(leftWidth * pixel);
                int vw = std::max(1, win.width - (int)((leftWidth + rightWidth) * pixel));
                lastVx = vx;
                lastVw = vw;
                orbit.target = target;
                orbit.distance = distance;
                bool showWorld = mode == 1 && currentWorld >= 0;
                if (showWorld) {
                    float fogEnd = std::max(fogEndKm, fogStartKm + 1) * 1000.0f;
                    env.fog = fog;
                    env.fogColor = {env.clearColor.x, env.clearColor.y, env.clearColor.z};
                    env.fogStart = distance + fogStartKm * 1000.0f;
                    env.fogEnd = distance + fogEnd;
                    float farPlane = fog ? env.fogEnd * 1.05f : distance + 1500000.0f;
                    orbit.apply(camera, 45, win.height > 0 ? (float)vw / win.height : 1, std::max(1.0f, distance * 0.01f), farPlane);
                    env.lightDirection = -sunTowards();
                } else {
                    env.fog = false;
                    orbit.apply(camera, 45, win.height > 0 ? (float)vw / win.height : 1, distance * 0.01f, distance + bounds.radius * 400);
                    env.lightDirection = -sunTowards();
                }
                renderer.options().shadows = shadows;
                renderer.options().shadowMask = treeShadows ? 0xffffffffu : ~WorldScene::kForestMask;
                renderer.options().shadowDistance = shadowRange > 0 ? kShadowRanges[shadowRange] : std::max(distance * 3, 100.0f);
                renderer.options().shadowCasterReach = showWorld ? 8000.0f : std::max(distance * 2, bounds.radius * 4);
                renderer.setWireframe(wireframe);
                r4::gpu::RenderTargetHandle rt = post.begin(win.width, win.height);
                renderer.setTarget(rt);
                env.time = SDL_GetTicks() / 1000.0f;
                r4::scene::NodePtr shownRoot = showWorld ? world.root() : root;
                if (top->children().size() != 2 || top->children()[0] != shownRoot) {
                    top->removeAllChildren();
                    top->addChild(shownRoot);
                    top->addChild(selection);
                }
                if (graph.selected && showGraph) {
                    auto *selected = const_cast<r4::scene::Node *>(graph.selected);
                    const r4::Sphere s = selected->bounds();
                    selectionSphere->setMatrix(graph.selectedParent * r4::translate(s.center) * r4::scale(r4::Vec3(std::max(s.radius, 1e-3f))));
                    r4::Box b = boxOf(*selected);
                    if (b.empty()) {
                        b.extend(s.center);
                    }
                    selectionBox->setMatrix(graph.selectedParent * r4::translate(b.min) * r4::scale(r4::max(b.max - b.min, r4::Vec3(1e-3f))));
                    selection->setMask(~0u);
                } else {
                    selection->setMask(0);
                }
                renderer.render(*top, camera, vx, 0, vw, win.height);
                renderer.setTarget({});
                post.end(0, 0, win.width, win.height, win.width, win.height);
                gui.render();
                device->takeStats();
                if (!shot.empty() && frames == 0) {
                    screenshot(shot.c_str(), win.width, win.height);
                }
                SDL_GL_SwapWindow(win.window);
            }
            for (Thumb &t : thumbs) {
                device->destroyTexture(t.texture);
            }
        }
        if (switchTo < 0) {
            break;
        }
        game = switchTo;
        startMission.clear();
        focus.clear();
        hasStartAt = false;
        frames = -1;
    }
    device.reset();
    return 0;
}

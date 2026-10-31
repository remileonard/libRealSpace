#pragma once
#include "precomp.h"
#include <random>

struct SCSmokeParticle {
    Vector3D position;
    Vector3D velocity;   // m/s
    float size{ 0.0f };  // m
    float life{ 0.0f };  // s restantes
    float shade{ 0.0f }; // partie entière = numéro de table FADE
    float frameAccumulator{ 0.0f };
    int shape{ -1 }; // -1 = aucune forme
    int frame{ 0 };  // rang dans SCRI s'il existe, sinon numéro d'image de la forme
};

struct SCSmokeSprite {
    Vector3D position;
    Texture *texture{ nullptr };
    float size{ 0.0f };     // m, côté du carré
    float distance{ 0.0f }; // m, de la caméra
};

// Fumée portée par un objet : ses particules, d'après un modèle du jeu de fumées partagé (RSSmokeSet).
class SCSmokeSet {
    RSSmokeSet *set{ nullptr };
    int model{ 0 };
    int step{ 0 };
    std::vector<SCSmokeParticle> particles;
    std::mt19937 rng;
    RSSmokeGeneratorInfo generator;
    float generatorRemaining{ 0.0f };
    float generatorCountdown{ 0.0f };
    bool generatorActive{ false };
    SCRenderer &Renderer = SCRenderer::getInstance();

    float jitter(float amount);
    int pickShape();
    int lastFrame(const SCSmokeParticle &p) const;
    int imageOf(const SCSmokeParticle &p) const;
    SCSmokeParticle create();

public:
    void init(RSSmokeSet *smokeSet, int smokeModel, uint32_t seed);
    void setModel(int smokeModel);
    int getModel() const;

    void spawn(Vector3D position, Vector3D velocity);
    // De P (point d'émission de cette image) vers Q (même point à l'image précédente), une particule tous les
    // « espacement » m (Particles_SpawnAlongSegment_4D1C9).
    void spawnAlongSegment(Vector3D p, Vector3D q, float dt);
    // Q = P − vitesse de l'objet × dt (SmokeTrail_Tick_3D57E).
    void emitTrail(Vector3D emissionPoint, Vector3D objectVelocity, float dt);
    void startGenerator(const RSSmokeGeneratorInfo &info);
    bool isGeneratorActive() const;
    bool emitGenerator(Vector3D position, float dt);

    void update(float dt);
    void sprites(Vector3D camera, std::vector<SCSmokeSprite> &out) const;
    void render(Vector3D camera);

    const std::vector<SCSmokeParticle> &getParticles() const;
    void clear();
};

// Point du chunk SMOK dans le monde, avec la matrice de SCRenderer::drawModel (translation, rotations Y, Z, X).
Vector3D smokeEmissionPoint(Vector3D position, Vector3D orientation, Vector3D localPoint);

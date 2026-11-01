#include "precomp.h"

#include <algorithm>
#include <cmath>

void SCSmokeSet::init(RSSmokeSet *smokeSet, int smokeModel, uint32_t seed) {
    set = smokeSet;
    particles.clear();
    rng.seed(seed);
    setModel(smokeModel);
}

void SCSmokeSet::setModel(int smokeModel) {
    model = smokeModel;
    if (model < 0 || model >= static_cast<int>(set->models.size())) {
        model = 0;
    }
    step = set->models[model].frameStep;
}

int SCSmokeSet::getModel() const {
    return model;
}

const std::vector<SCSmokeParticle> &SCSmokeSet::getParticles() const {
    return particles;
}

void SCSmokeSet::clear() {
    particles.clear();
    step = set->models[model].frameStep;
}

// Math_RandomJitter_54E1C : aléa dans [−amount, +amount].
float SCSmokeSet::jitter(float amount) {
    std::uniform_real_distribution<float> d(-amount, amount);
    return d(rng);
}

// Forme : aucune si A = 255 ; A si la chance vaut 100 ; sinon A avec la chance (Math_RandomChance_54E4F), B autrement.
int SCSmokeSet::pickShape() {
    const RSSmokeModel &m = set->models[model];
    if (m.shapeA == 255) {
        return -1;
    }
    int chosen = m.shapeA;
    if (m.chanceA != 100) {
        std::uniform_int_distribution<int> d(0, 99);
        if (d(rng) >= m.chanceA) {
            chosen = m.shapeB;
        }
    }
    if (chosen == 255 || chosen >= static_cast<int>(set->shapes.size())) {
        return -1;
    }
    return chosen;
}

int SCSmokeSet::lastFrame(const SCSmokeParticle &p) const {
    const RSSmokeModel &m = set->models[model];
    if (!m.frameOrder.empty()) {
        return static_cast<int>(m.frameOrder.size()) - 1;
    }
    if (p.shape < 0) {
        return 0;
    }
    return static_cast<int>(set->shapes[p.shape].images.size()) - 1;
}

int SCSmokeSet::imageOf(const SCSmokeParticle &p) const {
    const RSSmokeModel &m = set->models[model];
    int frame = p.frame;
    if (!m.frameOrder.empty()) {
        frame = m.frameOrder[(std::min)(frame, static_cast<int>(m.frameOrder.size()) - 1)];
    }
    return frame;
}

// Particle_InitFromSmokeModel_4C168 : taille, vie et teinte du début, vitesse nulle, image 0.
SCSmokeParticle SCSmokeSet::create() {
    const RSSmokeModel &m = set->models[model];
    SCSmokeParticle p;
    p.size = m.sizeStart;
    p.life = m.life;
    p.shade = m.shadeStart;
    p.velocity = Vector3D(0.0f, 0.0f, 0.0f);
    p.shape = pickShape();
    return p;
}

void SCSmokeSet::spawn(Vector3D position, Vector3D velocity) {
    const RSSmokeModel &m = set->models[model];
    SCSmokeParticle p = create();
    p.position.x = position.x + jitter(m.jitter);
    p.position.y = position.y + jitter(m.jitter);
    p.position.z = position.z + jitter(m.jitter);
    p.velocity = velocity;
    particles.push_back(p);
}

void SCSmokeSet::spawnAlongSegment(Vector3D p, Vector3D q, float dt) {
    const RSSmokeModel &m = set->models[model];
    Vector3D segment = q - p;
    float length = segment.Length();
    int n = static_cast<int>(length / m.spacing);
    if (n == 0 && length > 0.0f) {
        n = 1;
    }
    if (n == 0) {
        return;
    }
    Vector3D delta = segment * (1.0f / n);
    Vector3D position = p;
    for (int k = 0; k < n; k++) {
        float life = m.life - dt - k * dt / n;
        if (life <= 0.0f) {
            return;
        }
        SCSmokeParticle particle = create();
        particle.position = position;
        particle.life = life;
        particle.size = m.sizeStart + k * m.sizeRate() * dt / n;
        particle.shade = m.shadeStart + k * m.shadeRate() * dt / n;
        particle.frame = static_cast<int>(k * m.framesPerSecond * dt / n);
        particles.push_back(particle);
        position = position + delta;
        // Aléa cumulé à partir de la 3e particule (pas d'aléa entre la 1re et la 2e).
        if (k > 0 && k < n - 1) {
            position.x += jitter(m.jitter);
            position.y += jitter(m.jitter);
            position.z += jitter(m.jitter);
        }
    }
}

void SCSmokeSet::emitTrail(Vector3D emissionPoint, Vector3D objectVelocity, float dt) {
    Vector3D previous = emissionPoint - objectVelocity * dt;
    spawnAlongSegment(emissionPoint, previous, dt);
}

void SCSmokeSet::update(float dt) {
    const RSSmokeModel &m = set->models[model];
    size_t kept = 0;
    for (size_t i = 0; i < particles.size(); i++) {
        SCSmokeParticle &p = particles[i];
        p.life -= dt;
        if (p.life <= 0.0f) {
            continue;
        }
        bool removed = false;
        if ((m.renderMode == 1 || m.renderMode == 2) && step != 0) {
            p.frameAccumulator += m.framesPerSecond * dt;
            // Un seul pas par mise à jour, quand l'accumulateur dépasse une image.
            if (p.frameAccumulator > 1.0f) {
                p.frameAccumulator -= 1.0f;
                int last = lastFrame(p);
                int next = p.frame + step;
                if (m.animMode == 'R') {
                    p.frame = next;
                    if (p.frame >= last) {
                        step = -1;
                        p.frame = last;
                    }
                    if (p.frame <= 0) {
                        step = 1;
                        p.frame = 0;
                    }
                } else if (m.animMode == 'S') {
                    if (next > last) {
                        step = 0;
                    } else {
                        p.frame = next;
                    }
                } else if (m.animMode == 'T') {
                    if (next > last || next < 0) {
                        removed = true;
                    } else {
                        p.frame = next;
                    }
                } else if (m.animMode == 'W') {
                    if (next > last) {
                        p.frame = 0;
                    } else if (next < 0) {
                        p.frame = last;
                    } else {
                        p.frame = next;
                    }
                }
            }
        }
        if (removed) {
            continue;
        }
        p.size += m.sizeRate() * dt;
        p.shade += m.shadeRate() * dt;
        p.position = p.position + p.velocity * dt;
        particles[kept] = p;
        kept++;
    }
    particles.resize(kept);
}

// Côté du carré : la texture fait « côté » pixels d'image ; un pixel d'image = taille ÷ demi-taille de la forme
// mètres (échelle des sprites du jeu, SMOKE_SYSTEM.md § 5.3).
void SCSmokeSet::sprites(Vector3D camera, std::vector<SCSmokeSprite> &out) const {
    size_t first = out.size();
    for (const auto &p : particles) {
        if (p.shape < 0) {
            continue;
        }
        const RSSmokeShape &shape = set->shapes[p.shape];
        int image = imageOf(p);
        if (image < 0 || image >= static_cast<int>(shape.images.size())) {
            continue;
        }
        int shade = static_cast<int>(std::floor(p.shade));
        Texture *tex = set->getTexture(model, p.shape, image, shade);
        if (tex == nullptr) {
            continue;
        }
        SCSmokeSprite s;
        s.position = p.position;
        s.texture = tex;
        s.size = static_cast<float>(tex->width) * p.size / shape.halfSize;
        s.distance = (p.position - camera).Length();
        out.push_back(s);
    }
    std::stable_sort(out.begin() + static_cast<long>(first), out.end(),
                     [](const SCSmokeSprite &a, const SCSmokeSprite &b) {
                         return a.distance > b.distance;
                     });
}

void SCSmokeSet::render(Vector3D camera) {
    std::vector<SCSmokeSprite> list;
    sprites(camera, list);
    for (const auto &s : list) {
        Renderer.drawBillboard(s.position, s.texture, s.size);
    }
}

// SmokeGen_InitInstance_A6048 : compte à rebours à 0 (première particule à la première mise à jour), minuteur =
// durée, actif seulement si Math_RandomChance_54E4F(chance) (tirage de 0 à 99 < chance).
void SCSmokeSet::startGenerator(const RSSmokeGeneratorInfo &info) {
    generator = info;
    generatorRemaining = info.duration;
    generatorCountdown = 0.0f;
    std::uniform_int_distribution<int> d(0, 99);
    generatorActive = d(rng) < info.chance;
    setModel(info.type);
}

bool SCSmokeSet::isGeneratorActive() const {
    return generatorActive;
}

// SmokeGen_Tick_4C057 : à la fin de la durée, objet supprimé (false) si removeAtEnd ≠ 0, sinon arrêt après
// l'émission de cette mise à jour.
bool SCSmokeSet::emitGenerator(Vector3D position, float dt) {
    if (!generatorActive) {
        return true;
    }
    generatorRemaining -= dt;
    if (generatorRemaining <= 0.0f) {
        if (generator.removeAtEnd != 0) {
            generatorActive = false;
            return false;
        }
        generatorActive = false;
    }
    generatorCountdown -= dt;
    if (generatorCountdown <= 0.0f) {
        generatorCountdown = generator.interval;
        Vector3D velocity = {
            0.0f,
            generator.rise,
            0.0f
        };
        spawn(position, velocity);
    }
    return true;
}

Vector3D smokeEmissionPoint(Vector3D position, Vector3D orientation, Vector3D localPoint) {
    Matrix m;
    m.Clear();
    m.Identity();
    m.translateM(position.x, position.y, position.z);
    m.rotateM(orientation.x, 0.0f, 1.0f, 0.0f);
    m.rotateM(orientation.y, 0.0f, 0.0f, 1.0f);
    m.rotateM(orientation.z, 1.0f, 0.0f, 0.0f);
    // Matrice d'OpenGL (glMultMatrixf(m.v)) : v[colonne][ligne].
    Vector3D world;
    world.x = m.v[0][0] * localPoint.x + m.v[1][0] * localPoint.y + m.v[2][0] * localPoint.z + m.v[3][0];
    world.y = m.v[0][1] * localPoint.x + m.v[1][1] * localPoint.y + m.v[2][1] * localPoint.z + m.v[3][1];
    world.z = m.v[0][2] * localPoint.x + m.v[1][2] * localPoint.y + m.v[2][2] * localPoint.z + m.v[3][2];
    return world;
}

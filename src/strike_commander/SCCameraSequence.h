#pragma once
#include <cstdint>
#include <string>
#include "../commons/Matrix.h"
#include "../realspace/RSCameraCOMP.h"

class SCPlane;

//
// SCCameraSequence
// ----------------
// Exécute un RSCameraSequence (bytecode COMP décodé) et produit une position + un
// point de visée, via tick(dt). Portage de la logique du solveur de manoeuvre de
// l'assembleur (AI_ManeuverSolutionMain_781D0, ovr232), réexprimé en natif
// libRealSpace : tout en float, matrices 4x4, angles en degrés. La fidélité porte
// sur la donnée (valeurs d'opérandes, ordre des opérations, comportement
// résultant), pas sur l'arithmétique 24.8 d'origine.
//
// Correspondance repère : opérande fichier (a0,a1,a2) -> Vector3D{ a0, a2, -a1 }
// (règle RSMission : comp0->x, comp2->y altitude, comp1->z négation).
//
// Pas de temps : une mise à jour par frame, pilotée par dt réel (indépendant du
// framerate). Les valeurs du fichier sont calibrées pour le tick d'origine
// (dword_70458/256 s, ~25 fps) ; converties en "par seconde" par règle de 3, le
// facteur se simplifie à 1 -> par frame : champ += taux * dt. Les compteurs et
// distances (24.8) sont divisés par 256 au chargement (COMP_SHAPE.fix).
//
class SCCameraSequence {
public:
    enum Status {
        Idle,
        Running,
        Finished
    };

    SCCameraSequence();

    // Arme la séquence : reset de l'état, exécution des opcodes jusqu'au premier
    // mode de mouvement. `bound_target` = entité liée par OP_IA_BIND_ENTITY,
    // résolue par nom via le directeur ; peut rester nullptr.
    void start(const RSCameraSequence *sequence, SCPlane *bound_target);

    // Avance d'un tick fixe. Renvoie Running tant que la séquence tourne,
    // Finished quand OP_IA_END est atteint.
    Status tick(float dt);

    Status             getStatus() const;
    const Vector3D    &getPosition() const;
    const Vector3D    &getAimPoint() const;
    const Vector3D    &getUp() const;
    const std::string &getHandoffView() const;

    static bool s_debug;   // mettre à true pour tracer STARTCAM au stdout

private:
    void view_reset();
    void applyInstruction(const COMPInstr &instr);
    void runOpcodesUntilArmed();

    void tickModeHold();          // mode 1
    void tickModeTurn();          // mode 4
    void tickModeAim();           // mode 0xB
    void tickModeMoveSegment();   // mode 0x1D
    void tickGenericIntegrator(); // loc_796B4 : tout mode actif != 0xFF
    void refreshOutputs();
    void dump(const char *tag) const;

    Vector3D compToVec3(float a0, float a1, float a2) const;
    Vector3D compAngles(float o0, float o1, float o2) const;
    Vector3D forwardFromAngles() const;
    Vector3D upFromAngles() const;
    Vector3D orientRow(int row) const;
    Vector3D entityPos() const;
    Vector3D entityVelocity() const;
    Vector3D entityAngles() const;
    Vector3D rotateByEntity(const Vector3D &v) const;
    void     buildOrientation(Matrix &out, const Vector3D &angles_deg) const;


    // ==== état (miroir de l'objet 0x102 o, natif) ====
    const RSCameraSequence *seq{nullptr};
    SCPlane                *bound_target{nullptr};

    Status status{Idle};
    size_t ip{0};                 // index dans seq->program (curseur +0x8A)
    uint8_t mode{0xFF};           // +0xFE : 0xFF = exécuter des opcodes
    bool flag_recompute_anchor{false};   // +0xFF
    bool flag_anchor_path{false};        // +0x100
    bool flag_startcam{false};           // +0x101 (OP_IA_START_FLAG)
    bool aiming{false};                  // mode 0xB : orienter par lookAt(aim_point)

    Vector3D position;            // +0x14/18/1C
    Vector3D angles;              // +0x20 : orientation courante (pitch, yaw, roll) en degrés
    Vector3D target_angles;      // +0xE4/E8/EC vu comme orientation cible (mode 4)
    Vector3D aim_point;          // mode 0xB
    Vector3D anchor_offset;      // +0x94/98/9C
    Vector3D rel_position;       // +0xB0/B4/B8 : position relative à l'entité liée
    Vector3D velocity;          // +0xC0/C4/C8
    Vector3D accel;            // +0xCC/D0/D4
    Vector3D rot_rate;        // +0xD8/DC/E0 : degrés / tick appliqués aux angles
    Vector3D target_vec;    // +0xE4/E8/EC : cible relative du segment (mode 0x1D)

    float a0{0.0f};          // +0xA0 : compteur du segment
    float elapsed{0.0f};    // +0x8E : compteur de temps écoulé
    float dist{0.0f};      // +0xBC : recul le long de l'axe avant
    float div_a4{0.0f};   // +0xA4  (OP_IA_DIV_SETUP)
    float div_a8{0.0f};  // +0xA8
    float div_ac{0.0f}; // +0xAC
    float approach_p{0.0f};   // +0xF0
    float approach_rem{0.0f}; // +0xF2

    Vector3D    out_aim;
    Vector3D    out_up;
    std::string handoff_view;

    float step_scale{0.0f};  // = dt de la frame : multiplicateur des taux ET des compteurs
};

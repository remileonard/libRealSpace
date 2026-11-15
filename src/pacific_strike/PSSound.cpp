//
//  PSSound.cpp
//  libRealSpace
//

#include "PSSound.h"

void PSSound::addEffect(SoundEffectIds id, size_t rank) {
    if (rank < this->sounds.size()) {
        this->effects[id].push_back(this->sounds[rank]);
    }
}

// DigitalSFX_Construct_5AE86 : rang dans DSOUNDFX.PAK de chaque effet du jeu. Explosion = effet 2 (rangs 13 et 16,
// seuls sons numérisés de l'effet) ; chute dans la mer = 16 ; impact = 9 ; joueur touché = 8 ; départ d'une arme = 7 ;
// largage par le joueur = 26 ; train = 18 ; toucher des roues = 11. Le moteur n'est pas associé.
void PSSound::initEffects() {
    this->addEffect(SoundEffectIds::EXPLOSION_1, 13);
    this->addEffect(SoundEffectIds::EXPLOSION_1, 16);
    this->addEffect(SoundEffectIds::EXPLOSION_3, 13);
    this->addEffect(SoundEffectIds::EXPLOSION_3, 16);
    this->addEffect(SoundEffectIds::EXPLOSION_4, 13);
    this->addEffect(SoundEffectIds::EXPLOSION_4, 16);
    this->addEffect(SoundEffectIds::WATER_SPLASH, 21);
    this->addEffect(SoundEffectIds::GUN_IMPACT_1, 0);
    this->addEffect(SoundEffectIds::PLAYER_HIT, 1);
    this->addEffect(SoundEffectIds::AIM9_SHOOT, 7);
    this->addEffect(SoundEffectIds::POD_SHOOT, 7);
    this->addEffect(SoundEffectIds::MK82_DROP, 4);
    this->addEffect(SoundEffectIds::GEARS_UP, 9);
    this->addEffect(SoundEffectIds::GEARS_DOWN, 9);
    this->addEffect(SoundEffectIds::TOUCH_DOWN, 8);
    // Tir du canon : effet 1 (sub_3E640 → SoundFX_PlayOrUpdate_5EBFB), son 3 ; tir d'une tourelle sans chunk XBRL
    // (Swpn_TurretUpdate_403A8) : effet 24, son 24. Séquences XMIDI de même numéro en secours.
    this->addEffect(SoundEffectIds::GUN_FIRE, 3);
    this->fx_effects[SoundEffectIds::GUN_FIRE] = 1;
    this->addEffect(SoundEffectIds::TURRET_FIRE, 24);
    this->fx_effects[SoundEffectIds::TURRET_FIRE] = 24;
    // Séquences XMIDI (SOUNDFX.ADL) : même numéro que l'effet du jeu (SoundFX_Play_5EB62).
    this->fx_effects[SoundEffectIds::EXPLOSION_1] = 2;
    this->fx_effects[SoundEffectIds::EXPLOSION_3] = 2;
    this->fx_effects[SoundEffectIds::EXPLOSION_4] = 2;
    this->fx_effects[SoundEffectIds::WATER_SPLASH] = 16;
    this->fx_effects[SoundEffectIds::GUN_IMPACT_1] = 9;
    this->fx_effects[SoundEffectIds::PLAYER_HIT] = 8;
    this->fx_effects[SoundEffectIds::AIM9_SHOOT] = 7;
    this->fx_effects[SoundEffectIds::POD_SHOOT] = 7;
    this->fx_effects[SoundEffectIds::MK82_DROP] = 26;
    this->fx_effects[SoundEffectIds::GEARS_UP] = 18;
    this->fx_effects[SoundEffectIds::GEARS_DOWN] = 18;
    this->fx_effects[SoundEffectIds::TOUCH_DOWN] = 11;
    printf("PSSound: %zu sons, %zu effets associes\n", this->sounds.size(), this->effects.size());
}

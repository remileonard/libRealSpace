//
//  PSSound.cpp
//  libRealSpace
//

#include "PSSound.h"
#include "../engine/RSMixer.h"

void PSSound::addEffect(SoundEffectIds id, size_t rank) {
    if (rank < this->sounds.size()) {
        this->effects[id].push_back(this->sounds[rank]);
    }
}

// DigitalSFX_Construct_5AE86 : rang dans DSOUNDFX.PAK de chaque effet du jeu (SOUND_EFFECTS.md § 3.5). Explosion = effet 2
// (rangs 13 et 16, seuls sons numérisés de l'effet) ; gerbe d'une traçante dans l'eau = 15 ; impact = 9 ; joueur touché = 8 ;
// départ d'une roquette = 26 ; lâcher d'une bombe = 7 ; train qui rentre = 5, qui sort = 6 ; toucher des roues = 11. Le moteur
// n'est pas associé.
void PSSound::initEffects() {
    this->addEffect(SoundEffectIds::EXPLOSION_1, 13);
    this->addEffect(SoundEffectIds::EXPLOSION_1, 16);
    this->addEffect(SoundEffectIds::EXPLOSION_3, 13);
    this->addEffect(SoundEffectIds::EXPLOSION_3, 16);
    this->addEffect(SoundEffectIds::EXPLOSION_4, 13);
    this->addEffect(SoundEffectIds::EXPLOSION_4, 16);
    this->addEffect(SoundEffectIds::WATER_SPLASH, 20);
    this->addEffect(SoundEffectIds::GUN_IMPACT_1, 0);
    this->addEffect(SoundEffectIds::PLAYER_HIT, 1);
    this->addEffect(SoundEffectIds::AIM9_SHOOT, 4);
    this->addEffect(SoundEffectIds::POD_SHOOT, 4);
    this->addEffect(SoundEffectIds::MK82_DROP, 7);
    this->addEffect(SoundEffectIds::GEARS_UP, 5);
    this->addEffect(SoundEffectIds::GEARS_DOWN, 6);
    this->addEffect(SoundEffectIds::TOUCH_DOWN, 8);
    this->addEffect(SoundEffectIds::GUN_FIRE, 3);
    this->fx_effects[SoundEffectIds::GUN_FIRE] = 1;
    this->addEffect(SoundEffectIds::TURRET_FIRE, 24);
    this->fx_effects[SoundEffectIds::TURRET_FIRE] = 24;
    this->fx_effects[SoundEffectIds::EXPLOSION_1] = 2;
    this->fx_effects[SoundEffectIds::EXPLOSION_3] = 2;
    this->fx_effects[SoundEffectIds::EXPLOSION_4] = 2;
    this->fx_effects[SoundEffectIds::WATER_SPLASH] = 15;
    this->fx_effects[SoundEffectIds::GUN_IMPACT_1] = 9;
    this->fx_effects[SoundEffectIds::PLAYER_HIT] = 8;
    this->fx_effects[SoundEffectIds::AIM9_SHOOT] = 26;
    this->fx_effects[SoundEffectIds::POD_SHOOT] = 26;
    this->fx_effects[SoundEffectIds::MK82_DROP] = 7;
    this->fx_effects[SoundEffectIds::GEARS_UP] = 5;
    this->fx_effects[SoundEffectIds::GEARS_DOWN] = 6;
    this->fx_effects[SoundEffectIds::TOUCH_DOWN] = 11;
    // SoundFX_FindFreeChannel_5C62E : un seul effet XMIDI à la fois (`cmp di, 1`), donc un seul canal pris à la musique.
    RSMixer::getInstance().setSoundFxChannels(1);
    printf("PSSound: %zu sons, %zu effets associes\n", this->sounds.size(), this->effects.size());
}

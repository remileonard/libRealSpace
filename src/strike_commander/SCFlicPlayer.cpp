//
//  SCFlicPlayer.cpp
//  libRealSpace
//
//  Animation d'introduction de la version CD-ROM (DATA\FLIC\THEFLIC.FLX), jouée comme MENU.EXE.
//  strike_commander_re/analysis/menu_cd/FLX_FORMAT.md, analysis/IMPL_FLIC_INTRO.md ; adresses = MENU.EXE.
//
#include "precomp.h"
#include "SCFlicPlayer.h"

static const char *FLIC_FILE = "..\\..\\DATA\\FLIC\\THEFLIC.FLX";
static const char *FLIC_PAK = "..\\..\\DATA\\FLIC\\FLIC.PAK";

// Durée d'une image : 475 unités de l'horloge de MENU.EXE (72633 : 198 périodes du compteur à
// 1 193 182 Hz par unité), valeur choisie par 62497 sur une machine rapide.
static const double FRAME_MS = 475.0 * 198.0 * 1000.0 / 1193182.0;

// Sons numérisés : nombre d'images déjà dessinées → entrée de FLIC.PAK, -1 sans son (table en 65299,
// cibles 63400 à 64290). Le jeu ne les joue qu'avec une carte de son numérisé.
static int SoundEntryForFrame(int frame) {
    switch (frame) {
    case 1:
        return 0;
    case 128:
        return 10;
    case 151:
        return 1;
    case 165:
        return 2;
    case 215:
        return 3;
    case 230:
        return 12;
    case 265:
        return 11;
    case 305:
        return 4;
    case 360:
        return 5;
    case 385:
        return 11;
    case 415:
        return 11;
    case 550:
        return 6;
    case 680:
        return 7;
    case 705:
        return 8;
    case 760:
        return 9;
    case 830:
        return 13;
    default:
        return -1;
    }
}

// Un seul son numérisé à la fois : chaque nouveau son remplace le précédent.
static const int FLIC_SOUND_CHANNEL = 0;

bool SCFlicPlayer::IsAvailable(void) {
    AssetManager &assets = AssetManager::getInstance();
    return assets.GetEntryByName(FLIC_FILE) != nullptr;
}

SCFlicPlayer::SCFlicPlayer() {
}

SCFlicPlayer::~SCFlicPlayer() {
}

void SCFlicPlayer::init(void) {
    TreEntry *flicEntry = Assets.GetEntryByName(FLIC_FILE);
    if (flicEntry == nullptr || !this->flic.InitFromRAM(flicEntry->data, flicEntry->size)) {
        printf("SCFlicPlayer: %s illisible\n", FLIC_FILE);
        this->finished = true;
        return;
    }
    TreEntry *pakEntry = Assets.GetEntryByName(FLIC_PAK);
    if (pakEntry != nullptr) {
        this->flicPak.InitFromRAM("FLIC.PAK", pakEntry->data, pakEntry->size);
    }
    // Musique de l'intro selon la carte son (62523) : même choix que les PAK de musique de RSMixer.
    // Entrées 15 à 17 pour une image de 500 unités ou moins, 18 à 20 au-delà (non employées ici).
    size_t musicEntry = 15;
    if (Mixer.music != nullptr) {
        const std::string &pak = Mixer.music->GetMidgamesPak();
        if (pak == "RMUSIC.PAK") {
            musicEntry = 16;
        } else if (pak == "MMUSIC.PAK") {
            musicEntry = 17;
        }
    }
    if (this->flicPak.IsReady() && musicEntry < this->flicPak.GetNumEntries()) {
        PakEntry *entry = this->flicPak.GetEntry(musicEntry);
        this->music.data = entry->data;
        this->music.size = entry->size;
    }
}

void SCFlicPlayer::PlaySoundForFrame(int frame) {
    if (!this->flicPak.IsReady()) {
        return;
    }
    int sound = SoundEntryForFrame(frame);
    if (sound < 0 || (size_t)sound >= this->flicPak.GetNumEntries()) {
        return;
    }
    PakEntry *entry = this->flicPak.GetEntry(sound);
    Mixer.stopSound(FLIC_SOUND_CHANNEL);
    Mixer.playSoundVoc(entry->data, entry->size, FLIC_SOUND_CHANNEL);
}

void SCFlicPlayer::Finish(void) {
    if (this->started) {
        Mixer.stopSound(FLIC_SOUND_CHANNEL);
        Mixer.stopMusic();
    }
    this->finished = true;
    Game->stopTopActivity();
}

// Boucle de 62722 : une touche arrête l'animation ; sinon une image toutes les 475 unités, son
// numérisé selon le nombre d'images dessinées, palette montrée avec l'image qui la suit.
void SCFlicPlayer::runFrame(void) {
    if (this->finished) {
        Game->stopTopActivity();
        return;
    }
    Keyboard *kb = Game->getKeyboard();
    if (kb != nullptr) {
        bool skip = kb->isActionJustPressed(InputAction::KEY_ESCAPE);
        skip = skip || kb->isActionJustPressed(InputAction::KEY_SPACE);
        skip = skip || kb->isActionJustPressed(InputAction::KEY_RETURN);
        skip = skip || kb->isActionJustPressed(InputAction::CONTROLLER_BUTTON_BACK);
        if (skip) {
            this->Finish();
            return;
        }
    }
    double now = (double)SDL_GetTicks();
    if (!this->started) {
        this->started = true;
        this->nextFrameMs = now;
        if (this->music.data != nullptr) {
            Mixer.playMusic(&this->music, 1);
        }
    }
    if (now >= this->nextFrameMs) {
        if (!this->flic.DecodeNextFrame()) {
            this->Finish();
            return;
        }
        this->PlaySoundForFrame(this->flic.frameCount);
        this->nextFrameMs += FRAME_MS;
        if (now - this->nextFrameMs > FRAME_MS) {
            this->nextFrameMs = now;
        }
    }
    VGA.activate();
    memcpy(VGA.getFrameBuffer()->framebuffer, this->flic.screen, sizeof(this->flic.screen));
    VGA.setPalette(&this->flic.palette);
    VGA.vSync();
}

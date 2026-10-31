#include "precomp.h"

SCCredit::SCCredit() {
}

SCCredit::~SCCredit() {
}

// Tics de 1/60 s, l'horloge du jeu (dword_47ABB).
uint64_t SCCredit::now() const {
    return (uint64_t)SDL_GetTicks() * 60 / 1000;
}

void SCCredit::init(void) {
    TreEntry *entry = Assets.GetEntryByName("..\\..\\DATA\\MIDGAMES\\CREDITS.IFF");
    this->credits.InitFromRam(entry->data, entry->size);
    this->score_font = FontManager.GetFont("..\\..\\DATA\\FONTS\\CONVFONT.SHP");
    this->score_color = this->nearestColor(50, 50, 50);
    this->layoutLines();
    printf("CREDITS: %zu lignes placées, couleur du score %u\n", this->layout.size(), this->score_color);
}

// Première image de l'activité : la musique et l'horloge partent quand le générique s'affiche, pas quand le
// déroulement le met en file.
void SCCredit::begin(void) {
    this->started = true;
    this->escape_latched = false;
    if (this->the_end_only) {
        this->screen = Screen::THE_END;
        this->setShownPalette(std::vector<uint8_t>(768, 0));
        this->startFade(this->credits.palette_rgb, Phase::THE_END_HOLD);
        printf("CREDITS: « THE END » seul\n");
        return;
    }
    Mixer.switchBank(0);
    Mixer.playMusic(24);
    this->setShownPalette(this->credits.palette_rgb);
    this->group_y = 200;
    this->first_line = 0;
    this->end_reached = false;
    this->frame_tick = this->now();
    this->screen = Screen::SCROLL;
    this->phase = Phase::SCROLL;
    printf("CREDITS: défilement\n");
}

// Credits_Play_BDE10 : y courant parti de 0, augmenté de l'écart de chaque ligne ; haut = y, bas = y + hauteur.
// Types 2 à 7 : justification, x, police, écart et hauteur pris dans le chunk INFO.
void SCCredit::layoutLines(void) {
    FrameBuffer *fb = VGA.getFrameBuffer();
    int y = 0;
    for (const RSCreditsLine &line : this->credits.lines) {
        CreditLine placed;
        placed.source = &line;
        int gap = line.gap;
        int height = line.height;
        int justify = line.justify;
        int x = line.x;
        int font = line.font;
        switch (line.type) {
        case 2:
            justify = 1;
            x = this->credits.right_x;
            font = this->credits.right_font;
            gap = this->credits.role_gap;
            height = this->credits.right_height;
            break;
        case 3:
            justify = 2;
            x = this->credits.left_x;
            font = this->credits.left_font;
            gap = this->credits.name_gap;
            height = this->credits.left_height;
            break;
        case 4:
            justify = 2;
            x = this->credits.left_x;
            font = this->credits.left_font;
            gap = 0;
            height = this->credits.left_height;
            break;
        case 5:
            justify = 1;
            x = this->credits.right_x;
            font = this->credits.right_font;
            gap = this->credits.name_gap;
            height = this->credits.right_height;
            break;
        case 6:
            justify = 1;
            x = this->credits.right_x;
            font = this->credits.right_font;
            gap = this->credits.section_gap;
            height = this->credits.right_height;
            break;
        case 7:
            justify = 0;
            x = 160;
            font = this->credits.title_font;
            gap = this->credits.title_gap;
            height = this->credits.title_height;
            break;
        case 8:
            height = 15;
            placed.is_end = true;
            break;
        default:
            break;
        }
        y += gap;
        placed.top = y;
        placed.bottom = y + height;
        placed.left = x;
        if (line.type >= 1 && line.type <= 7 && font < (int)this->credits.fonts.size()) {
            placed.font = this->credits.fonts[font];
            int width = fb->getJustifiedTextWidth(placed.font, line.text, 2);
            if (justify == 0) {
                placed.left = x - width / 2;
            } else if (justify == 1) {
                placed.left = x - width;
            }
        }
        this->layout.push_back(placed);
    }
}

// sub_FA5F : indice de palette dont la somme des écarts absolus avec (r, g, b) est la plus petite (composantes 0 à 63).
uint8_t SCCredit::nearestColor(int r, int g, int b) const {
    int best = 0;
    int best_distance = 0x7FFF;
    for (int c = 0; c < 256; c++) {
        int distance = std::abs(r - this->credits.palette_rgb[c * 3]) + std::abs(g - this->credits.palette_rgb[c * 3 + 1]) +
                       std::abs(b - this->credits.palette_rgb[c * 3 + 2]);
        if (distance < best_distance) {
            best_distance = distance;
            best = c;
        }
    }
    return (uint8_t)best;
}

void SCCredit::setShownPalette(const std::vector<uint8_t> &rgb) {
    this->shown_rgb = rgb;
    for (int c = 0; c < 256; c++) {
        this->palette.colors[c].r = convertFrom6To8(this->shown_rgb[c * 3]);
        this->palette.colors[c].g = convertFrom6To8(this->shown_rgb[c * 3 + 1]);
        this->palette.colors[c].b = convertFrom6To8(this->shown_rgb[c * 3 + 2]);
        this->palette.colors[c].a = 255;
    }
}

// Palette_FadeFromColor_2D165 / Palette_FadeToColor_2D09C : chaque composante avance de 2 vers la cible à chaque pas
// (un pas par tic).
void SCCredit::startFade(const std::vector<uint8_t> &target, Phase next) {
    this->fade_target = target;
    this->after_fade = next;
    this->phase_tick = this->now();
    this->phase = Phase::FADE;
}

bool SCCredit::stepFade(void) {
    uint64_t current = this->now();
    std::vector<uint8_t> rgb = this->shown_rgb;
    bool done = false;
    while (this->phase_tick < current && !done) {
        this->phase_tick++;
        done = true;
        for (size_t i = 0; i < rgb.size(); i++) {
            int value = rgb[i];
            int goal = this->fade_target[i];
            if (value < goal) {
                value = (std::min)(value + 2, goal);
            } else if (value > goal) {
                value = (std::max)(value - 2, goal);
            }
            rgb[i] = (uint8_t)value;
            if (value != goal) {
                done = false;
            }
        }
    }
    this->setShownPalette(rgb);
    return rgb == this->fade_target;
}

// Input_CheckSkip_D24A : Échap (qui reste pris), Espace ou clic.
bool SCCredit::checkSkip(void) {
    bool skip = this->escape_latched;
    Keyboard *kb = Game->getKeyboard();
    if (kb != nullptr) {
        if (kb->isActionJustPressed(InputAction::KEY_ESCAPE)) {
            this->escape_latched = true;
            skip = true;
        }
        if (kb->isActionJustPressed(InputAction::KEY_SPACE)) {
            skip = true;
        }
    }
    if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::PRESSED) {
        Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
        skip = true;
    }
    return skip;
}

// Une image du défilement : entrées et sorties des lignes, puis le groupe remonte d'un pixel.
void SCCredit::scrollStep(void) {
    uint8_t last_state = 0;
    for (size_t i = this->first_line; i < this->layout.size(); i++) {
        CreditLine &line = this->layout[i];
        if (line.state == 0) {
            if (this->group_y + line.top >= 200) {
                last_state = line.state;
                break;
            }
            if (line.is_end) {
                this->end_reached = true;
            }
            line.state = 1;
        } else if (line.state == 1 && this->group_y + line.bottom < 0) {
            line.state = 2;
            this->first_line = i;
        }
        last_state = line.state;
    }
    bool finished = this->checkSkip();
    if (this->end_reached) {
        printf("CREDITS: fin du générique, attente de 1200 tics\n");
        this->phase = Phase::SCROLL_END_HOLD;
        this->phase_tick = this->now();
        return;
    }
    if (last_state == 2) {
        finished = true;
    }
    this->group_y--;
    if (finished) {
        printf("CREDITS: défilement interrompu ou terminé\n");
        this->enterScore();
    }
}

// Écran noir, textes du score, fondu depuis le noir ; Échap n'est plus pris (byte_495FC = 0) avant l'attente.
void SCCredit::enterScore(void) {
    printf("CREDITS: score final %d\n", GameState.score);
    this->screen = Screen::SCORE;
    this->setShownPalette(std::vector<uint8_t>(768, 0));
    this->startFade(this->credits.palette_rgb, Phase::SCORE_WAIT);
}

void SCCredit::drawScroll(FrameBuffer *fb) {
    for (const CreditLine &line : this->layout) {
        if (line.state != 1 || line.is_end) {
            continue;
        }
        int y = this->group_y + line.top;
        if (line.source->image != nullptr) {
            RLEShape *image = line.source->image;
            Point2D position;
            position.x = line.left - image->leftDist;
            position.y = y - image->topDist;
            image->SetPosition(&position);
            fb->drawShape(image);
            continue;
        }
        if (line.font == nullptr || line.source->text.empty()) {
            continue;
        }
        Point2D origin;
        origin.x = line.left;
        origin.y = y;
        fb->printText(line.font, &origin, (char *)line.source->text.c_str(), 0, 0,
                      (uint32_t)line.source->text.size(), 2, 0, TEXT_JUSTIFY_LEFT, 320);
    }
}

// Écran du score : textes centrés de convfont, y = 60, 75, 90 et 125.
void SCCredit::drawScore(FrameBuffer *fb) {
    std::string score = std::to_string(GameState.score) + ".";
    std::vector<std::pair<std::string, int>> texts = {
        { "Your well-deserved",
          60 },
        { "Strike Commander final score is",
          75 },
        { score,
          90 },
        { "Congratulations.",
          125 }
    };
    for (auto &text : texts) {
        Point2D origin;
        origin.x = 0;
        origin.y = text.second;
        fb->printText(this->score_font, &origin, (char *)text.first.c_str(), this->score_color, 0,
                      (uint32_t)text.first.size(), 2, 0, TEXT_JUSTIFY_CENTER, 320);
    }
}

// « THE END » : police 0 du fichier, centré, y = 97.
void SCCredit::drawTheEnd(FrameBuffer *fb) {
    if (this->credits.fonts.empty()) {
        return;
    }
    std::string text = "THE END";
    Point2D origin;
    origin.x = 0;
    origin.y = 97;
    fb->printText(this->credits.fonts[0], &origin, (char *)text.c_str(), 0, 0, (uint32_t)text.size(), 2, 0,
                  TEXT_JUSTIFY_CENTER, 320);
}

void SCCredit::runFrame(void) {
    if (!this->started) {
        this->begin();
    }
    VGA.activate();
    FrameBuffer *fb = VGA.getFrameBuffer();
    fb->fillWithColor(0);
    uint64_t current = this->now();
    switch (this->phase) {
    case Phase::FADE:
        if (this->stepFade()) {
            this->phase = this->after_fade;
            this->phase_tick = current;
            if (this->phase == Phase::SCORE_WAIT) {
                this->escape_latched = false;
            }
        }
        break;
    case Phase::THE_END_HOLD:
        if (this->checkSkip() ||
            (current - this->phase_tick >= 300 && (!this->wait_music_end || Mixer.getMusicID() == UINT32_MAX))) {
            this->startFade(std::vector<uint8_t>(768, 0), Phase::DONE);
        }
        break;
    case Phase::SCROLL: {
        uint64_t frame_ticks = 60 / (std::max)(1, (int)this->credits.frames_per_second);
        if (frame_ticks < 1) {
            frame_ticks = 1;
        }
        if (current - this->frame_tick >= frame_ticks) {
            this->frame_tick += frame_ticks;
            if (current - this->frame_tick >= frame_ticks) {
                this->frame_tick = current;
            }
            this->scrollStep();
        }
    } break;
    case Phase::SCROLL_END_HOLD:
        if (current - this->phase_tick >= 1200 || this->checkSkip()) {
            this->startFade(std::vector<uint8_t>(768, 0), Phase::SCORE_START);
        }
        break;
    case Phase::SCORE_START:
        this->enterScore();
        break;
    case Phase::SCORE_WAIT:
        if (this->checkSkip()) {
            this->startFade(std::vector<uint8_t>(768, 0), Phase::DONE);
        }
        break;
    case Phase::DONE:
        // sub_28D49 : la musique n'est arrêtée que par le générique déroulant, qui l'a lancée.
        if (!this->the_end_only) {
            Mixer.stopMusic();
        }
        Game->stopTopActivity();
        return;
    }
    if (this->screen == Screen::THE_END) {
        this->drawTheEnd(fb);
    } else if (this->screen == Screen::SCORE) {
        this->drawScore(fb);
    } else {
        this->drawScroll(fb);
    }
    VGA.setPalette(&this->palette);
    VGA.vSync();
}

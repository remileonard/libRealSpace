#pragma once
#include "precomp.h"

// OPTEST.EXE Credits_Play_BDE10 : générique déroulant de MIDGAMES\CREDITS.IFF puis score final, ou « THE END » seul.
class SCCredit : public IActivity {
public:
    SCCredit();
    ~SCCredit();
    bool the_end_only{ false };   // argument 1 de Credits_Play_BDE10
    bool wait_music_end{ false }; // fin de la cinématique 33 (sub_A3C30) : « THE END » attend aussi la fin de la musique
    void init(void) override;
    void runFrame(void) override;

private:
    enum class Phase {
        FADE,
        THE_END_HOLD,
        SCROLL,
        SCROLL_END_HOLD,
        SCORE_START,
        SCORE_WAIT,
        DONE
    };
    enum class Screen {
        THE_END,
        SCROLL,
        SCORE
    };
    // Une ligne placée : position dans le groupe qui défile (y = position du groupe + top).
    struct CreditLine {
        const RSCreditsLine *source{ nullptr };
        RSFont *font{ nullptr };
        int left{ 0 };
        int top{ 0 };
        int bottom{ 0 };
        bool is_end{ false };
        uint8_t state{ 0 }; // 0 pas encore entrée, 1 à l'écran, 2 sortie
    };
    SCState &GameState = SCState::getInstance();
    RSCredits credits;
    std::vector<CreditLine> layout;
    RSFont *score_font{ nullptr };
    uint8_t score_color{ 0 };
    Phase phase{ Phase::SCROLL };
    Screen screen{ Screen::SCROLL };
    int group_y{ 200 };
    size_t first_line{ 0 };
    bool end_reached{ false };
    bool started{ false };
    bool escape_latched{ false }; // byte_495FC : Échap reste pris jusqu'à sa remise à 0
    uint64_t frame_tick{ 0 };
    uint64_t phase_tick{ 0 };
    std::vector<uint8_t> shown_rgb; // palette affichée, composantes de 0 à 63
    std::vector<uint8_t> fade_target;
    Phase after_fade{ Phase::DONE };

    uint64_t now() const;
    bool checkSkip(void);
    void layoutLines(void);
    uint8_t nearestColor(int r, int g, int b) const;
    void setShownPalette(const std::vector<uint8_t> &rgb);
    void startFade(const std::vector<uint8_t> &target, Phase next);
    bool stepFade(void);
    void scrollStep(void);
    void enterScore(void);
    void begin(void);
    void drawScroll(FrameBuffer *fb);
    void drawScore(FrameBuffer *fb);
    void drawTheEnd(FrameBuffer *fb);
};

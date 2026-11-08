//
//  TOKillBoardScene.cpp
//  libRealSpace
//

#include "TOKillBoardScene.h"


static std::unordered_map<uint8_t, std::string> to_pilot_names = {
    {PilotsId::PRIMETIME, "PRIMETIME"},
    {PilotsId::PHOENIX, "PHOENIX"},
    {PilotsId::BASELINE, "BASELINE"},
    {PilotsId::ZORRO, "ZORRO"},
    {PilotsId::TEX, "TEX"},
    {PilotsId::VIXEN, "TEFLON"},
    {PilotsId::HAWK, "HAWK"}
};

std::vector<SCZone *> * TOKillBoardScene::init(GAMEFLOW_SCEN *gf, SCEN *sc_opts, std::function<void(std::vector<EFCT *> *script, uint8_t id)> onclick) {
        this->sceneOpts  = sc_opts;
    PakEntry *entry  = this->optShps->GetEntry(123);
    this->rawPalette = nullptr;
    this->forPalette = nullptr;
    if (entry != nullptr) {
        RSImageSet *img = new RSImageSet();
        img->InitFromPakEntry(entry);
        background *tmpbg = new background();
        tmpbg->img        = img;
        this->layers.push_back(tmpbg);
        this->palette = *img->palettes[0]->GetColorPalette();
    }
    SCZone *z = new SCZone();
    z->id     = 0;
    z->active = true;
    z->onclick = onclick;
    z->label  = new std::string("Return to base");
    z->quad = new std::vector<Point2D *>();
    Point2D *p = new Point2D();
    p->x = 0;
    p->y = 0;
    z->quad->push_back(p);
    p = new Point2D();
    p->x = 320;
    p->y = 0;
    z->quad->push_back(p);
    p = new Point2D();
    p->x = 320;
    p->y = 200;
    z->quad->push_back(p);
    p = new Point2D();
    p->x = 0;
    p->y = 200;
    z->quad->push_back(p);
    this->zones.push_back(z);
    this->font = FontManager.GetFont("..\\..\\DATA\\FONTS\\BOARDFNT.SHP");
    to_pilot_names[0] = GameState.player_callsign;
    return (&this->zones);    
}
bool TOKillBoardScene::moreKills(const std::pair<uint8_t, std::unordered_map<uint8_t, int16_t>> &a, const std::pair<uint8_t, std::unordered_map<uint8_t, int16_t>> &b) {
    int scoreA = a.second.at(KillBoardType::AIR_KILL) + a.second.at(KillBoardType::GROUND_KILL);
    int scoreB = b.second.at(KillBoardType::AIR_KILL) + b.second.at(KillBoardType::GROUND_KILL);
    return scoreA > scoreB;
}

// Copie de KillBoardScene::Render avec les indicatifs de Tactical Operations.
void TOKillBoardScene::Render() {
    this->palette = *this->layers[0]->img->palettes[0]->GetColorPalette();
    VGA.setPalette(&this->palette);
    FrameBuffer *fb = VGA.getFrameBuffer();
    fb->clear();
    fb->drawShape(this->layers[0]->img->shapes[1]);
    Point2D position = {
        80,
        72
    };

    // trier par score total (air + ground) décroissant pour affichage
    std::vector<std::pair<uint8_t, std::unordered_map<uint8_t, int16_t>>> sorted_board(GameState.kill_board.begin(), GameState.kill_board.end());
    std::sort(sorted_board.begin(), sorted_board.end(), TOKillBoardScene::moreKills);

    Point2D ground_title = {
        152,
        60
    };
    fb->printText(this->font, ground_title, std::string("GROUND"), 0);
    Point2D air_title = {
        240,
        60
    };
    fb->printText(this->font, air_title, std::string("AIR"), 0);
    for (auto pil : sorted_board) {
        position.x = 80;
        fb->printText(this->font, position, to_pilot_names[pil.first], 0);
        position.x += 130;
        fb->printText(this->font, position, std::to_string(pil.second[KillBoardType::GROUND_KILL]), 0);
        position.x += 30;
        fb->printText(this->font, position, std::to_string(pil.second[KillBoardType::AIR_KILL]), 0);
        position.y += 12;
    }
}

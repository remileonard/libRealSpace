//
//  SCNavMap.h
//  libRealSpace
//
//  Created by Rémi LEONARD on 31/08/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once
#include "precomp.h"

class SCNavMap : public IActivity {
    
public:
    SCNavMap();
    ~SCNavMap();
    
    void init();
    void SetName(char *name);
    void runFrame(void);
    RSMission *missionObj;
    SCMission *mission{nullptr};
    uint8_t *current_nav_point;
    RSNavMap* navMap;
    
private:
    enum NavActionOfst {
        NAV_ESCAPE = 1,
        NAV_NEXT_WP = 2,
        NAV_PREV_WP = 3,
        NAV_ESCAPE2 = 4,
    };
    // Rectangles déjà occupés par un repère ou un texte (NavMap_DrawMissionObjects_7EDCA, au plus 32).
    struct LabelBox {
        int left;
        int top;
        int right;
        int bottom;
    };
    std::vector<LabelBox> label_boxes;
    bool label_first_try_only{ false }; // byte_6D5DE
    int color{134};
    std::string *name;
    Keyboard *m_keyboard;
    
    VGAPalette palette;
    VGAPalette objpal;

    bool show_area{false};
    bool show_obj{true};
    bool show_waypoint{true};

    void checkKeyboard(void);
    Point2D worldToMap(float x, float z);
    AREA *sceneArea(MISN_SCEN *scene);
    AREA *objectiveArea();
    bool isNavigationArea(AREA *area);
    void reserveBox(int left, int top, int width, int height);
    bool boxIsFree(int left, int top, int width, int height);
    void clampBox(int &left, int &top, int width, int height);
    void drawLabel(int x, int y, std::string text, uint8_t label_color);
    void drawAreaOutlines();
    void drawObjects();
    void drawAreaNames();
};
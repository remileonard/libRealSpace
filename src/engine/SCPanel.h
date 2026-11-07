//
//  SCPanel.h
//  libRealSpace
//
//  Panneau d'écran : une image de fond à une position et les boutons posés dessus, à des positions relatives au coin
//  du panneau. Modèle des écrans d'OPTEST.EXE et du panneau ALT+O de STRIKE.EXE (WidgetPanel_Construct_DAEB :
//  position, taille, image ; boutons rattachés au panneau).
//
//  Le panneau possède ses boutons. Une activité affiche un panneau en faisant pointer IActivity::buttons sur
//  SCPanel::buttons : checkButtons et drawButtons les traitent alors comme les siens, sans les détruire.
//

#pragma once
#include <functional>
#include <vector>
#include "../realspace/PakArchive.h"
#include "../realspace/RLEShape.h"
#include "SCButton.h"

class FrameBuffer;

class SCPanel {
public:
    Point2D position;
    RLEShape background;
    std::vector<SCButton *> buttons;

    SCPanel();
    ~SCPanel();
    SCPanel(const SCPanel &) = delete;
    SCPanel &operator=(const SCPanel &) = delete;

    // Image de fond, coin haut gauche en `position`.
    void init(PakEntry *image, Point2D position);
    // Bouton à (x, y) du coin du panneau, de taille width × height ; image normale `up`, image enfoncée `down`.
    SCButton *addButton(int x, int y, int width, int height, PakEntry *up, PakEntry *down, std::function<void()> action);
    // Dessine l'image de fond ; les boutons sont dessinés par IActivity::drawButtons.
    void draw(FrameBuffer *frameBuffer);
};

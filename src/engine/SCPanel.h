//
//  SCPanel.h
//  libRealSpace
//
//  Panneau d'écran : une image de fond à une position et les boutons posés dessus, à des positions relatives au coin
//  du panneau. Modèle des écrans d'OPTEST.EXE et du panneau ALT+O de STRIKE.EXE (WidgetPanel_Construct_DAEB :
//  position, taille, image ; boutons rattachés au panneau).
//
//  Le panneau possède ses boutons : il les crée, suit la souris sur eux (checkButtons) et les dessine (draw). Une
//  activité qui affiche un panneau appelle ces deux méthodes à chaque image.
//

#pragma once
#include <functional>
#include <vector>
#include "../realspace/PakArchive.h"
#include "../realspace/RLEShape.h"
#include "SCButton.h"
#include "SCMouse.h"

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
    // Même chose pour un bouton à deux états, qui reste enfoncé tant qu'il est actif (SCToggleButton::setActive).
    SCToggleButton *addToggleButton(int x, int y, int width, int height, PakEntry *up, PakEntry *down,
                                    std::function<void()> action);
    // Bouton sous la souris : enfoncé pendant l'appui, action au relâchement. Renvoie ce bouton, nullptr sinon.
    SCButton *checkButtons();
    // Image de fond, puis chaque bouton dans son apparence.
    void draw(FrameBuffer *frameBuffer);

private:
    SCMouse &Mouse = SCMouse::getInstance();

    void placeButton(SCButton *button, int x, int y, int width, int height, PakEntry *up, PakEntry *down,
                     std::function<void()> action);
};

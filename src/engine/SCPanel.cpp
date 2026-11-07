//
//  SCPanel.cpp
//  libRealSpace
//
//  Panneau d'écran : image de fond et boutons à positions relatives (SCPanel.h).
//

#include "SCPanel.h"
#include "FrameBuffer.h"

SCPanel::SCPanel() {
    this->position.x = 0;
    this->position.y = 0;
}

SCPanel::~SCPanel() {
    for (auto button : this->buttons) {
        delete button;
    }
    this->buttons.clear();
}

void SCPanel::init(PakEntry *image, Point2D position) {
    this->position = position;
    this->background.init(image->data, image->size);
    this->background.SetPosition(&this->position);
}

SCButton *SCPanel::addButton(int x, int y, int width, int height, PakEntry *up, PakEntry *down,
                             std::function<void()> action) {
    SCButton *button = new SCButton();
    Point2D buttonPosition = {
        this->position.x + x,
        this->position.y + y,
    };
    Point2D dimension = {
        width,
        height,
    };
    button->initBehavior(action, buttonPosition, dimension);
    button->appearance[SCButton::APR_UP].InitWithPosition(up->data, up->size, &buttonPosition);
    button->appearance[SCButton::APR_DOWN].InitWithPosition(down->data, down->size, &buttonPosition);
    this->buttons.push_back(button);
    return button;
}

void SCPanel::draw(FrameBuffer *frameBuffer) {
    frameBuffer->drawShape(&this->background);
}

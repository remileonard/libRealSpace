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

void SCPanel::placeButton(SCButton *button, int x, int y, int width, int height, PakEntry *up, PakEntry *down,
                          std::function<void()> action) {
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
}

SCButton *SCPanel::addButton(int x, int y, int width, int height, PakEntry *up, PakEntry *down,
                             std::function<void()> action) {
    SCButton *button = new SCButton();
    this->placeButton(button, x, y, width, height, up, down, action);
    return button;
}

SCToggleButton *SCPanel::addToggleButton(int x, int y, int width, int height, PakEntry *up, PakEntry *down,
                                         std::function<void()> action) {
    SCToggleButton *button = new SCToggleButton();
    this->placeButton(button, x, y, width, height, up, down, action);
    return button;
}

SCButton *SCPanel::checkButtons() {
    for (size_t i = 0; i < this->buttons.size(); i++) {
        SCButton *button = this->buttons[i];
        if (!button->isEnabled()) {
            continue;
        }
        if (Mouse.getPosition().x < button->position.x ||
            Mouse.getPosition().x > button->position.x + button->dimension.x ||
            Mouse.getPosition().y < button->position.y ||
            Mouse.getPosition().y > button->position.y + button->dimension.y) {
            button->setAppearance(SCButton::APR_UP);
            continue;
        }
        Mouse.setMode(SCMouse::VISOR);
        if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::PRESSED) {
            button->setAppearance(SCButton::APR_DOWN);
        }
        if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED) {
            Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
            button->setAppearance(SCButton::APR_UP);
            button->onAction();
        }
        return button;
    }
    Mouse.setMode(SCMouse::CURSOR);
    return nullptr;
}

void SCPanel::draw(FrameBuffer *frameBuffer) {
    frameBuffer->drawShape(&this->background);
    for (auto button : this->buttons) {
        frameBuffer->drawShape(&button->appearance[button->getAppearance()]);
    }
}

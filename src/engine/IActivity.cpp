//
//  IActivity.cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/25/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "IActivity.h"
#include "GameEngine.h"
#include "SCZone.h"

char *strtoupper(char *dest, const char *src) {
    char *result = dest;
    while ((*dest++ = toupper(*src++)))
        ;
    return result;
}
/**
 * @brief Test if a 2D point is inside a quad.
 *
 * This is the "Ray Casting Algorithm" also known as the "Even-Odd Rule Algorithm"
 * (https://en.wikipedia.org/wiki/Point_in_polygon#Ray_casting_algorithm).
 *
 * @param p The point to test.
 * @param quad The quad to test against.
 * @return true if the point is inside the quad, false otherwise.
 */
bool isPointInQuad(const Point2D &p, const std::vector<Point2D *> *quad) {
    int intersections = 0;
    for (size_t i = 0; i < quad->size(); ++i) {
        Point2D a = *quad->at(i);
        Point2D b = *quad->at((i + 1) % quad->size());

        if ((a.y > p.y) != (b.y > p.y)) {
            double atX = (double)(b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x;
            if (p.x < atX) {
                intersections++;
            }
        }
    }
    return (intersections % 2) != 0;
}
IActivity::IActivity() {
    Game = &GameEngine::instance();
}

IActivity::~IActivity() {}

void IActivity::stop(void) {
    VGA.fadeOut(30, 6);
    this->running = false; 
}

void IActivity::setTitle(const char *title) { 
    Screen->setTitle(title);
}

void IActivity::checkKeyboard(void) {
    Keyboard* kb = Game->getKeyboard();
    if (!kb)
        return;
    if (kb->isActionJustPressed(InputAction::KEY_ESCAPE)) {
        Game->stopTopActivity();
    }
    if (kb->isActionJustPressed(InputAction::CONTROLLER_BUTTON_BACK)) {
        Game->stopTopActivity();
    }
}

SCButton *IActivity::checkButtons(void) {
    for (size_t i = 0; i < buttons.size(); i++) {

        SCButton *button = buttons[i];

        if (!button->isEnabled())
            continue;

        if (Mouse.getPosition().x < button->position.x ||
            Mouse.getPosition().x > button->position.x + button->dimension.x ||
            Mouse.getPosition().y < button->position.y ||
            Mouse.getPosition().y > button->position.y + button->dimension.y) {
            button->setAppearance(SCButton::APR_UP);
            continue;
        }
        this->timer = 4200;
        // HIT !
        Mouse.setMode(SCMouse::VISOR);

        if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::PRESSED) {
            button->setAppearance(SCButton::APR_DOWN);
        }
        // If the Mouse button has just been released: trigger action.
        if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED) {
            Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
            button->setAppearance(SCButton::APR_UP);
            button->onAction();
        }
            

        return button;
    }

    Mouse.setMode(SCMouse::CURSOR);
    return NULL;
}

SCZone *IActivity::checkZones(std::vector<SCZone *> *zones) {
    static uint8_t color = 0;
    if (zones == nullptr) {
        return nullptr;
    }
    RSFont *fnt = FontManager.GetFont("..\\..\\DATA\\FONTS\\OPTFONT.SHP");
    for (auto zone : *zones) {
        if (!zone->active) {
            continue;
        }
        bool inside = false;
        if (zone->quad != nullptr) {
            inside = isPointInQuad(Mouse.getPosition(), zone->quad);
        }
        if (!inside) {
            inside = Mouse.getPosition().x > zone->position.x &&
                     Mouse.getPosition().x < zone->position.x + zone->dimension.x &&
                     Mouse.getPosition().y > zone->position.y &&
                     Mouse.getPosition().y < zone->position.y + zone->dimension.y;
        }
        if (!inside) {
            continue;
        }
        Mouse.setMode(SCMouse::VISOR);
        // If the mouse button has just been released: trigger action.
        if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED) {
            Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
            zone->onAction();
        }
        Point2D p = {
            160 - static_cast<int32_t>(zone->label->length() / 2) * 8,
            180,
        };
        VGA.getFrameBuffer()->printText(fnt, &p, (char *)zone->label->c_str(), color, 0,
                                        static_cast<uint32_t>(zone->label->length()), 3, 5);
        return zone;
    }
    Mouse.setMode(SCMouse::CURSOR);
    return nullptr;
}

void IActivity::drawButtons(void) {

    for (auto button : buttons) {
        RLEShape ap = button->appearance[button->getAppearance()];
        VGA.getFrameBuffer()->drawShape(&ap);
    }
}
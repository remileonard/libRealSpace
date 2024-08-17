//
//  IActivity.cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/25/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "precomp.h"


bool isPointInQuad(const Point2D& p, const std::vector<Point2D *>* quad) {
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
IActivity::IActivity(){
    
}

IActivity::~IActivity()
{
    
}


void IActivity::SetTitle(const char* title){
    Screen.SetTitle(title);
}


SCButton* IActivity::CheckButtons(void){
    for(size_t i = 0 ; i < buttons.size() ; i++){
        
        SCButton* button = buttons[i];
        
        if (!button->IsEnabled())
            continue;
        
        if (Mouse.GetPosition().x < button->position.x ||
            Mouse.GetPosition().x > button->position.x + button->dimension.x ||
            Mouse.GetPosition().y < button->position.y ||
            Mouse.GetPosition().y > button->position.y + button->dimension.y 
            )
        {
            button->SetAppearance(SCButton::APR_UP);
            continue;
        }
        
        //HIT !
        Mouse.SetMode(SCMouse::VISOR);
    
        if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::PRESSED)
            button->SetAppearance(SCButton::APR_DOWN);
    
        //If the mouse button has just been released: trigger action.
        if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED)
            button->OnAction();
        
        
        
        return button;
    }
    
    Mouse.SetMode(SCMouse::CURSOR);
    return NULL;
}

SCZone* IActivity::CheckZones(void) {
    for (size_t i = 0; i < zones.size(); i++) {

        SCZone* zone = zones[i];
        if (zone->quad != nullptr) {
            if (isPointInQuad(Mouse.GetPosition(), zone->quad)) {
                Mouse.SetMode(SCMouse::VISOR);

                //If the mouse button has just been released: trigger action.
                if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED)
                    zone->OnAction();
                Point2D p = { 160 - ((zone->label->length() / 2) * 8),180 };
                VGA.DrawTextA(FontManager.GetFont(""), &p, (char*)zone->label->c_str(), 64, 0, zone->label->length(), 3, 5);
                return zone;
            }
        }
        if (Mouse.GetPosition().x > zone->position.x &&
            Mouse.GetPosition().x < zone->position.x + zone->dimension.x &&
            Mouse.GetPosition().y > zone->position.y &&
            Mouse.GetPosition().y < zone->position.y + zone->dimension.y
            ) {
            //HIT !
            Mouse.SetMode(SCMouse::VISOR);

            //If the mouse button has just been released: trigger action.
            if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED)
                zone->OnAction();
            Point2D p = { 160-((zone->label->length()/2)*8),180};
            VGA.DrawTextA(FontManager.GetFont(""), &p, (char*)zone->label->c_str(), 64, 0, zone->label->length(), 3, 5);

            return zone;
        }
    }

    Mouse.SetMode(SCMouse::CURSOR);
    return NULL;
    return nullptr;
}

void IActivity::DrawButtons(void){
    
    
    for(size_t i = 0 ; i < buttons.size() ; i++){
        SCButton* button = buttons[i];
        if (button->IsEnabled())
            VGA.DrawShape(&button->appearance[button->GetAppearance()]);
        else
            VGA.DrawShape(&button->appearance[SCButton::Appearance::APR_DOWN]);
    }
    
}
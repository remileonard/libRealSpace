//
//  SCMouse.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/25/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once

#include "../realspace/RLEShape.h"
#include "../realspace/TreArchive.h"
#include "RSVGA.h"
#include "../realspace/AssetManager.h"


typedef struct MouseButton{
    enum BUTTON_ID {LEFT, MIDDLE, RIGHT} ;
    enum EventType{NONE, PRESSED, RELEASED} ;
    
    EventType event;
} MouseButton;

class SCMouse{
public:
    enum Mode {CURSOR, VISOR };
private:
    RLEShape* appearances[4];
    Mode mode;
    Point2D position;
    bool visible;
    inline static std::unique_ptr<SCMouse> s_instance{};
    RSVGA &VGA = RSVGA::getInstance();
public:
    static SCMouse& getInstance() {
        if (!SCMouse::hasInstance()) {
            SCMouse::setInstance(std::make_unique<SCMouse>());
        }
        SCMouse& instance = SCMouse::instance();
        return instance;
    };
    static SCMouse& instance() {
        return *s_instance;
    }
    static void setInstance(std::unique_ptr<SCMouse> inst) {
        s_instance = std::move(inst);
    }
    static bool hasInstance() { return (bool)s_instance; }

    SCMouse();
    ~SCMouse();
    
    void init(void);
    
    inline bool iIsVisible(void){ return this->visible ; }
    void setVisible(bool visible){ this->visible = visible; }
    
    inline void setPosition(Point2D position){ this->position = position;}
    inline Point2D getPosition(void) { return this->position ; }
    
    void draw(void);
    void setMode(Mode mode){this->mode = mode;}
    void flushEvents(void);
    MouseButton buttons[3];

};

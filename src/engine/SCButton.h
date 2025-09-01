//
//  Button.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/26/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once
#include <functional>
#include "../realspace/RLEShape.h"

class SCButton{
public:
    enum Appearance { APR_UP, APR_DOWN};
private:
    bool enabled ;
    Appearance apre;
    std::function<void()> onClick;

public:

    Point2D position;
    Point2D dimension;
    RLEShape appearance[2];
    
    SCButton();
    ~SCButton();
    
    void initBehavior(std::function<void()> fct, Point2D position, Point2D dimension);
    void onAction(void);
    inline bool isEnabled(void){ return this->enabled; }
    inline void setEnable(bool enabled){ this->enabled = enabled;}
    inline void setAppearance(Appearance app){ this->apre = app;}
    inline Appearance getAppearance(void){ return this->apre; }
    
};

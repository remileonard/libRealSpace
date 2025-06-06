//
//  NSObjectViewer.h
//  libRealSpace
//
//  Created by fabien sanglard on 1/3/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#ifndef __libRealSpace__NSObjectViewer__
#define __libRealSpace__NSObjectViewer__

#include "../realspace/RSObjviewer.h"

class SCObjectViewer : public IActivity{
    
public:
    SCObjectViewer();
    ~SCObjectViewer();
    
    void init( );
    
    void runFrame(void);
    void Focus(void);
    void UnFocus(void);
    
    void NextObject(void);
    float rotateUpDownAngle;
    float rotateLeftRightAngle;
    float zoomFactor;
    void renderMenu();
private:

    RSObjViewer objs;
    
    void ParseAssets(PakArchive* archive);
    
    RLEShape bluePrint;
    RLEShape title;
    RLEShape board;
    
    uint32_t currentObject;

    //For rotating the object
    uint32_t startTime;

    void OnExit(void);
    void OnNext(void);
    void OnZoomOut(void);
    void OnZoomIn(void);
    void OnRotateLeft(void);
    void OnRotateRight(void);
    void OnRotateUp(void);
    void OnRotateDown(void);
};

#endif /* defined(__libRealSpace__NSObjectViewer__) */

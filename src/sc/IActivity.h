//
//  IActivity.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/25/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#ifndef __libRealSpace__IActivity__
#define __libRealSpace__IActivity__
char* strtoupper(char* dest, const char* src);
bool isPointInQuad(const Point2D &p, const std::vector<Point2D *> *quad);

class IActivity{
private:
    bool running;

protected:
    bool music_playing;
    bool focused;
    int timer{0};
    IActivity();
    SCButton* CheckButtons(void);
    std::vector<SCButton*> buttons;
    void DrawButtons(void);
    
public:
    VGAPalette palette;

    virtual void Init(void) = 0;
    virtual void Start(void) { this->running = true;}
    void Stop(void) { this->running = false;}
    virtual void RunFrame (void) = 0;
    void SetTitle(const char* title);
    inline bool IsRunning(void){ return this->running; }
    virtual ~IActivity();
    virtual void Focus(void)  { this->focused = true;}
    virtual void UnFocus(void){ this->focused = false;}
    void CheckKeyboard(void);
};


#endif /* defined(__libRealSpace__IActivity__) */

#include "Loader.h"
#include "SDL_ttf.h"


Loader::Loader() : loadingComplete(false), loadingProgress(0.0f) {}

Loader::~Loader() {
    if (loadingThread.joinable()) {
        loadingThread.join();
    }
}

void Loader::init() {
    if (TTF_Init() < 0) {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
        exit(1);
    }
}
void Loader::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(progressMutex);
    logMessages.push_back(message);
    if (logMessages.size() > 20) {
        logMessages.erase(logMessages.begin());
    }
}
void Loader::startLoading(std::function<void(Loader*)> loadingCallback) {
    if (loadingThread.joinable()) {
        loadingThread.join();
    }
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        loadingComplete = false;
        loadingProgress = 0.0f;
        logMessages.clear();
    }
    
    loadingThread = std::thread([this, loadingCallback]() {
        loadingComplete = false;
        loadingCallback(this);

        loadingComplete = true;
        log("Finished loading.");
    });
}

bool Loader::isLoadingComplete() const {
    return loadingComplete;
}
void Loader::close() {
    if (loadingThread.joinable()) {
        loadingThread.join();
    }

    TTF_Quit();
}
void Loader::setProgress(float progress) {
    loadingProgress = progress;
}

void Loader::runFrame() {
    if (this->isLoadingComplete()) {
        return;
    }
    SDL_Surface* loadingSurface = SDL_CreateRGBSurface(0, 1920, 1080, 32, 
                                  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    
    if (!loadingSurface) {
        std::cerr << "Failed to create loading surface: " << SDL_GetError() << std::endl;
        return;
    }
    
    SDL_FillRect(loadingSurface, NULL, SDL_MapRGB(loadingSurface->format, 0, 0, 0));
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    TTF_Font* font = TTF_OpenFont("./assets/fonts/AcPlus_IBM_VGA_9x16.ttf", 36);
    if (font) {
        SDL_Color textColor = {255, 255, 255, 255}; // White text

        char loadingText[64];
        snprintf(loadingText, sizeof(loadingText), "Loading... %.0f", (double) this->loadingProgress);
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, loadingText, textColor);
        
        if (textSurface) {
            SDL_Rect textRect;
            SDL_Surface *titleSurface = TTF_RenderText_Solid(font, "Neo Strike Commander", textColor);
            if (titleSurface) {
                textRect.x = 50;
                textRect.y = 50;
                SDL_BlitSurface(titleSurface, NULL, loadingSurface, &textRect);
                SDL_FreeSurface(titleSurface);
            }

            textRect.x = 100;
            textRect.y = 150;
            SDL_BlitSurface(textSurface, NULL, loadingSurface, &textRect);
            for ( auto & log : logMessages ) {
                
                if (log.empty()) continue;
                
                SDL_Surface* logSurface = TTF_RenderText_Solid(font, log.c_str(), textColor);
                if (logSurface) {
                    textRect.y += textSurface->h + 5;
                    SDL_BlitSurface(logSurface, NULL, loadingSurface, &textRect);
                    SDL_FreeSurface(logSurface);
                }
            }
            int barWidth = 1720;
            int barHeight = 20;
            SDL_Rect progressBarBg = {
                100,
                1040,
                barWidth,
                barHeight
            };
            SDL_FillRect(loadingSurface, &progressBarBg, SDL_MapRGB(loadingSurface->format, 100, 100, 100));
            SDL_Rect progressBarFill = {
                progressBarBg.x + 2,
                progressBarBg.y + 2,
                static_cast<int>((barWidth - 4) * (this->loadingProgress/100.0f)),
                barHeight - 4
            };
            SDL_FillRect(loadingSurface, &progressBarFill, SDL_MapRGB(loadingSurface->format, 0, 255, 0));
            
            SDL_FreeSurface(textSurface);
        }
        TTF_CloseFont(font);
    }
    GLuint loadingTexture;
    glGenTextures(1, &loadingTexture);
    glBindTexture(GL_TEXTURE_2D, loadingTexture);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, loadingSurface->w, loadingSurface->h, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, loadingSurface->pixels);

    SDL_FreeSurface(loadingSurface);

    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, loadingTexture);

    
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, 1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, -1.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);

    glDeleteTextures(1, &loadingTexture);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glFlush();
    glFinish();
}
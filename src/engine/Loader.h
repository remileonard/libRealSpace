#pragma once
#include <SDL.h>
#ifdef _WIN32
#include <Windows.h>
#include <GL/GL.h>
#endif
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <iostream>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
class Loader {
private:
    std::thread loadingThread;
    std::atomic<bool> loadingComplete;
    std::atomic<float> loadingProgress;
    std::vector<std::string> logMessages;
    std::mutex progressMutex;
public:
    Loader();
    ~Loader();

    void init();
    void startLoading(std::function<void(Loader*)> loadingCallback);
    void runFrame();
    bool isLoadingComplete() const;
    void log(const std::string& message);
    void setProgress(float progress);
    void close();
    static Loader& getInstance() {
        static Loader instance;
        return instance;
    };    
};
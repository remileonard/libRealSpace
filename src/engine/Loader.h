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
    inline static std::unique_ptr<Loader> s_instance{};
public:

    static Loader& getInstance() {
        if (!Loader::hasInstance()) {
            Loader::setInstance(std::make_unique<Loader>());
        }
        Loader& instance = Loader::instance();
        return instance;
    };
    static Loader& instance() {
        return *s_instance;
    }
    static void setInstance(std::unique_ptr<Loader> inst) {
        s_instance = std::move(inst);
    }

    static bool hasInstance() { return (bool)s_instance; }

    Loader();
    ~Loader();

    void init();
    void startLoading(std::function<void(Loader*)> loadingCallback);
    void runFrame();
    bool isLoadingComplete() const;
    void log(const std::string& message);
    void setProgress(float progress);
    void close();    
};
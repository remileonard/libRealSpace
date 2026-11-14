//
//  main.cpp
//  libRealSpace
//
//  Exécutable principal de Pacific Strike.
//

#include "../../strike_commander/precomp.h"
#include "../../pacific_strike/PSGameFlow.h"
#include "../../pacific_strike/PSState.h"
#include "../../pacific_strike/PSConvAssetManager.h"
#include "../../engine/gametimer.h"
#include "../../engine/desktoptimer.h"

int main(int argc, char* argv[]) {
    Config &config = Config::getInstance();
    config.load("./assets/config.ini");
    GameTimer::getInstance().setTimer(std::make_unique<DesktopTimer>());
    SCRenderer::setInstance(std::make_unique<SCRenderer>());
    RSScreen::setInstance(std::make_unique<RSScreen>());
    GameEngine::setInstance(std::make_unique<GameEngine>());

    RSScreen &screen = RSScreen::instance();
    Loader& loader = Loader::getInstance();
    AssetManager& assets = AssetManager::getInstance();

    int width = config.getInt("Window", "width", 800);
    int height = config.getInt("Window", "height", 600);
    bool fullscreen = config.getBool("Window", "fullscreen", false);
    screen.init(width,height,fullscreen);
    width = screen.width;
    height = screen.height;
    config.setInt("Window", "width", width);
    config.setInt("Window", "height", height);
    screen.is_spfx_finished = false;
    while (!screen.is_spfx_finished) {
        screen.fxTurnOnTv();
        screen.refresh();
        SDL_PumpEvents();
    }
    assets.SetBase("./assets");
    loader.init();
    loader.startLoading([](Loader* loader) {
        AssetManager& assets = AssetManager::getInstance();
        loader->setProgress(0.0f);
        std::vector<std::string> cdTreFiles = {
            "PACIFIC.DAT"
        };
        assets.init(cdTreFiles);
        loader->setProgress(50.0f);
        assets.intel_root_path = "..\\..\\DATA\\INTEL\\";
        assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
        assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
        assets.sound_root_path = "..\\..\\DATA\\SOUND\\";
        assets.texture_root_path = "..\\..\\DATA\\TXM\\";
        assets.gameflow_root_path = "..\\..\\DATA\\GAMEFLOW\\";

        assets.gameflow_filename = assets.gameflow_root_path+"GAMEFLOW.IFF";
        assets.optshps_filename = assets.gameflow_root_path+"OPTSHPS.PAK";
        assets.optpals_filename = assets.gameflow_root_path+"OPTPALS.PAK";
        assets.optfont_filename = assets.gameflow_root_path+"OPTFONT.IFF";
        assets.navmap_filename = "..\\..\\DATA\\COCKPITS\\NAVMAP.IFF";
        assets.conv_pak_filename = assets.gameflow_root_path+"CONVSHPS.PAK";
        assets.option_filename = assets.gameflow_root_path+"OPTIONS.IFF";
        assets.conv_data_filename = assets.gameflow_root_path+"CONVDATA.IFF";
        assets.conv_pal_filename = assets.gameflow_root_path+"CONVPALS.PAK";
        assets.txm_filename = assets.texture_root_path+"TXMPACK.PAK";
        assets.acc_filename = assets.texture_root_path+"ACCPACK.PAK";
        assets.convpak_filename = assets.gameflow_root_path+"CONV.PAK";
        RSMixer::getInstance().init();
        loader->setProgress(60.0f);
        RSFontManager::getInstance().init();
        loader->setProgress(70.0f);
        ConvAssetManager::setInstance(std::make_unique<PSConvAssetManager>());
        ConvAssetManager::getInstance().init();
        loader->setProgress(90.0f);
        RSSound::getInstance().init();
        loader->setProgress(100.0f);
    });
    while (!loader.isLoadingComplete()) {
        loader.runFrame();
        screen.refresh();
        SDL_PumpEvents();
    }
    GameEngine *game = &GameEngine::instance();
    game->init();
    SCState::getInstance().Reset();
    PSState::getInstance().Reset();
    // Pas encore de menu principal porté : la campagne démarre directement sur le gameflow.
    PSGameFlow *gameflow = new PSGameFlow();
    gameflow->init();
    game->addActivity(gameflow);
    game->run();

    return EXIT_SUCCESS;
}

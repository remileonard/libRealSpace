# r4game : l'écran, le VGA, le chargement et la 3D du jeu sur R4nkine, pour l'exécutable neosc_r4.
# Il demande le patch patches/r4-hooks.patch de libRealSpace (les méthodes virtuelles de RSVGA et de Loader).
add_library(r4game STATIC R4Screen.cpp R4Screen.h R4VGA.cpp R4VGA.h R4Loader.cpp R4Loader.h R4Panel.cpp R4Panel.h R4ConfigPanel.cpp R4ConfigPanel.h R4Mission.cpp R4Mission.h R4Timer.cpp R4Timer.h R4Renderer.cpp R4Renderer.h R4Strike.cpp R4Strike.h)
target_include_directories(r4game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
target_link_libraries(r4game PUBLIC r4bridge r4nkine::r4nkine)
# la VR de neosc_r4 : r4::xr de R4nkine, quand il est construit (OpenXR trouvé, Windows)
if(TARGET r4nkine::r4nkine_xr)
    target_link_libraries(r4game PUBLIC r4nkine::r4nkine_xr)
    target_compile_definitions(r4game PUBLIC R4GAME_XR)
endif()

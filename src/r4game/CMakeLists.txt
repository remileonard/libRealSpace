# r4game : l'écran, le VGA, le chargement et la 3D du jeu sur R4nkine, pour l'exécutable neosc_r4.
# Il demande le patch patches/r4-hooks.patch de libRealSpace (les méthodes virtuelles de RSVGA et de Loader).
add_library(r4game STATIC R4Screen.cpp R4Screen.h R4VGA.cpp R4VGA.h R4Loader.cpp R4Loader.h R4Renderer.cpp R4Renderer.h R4Strike.cpp R4Strike.h)
target_include_directories(r4game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
target_link_libraries(r4game PUBLIC r4bridge r4nkine::r4nkine)

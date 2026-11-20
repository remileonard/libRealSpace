# r4bridge : les objets de libRealSpace (RSEntity, RSPalette) en modèles R4nkine.
# Il n'utilise que les structures de realspace et engine (en-têtes) ; le
# programme qui s'en sert lie les bibliothèques du jeu, comme le debugger.
add_library(r4bridge STATIC R4Bridge.cpp R4Bridge.h R4World.cpp R4World.h)
target_include_directories(r4bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src
                           ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
target_link_libraries(r4bridge PUBLIC r4nkine::r4nkine)

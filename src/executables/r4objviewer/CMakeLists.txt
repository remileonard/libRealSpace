# r4objviewer : les objets 3D du jeu (OBJECTS\*.IFF) vus par R4nkine
add_executable(r4objviewer main.cpp WorldScene.cpp WorldScene.h TerrainDiag.cpp TerrainDiag.h)
target_include_directories(r4objviewer PRIVATE ${CMAKE_SOURCE_DIR}/src ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
# comme le debugger : les bibliothèques du jeu, puis r4bridge (et R4nkine par lui)
target_link_libraries(r4objviewer PRIVATE r4bridge strike_commander engine realspace commons
                      pacific_strike tactical_operation engine realspace commons ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES})
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    target_link_options(r4objviewer PRIVATE "-Wl,-no_fixup_chains")
endif()
set(R4OBJVIEWER_TARGET_NAME r4objviewer PARENT_SCOPE)

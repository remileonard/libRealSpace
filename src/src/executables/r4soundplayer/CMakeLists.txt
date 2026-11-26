# r4soundplayer : les sons du jeu tels que neosc_r4 les joue avec R4Mixer (effets XMIDI en PCM, VOC, musique, dans l'espace).
add_executable(r4soundplayer main.cpp)
target_include_directories(r4soundplayer PRIVATE ${SC_INCLUDES} ${CMAKE_SOURCE_DIR}/src ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
target_link_libraries(r4soundplayer PRIVATE r4game tactical_operation strike_commander engine strike_commander realspace commons ${SDL2_LIBRARIES} imgui::imgui ${OPENGL_LIBRARIES})
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    target_link_options(r4soundplayer PRIVATE "-Wl,-no_fixup_chains")
endif()

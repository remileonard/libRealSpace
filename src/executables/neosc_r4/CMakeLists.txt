# neosc_r4 : Strike Commander sur R4nkine (l'écran, le VGA et le chargement de r4game).
add_executable(neosc_r4 main.cpp)
target_include_directories(neosc_r4 PRIVATE ${SC_INCLUDES} ${CMAKE_SOURCE_DIR}/src ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
target_link_libraries(neosc_r4 PRIVATE r4game tactical_operation strike_commander engine realspace commons ${SDL2_LIBRARIES} imgui::imgui ${OPENGL_LIBRARIES})
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    target_link_options(neosc_r4 PRIVATE "-Wl,-no_fixup_chains")
endif()

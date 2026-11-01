file (GLOB SCE_EXE_SOURCE "*.cpp")
file (GLOB SCE_EXE_HEADER "*.h")
if(WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_SOURCE_DIR}/resources/debugger.rc")
endif()
add_executable (neosc_enhanced ${SCE_EXE_SOURCE} ${SCE_EXE_HEADER} ${APP_ICON_RESOURCE})
set(SCE_TARGET_NAME neosc_enhanced PARENT_SCOPE)
target_include_directories(neosc_enhanced PUBLIC ${SC_INCLUDES})
target_include_directories(neosc_enhanced PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(neosc_enhanced PUBLIC ${OPENGL_INCLUDE_DIR})
target_include_directories(neosc_enhanced PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(neosc_enhanced PRIVATE strike_commander_enhanced strike_commander engine realspace commons)
target_link_libraries(neosc_enhanced PUBLIC ${SDL2_LIBRARIES})
target_link_libraries(neosc_enhanced PRIVATE imgui::imgui )
target_link_libraries(neosc_enhanced PUBLIC ${OPENGL_LIBRARIES})
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    target_link_options(neosc_enhanced PRIVATE "-Wl,-no_fixup_chains")
endif()
# Copie des fichiers resources vers assets en mode Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR "${CMAKE_CONFIGURATION_TYPES}" MATCHES "Debug")
    add_custom_command(TARGET neosc_enhanced POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:neosc_enhanced>/assets"
        COMMENT "Copie des fichiers resources vers assets (Debug) $<TARGET_FILE_DIR:neosc_enhanced>/assets"
    )
    add_custom_command(TARGET neosc_enhanced POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/resources/config.ini"
        "${CMAKE_BINARY_DIR}/config.ini"
        COMMENT "Copie du fichier config.ini vers le dossier de build (Debug) ${CMAKE_BINARY_DIR}/config.ini"
    )
endif()
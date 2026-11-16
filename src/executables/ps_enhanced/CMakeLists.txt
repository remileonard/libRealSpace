file (GLOB PSE_SOURCE "*.cpp")
file (GLOB PSE_HEADER "*.h")
if(WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_SOURCE_DIR}/resources/debugger.rc")
endif()
add_executable (neops_enhanced ${PSE_SOURCE} ${PSE_HEADER} ${APP_ICON_RESOURCE})
set(PSE_TARGET_NAME neops_enhanced PARENT_SCOPE)
target_include_directories(neops_enhanced PUBLIC ${SC_INCLUDES})
target_include_directories(neops_enhanced PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(neops_enhanced PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(neops_enhanced PRIVATE pacific_strike strike_commander_enhanced strike_commander engine realspace commons)
target_link_libraries(neops_enhanced PUBLIC ${SDL2_LIBRARIES})
target_link_libraries(neops_enhanced PRIVATE imgui::imgui )
target_link_libraries(neops_enhanced PUBLIC ${OPENGL_LIBRARIES})
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    target_link_options(neops_enhanced PRIVATE "-Wl,-no_fixup_chains")
endif()
# Copie des fichiers resources vers assets en mode Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR "${CMAKE_CONFIGURATION_TYPES}" MATCHES "Debug")
    add_custom_command(TARGET neops_enhanced POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:neops_enhanced>/assets"
        COMMENT "Copie des fichiers resources vers assets (Debug) $<TARGET_FILE_DIR:neops_enhanced>/assets"
    )
    add_custom_command(TARGET neops_enhanced POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/resources/config.ini"
        "${CMAKE_BINARY_DIR}/config.ini"
        COMMENT "Copie du fichier config.ini vers le dossier de build (Debug) ${CMAKE_BINARY_DIR}/config.ini"
    )
endif()

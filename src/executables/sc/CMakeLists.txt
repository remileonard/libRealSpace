file (GLOB SC_SOURCE "*.cpp")
file (GLOB SC_HEADER "*.h")
if(WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_SOURCE_DIR}/resources/debugger.rc")
endif()
add_executable (neosc ${SC_SOURCE} ${SC_HEADER} ${APP_ICON_RESOURCE})
set(SC_TARGET_NAME neosc PARENT_SCOPE)
target_include_directories(neosc PUBLIC ${SC_INCLUDES})
target_include_directories(neosc PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(neosc PUBLIC ${OPENGL_INCLUDE_DIR})
target_include_directories(neosc PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(neosc PRIVATE strike_commander engine realspace commons)
target_link_libraries(neosc PUBLIC ${SDL2_LIBRARIES})
target_link_libraries(neosc PRIVATE imgui::imgui )
target_link_libraries(neosc PUBLIC ${OPENGL_LIBRARIES})

# Copie des fichiers resources vers assets en mode Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR "${CMAKE_CONFIGURATION_TYPES}" MATCHES "Debug")
    add_custom_command(TARGET neosc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:neosc>/assets"
        COMMENT "Copie des fichiers resources vers assets (Debug) $<TARGET_FILE_DIR:neosc>/assets"
    )
endif()
file (GLOB SC_SOURCE "*.cpp")
file (GLOB SC_HEADER "*.h")

if(WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_SOURCE_DIR}/resources/config.rc")
endif()
find_package(nfd CONFIG REQUIRED)

add_executable (config ${SC_SOURCE} ${SC_HEADER} ${APP_ICON_RESOURCE})
set(CONFIG_TARGET_NAME config PARENT_SCOPE)
target_sources(config PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../debugger/DebugControlMapping.cpp"
)
target_include_directories(config PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../debugger"
)
target_include_directories(config PUBLIC ${SC_INCLUDES})
target_include_directories(config PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(config PUBLIC ${OPENGL_INCLUDE_DIR})
target_include_directories(config PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(config PRIVATE strike_commander engine realspace commons pacific_strike)
target_link_libraries(config PUBLIC ${SDL2_LIBRARIES})
target_link_libraries(config PRIVATE imgui::imgui )
target_link_libraries(config PRIVATE nfd::nfd)
target_link_libraries(config PUBLIC ${OPENGL_LIBRARIES})
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    target_link_options(config PRIVATE "-Wl,-no_fixup_chains")
endif()
# Copie des fichiers resources vers assets en mode Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR "${CMAKE_CONFIGURATION_TYPES}" MATCHES "Debug")
    add_custom_command(TARGET config POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:config>/assets"
        COMMENT "Copie des fichiers resources vers assets (Debug) $<TARGET_FILE_DIR:config>/assets"
    )
endif()
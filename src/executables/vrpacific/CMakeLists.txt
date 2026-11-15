file (GLOB VRPACIFIC_SOURCE "*.cpp")
file (GLOB VRPACIFIC_HEADER "*.h")
set(VRSCREEN_DIR "${CMAKE_SOURCE_DIR}/src/executables/vrstrike")
if(WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_SOURCE_DIR}/resources/vrstrike.rc")
endif()
find_package(OpenXR REQUIRED)

add_executable (vrpacific ${VRPACIFIC_SOURCE} ${VRPACIFIC_HEADER}
    "${VRSCREEN_DIR}/VRScreen.cpp"
    "${VRSCREEN_DIR}/VRScreen.h"
    "${VRSCREEN_DIR}/vrtimer.h"
    ${APP_ICON_RESOURCE})
set(VRPACIFIC_TARGET_NAME vrpacific PARENT_SCOPE)
target_include_directories(vrpacific PUBLIC ${SC_INCLUDES})
target_include_directories(vrpacific PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(vrpacific PUBLIC ${OPENGL_INCLUDE_DIR})
target_include_directories(vrpacific PUBLIC ${VRSCREEN_DIR})
target_include_directories(vrpacific PUBLIC OpenXR::headers)

target_link_libraries(vrpacific PRIVATE pacific_strike strike_commander engine realspace commons)
target_link_libraries(vrpacific PUBLIC ${SDL2_LIBRARIES})
target_link_libraries(vrpacific PRIVATE imgui::imgui )
target_link_libraries(vrpacific PUBLIC ${OPENGL_LIBRARIES})
target_link_libraries(vrpacific PRIVATE OpenXR::openxr_loader)

# Copie des fichiers resources vers assets en mode Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR "${CMAKE_CONFIGURATION_TYPES}" MATCHES "Debug")
    add_custom_command(TARGET vrpacific POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:vrpacific>/assets"
        COMMENT "Copie des fichiers resources vers assets (Debug) $<TARGET_FILE_DIR:vrpacific>/assets"
    )
endif()

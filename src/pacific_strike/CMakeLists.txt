file(GLOB LIBRARY_SOURCES "*.cpp")
file(GLOB LIBRARY_HEADERS "*.h")


add_library(pacific_strike STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})

target_include_directories(pacific_strike PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(pacific_strike PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(pacific_strike PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer_ext::SDL2_mixer_ext>,SDL2_mixer_ext::SDL2_mixer_ext,SDL2_mixer_ext::SDL2_mixer_ext_Static>)
target_include_directories(pacific_strike PUBLIC ${LIBRARY_INCLUDES})
target_link_libraries(pacific_strike PUBLIC ${LIBRARY_LINKS})
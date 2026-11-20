# cmake/R4nkine.cmake - R4nkine dans libRealSpace : la bibliothèque, le pont
# r4bridge et le viewer r4objviewer.
#
# Dans le CMakeLists.txt principal, après les add_subdirectory(src/...) :
#
#     include(cmake/R4nkine.cmake)
#
# R4nkine vient :
#   - d'un dossier local :  cmake -DR4NKINE_DIR=/chemin/vers/r4nkine ...
#   - sinon de GitHub (FetchContent), à la branche R4NKINE_TAG.
option(REALSPACE_R4 "Construire r4bridge et r4objviewer (R4nkine)" ON)
if(NOT REALSPACE_R4)
    return()
endif()

set(R4NKINE_DIR "" CACHE PATH "Un clone local de R4nkine (vide : FetchContent)")
set(R4NKINE_TAG "claude/magical-bell-y9meqd" CACHE STRING "La branche ou le tag de R4nkine")

# seulement la bibliothèque : pas les applications, exemples ni tests de R4nkine
set(R4_BUILD_APPS OFF CACHE BOOL "" FORCE)
set(R4_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(R4_BUILD_TESTS OFF CACHE BOOL "" FORCE)

if(R4NKINE_DIR)
    add_subdirectory(${R4NKINE_DIR} ${CMAKE_BINARY_DIR}/r4nkine)
else()
    include(FetchContent)
    FetchContent_Declare(r4nkine
        GIT_REPOSITORY https://github.com/remileonard/r4nkine.git
        GIT_TAG ${R4NKINE_TAG}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(r4nkine)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/src/r4bridge)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/executables/r4objviewer)

# neosc_r4 demande le patch patches/r4-hooks.patch (les méthodes virtuelles de RSVGA, de Loader et du rendu de SCStrike)
file(READ ${CMAKE_SOURCE_DIR}/src/engine/RSVGA.h R4_RSVGA_HEADER)
file(READ ${CMAKE_SOURCE_DIR}/src/strike_commander/SCStrike.h R4_STRIKE_HEADER)
if(R4_RSVGA_HEADER MATCHES "virtual void displayBuffer" AND R4_STRIKE_HEADER MATCHES "virtual void renderWorld")
    add_subdirectory(${CMAKE_SOURCE_DIR}/src/r4game)
    add_subdirectory(${CMAKE_SOURCE_DIR}/src/executables/neosc_r4)
else()
    message(WARNING "neosc_r4 n'est pas construit : appliquer patches/r4-hooks.patch à libRealSpace (git apply)")
endif()

cmake_minimum_required(VERSION 3.26.0)
project(NeoSC VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer_ext CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
set (OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

add_subdirectory(src/executables/sc)
add_subdirectory(src/executables/to)
add_subdirectory(src/executables/debugger)
add_subdirectory(src/strike_commander)
add_subdirectory(src/pacific_strike)
add_subdirectory(src/engine)
add_subdirectory(src/realspace)
add_subdirectory(src/commons/pklib)
add_subdirectory(src/commons)


# Configuration du package CPack
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "NeoSC")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NeoSC - Strike Commander")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

set(CPACK_NSIS_ENABLE OFF CACHE BOOL "Ne pas utiliser NSIS" FORCE)
set(CPACK_SOURCE_GENERATOR "ZIP")

# Configuration du packaging et des assets
include(InstallRequiredSystemLibraries)
include(CPack)

# Dossier où seront copiés les assets et les DLLs pour le développement
set(ASSETS_DIR "${CMAKE_BINARY_DIR}/assets")
set(FONTS_DIR "${ASSETS_DIR}/fonts")

# Création des dossiers pour les assets
file(MAKE_DIRECTORY ${ASSETS_DIR})
file(MAKE_DIRECTORY ${FONTS_DIR})

# Copie des assets (ajustez les chemins selon votre structure de projet)
file(COPY "${CMAKE_SOURCE_DIR}/resources/STRIKE.wopl" DESTINATION "${ASSETS_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/resources/AcPlus_IBM_VGA_9x16.ttf" DESTINATION "${FONTS_DIR}")

# Installation des exécutables et des DLLs
install(TARGETS ${SC_TARGET_NAME} ${TO_TARGET_NAME} ${DEBUGGER_TARGET_NAME} DESTINATION ".")

# Installation des assets
install(DIRECTORY "${ASSETS_DIR}/" DESTINATION "assets")



# Pour les DLLs des dépendances (Windows)
if(WIN32)
    # Récupération des DLLs de runtime Visual C++
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    include(InstallRequiredSystemLibraries)
    
    # Récupération des DLLs depuis VCPKG
    if(DEFINED VCPKG_INSTALLED_DIR)
        # Obtention de l'architecture cible
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(VCPKG_TARGET_TRIPLET "x64-windows")
        else()
            set(VCPKG_TARGET_TRIPLET "x86-windows")
        endif()
        
        # Chemin vers les DLLs dans VCPKG
        set(VCPKG_BIN_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
        
        # Installation des DLLs requises
        file(GLOB VCPKG_DLLS
            "${VCPKG_BIN_DIR}/SDL2.dll"
            "${VCPKG_BIN_DIR}/SDL2_ttf.dll"
            "${VCPKG_BIN_DIR}/SDL2_mixer.dll"
            "${VCPKG_BIN_DIR}/SDL2_mixer_ext.dll"
            "${VCPKG_BIN_DIR}/openmpt.dll"
            "${VCPKG_BIN_DIR}/libmpg123-0.dll"
            "${VCPKG_BIN_DIR}/libopusfile-0.dll"
            "${VCPKG_BIN_DIR}/libopus-0.dll"
            "${VCPKG_BIN_DIR}/libflac.dll"
            "${VCPKG_BIN_DIR}/libogg.dll"
            "${VCPKG_BIN_DIR}/zlib1.dll"
            "${VCPKG_BIN_DIR}/freetype.dll"
            "${VCPKG_BIN_DIR}/bz2.dll"
            "${VCPKG_BIN_DIR}/brotlidec.dll"
            "${VCPKG_BIN_DIR}/brotlicommon.dll"
            "${VCPKG_BIN_DIR}/libpng16.dll"
            "${VCPKG_BIN_DIR}/adlmidi.dll"
            "${VCPKG_BIN_DIR}/opnmidi.dll"
        )
        
        message(STATUS "VCPKG DLLs: ${VCPKG_DLLS}")
        install(FILES ${VCPKG_DLLS} DESTINATION ".")
    else()
        message(WARNING "VCPKG_INSTALLED_DIR n'est pas défini. Impossible de localiser les DLLs VCPKG automatiquement.")
    endif()
endif()
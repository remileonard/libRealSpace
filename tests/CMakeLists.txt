file(GLOB TEST_SOURCE "*.cpp")
file(GLOB TEST_HEADER "*.h")

find_package(GTest CONFIG REQUIRED)

# Créer l'exécutable de tests
add_executable(libRealSpace_tests ${TEST_SOURCE} ${TEST_HEADER})

# Inclure les répertoires nécessaires
target_include_directories(libRealSpace_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Lier avec les bibliothèques du projet et Google Test
target_link_libraries(libRealSpace_tests 
    PRIVATE 
    commons
    engine
    realspace
    GTest::gtest
    GTest::gtest_main
)

# Ajouter les tests à CTest
include(GoogleTest)
gtest_discover_tests(libRealSpace_tests)
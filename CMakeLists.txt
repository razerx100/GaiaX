cmake_minimum_required(VERSION 3.21)

project(GaiaX
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ADD_TEST_GAIAX "If test should be built" OFF)

add_subdirectory(library)

if(ADD_TEST_GAIAX)
    enable_testing()
    add_subdirectory(test)
endif()

add_library(razer::gaiaX ALIAS GaiaXLib)

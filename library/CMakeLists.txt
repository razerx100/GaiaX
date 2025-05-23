cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE SRC src/*.cpp src/*.ixx includes/*.hpp templates/*.hpp exports/*.hpp)

add_library(GaiaXLib STATIC ${SRC})

set(GAIAX_PRIVATE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/templates/)

set(GAIAX_PRIVATE_INCLUDES ${GAIAX_PRIVATE_INCLUDES} PARENT_SCOPE)

target_include_directories(GaiaXLib PRIVATE ${GAIAX_PRIVATE_INCLUDES})

target_include_directories(GaiaXLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/exports/ ${CMAKE_CURRENT_SOURCE_DIR}/includes/ ${CMAKE_CURRENT_SOURCE_DIR}/includes/D3D/)

target_link_libraries(GaiaXLib PRIVATE
    d3d12.lib dxgi.lib dxguid.lib uuid.lib d3dcompiler.lib
)

target_compile_options(GaiaXLib PRIVATE /fp:fast /MP /Ot /W4 /Gy /std:c++latest /Zc:__cplusplus)

include(FetchContent)

FetchContent_Declare(
    Callisto
    GIT_REPOSITORY https://github.com/razerx100/Callisto.git
    GIT_TAG master
)

FetchContent_MakeAvailable(Callisto)

FetchContent_Declare(
    Venus
    GIT_REPOSITORY https://github.com/razerx100/Venus.git
    GIT_TAG master
)

FetchContent_MakeAvailable(Venus)

FetchContent_Declare(
    DirectXMath
    GIT_REPOSITORY https://github.com/razerx100/DirectXMath.git
    GIT_TAG master
)

FetchContent_MakeAvailable(DirectXMath)

target_link_libraries(GaiaXLib PRIVATE razer::callisto razer::DxMath razer::venus)

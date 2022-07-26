cmake_minimum_required(VERSION 3.21)

project(GaiaX
    LANGUAGES CXX
)

set(PROJECTDIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
else()
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
endif()

file(GLOB_RECURSE SRC src/*.cpp includes/*.hpp templates/*.hpp exports/*.hpp)
file(GLOB_RECURSE MODULESZ interfaces/*.ixx)

add_library(${PROJECT_NAME} SHARED
    ${SRC} ${MODULESZ}
)

source_group("Interfaces" FILES ${MODULESZ})

target_include_directories(${PROJECT_NAME} PRIVATE includes/ includes/D3D/ includes/Exceptions/ templates/ exports/ DirectXMath/Inc/ DirectXMath/Extensions/)

target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:_DEBUG>" "$<$<CONFIG:RELEASE>:NDEBUG>" BUILD_GAIAX)

target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d12.lib dxgi.lib dxguid.lib uuid.lib d3dcompiler.lib
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast /MP /EHa /Ot /W4)
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

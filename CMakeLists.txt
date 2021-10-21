cmake_minimum_required(VERSION 3.21)

project(GaiaX
    LANGUAGES CXX
)

set(PROJECTDIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
endif()

if(IMGUIFLAG STREQUAL "ENABLE")
    file(GLOB_RECURSE SRC src/*.cpp includes/*.hpp includes/D3D/*.hpp templates/*.hpp exports/*.hpp ImGui/*.cpp ImGui/*.h)
else()
    file(GLOB_RECURSE SRC src/*.cpp includes/*.hpp includes/D3D/*.hpp templates/*.hpp exports/*.hpp)
endif()

add_library(${PROJECT_NAME} SHARED
    ${SRC}
)

target_include_directories(${PROJECT_NAME} PRIVATE includes/ includes/D3D/ includes/Exceptions/ templates/ exports/)

target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:_DEBUG>" "$<$<CONFIG:RELEASE>:NDEBUG>" BUILD_GAIAX)

target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d12.lib dxgi.lib dxguid.lib uuid.lib d3dcompiler.lib
)

if(IMGUIFLAG STREQUAL "ENABLE")
    target_include_directories(${PROJECT_NAME} PRIVATE ImGui/includes/)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _IMGUI)
endif()

if(MSVC)
    string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast /MP /EHa /Ot)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "GraphicsEngine")

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

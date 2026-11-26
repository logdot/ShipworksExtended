cmake_minimum_required(VERSION 3.16)
project(ShipworksExtended VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(${PROJECT_NAME} SHARED
        src/crash.cpp
        src/dllmain.cpp
        src/game.cpp
        src/hook.cpp
        src/log.cpp
        src/mem.cpp
        src/menus.cpp
        src/palettes.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE libs/json src)

# MinGW would otherwise call the output libShipworksExtended.dll.
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /utf-8)
endif()

# Static linking for MinGW (avoids Wine runtime conflicts)
if(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE -static -static-libgcc -static-libstdc++)
endif()

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/Shipworks.json
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shipworks.json)

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_language(CXX ASM_MASM)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


include_directories(libs/json)

set(SHIPWORKS_EXTENDED_SOURCE
        src/dllmain.cpp
        src/mem.cpp
        src/pch.cpp
        src/shipworks.cpp
        src/teebuf.cpp
        src/shipworksASM.asm)

project(ShipworksExtended VERSION 0.1.1)
add_library(${PROJECT_NAME} SHARED ${SHIPWORKS_EXTENDED_SOURCE})

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/Shipworks.json
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shipworks.json)

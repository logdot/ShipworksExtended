cmake_minimum_required(VERSION 3.10)
project(ShipworksExtended VERSION 0.1.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
        src/stubs.cpp)

add_library(${PROJECT_NAME} SHARED ${SHIPWORKS_EXTENDED_SOURCE})

# Static linking for MinGW (avoids Wine runtime conflicts)
if(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE -static -static-libgcc -static-libstdc++)
endif()

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/Shipworks.json
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shipworks.json)

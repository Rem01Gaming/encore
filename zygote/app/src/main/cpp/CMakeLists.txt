cmake_minimum_required(VERSION 3.22.1)

project(encorezygisk C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT DEFINED ANDROID_ABI)
    set(ANDROID_ABI "arm64-v8a")
endif()

if (NOT DEFINED DEX_BLOB_DIR)
    set(DEX_BLOB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/dexBlob")
endif()

add_subdirectory(external)

set(ENCOREZYGISK_SOURCES
    Main.cpp
    atexit.cpp
)

add_library(encorezygisk SHARED ${ENCOREZYGISK_SOURCES})

target_compile_options(encorezygisk PRIVATE
    -fexceptions
    -Wpedantic
    -Wall
    -Wextra
    -Werror
    -Wformat
    -Wuninitialized
)

target_include_directories(encorezygisk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${DEX_BLOB_DIR}
)

find_library(LOG_LIB log)

target_link_libraries(encorezygisk PRIVATE
    ${LOG_LIB}
    zygisk
    jnihelper
    dex_loader
)

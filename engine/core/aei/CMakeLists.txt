add_library(papagedon-aei
    src/AudioInput.cpp
)

add_library(papagedon::aei ALIAS papagedon-aei)

target_compile_features(papagedon-aei PUBLIC cxx_std_20)

target_include_directories(papagedon-aei
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

find_path(
    MINIAUDIO_INCLUDE_DIR
    NAMES miniaudio.h
    PATH_SUFFIXES include
    PATHS
        "$ENV{VCPKG_ROOT}/installed/x64-mingw-static/include"
        "$ENV{VCPKG_ROOT}/installed/x64-mingw-dynamic/include"
        "$ENV{VCPKG_ROOT}/installed/x64-windows-static/include"
        "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
)

if(NOT MINIAUDIO_INCLUDE_DIR)
    find_path(MINIAUDIO_INCLUDE_DIR NAMES miniaudio.h REQUIRED)
endif()

target_include_directories(papagedon-aei PRIVATE ${MINIAUDIO_INCLUDE_DIR})

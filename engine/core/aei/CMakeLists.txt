add_library(papagedon-aei
    src/AudioInput.cpp
)

add_library(papagedon::aei ALIAS papagedon-aei)

target_compile_features(papagedon-aei PUBLIC cxx_std_20)

target_include_directories(papagedon-aei
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

find_path(MINIAUDIO_INCLUDE_DIR NAMES miniaudio.h REQUIRED)
target_include_directories(papagedon-aei PRIVATE ${MINIAUDIO_INCLUDE_DIR})

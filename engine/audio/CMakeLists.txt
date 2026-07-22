add_library(papagedon-audio
    AudioAnalyzer.cpp
    AudioFrame.cpp
    ExperienceSignals.cpp
)

add_library(papagedon::audio ALIAS papagedon-audio)

target_compile_features(papagedon-audio PUBLIC cxx_std_20)

target_include_directories(papagedon-audio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

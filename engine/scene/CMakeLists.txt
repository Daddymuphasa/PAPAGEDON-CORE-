add_library(papagedon-scene
    SceneDNA.cpp
    SceneProfile.cpp
    SceneState.cpp
)

add_library(papagedon::scene ALIAS papagedon-scene)

target_compile_features(papagedon-scene PUBLIC cxx_std_20)

target_include_directories(papagedon-scene
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

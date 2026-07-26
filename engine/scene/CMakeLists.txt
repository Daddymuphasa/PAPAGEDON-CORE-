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
    PRIVATE
        # SceneDNA.cpp includes ExperienceTypes.h (full definition of
        # ExperienceGraphOutput).  This is a PRIVATE include — it does not
        # propagate to consumers of papagedon::scene.
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine/experience>
)

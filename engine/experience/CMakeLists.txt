add_library(papagedon-experience
    ExperienceGraph.cpp
    ExperienceState.cpp
    ExperienceEvent.cpp
)

add_library(papagedon::experience ALIAS papagedon-experience)

target_compile_features(papagedon-experience PUBLIC cxx_std_20)

target_include_directories(papagedon-experience
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(papagedon-experience
    PUBLIC
        papagedon::audio
)

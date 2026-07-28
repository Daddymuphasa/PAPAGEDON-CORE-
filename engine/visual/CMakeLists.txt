add_library(papagedon-visual
    Theme.cpp
    ThemeManager.cpp
)

add_library(papagedon::visual ALIAS papagedon-visual)

target_compile_features(papagedon-visual PUBLIC cxx_std_20)

# Self-contained: the theme layer serializes its own JSON, so it carries no
# third-party dependency and can be consumed by any other module.
target_include_directories(papagedon-visual
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

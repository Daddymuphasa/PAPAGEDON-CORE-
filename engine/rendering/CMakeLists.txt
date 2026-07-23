add_library(papagedon-rendering
    Renderer.cpp
    backends/OpenGLRenderer.cpp
    RenderFrame.cpp
    RenderSettings.cpp
)

add_library(papagedon::rendering ALIAS papagedon-rendering)

target_compile_features(papagedon-rendering PUBLIC cxx_std_20)

target_include_directories(papagedon-rendering
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(papagedon-rendering
    PUBLIC
        papagedon::scene
)

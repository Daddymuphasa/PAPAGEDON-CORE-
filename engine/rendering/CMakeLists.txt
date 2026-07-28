add_library(papagedon-rendering
    Renderer.cpp
    ShaderManager.cpp
    backends/OpenGLRenderer.cpp
    backends/DebugOverlayRenderer.cpp
    RenderFrame.cpp
    RenderSettings.cpp
)

add_library(papagedon::rendering ALIAS papagedon-rendering)

target_compile_features(papagedon-rendering PUBLIC cxx_std_20)

find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

target_include_directories(papagedon-rendering
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # IRenderer.h consumes the data-only ExperiencePreset from the experience
        # module.  This is a header-only include (the struct definition) — no link
        # dependency — so the renderer stays backend-neutral and allocation-free.
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine/experience>
        # The OpenGL backend reads the active visual::Theme when building
        # ShaderUniforms.  Header-only too (it only reads theme data members), so
        # the renderer needs the include path but no link to the visual library.
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine/visual>
)

target_link_libraries(papagedon-rendering
    PUBLIC
        papagedon::scene
    PRIVATE
        glad::glad
        glfw
)

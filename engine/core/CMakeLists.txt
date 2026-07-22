add_library(papagedon-core
    src/Engine.cpp
)

add_library(papagedon::core ALIAS papagedon-core)

target_compile_features(papagedon-core PUBLIC cxx_std_20)

target_include_directories(papagedon-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(papagedon-core
    PUBLIC
        papagedon::runtime
        papagedon::utilities
)

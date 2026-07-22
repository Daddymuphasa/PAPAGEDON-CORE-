add_library(papagedon-runtime
    src/Runtime.cpp
)

add_library(papagedon::runtime ALIAS papagedon-runtime)

target_compile_features(papagedon-runtime PUBLIC cxx_std_20)

target_include_directories(papagedon-runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(papagedon-runtime
    PUBLIC
        papagedon::audio
        papagedon::utilities
)

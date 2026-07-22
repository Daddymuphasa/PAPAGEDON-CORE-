add_library(papagedon-utilities
    src/Logger.cpp
)

add_library(papagedon::utilities ALIAS papagedon-utilities)

target_compile_features(papagedon-utilities PUBLIC cxx_std_20)

target_include_directories(papagedon-utilities
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

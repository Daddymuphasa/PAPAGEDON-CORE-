add_executable(papagedon-player
    src/main.cpp
)

# Enforce C++20 for this target
target_compile_features(papagedon-player PRIVATE cxx_std_20)

target_link_libraries(papagedon-player PRIVATE papagedon::core)

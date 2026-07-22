cmake_minimum_required(VERSION 3.20)

project(PAPAGEDON-CORE VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Engine modules
add_subdirectory(engine/utilities)
add_subdirectory(engine/runtime)
add_subdirectory(engine/core)

# Applications
add_subdirectory(apps/player)

cmake_minimum_required(VERSION 3.20)

project(PAPAGEDON-CORE VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the apps/player subdirectory
add_subdirectory(apps/player)

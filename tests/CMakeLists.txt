add_executable(papagedon-show-tests ShowModeTests.cpp)
target_link_libraries(papagedon-show-tests PRIVATE papagedon::experience papagedon::visual)
target_include_directories(papagedon-show-tests PRIVATE
    "${CMAKE_SOURCE_DIR}/engine/audio"
    "${CMAKE_SOURCE_DIR}/engine/rendering")
add_test(NAME show-mode COMMAND papagedon-show-tests)
set_tests_properties(show-mode PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

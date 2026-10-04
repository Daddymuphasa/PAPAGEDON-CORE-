add_executable(papagedon-show-tests ShowModeTests.cpp)
target_link_libraries(papagedon-show-tests PRIVATE papagedon::experience papagedon::visual)
add_test(NAME show-mode COMMAND papagedon-show-tests)
set_tests_properties(show-mode PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

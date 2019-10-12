cmake_minimum_required(VERSION 3.1)
project(se-task1 LANGUAGES CXX)

function(simple name)
    add_executable("${name}" "${name}.cc")
    set_property(TARGET "${name}" PROPERTY CXX_STANDARD 17)
    target_compile_options("${name}" PRIVATE -Wall -Wextra)
endfunction()

simple(kasa)
simple(sand)

file(COPY tests DESTINATION ${CMAKE_BINARY_DIR})
cmake_minimum_required(VERSION 3.10.2)

project(bc_example)

include_directories(/usr/local/lib/include)
link_directories(/usr/local/lib)

find_library(converter REQUIRED)

add_executable(example example.c)
target_link_libraries(example PRIVATE converter)


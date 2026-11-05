cmake_minimum_required(VERSION 3.10.2)

project(bc_example)

# Finds the installed converter.h and libconverter.so (by default in
# /usr/local); set CMAKE_PREFIX_PATH for another location.
find_path(CONVERTER_INCLUDE_DIR converter.h PATHS /usr/local/include)
find_library(CONVERTER_LIBRARY converter PATHS /usr/local/lib)
if (NOT CONVERTER_INCLUDE_DIR OR NOT CONVERTER_LIBRARY)
    message(FATAL_ERROR "converter.h or libconverter not found; install the converter first")
endif ()

add_executable(example example.c)
target_include_directories(example PRIVATE ${CONVERTER_INCLUDE_DIR})
target_link_libraries(example PRIVATE ${CONVERTER_LIBRARY})

cmake_minimum_required(VERSION 3.10.2)

project(biometric_converter)

set(CMAKE_C_FLAGS "-fPIC")

# path dependency libs
set(DEP_INCLUDE_DIR /usr/local/include)
set(DEP_LINK_DIR /usr/local/lib)

# path to install
set(INSTALL_INCLUDE_DIR /usr/local/include)
set(INSTALL_LINK_DIR /usr/local/lib)
set(INSTALL_BIN_DIR /usr/local/bin)

include_directories(${DEP_INCLUDE_DIR})
link_directories(${DEP_LINK_DIR})

# NBIS use openjp2
add_definitions(-D__NBIS_OPENJP2__=ON)
# fix stdin for openjp2
add_definitions(-DOPJ_STDINT_H=OFF)

find_package(Threads REQUIRED)

add_library(converter SHARED lib/converter.c)
INSTALL(TARGETS converter LIBRARY DESTINATION ${INSTALL_LINK_DIR})
INSTALL(FILES include/converter.h DESTINATION ${INSTALL_INCLUDE_DIR})
target_link_libraries(converter PRIVATE
        biomdi
        fmr
        mindtct
        image
        wsq
        an2k
        ihead
        jpegl
        jpegb
        fet
        cblas
        ioutil
        util
        openjp2
        png
        z
        m
        Threads::Threads)
target_include_directories(converter PRIVATE include)

add_executable(convert bin/convert.c)
target_link_libraries(convert PRIVATE
        converter
        biomdi
        fmr
        mindtct
        image
        wsq
        an2k
        ihead
        jpegl
        jpegb
        fet
        cblas
        ioutil
        util
        openjp2
        png
        z
        m)
target_include_directories(convert PRIVATE include)
INSTALL(TARGETS convert RUNTIME DESTINATION ${INSTALL_BIN_DIR})

# Tests: run with 'ctest' (or 'make test') in the build directory
enable_testing()
add_executable(test_converter tests/test_converter.c)
# wsq and its helpers decode the WSQ test image for the raw2fmr() check
target_link_libraries(test_converter PRIVATE converter fmr biomdi wsq fet ioutil util Threads::Threads m)
target_include_directories(test_converter PRIVATE include)
add_test(NAME converter
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run.sh ${CMAKE_CURRENT_BINARY_DIR})

cmake_minimum_required(VERSION 3.16)

project(biometric_converter)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nbis/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nbis/lib)

add_library(biomdi biomdi/biomdi.c)
target_include_directories(biomdi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/biomdi/include)

file(GLOB FMR_SRC fmr/*.c)
add_library(fmr ${FMR_SRC})
target_link_libraries(fmr PRIVATE biomdi)
target_include_directories(fmr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/fmr/include)

add_executable(convert bin/convert.c)
target_link_libraries(convert PRIVATE
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

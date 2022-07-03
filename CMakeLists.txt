cmake_minimum_required(VERSION 3.16)

project(biometric_converter)

set(CMAKE_C_FLAGS "-fPIC")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nbis/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nbis/lib)

add_library(biomdi biomdi/biomdi.c)
target_include_directories(biomdi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/biomdi/include)

file(GLOB FMR_SRC fmr/*.c)
add_library(fmr ${FMR_SRC})
target_link_libraries(fmr PRIVATE biomdi)
target_include_directories(fmr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/fmr/include)

add_library(converter SHARED lib/converter.h lib/converter.c)
install(TARGETS converter DESTINATION lib)
include(GenerateExportHeader)
GENERATE_EXPORT_HEADER(converter
        BASE_NAME converter
        EXPORT_MACRO_NAME EXPORT
        EXPORT_FILE_NAME ${CMAKE_SOURCE_DIR}/exports/converter_EXPORTS.h
        STATIC_DEFINE SHARED_EXPORTS_BUILT_AS_STATIC)
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
        m)
target_include_directories(converter PUBLIC ${CMAKE_SOURCE_DIR}/exports)

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
target_include_directories(convert PUBLIC ${CMAKE_SOURCE_DIR}/lib)
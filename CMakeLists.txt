cmake_minimum_required(VERSION 3.10.2)

project(biometric_converter)

set(CMAKE_C_FLAGS "-fPIC")

# path libs
include_directories(/usr/local/lib/include)
link_directories(/usr/local/lib)

# NBIS use openjp2
add_definitions(-D__NBIS_OPENJP2__=ON)
# fix stdin for openjp2
add_definitions(-DOPJ_STDINT_H=OFF)

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
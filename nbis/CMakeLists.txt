find_package (JPEG REQUIRED)

include_directories(${JPEG_INCLUDE_DIR})

file(GLOB SRCC lib/*.c lib/*/*.c)
add_library(nbis ${SRCC})
target_link_libraries(nbis ${JPEG_LIBRARIES})

target_include_directories (nbis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
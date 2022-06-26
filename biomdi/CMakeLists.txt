file(GLOB SRCC lib/*.c)
add_library(biomdi ${SRCC})

target_include_directories (biomdi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(converter lib/converter.h lib/converter.c)

add_executable(convert bin/convert.c)

target_link_libraries(convert converter)
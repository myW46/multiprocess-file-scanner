include_directories(include)
include_directories(../common)

add_executable(scanner_stats src/main.cpp)

if(WIN32)
    target_link_libraries(scanner_stats ws2_32)
endif()
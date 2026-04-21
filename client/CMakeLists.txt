include_directories(include)

add_executable(scanner_client src/main.cpp)

if(WIN32)
    target_link_libraries(scanner_client ws2_32)
endif()
include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/server/include)
include_directories(${CMAKE_SOURCE_DIR}/common)
add_executable(unit_tests main.cpp ../server/src/AhoCorasick.cpp ../server/src/ConfigManager.cpp)

if(WIN32)
    target_link_libraries(unit_tests ws2_32)
endif()
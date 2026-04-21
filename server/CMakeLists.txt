include_directories(include)
include_directories(../common)


add_executable(scanner_server src/main.cpp src/ConfigManager.cpp src/Stats.cpp src/ProcessManager.cpp src/Worker.cpp "src/AhoCorasick.cpp")

if(WIN32)
	target_link_libraries(scanner_server ws2_32)
endif()
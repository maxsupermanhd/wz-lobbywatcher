cmake_minimum_required(VERSION 3.4.3)
project(wz-lobbywatcher VERSION 1.0)

add_subdirectory("wz-lobbyreader")

add_executable(wz-lobbywatcher main.cpp)
target_include_directories(wz-lobbywatcher PRIVATE wz-lobbyreader:lobbyreader)
target_link_libraries(wz-lobbywatcher lobbyreader)
cmake_minimum_required(VERSION 3.0)
project(string_or_view)

set(CMAKE_CXX_STANDARD 17)

add_library(string_or_view INTERFACE)
target_sources(string_or_view INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include/string_or_view.h)
target_include_directories(string_or_view INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

add_executable(string_or_view_sample ${CMAKE_CURRENT_LIST_DIR}/sample/sample.cpp)
target_link_libraries(string_or_view_sample PRIVATE string_or_view)

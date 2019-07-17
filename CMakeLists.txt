CMAKE_MINIMUM_REQUIRED (VERSION 3.14 FATAL_ERROR)
project(test-ttf)

SET (CMAKE_CXX_STANDARD 17)
SET (CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main main.cpp)
target_link_libraries(main)
cmake_minimum_required(VERSION 3.10)
project(BezierCurve)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 14)

include_directories(/usr/local/include ./include)
include_directories(/opt/homebrew/opt/eigen/include/eigen3)
include_directories(/opt/homebrew/opt/opencv/include/opencv4)

add_executable(BezierCurve main.cpp)

target_link_libraries(BezierCurve ${OpenCV_LIBRARIES})

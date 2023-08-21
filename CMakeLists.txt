cmake_minimum_required(VERSION 3.5)
project(ColorDetection LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

find_package(OpenCV REQUIRED)

include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable (color_detection 
orientation.cpp
orientation.h

#main
main.cpp
)

target_link_libraries(color_detection ${OpenCV_LIBS})

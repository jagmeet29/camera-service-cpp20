cmake_minimum_required(VERSION 3.20)

project(camera_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core videoio highgui imgproc)

add_executable(camera_view
    src/main.cpp
    src/bounded_frame_queue.cpp
    src/processor.cpp
)

target_include_directories(camera_view PRIVATE include)

target_link_libraries(camera_view PRIVATE
    opencv_core
    opencv_videoio
    opencv_highgui
    opencv_imgproc
)

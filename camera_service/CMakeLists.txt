cmake_minimum_required(VERSION 3.20)

project(camera_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core videoio highgui imgproc)

add_executable(camera_view
    src/main.cpp
    src/bounded_frame_queue.cpp
    src/processor.cpp
    src/capture.cpp
)

target_include_directories(camera_view PRIVATE include)

target_link_libraries(camera_view PRIVATE
    opencv_core
    opencv_videoio
    opencv_highgui
    opencv_imgproc
)

find_package(Threads REQUIRED)

add_executable(queue_test
    tests/queue_test.cpp
    src/bounded_frame_queue.cpp
)

target_include_directories(queue_test PRIVATE include)

target_link_libraries(queue_test PRIVATE
    opencv_core
    Threads::Threads
)

enable_testing()

add_test(NAME queue_behavior COMMAND queue_test)

set_tests_properties(queue_behavior PROPERTIES TIMEOUT 5)
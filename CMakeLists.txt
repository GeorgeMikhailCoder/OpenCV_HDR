# CMakeList.txt : CMake project for OpenCV_HDR, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("OpenCV_HDR")

# Add source to this project's executable.
add_executable (OpenCV_HDR "OpenCV_HDR.cpp" "OpenCV_HDR.h")

# TODO: Add tests and install targets if needed.
find_package(OpenCV REQUIRED)
target_link_libraries(OpenCV_HDR ${OpenCV_LIBS})
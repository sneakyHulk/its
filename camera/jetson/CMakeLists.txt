cmake_minimum_required(VERSION 3.10)

# Set project name and language
project(CaptureToVideo)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)

# Find Pylon SDK
find_package(pylon REQUIRED)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Add the source files
add_executable(capture_to_video main.cpp)

# Link the Pylon and OpenCV libraries
target_link_libraries(capture_to_video
    pylon::pylon
    ${OpenCV_LIBS}
)

# Print information about the found packages (optional, for debugging)
message(STATUS "Found Pylon: ${Pylon_INCLUDE_DIRS}")
message(STATUS "Found OpenCV: ${OpenCV_INCLUDE_DIRS}")
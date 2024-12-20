cmake_minimum_required(VERSION 3.28)
project(its)

set(CMAKE_CXX_STANDARD 23)
option(STREAMING_TEST "Enable testing various streaming technology" OFF)

########## find local packages #########################################################################################
### OpenCV
find_package(OpenCV REQUIRED)

message(${OpenCV_INCLUDE_DIRS})
message(${OpenCV_LIBS})

### Intel Threading Blocks
find_package(TBB REQUIRED)

### libtorch
set(TORCH_CUDA_ARCH_LIST "8.0 8.6 8.9 9.0") # new CUDA arch is 9.0a, therefore the CAFFE2 regex failed, uff
set(CMAKE_CUDA_ARCHITECTURES "native")
find_package(Torch REQUIRED PATHS ~/src/libtorch)

### Eigen
find_package(Eigen3 REQUIRED)

### Pylon
find_package(pylon)

### eCAL
find_package(eCAL)

### Protobuf
find_package(Protobuf REQUIRED)

### Gstreamer
# apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio libgstrtspserver-1.0-dev gstreamer1.0-rtsp
find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.4)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)
pkg_search_module(gstreamer-rtsp-server REQUIRED IMPORTED_TARGET gstreamer-rtsp-server-1.0>=1.4)
pkg_search_module(gstreamer-rtsp REQUIRED IMPORTED_TARGET gstreamer-rtsp-1.0>=1.4)
pkg_search_module(gstreamer-rtp REQUIRED IMPORTED_TARGET gstreamer-rtp-1.0>=1.4)

### gtk+
pkg_check_modules(gtk+-3.0 REQUIRED IMPORTED_TARGET gtk+-3.0)

### gpiod
pkg_check_modules(gpiod IMPORTED_TARGET libgpiod)

########## add submodules #########################################################################################
### autodiff
set(AUTODIFF_BUILD_TESTS OFF)
set(AUTODIFF_BUILD_PYTHON OFF)
set(AUTODIFF_BUILD_EXAMPLES OFF)
set(AUTODIFF_BUILD_DOCS OFF)
add_subdirectory(thirdparty/autodiff)

### OpenDrive
add_subdirectory(thirdparty/OpenDRIVE)
set_property(TARGET OpenDrive PROPERTY POSITION_INDEPENDENT_CODE ON)

### Range v3
add_subdirectory(thirdparty/range-v3)

### Json
add_subdirectory(thirdparty/json)

### Boost 1.85.0
add_subdirectory(thirdparty/boost)

### bytetrack
add_subdirectory(thirdparty/bytetrack)

### common
add_library(common INTERFACE)
target_include_directories(common INTERFACE thirdparty/common/include)
target_link_libraries(common INTERFACE)
target_compile_features(common INTERFACE cxx_std_20)

### mqtt
# git submodule add https://github.com/eclipse/paho.mqtt.cpp.git thirdparty/mqtt
set(PAHO_WITH_MQTT_C ON CACHE INTERNAL "")
set(PAHO_WITH_SSL OFF CACHE INTERNAL "")
set(PAHO_HIGH_PERFORMANCE ON CACHE INTERNAL "")
add_subdirectory(thirdparty/mqtt)

### yas
add_library(yas INTERFACE)
target_include_directories(yas INTERFACE thirdparty/yas/include)
target_link_libraries(yas INTERFACE)
target_compile_features(yas INTERFACE cxx_std_23)


########## add local stuff #############################################################################################

#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_subdirectory(msg)
add_subdirectory(node)
#add_subdirectory(external)
add_subdirectory(utils)
add_subdirectory(yolo)
add_subdirectory(transformation)
add_subdirectory(camera)
add_subdirectory(visualization)
add_subdirectory(tracking)
add_subdirectory(communication)
#add_subdirectory(dgon)
add_subdirectory(image_processing)


########## add main executable #########################################################################################

add_library(config INTERFACE config.h)
target_include_directories(config INTERFACE ${OpenCV_INCLUDE_DIRS})
target_include_directories(config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(config INTERFACE ${OpenCV_LIBS})
target_link_libraries(config INTERFACE eigen_utils)
target_compile_features(config INTERFACE cxx_std_23)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC yolo_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC camera_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC visualization_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC transformation_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC tracking_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC visualization_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC communication_nodes)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

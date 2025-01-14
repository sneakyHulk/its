cmake_minimum_required(VERSION 3.28)
project(its)

set(CMAKE_CXX_STANDARD 23)

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
find_package(pylon REQUIRED)

### JSON
find_package(nlohmann_json REQUIRED)

### eCAL
find_package(eCAL)

### Protobuf
find_package(Protobuf REQUIRED)


########## find local pkg packages #####################################################################################
find_package(PkgConfig REQUIRED)

### Gstreamer
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

### mosquitto
pkg_check_modules(Mosquitto IMPORTED_TARGET libmosquitto REQUIRED)


########## add submodules ##############################################################################################
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

### Boost 1.85.0
add_subdirectory(thirdparty/boost)

### bytetrack
add_subdirectory(thirdparty/bytetrack)

### common
add_library(common INTERFACE)
target_include_directories(common INTERFACE thirdparty/common/include)
target_link_libraries(common INTERFACE)
target_compile_features(common INTERFACE cxx_std_20)

### yas
add_library(yas INTERFACE)
target_include_directories(yas INTERFACE thirdparty/yas/include)
target_link_libraries(yas INTERFACE)
target_compile_features(yas INTERFACE cxx_std_23)

### concurra
add_subdirectory(thirdparty/concurra)

########## add local stuff #############################################################################################

#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

enable_testing()

add_subdirectory(msg)
add_subdirectory(external)
add_subdirectory(utils)
add_subdirectory(yolo)
add_subdirectory(transformation)
add_subdirectory(camera)
add_subdirectory(visualization)
add_subdirectory(tracking)
add_subdirectory(communication)
add_subdirectory(image_processing)


########## add main executable #########################################################################################

add_library(config INTERFACE config.h)
target_include_directories(config INTERFACE ${OpenCV_INCLUDE_DIRS})
target_include_directories(config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(config INTERFACE ${CMAKE_SOURCE_DIR}/config)
target_link_libraries(config INTERFACE ${OpenCV_LIBS})
target_link_libraries(config INTERFACE eigen_utils)
target_compile_features(config INTERFACE cxx_std_23)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC yolo_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC config)
target_link_libraries(${PROJECT_NAME} PUBLIC cameras_simulator_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC image_visualization_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC image_processing_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC transformation_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC bird_eye_visualization_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC image_tracking_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC image_communication_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC data_communication_nodes)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
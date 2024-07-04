cmake_minimum_required(VERSION 3.28)
project(its)

set(CMAKE_CXX_STANDARD 23)

########## find local packages #########################################################################################
### OpenCV
find_package(OpenCV REQUIRED)

### Intel Threading Blocks
find_package(TBB REQUIRED)

### libtorch
set(TORCH_CUDA_ARCH_LIST "8.0 8.6 8.9 9.0") # new CUDA arch is 9.0a, therefore the CAFFE2 regex failed, uff
find_package(Torch REQUIRED PATHS ~/src/libtorch NO_DEFAULT_PATH)

### Eigen
find_package(Eigen3 REQUIRED)

### RTP
find_package(PkgConfig REQUIRED)
pkg_check_modules(CCRTP REQUIRED libccrtp)

### Gstreamer
find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.4)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)
pkg_search_module(gstreamer-rtsp-server REQUIRED IMPORTED_TARGET gstreamer-rtsp-server-1.0>=1.4)


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

### common
add_library(common INTERFACE)
target_include_directories(common INTERFACE thirdparty/common/include)
target_link_libraries(common INTERFACE)
target_compile_features(common INTERFACE cxx_std_23)


########## add local stuff #############################################################################################
add_subdirectory(msg)
add_subdirectory(node)
add_subdirectory(yolo)
add_subdirectory(transformation)
add_subdirectory(camera_simulator)
add_subdirectory(visualization)
add_subdirectory(tracking)
add_subdirectory(communication)


########## add main executable #########################################################################################
add_executable(its main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC yolo_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC camera_simulator_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC visualization_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC transformation_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC tracking_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC visualization_nodes)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

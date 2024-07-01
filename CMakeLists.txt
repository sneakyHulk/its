cmake_minimum_required(VERSION 3.28)
project(its)

## find packages
find_package(OpenCV REQUIRED)
find_package(TBB REQUIRED)
set(TORCH_CUDA_ARCH_LIST "8.0 8.6 8.9 9.0") # new CUDA arch is 9.0a, therefore the CAFFE2 regex failed, uff
find_package(Torch REQUIRED PATHS ~/src/libtorch NO_DEFAULT_PATH)


add_subdirectory(msg)
add_subdirectory(node)
add_subdirectory(yolo)

add_executable(its main.cpp)

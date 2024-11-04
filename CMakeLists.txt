cmake_minimum_required(VERSION 3.28)
project(its)

set(CMAKE_CXX_STANDARD 23)
option(STREAMING_TEST "Enable testing various streaming technology" OFF)

########## find local packages #########################################################################################
### OpenCV
find_package(OpenCV REQUIRED)

### Intel Threading Blocks
find_package(TBB REQUIRED)

### libtorch
set(TORCH_CUDA_ARCH_LIST "8.0 8.6 8.9 9.0") # new CUDA arch is 9.0a, therefore the CAFFE2 regex failed, uff
find_package(Torch REQUIRED PATHS ~/src/libtorch)

### Eigen
find_package(Eigen3 REQUIRED)

### Pylon
if (DEFINED ENV{LIVE})
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindPylon.cmake")
    message("Pylon found!")
endif ()

### eCAL
find_package(eCAL REQUIRED)

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

if (STREAMING_TEST)
    ### RTP
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CCRTP REQUIRED libccrtp)

    ### mosquitto
    # apt install libmosquitto-dev
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Mosquitto IMPORTED_TARGET libmosquitto REQUIRED)

    ### SRT
    # apt install libsrt-gnutls-dev
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(srt-gnutls REQUIRED IMPORTED_TARGET srt)

    ### rtmp
    find_library(RTMP_LIB rtmp REQUIRED)

    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
    find_library(AVCODEC_LIBRARY avcodec)

    find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
    find_library(AVFORMAT_LIBRARY avformat)

    find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
    find_library(AVUTIL_LIBRARY avutil)

    find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
    find_library(AVDEVICE_LIBRARY avdevice)

    find_path(AVFILTER_INCLUDE_DIR libavfilter/avfilter.h)
    find_library(AVFILTER_LIBRARY avfilter)

    find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
    find_library(SWSCALE_LIBRARY swscale)
endif ()


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
target_compile_features(common INTERFACE cxx_std_23)

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

if (STREAMING_TEST)
    ### libdatachannel
    # git submodule add https://github.com/paullouisageneau/libdatachannel.git thirdparty/libdatachannel
    set(USE_GNUTLS OFF)
    add_subdirectory(thirdparty/libdatachannel)

    ### SRT
    #set(ENABLE_SHARED ON)
    #set(ENABLE_EXAMPLES ON)
    #add_subdirectory(thirdparty/srt)
    #
    #SET(SRT_INCLUDE_DIR
    #
    #        ${CMAKE_BINARY_DIR}/${WITH_SRT_DIR}
    #
    #        ${CMAKE_SOURCE_DIR}/${WITH_SRT_DIR}/haicrypt
    #)
    #
    #function(export_headers TARGET HEADER_SOURCE_DIR HEADER_DEST_DIR)
    #
    #    # Put all headers that are in the source directory into EXPORT_HEADERS variable
    #    file(GLOB_RECURSE EXPORT_HEADERS CONFIGURE_DEPENDS
    #            RELATIVE "${HEADER_SOURCE_DIR}"
    #            "${HEADER_SOURCE_DIR}/*.h"
    #    )
    #
    #    # For each header that will be exported
    #    foreach(HEADER ${EXPORT_HEADERS})
    #
    #        # Get the directory portion that needs to be created
    #        get_filename_component(HEADER_DIRECTORY "${HEADER}" DIRECTORY)
    #
    #        # Create the directory
    #        add_custom_command(TARGET ${TARGET} POST_BUILD
    #                COMMAND ${CMAKE_COMMAND} -E make_directory "${HEADER_DEST_DIR}/${HEADER_DIRECTORY}"
    #        )
    #
    #        if (MSVC)
    #
    #            # Make a hard link to the file
    #            add_custom_command(TARGET ${TARGET} POST_BUILD
    #                    COMMAND if not exist "${HEADER_DEST_DIR}/${HEADER}" \( mklink /h "${HEADER_DEST_DIR}/${HEADER}" "${HEADER_SOURCE_DIR}/${HEADER}" \)
    #            )
    #
    #        else()
    #
    #            # Make a symbolic link to the file
    #            add_custom_command(TARGET ${TARGET} POST_BUILD
    #                    COMMAND ln -sf "${HEADER_SOURCE_DIR}/${HEADER}" "${HEADER_DEST_DIR}/${HEADER}"
    #            )
    #            message(${HEADER_SOURCE_DIR}/${HEADER} to ${HEADER_DEST_DIR}/${HEADER})
    #        endif()
    #
    #
    #    endforeach(HEADER)
    #
    #endfunction()
    #add_library(srt_ SHARED dummy.cpp)
    #export_headers(srt_ ${CMAKE_SOURCE_DIR}/thirdparty/srt/srtcore ${CMAKE_BINARY_DIR}/include/srt)
    #export_headers(srt_ ${CMAKE_SOURCE_DIR}/thirdparty/srt/common ${CMAKE_BINARY_DIR}/include/srt)
    #export_headers(srt_ ${CMAKE_SOURCE_DIR}/thirdparty/srt/haicrypt ${CMAKE_BINARY_DIR}/include/srt)
    #export_headers(srt_ ${CMAKE_BINARY_DIR}/thirdparty/srt ${CMAKE_BINARY_DIR}/include/srt)
    #target_include_directories(srt_ PUBLIC ${CMAKE_BINARY_DIR}/include)
    #target_link_libraries(srt_ PUBLIC ${srt_link_library} ${DEPENDS_srt})


    ### srt live server
    # git submodule add https://github.com/Edward-Wu/srt-live-server.git thirdparty/sls
    file(GLOB sls_SOURCES "thirdparty/sls/slscore/*.cpp")
    message(${sls_SOURCES})
    add_library(sls SHARED ${sls_SOURCES})
    target_include_directories(sls PUBLIC thirdparty/sls/slscore)
    target_link_libraries(sls PUBLIC srt-gnutls)
endif ()

########## add local stuff #############################################################################################

#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_subdirectory(msg)
add_subdirectory(node)
add_subdirectory(external)
add_subdirectory(utils)
add_subdirectory(yolo)
add_subdirectory(transformation)
add_subdirectory(camera)
add_subdirectory(visualization)
add_subdirectory(tracking)
add_subdirectory(communication)
add_subdirectory(area_blocking)
#add_subdirectory(dgon)
add_subdirectory(image_processing)


########## add main executable #########################################################################################
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

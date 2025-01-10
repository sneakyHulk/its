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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test/sls.conf ${CMAKE_CURRENT_BINARY_DIR}/sls.conf COPYONLY)
add_executable(sls_server test/srt-live-server.cpp)
target_link_libraries(sls_server PUBLIC sls)

add_executable(test_${PROJECT_NAME}_rtmp test/test_rtmp.cpp)
target_link_libraries(test_${PROJECT_NAME}_rtmp PUBLIC ${PROJECT_NAME})
target_link_libraries(test_${PROJECT_NAME}_rtmp PUBLIC camera_nodes)
target_link_libraries(test_${PROJECT_NAME}_rtmp PUBLIC ${RTMP_LIB})
target_link_libraries(test_${PROJECT_NAME}_rtmp PUBLIC ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY} ${AVFILTER_LIBRARY} ${SWSCALE_LIBRARY})
target_compile_features(test_${PROJECT_NAME}_rtmp PRIVATE cxx_std_23)
target_compile_definitions(test_${PROJECT_NAME}_rtmp PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

### mqtt
# git submodule add https://github.com/eclipse/paho.mqtt.cpp.git thirdparty/mqtt
set(PAHO_WITH_MQTT_C ON CACHE INTERNAL "")
set(PAHO_WITH_SSL OFF CACHE INTERNAL "")
set(PAHO_HIGH_PERFORMANCE ON CACHE INTERNAL "")
add_subdirectory(thirdparty/mqtt)
target_link_libraries(${PROJECT_NAME} PUBLIC paho-mqttpp3)
target_compile_definitions(${PROJECT_NAME} PRIVATE PAHO_MQTTPP_IMPORTS)

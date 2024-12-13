
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
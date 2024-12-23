project(test_read_config)

add_executable(${PROJECT_NAME} test/${PROJECT_NAME}.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN3_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC transformation)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
#target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

project(eval_latency)

add_executable(${PROJECT_NAME} src/${PROJECT_NAME}.cpp src/YoloNode.cpp src/ImageTrackerNode.cpp src/TrackToTrackFusion.cpp src/TrackingVisualizationNode.cpp src/SortTracking.cpp src/AfterReturnTimeMeasure.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN3_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC node)
target_link_libraries(${PROJECT_NAME} PUBLIC msg)
target_link_libraries(${PROJECT_NAME} PUBLIC yolo_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC transformation)
target_link_libraries(${PROJECT_NAME} PUBLIC tracking_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC visualization_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer-sdp)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer-app)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer-video)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer-rtsp-server)
target_link_libraries(${PROJECT_NAME} PUBLIC msg_yas)
target_link_libraries(${PROJECT_NAME} PUBLIC paho-mqttpp3)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE PAHO_MQTTPP_IMPORTS)
#target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

project(eval_latency_stream)

add_executable(${PROJECT_NAME} src/${PROJECT_NAME}.cpp src/YoloNode.cpp src/ImageTrackerNode.cpp src/TrackToTrackFusion.cpp src/TrackingVisualizationNode.cpp src/SortTracking.cpp src/AfterReturnTimeMeasure.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN3_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC node)
target_link_libraries(${PROJECT_NAME} PUBLIC msg)
target_link_libraries(${PROJECT_NAME} PUBLIC yolo_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC transformation)
target_link_libraries(${PROJECT_NAME} PUBLIC tracking_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC visualization_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer-sdp)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer-app)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer-video)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer-rtsp-server)
target_link_libraries(${PROJECT_NAME} PUBLIC msg_yas)
target_link_libraries(${PROJECT_NAME} PUBLIC paho-mqttpp3)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE PAHO_MQTTPP_IMPORTS)
#target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

project(test_kalman_filer_multiple_predict)

add_executable(${PROJECT_NAME} test/${PROJECT_NAME}.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN3_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC node)
target_link_libraries(${PROJECT_NAME} PUBLIC msg)
target_link_libraries(${PROJECT_NAME} PUBLIC yolo_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC transformation)
target_link_libraries(${PROJECT_NAME} PUBLIC tracking_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC visualization_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
#target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
project(image_tracking_nodes)

add_library(${PROJECT_NAME} SHARED src/KalmanFilter.cpp src/KalmanBoxSourceTrack.cpp src/ImageTrackerNode.cpp src/TrackToTrackFusion.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN3_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC node)
target_link_libraries(${PROJECT_NAME} PUBLIC msg)
target_link_libraries(${PROJECT_NAME} PUBLIC range-v3)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

add_executable(test_${PROJECT_NAME} test/test_${PROJECT_NAME}.cpp)
target_link_libraries(test_${PROJECT_NAME} PUBLIC ${PROJECT_NAME})
target_link_libraries(test_${PROJECT_NAME} PUBLIC image_visualization_nodes)
target_link_libraries(test_${PROJECT_NAME} PUBLIC image_processing_nodes)
target_link_libraries(test_${PROJECT_NAME} PUBLIC cameras_simulator_nodes)
target_compile_features(test_${PROJECT_NAME} PRIVATE cxx_std_23)


# project(test_bytetrack)
#
# add_executable(${PROJECT_NAME} test/${PROJECT_NAME}.cpp)
# target_link_libraries(${PROJECT_NAME} PUBLIC bytetrack)
# target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
# target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
# target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
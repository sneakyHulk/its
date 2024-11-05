project(image_processing_nodes)

add_library(${PROJECT_NAME} SHARED src/DownscalingNode.cpp src/PreprocessingNode.cpp src/ImageUndistortionNode.cpp src/SavingNode.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC node)
target_link_libraries(${PROJECT_NAME} PUBLIC msg)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

project(test_image_processing_nodes)

add_executable(${PROJECT_NAME} test/test_image_processing_nodes.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC image_processing_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC cameras_simulator_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC image_visualization_nodes)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
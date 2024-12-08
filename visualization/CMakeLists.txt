project(image_visualization_nodes)

add_library(${PROJECT_NAME} SHARED src/ImageVisualizationNode.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC msg)
target_link_libraries(${PROJECT_NAME} PUBLIC node)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gtkmm-4.0)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

project(visualization)

add_library(${PROJECT_NAME} STATIC src/DrawingUtils.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN3_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE transformation) # is actually C++23 lib <- needs to be private to work within
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenDrive)
target_link_libraries(${PROJECT_NAME} PRIVATE autodiff::autodiff) # needs C++17
target_link_libraries(${PROJECT_NAME} PUBLIC boost_circular_buffer)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
set_property(TARGET visualization PROPERTY POSITION_INDEPENDENT_CODE ON)

project(visualization_nodes)

add_library(${PROJECT_NAME} SHARED src/visualization_node.cpp src/tracking_visualization_node.cpp src/video_node.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN3_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC node)
target_link_libraries(${PROJECT_NAME} PUBLIC msg)
target_link_libraries(${PROJECT_NAME} PUBLIC transformation)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
target_link_libraries(${PROJECT_NAME} PUBLIC boost_circular_buffer)
target_link_libraries(${PROJECT_NAME} PRIVATE visualization)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

project(test_video_node)
add_executable(${PROJECT_NAME} test/test_video_node.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC cameras_simulator_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC image_visualization_nodes)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

project(test_image_visualization_node)
add_executable(${PROJECT_NAME} test/test_image_visualization_node.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC cameras_simulator_nodes)
target_link_libraries(${PROJECT_NAME} PUBLIC image_visualization_nodes)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

project(create_video)
add_executable(${PROJECT_NAME} test/create_video.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_link_libraries(${PROJECT_NAME} PUBLIC visualization_nodes)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
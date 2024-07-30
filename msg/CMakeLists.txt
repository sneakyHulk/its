project(msg)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} INTERFACE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} INTERFACE boost_geometry)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

add_library(${PROJECT_NAME}_yas INTERFACE)
target_include_directories(${PROJECT_NAME}_yas INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}_yas INTERFACE ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}_yas INTERFACE yas)
target_compile_features(${PROJECT_NAME}_yas INTERFACE cxx_std_23)
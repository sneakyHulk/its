project(node)

add_library(${PROJECT_NAME} SHARED src/node.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${TBB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC TBB::tbb)
target_link_libraries(${PROJECT_NAME} PUBLIC common)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
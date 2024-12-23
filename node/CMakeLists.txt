project(node)

add_library(${PROJECT_NAME} SHARED src/Node.cpp src/Pusher.cpp src/Processor.cpp src/Runner.cpp src/RunnerSynchronous.cpp src/RunnerSynchronousPair.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${TBB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC TBB::tbb)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

add_executable(test_${PROJECT_NAME}_multiple test/test_multiple_inputs_outputs.cpp)
target_link_libraries(test_${PROJECT_NAME}_multiple PUBLIC node)
target_link_libraries(test_${PROJECT_NAME}_multiple PUBLIC common)
target_link_libraries(test_${PROJECT_NAME}_multiple PUBLIC msg)
target_compile_features(test_${PROJECT_NAME}_multiple PUBLIC cxx_std_23)

add_executable(test_${PROJECT_NAME}_pair test/test_${PROJECT_NAME}_pair.cpp)
target_link_libraries(test_${PROJECT_NAME}_pair PUBLIC node)
target_link_libraries(test_${PROJECT_NAME}_pair PUBLIC common)
target_link_libraries(test_${PROJECT_NAME}_pair PUBLIC msg)
target_compile_features(test_${PROJECT_NAME}_pair PUBLIC cxx_std_23)
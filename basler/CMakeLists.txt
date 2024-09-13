project(basler_support)

set(CMAKE_CXX_STANDARD 23)

include("${CMAKE_CURRENT_SOURCE_DIR}/FindPylon.cmake")

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${Pylon_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Pylon_LIBRARIES})

cmake_minimum_required(VERSION 3.7)
project(HF_CLion_Template)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(SOURCE_FILES
        framework.cpp
        framework.h
        Skeleton.cpp)

include_directories(include)
link_directories(lib)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} opengl32 freeglut glew32)
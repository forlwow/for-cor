cmake_minimum_required(VERSION 3.25)

project(web_server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

set(CMAKE_CXX_COMPILER /usr/local/bin/g++)
set(CMAKE_C_COMPILER /usr/local/bin/gcc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -w -g2 -ggdb -std=c++20")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/server/log/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/util/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/ethread/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/ecoroutine/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/fiber/inc)


aux_source_directory(${PROJECT_SOURCE_DIR}/server src_dir)
# 第三方库
include_directories(${PROJECT_SOURCE_DIR}/server/ThirdModule/include)
link_directories(${PROJECT_SOURCE_DIR}/server/ThirdModule/lib)
# 项目自己的库
link_directories(${PROJECT_SOURCE_DIR}/lib)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/server)

add_executable(
        ${PROJECT_NAME}
        ${src_dir}
)

target_link_libraries(${PROJECT_NAME} log util ethread ecoroutine fiber)
target_link_libraries(${PROJECT_NAME} yaml-cpp)
target_link_libraries(${PROJECT_NAME} pthread)



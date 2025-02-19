cmake_minimum_required(VERSION 3.25)

project(web_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 
# 测试选项
set(TEST ON)

set(CMAKE_CXX_COMPILER /usr/local/bin/g++)
set(CMAKE_C_COMPILER /usr/local/bin/gcc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -ldl")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/server/log/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/memory/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/util/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/ethread/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/ecoroutine/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/fiber/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/socket/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/http/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/server/inc)
include_directories(${PROJECT_SOURCE_DIR}/server/dev)

# 测试文件
if(${TEST})
        include_directories(${PROJECT_SOURCE_DIR}/test)
        add_subdirectory(${PROJECT_SOURCE_DIR}/test)
        aux_source_directory(${PROJECT_SOURCE_DIR}/test test_dir)
else()
        include_directories(${PROJECT_SOURCE_DIR}/User)
        aux_source_directory(${PROJECT_SOURCE_DIR}/User user_dir)
endif()

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
        ${test_dir}
        ${user_dir}
)


target_link_libraries(${PROJECT_NAME} log memory util ethread ecoroutine fiber socket dev http server)
target_link_libraries(${PROJECT_NAME} thirdMould)
target_link_libraries(${PROJECT_NAME} yaml-cpp)
target_link_libraries(${PROJECT_NAME} llhttp)
target_link_libraries(${PROJECT_NAME} pthread)

if(${TEST})
        target_link_libraries(${PROJECT_NAME} test)
endif()



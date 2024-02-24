set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/log)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/util)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ethread)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ecoroutine)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fiber)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/socket)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dev)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/http)


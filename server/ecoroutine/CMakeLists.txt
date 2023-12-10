set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src src_files)
add_library(ecoroutine SHARED ${src_files})

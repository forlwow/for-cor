set(CMAKE_LIBRARY_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/lib)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src src_files)
add_library(http SHARED ${src_files})

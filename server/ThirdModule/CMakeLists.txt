set(CMAKE_LIBRARY_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/lib)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/include/http_parse src_files)
add_library(thirdMould SHARED ${src_files})

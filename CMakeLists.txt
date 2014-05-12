project(server)
cmake_minimum_required(VERSION 2.8)
add_definitions(
    -std=c++11
)
aux_source_directory(. SRC_LIST)
add_library(${PROJECT_NAME} SHARED ${SRC_LIST})


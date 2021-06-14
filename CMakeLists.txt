cmake_minimum_required(VERSION 3.10)
project(TinyEdgePlayer)

set(CMAKE_CXX_STANDARD 17)


AUX_SOURCE_DIRECTORY(. SRC_LIST)

ADD_EXECUTABLE(TinyEdgePlayer ${SRC_LIST} Storage.cpp)
target_link_libraries(TinyEdgePlayer pthread)
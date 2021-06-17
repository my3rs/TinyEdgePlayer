CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(TinyEdgePlayer)

SET(CMAKE_CXX_STANDARD 17)

ADD_SUBDIRECTORY(rate_limiter)

AUX_SOURCE_DIRECTORY(. SRC_LIST)

list(REMOVE_ITEM SRC_LIST "./test.cpp")     # 从SRC_LIST中删除"test.cpp"，不然会出现“多次定义main”的error

ADD_EXECUTABLE(TinyEdgePlayer ${SRC_LIST})
TARGET_LINK_LIBRARIES(TinyEdgePlayer pthread glog gflags rate)

ADD_EXECUTABLE(tmp test.cpp)

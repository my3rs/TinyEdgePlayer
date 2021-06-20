CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(TinyEdgePlayer)

SET(CMAKE_CXX_STANDARD 17)

# glog begin
find_path(GLOG_INCLUDE_PATH NAMES glog/logging.h)
find_library(GLOG_LIB NAMES glog)
if ((NOT GLOG_INCLUDE_PATH) OR (NOT GLOG_LIB))
    message(FATAL_ERROR "Fail to find glog")
else()
    message(STATUS "Found glog")
endif()
include_directories(${GLOG_INCLUDE_PATH})
# glog end

# gflags being
find_path(GFLAGS_INCLUDE_PATH gflags/gflags.h)
find_library(GFLAGS_LIBRARY NAMES gflags libgflags)

if((NOT GFLAGS_INCLUDE_PATH) OR (NOT GFLAGS_LIBRARY))
    message(FATAL_ERROR "Fail to find gflags")
endif()
include_directories(${GFLAGS_INCLUDE_PATH})

execute_process(
        COMMAND bash -c "grep \"namespace [_A-Za-z0-9]\\+ {\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $2}' | tr -d '\n'"
        OUTPUT_VARIABLE GFLAGS_NS
)
if(${GFLAGS_NS} STREQUAL "GFLAGS_NAMESPACE")
    execute_process(
            COMMAND bash -c "grep \"#define GFLAGS_NAMESPACE [_A-Za-z0-9]\\+\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $3}' | tr -d '\n'"
            OUTPUT_VARIABLE GFLAGS_NS
    )
endif()


set(CMAKE_CPP_FLAGS "${DEFINE_CLOCK_GETTIME} -DGFLAGS_NS=${GFLAGS_NS}")
# gflags end

ADD_SUBDIRECTORY(rate_limiter)

AUX_SOURCE_DIRECTORY(. SRC_LIST)

list(REMOVE_ITEM SRC_LIST "./test.cpp")     # 从SRC_LIST中删除"test.cpp"，不然会出现“多次定义main”的error

ADD_EXECUTABLE(TinyEdgePlayer ${SRC_LIST})
TARGET_LINK_LIBRARIES(TinyEdgePlayer pthread glog gflags rate)

ADD_EXECUTABLE(tmp test.cpp)

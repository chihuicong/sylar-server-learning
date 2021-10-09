CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(Sylar C CXX)

#include(cmake/utils.cmake)

SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
    sylar/log.cc
    sylar/config.cc
    sylar/util.cc
    sylar/thread.cc
    sylar/fiber.cc
)

ADD_LIBRARY(sylar SHARED ${LIB_SRC})
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/sylar/)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/tests/)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/thirdpart/head/yaml-cpp/)

INCLUDE_DIRECTORIES(${LINK_DIRECTORIES}/thirdpart/lib/yaml-cpp/)

ADD_EXECUTABLE(test01 tests/test.cc)
ADD_DEPENDENCIES(test01 sylar)
TARGET_LINK_LIBRARIES(test01 sylar yaml-cpp pthread)

ADD_EXECUTABLE(test02 tests/test_thread.cc)
ADD_DEPENDENCIES(test02 sylar)
TARGET_LINK_LIBRARIES(test02 sylar yaml-cpp pthread)

ADD_EXECUTABLE(test03 tests/test_util.cc)
ADD_DEPENDENCIES(test03 sylar)
TARGET_LINK_LIBRARIES(test03 sylar yaml-cpp pthread)

ADD_EXECUTABLE(testFiber tests/test_fiber.cc)
ADD_DEPENDENCIES(testFiber sylar)
TARGET_LINK_LIBRARIES(testFiber sylar yaml-cpp pthread)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(${PACKAGE_NAME} VERSION 1.0.3 LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-write-strings -Wno-deprecated-declarations -DUSE_PIPE_BUFFER=1 -Wfatal-errors -g0 -Os")

file(GLOB_RECURSE build_SOURCES
	"*.cpp"
)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

include_directories(${CMAKE_INSTALL_PREFIX}/include)

add_library(${PACKAGE_NAME} SHARED ${build_SOURCES})

target_link_libraries(${PACKAGE_NAME}
	lzUtils
)

install(TARGETS cppUtils DESTINATION "lib")
install(DIRECTORY "./"
	DESTINATION "include/cppUtils"
	FILES_MATCHING PATTERN "*.h"
)
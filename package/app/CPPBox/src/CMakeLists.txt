cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(${PACKAGE_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wfatal-errors -g0 -Os")

file(GLOB_RECURSE build_SOURCES
	"MTQueue/*.cpp"
	"RtPlayer/*.cpp"
	"RePlayer/*.cpp"
	"main.cpp"
)
link_libraries(
	lzUtils
	cppUtils
	pthread
)
include_directories(${CMAKE_INSTALL_PREFIX}/include)
link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_executable(${PACKAGE_NAME} ${build_SOURCES})

install(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "bin")
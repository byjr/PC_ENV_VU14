cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(${PACKAGE_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wfatal-errors")

file(GLOB_RECURSE build_SOURCES
	"*.c"
)

include_directories(${CMAKE_INSTALL_PREFIX}/include)
link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_executable(${PACKAGE_NAME} ${build_SOURCES})

target_link_libraries(${PACKAGE_NAME}
	curl
	lzUtils
)

install(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "bin")
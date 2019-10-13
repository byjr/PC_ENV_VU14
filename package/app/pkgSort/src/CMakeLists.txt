cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(${PACKAGE_NAME} CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

file(GLOB_RECURSE build_SOURCES
	main.cpp
	# test.cpp
)

include_directories(${CMAKE_INSTALL_PREFIX}/include)
link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_executable(${PACKAGE_NAME} ${build_SOURCES})



target_link_libraries(${PACKAGE_NAME}
	lzUtils
)

install(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "bin")
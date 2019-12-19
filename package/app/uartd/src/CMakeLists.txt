cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(${PACKAGE_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-write-strings -Wno-deprecated-declarations -DUSE_PIPE_BUFFER=1 -Wfatal-errors -g0 -Os")

file(GLOB_RECURSE build_SOURCES
	"*.cpp"
)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_executable(${PACKAGE_NAME} ${build_SOURCES})

target_include_directories(${PACKAGE_NAME} PUBLIC
	${CMAKE_INSTALL_PREFIX}/include
)

target_link_libraries(${PACKAGE_NAME}
	lzUtils
	pthread
)

install(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "bin")
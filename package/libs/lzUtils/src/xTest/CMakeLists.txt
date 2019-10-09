cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(xTest)

file(GLOB_RECURSE xTest_SOURCES
	"*.c"
)

link_directories(${BUILD_DIR}/lzUtils)

add_executable(xTest ${xTest_SOURCES})

target_include_directories(xTest PUBLIC
	"../lzUtils"
)

target_link_libraries(xTest
	asound
	lzUtils
)

install(TARGETS xTest RUNTIME DESTINATION "bin")
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(${PACKAGE_NAME})

file(GLOB_RECURSE build_SOURCES
	"*.c"
)

add_library(${PACKAGE_NAME} SHARED ${build_SOURCES})

target_include_directories(${PACKAGE_NAME} PUBLIC
	"."
)

target_link_libraries(${PACKAGE_NAME}
	asound
)

install(TARGETS ${PACKAGE_NAME} LIBRARY DESTINATION "lib")

install(DIRECTORY "./"
	DESTINATION "include/${PACKAGE_NAME}"
	FILES_MATCHING PATTERN "*.h"
)
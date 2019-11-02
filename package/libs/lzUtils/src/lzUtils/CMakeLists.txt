cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(${PACKAGE_NAME})
if(ALSA_SRC_ENABLE STREQUAL "ON")
	add_definitions(ALSA_MOUDLE_SRC="alsa_ctrl/*.c")
	add_definitions(ALSA_MOUDLE_LIBS="asound")
endif()
file(GLOB_RECURSE build_SOURCES
	${ALSA_MOUDLE_SRC}
	autom/*.c
	ccsrb/*.c
	common/*.c
	csrb/*.c
	shrb/*.c
	slog/*.c
	trd_timer/*.c
	un_fifo_ops/*.c
	user_fifo/*.c
	user_timer/*.c
	ut_socket/*.c
	vad_op/*.c
)

add_library(${PACKAGE_NAME} SHARED ${build_SOURCES})

target_include_directories(${PACKAGE_NAME} PUBLIC
	"."
)

target_link_libraries(${PACKAGE_NAME}
	${ALSA_MOUDLE_LIBS}
	pthread
	rt
)

install(TARGETS ${PACKAGE_NAME} LIBRARY DESTINATION "lib")

install(DIRECTORY "./"
	DESTINATION "include/${PACKAGE_NAME}"
	FILES_MATCHING PATTERN "*.h"
)
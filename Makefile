TOP_DIR			:=$(shell pwd)
BUILD_ROOT		:=$(TOP_DIR)/build
STAGING_ROOT	:=$(TOP_DIR)/staging

export TOP_DIR BUILD_ROOT STAGING_ROOT

PACKAGE=$(patsubst %/,%,$(pkg))
PACKAGE_PATH		:= $(TOP_DIR)/$(PACKAGE)
PACKAGE_NAME 		:= $(notdir $(PACKAGE_PATH))

ifeq ($(showLog),y)
	LOG_PATH=/dev/tty
else
	LOG_PATH=$(BUILD_ROOT)/packageLogs/$(PACKAGE_NAME).log
endif

ifeq ($(pkg),)
all:
	@echo "	ERR:not special any package,please input package!"
	@echo "	eg:make pkg=package/libs/lzUtils showLog=y cmd=rebuild"
else
all:
	@mkdir -p $(BUILD_ROOT)/packageLogs
	@make -C package PACKAGE_PATH=$(PACKAGE_PATH) COMMNAD=$(cmd) > $(LOG_PATH) 2>&1
endif

	
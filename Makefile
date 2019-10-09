TOP_DIR			:=$(shell pwd)
BUILD_ROOT		:=$(TOP_DIR)/build
STAGING_ROOT	:=$(TOP_DIR)/staging

PACKAGE=$(patsubst %/,%,$(pkg))
COMMNAD=$(cmd)

PACKAGE_PATH		:= $(TOP_DIR)/$(PACKAGE)
PACKAGE_NAME 		:= $(notdir $(PACKAGE_PATH))
PACKAGE_DEPS_PATH	:=$(BUILD_ROOT)/packageDeps/$(PACKAGE_NAME).deps

export TOP_DIR BUILD_ROOT STAGING_ROOT PACKAGE_PATH PACKAGE_NAME PACKAGE_DEPS_PATH


LOG_PATH=$(BUILD_ROOT)/packageLogs/$(PACKAGE_NAME).log

ifeq ($(showLog),y)
all:
	@make -C $(PACKAGE) $(COMMNAD)
else
all:
	@mkdir -p $(BUILD_ROOT)/packageLogs
	@echo ---building $(PACKAGE) $(COMMNAD) started ...
	@make -C $(PACKAGE) $(COMMNAD) > $(LOG_PATH) 2>&1
	@echo ---building $(PACKAGE) $(COMMNAD) finished !
endif
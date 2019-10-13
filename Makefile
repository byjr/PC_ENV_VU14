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
COMMNAD :=$(cmd)
ifeq ($(COMMNAD),)
	SHOW_COMMAND=install
else
	SHOW_COMMAND=$(COMMNAD)
endif

define show_tip
	@echo xxx:[$(SHOW_COMMAND) $(1)]
endef

define make_one_pkg	
	$(call show_tip,$(1) start ...)
	@make -C $(1) $(COMMNAD) PACKAGE_PATH=$(TOP_DIR)/$(1) >> $(LOG_PATH) 2>&1
	$(call show_tip,$(1) finished)	
endef

ifeq ($(PACKAGE),)
ACTIVE_PKG_LIST=$(shell pkgSort -f .pkg_config)
else
ACTIVE_PKG_LIST=$(shell pkgSort -s $(PACKAGE))
endif
make_one_pkg_func = make_one_pkg


all:
	@mkdir -p $(BUILD_ROOT)/packageLogs
	$(foreach i,$(ACTIVE_PKG_LIST),$(call make_one_pkg,$(i)))

	
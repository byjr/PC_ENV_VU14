Hgray			:= '\033[1;30m'
Lgray			:= '\033[0;30m'
Hred			:= '\033[1;31m'
Lred			:= '\033[0;31m'
Hgreen			:= '\033[1;32m'
Lgreen			:= '\033[0;32m'
Hyellow			:= '\033[1;33m'
Lyellow			:= '\033[0;33m'
Hblue			:= '\033[1;34m'
Lblue			:= '\033[0;34m'
Hpurple			:= '\033[1;35m'
Lpurple			:= '\033[0;35m'
Hindigo			:= '\033[1;36m'
Lindigo			:= '\033[0;36m'
Hwhite			:= '\033[1;37m'
Lwhite			:= '\033[0;37m'
ColEnd			:= '\033[0m'
Execs			:= '>>>'

define new_line


endef

BUILD_ROOT		:=$(TOP_DIR)/build
STAGING_ROOT	:=$(TOP_DIR)/staging

export TOP_DIR BUILD_ROOT STAGING_ROOT

PACKAGE=$(patsubst %/,%,$(pkg))
PACKAGE_PATH		:= $(TOP_DIR)/$(PACKAGE)
PACKAGE_NAME 		:= $(notdir $(PACKAGE_PATH))

ifeq ($(V),s)
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

ifeq ($(PACKAGE),)
ACTIVE_PKG_LIST=$(shell pkgSort -f .pkg_config)
else
ACTIVE_PKG_LIST=$(shell pkgSort -s $(PACKAGE))
endif
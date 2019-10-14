TOP_DIR			:=$(shell pwd)
include $(TOP_DIR)/include/top_level.mk

define show_tip_srart
	@echo $(Lindigo)$(Execs)[$(SHOW_COMMAND) $(Hindigo)$(1) $(Lindigo)start ...]$(ColEnd)
endef

define show_tip_end
	@echo $(Lgreen)$(Execs)[$(SHOW_COMMAND) $(1) finished]$(ColEnd)
endef

define make_one_pkg	
	$(call show_tip_srart,$(1))
	@make -C $(1) $(COMMNAD) PACKAGE_PATH=$(TOP_DIR)/$(1) >> $(LOG_PATH) 2>&1 
	$(call show_tip_end,$(1))
endef

active_pkg_list:=$(ACTIVE_PKG_LIST)

all:
	@mkdir -p $(BUILD_ROOT)/packageLogs
	$(foreach i,$(active_pkg_list),$(call make_one_pkg,$(i)) $(new_line))

	
TOOL_BIN_PATH 		:=$(STAGING_ROOT)/usr/bin/$(notdir $(shell pwd))
ifneq ($(TOOL_BIN_PATH),$(wildcard $(TOOL_BIN_PATH)))
all:
	g++ -std=c++11 -o $(STAGING_ROOT)/usr/bin/pkgSort main.cpp	
else
all:
endif
	@echo $(TOOL_BIN_PATH) installed.


	
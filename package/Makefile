ifeq ($(COMMNAD),)
	SHOW_COMMAND=install
else
	SHOW_COMMAND=$(COMMNAD)
endif

define show_tip
	@echo [$(SHOW_COMMAND) $(1)] >> /dev/tty
endef

define make_one_pkg
	$(call show_tip,$(1) start ...)
	make -C $(TOP_DIR)/$(1) $(COMMNAD) PACKAGE_PATH=$(TOP_DIR)/$(1)
	$(call show_tip,$(1) finished)
endef

PACKAGE=$(patsubst $(TOP_DIR)/%,%,$(PACKAGE_PATH))

ifeq ($(COMMNAD),sclean)
all:
	make -C $(PACKAGE_PATH) clean
else ifeq ($(COMMNAD),build)
all:
	$(call show_tip,$(PACKAGE) start ...)
	make -C $(PACKAGE_PATH) $(COMMNAD)
	$(call show_tip,$(PACKAGE) finished)
else
all:
	$(call show_tip,$(PACKAGE) start ...)
	make -C $(PACKAGE_PATH) $(COMMNAD)
	$(call show_tip,$(PACKAGE) finished)
endif	
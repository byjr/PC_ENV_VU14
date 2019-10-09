
PACKAGE_DEPS_PATH	=$(BUILD_ROOT)/packageDeps/$(PACKAGE_NAME).deps
PACKAGE_DEPS_LIST	=$(shell cat $(PACKAGE_DEPS_PATH))

define build_cur_pkg
	@make -C $(1) depend
ifeq ($(PACKAGE_DEPS_LIST),"")	
else
	build_every_dep $(PACKAGE_DEPS_LIST)
endif
endef

define build_every_dep 
	@echo $(foreach(pkg_path,$(1),build_cur_pkg $(pkg_path))
endef

all:
	build_cur_pkg $(PACKAGE_PATH)


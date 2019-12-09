PACKAGE_VER		:= v1.0.0

ifneq ($(TOP_DIR),)
include $(TOP_DIR)/include/package.mk
endif

PACKAGE_DEPS_LIST	:= 

default:install

depend:
	@echo $(PACKAGE_DEPS_LIST)

build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) ;\
	cmake $(PACKAGE_PATH)/src -DCMAKE_INSTALL_PREFIX=$(STAGING_DIR) \
			-DCMAKE_OPTIONS=$(CMAKE_OPTIONS) && make ;\
	cd -
	
clean:
	rm -rf $(BUILD_DIR)
	
install:build
	make -C $(BUILD_DIR) install
	ln -sf ffbox $(STAGING_DIR)/bin/deMuxdec
	ln -sf ffbox $(STAGING_DIR)/bin/muxEnc
	
uninstall:
	rm -rf $(STAGING_DIR)/bin/$(TARGET)
	
rebuild:clean build

reinst:uninstall install

.PHONY :default depend build clean install rebuild reinst
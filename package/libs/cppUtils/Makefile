PACKAGE_VER		:= v1.0.0

ifneq ($(TOP_DIR),)
include $(TOP_DIR)/include/package.mk
endif

PACKAGE_DEPS_LIST	:= 

default:install

depend:
	@echo $(PACKAGE_DEPS_LIST)

CMAKE_OPTIONS+=-DALSA_SRC_ENABLE=OFF
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
	
uninstall: 
	rm -rf $(STAGING_DIR)/include/$(TARGET)
	rm -rf $(STAGING_DIR)/lib/lib$(TARGET).so*
	
rebuild:clean build

reinst:uninstall install

.PHONY :default depend build clean install rebuild reinst
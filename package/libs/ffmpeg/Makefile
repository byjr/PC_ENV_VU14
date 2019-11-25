PACKAGE_VER		:= 3.3.2

ifneq ($(TOP_DIR),)
include $(TOP_DIR)/include/package.mk
endif

PACKAGE_DEPS_LIST	:= 

default:install

depend:
	@echo $(PACKAGE_DEPS_LIST)

build: 
	tar xf $(notdir $(BUILD_DIR)).tar.gz -C $(BUILD_ROOT)
	cd $(BUILD_DIR);\
	./configure;\
	make -j8;\
	cd -
	
clean:
	rm -rf $(BUILD_DIR)
	
install:build
	make -C $(BUILD_DIR) install
	
uninstall: 
	rm -rf $(STAGING_DIR)/include/$(TARGET)
	rm -rf $(STAGING_DIR)/lib/lib$(TARGET).so*
	rm -rf $(STAGING_DIR)/bin/xTest
	
rebuild:clean build

reinst:uninstall install

.PHONY :default depend build clean install rebuild reinst
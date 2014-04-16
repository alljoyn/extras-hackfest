include $(TOPDIR)/rules.mk

PKG_NAME:=joystick
PKG_RELEASE:=1
PKG_VERSION:=$(shell git log -n 1 --pretty='format:%ci' | cut -d ' ' -f 1)

HF_PKG_SOURCE_DIR:=../../linino/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  TITLE:=Joystick library
  SECTION:=opt
  CATEGORY:=AJ-Tutorial
  DEPENDS:=+libstdcpp +smsg
endef

define Package/$(PKG_NAME)/description
Joystick shield communication abstraction for Arduino Yun.
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(TAR) c -C $(HF_PKG_SOURCE_DIR) . \
		--exclude=jstest \
		--exclude='.git*' \
		--exclude='*.os' \
		--exclude='*.o' \
		--exclude='*.so' \
	| tar x -C $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef


define Build/Compile
	TARGET_PATH="$(PATH)" \
	scons -C $(PKG_BUILD_DIR)
endef


define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DIR) $(1)/usr/include/aj_tutorial
	$(CP) $(PKG_BUILD_DIR)/inc/aj_tutorial/*.h $(1)/usr/include/aj_tutorial
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libjoystick.so $(1)/usr/lib/

endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libjoystick.so $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/test/jstest $(1)/usr/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))


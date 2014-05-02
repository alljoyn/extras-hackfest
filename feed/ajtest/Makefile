include $(TOPDIR)/rules.mk

PKG_NAME:=ajtest
PKG_RELEASE:=1
PKG_VERSION:=$(shell git log -n 1 --pretty='format:%ci' | cut -d ' ' -f 1)

HF_PKG_SOURCE_DIR:=../../linino/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/Default
  SECTION:=opt
  CATEGORY:=AJ-Tutorial
endef

define Package/$(PKG_NAME)-display
  TITLE:=AllJoyn Display test program
  SECTION:=opt
  CATEGORY:=AJ-Tutorial
  DEPENDS:=+libstdcpp +display +alljoyn +libpthread
endef

define Package/$(PKG_NAME)-joystick
  TITLE:=AllJoyn Joystick test program
  SECTION:=opt
  CATEGORY:=AJ-Tutorial
  DEPENDS:=+libstdcpp +joystick +alljoyn +libpthread
endef


define Package/$(PKG_NAME)-display/description
AllJoyn Display test program
endef

define Package/$(PKG_NAME)-joystick/description
AllJoyn Joystick test program
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


define Package/$(PKG_NAME)-display/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ajdisptest $(1)/usr/bin
endef

define Package/$(PKG_NAME)-joystick/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ajjstest $(1)/usr/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)-display))
$(eval $(call BuildPackage,$(PKG_NAME)-joystick))


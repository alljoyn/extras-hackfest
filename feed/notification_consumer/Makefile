include $(TOPDIR)/rules.mk

PKG_NAME:=notification_consumer
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

HF_PKG_SOURCE_DIR:=../../aj_samples/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  TITLE:=AllJoyn Notification Consumer Example
  SECTION:=opt
  CATEGORY:=AJ-Tutorial
  DEPENDS:=+libstdcpp +smsg +alljoyn +alljoyn-about +alljoyn-notification +alljoyn-services_common
endef

define Package/$(PKG_NAME)/description
AllJoyn Notification Consumer Example. Prints out any AllJoyn Notification received.
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(TAR) c -C $(HF_PKG_SOURCE_DIR) . \
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
	scons -C $(PKG_BUILD_DIR) $(MAKE_FLAGS)
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/build/$(PKG_NAME) $(1)/usr/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))


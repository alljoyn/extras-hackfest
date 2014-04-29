include $(TOPDIR)/rules.mk

PKG_NAME:=myapp
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
#PKG_VERSION:=$(shell git log -n 1 --pretty='format:%ci' | cut -d ' ' -f 1)
#PKG_VERSION:=01.01

HF_PKG_SOURCE_DIR:=../../aj_samples/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  TITLE:=AllJoyn MyApp
  SECTION:=opt
  CATEGORY:=AJ-Tutorial
  DEPENDS:=+libstdcpp +smsg +alljoyn +alljoyn-about +alljoyn-notification +alljoyn-services_common +display +joystick

endef

define Package/$(PKG_NAME)/description
AllJoyn MyApp package
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
	out=$(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/bin
	find $(PKG_BUILD_DIR)/build/ -type f ! -name "*.*" | xargs $(INSTALL_BIN) -t $(1)/usr/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))


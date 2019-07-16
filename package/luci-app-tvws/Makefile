#
# Copyright (C) 2008-2014 The LuCI Team <luci@lists.subsignal.org>
#
# This is free software, licensed under the Apache License, Version 2.0 .
#

include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-tvws
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=3. Applications
  TITLE:= LuCI Support for TVWS tuning
  DEPENDS:=+luci +luci-lib-json +uhttpd-mod-lua +mixer
endef


define Package/$(PKG_NAME)/description
	TVWS web panel
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	mkdir -p $(PKG_BUILD_DIR)/etc
	mkdir -p $(PKG_BUILD_DIR)/usr/lib/lua/luci
	mkdir -p $(PKG_BUILD_DIR)/etc/config
	mkdir -p $(PKG_BUILD_DIR)/etc/init.d
	$(CP) ./root/etc/* $(PKG_BUILD_DIR)/etc
	$(CP) ./luasrc/* $(PKG_BUILD_DIR)/usr/lib/lua/luci
endef

define Build/Compile
	# NOOP
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/
	$(INSTALL_DIR) $(1)/usr/
	$(CP) $(PKG_BUILD_DIR)/etc/* $(1)/etc/

	#$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller/admin
	#$(INSTALL_DIR) $(1)/usr/lib/lua/luci/view/dragino
	
	#find $(PKG_BUILD_DIR)/usr/ -name '*.lua' -exec luac -s -o {} {} \;

	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./root/etc/init.d/tvws.init $(1)/etc/init.d/tvws

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./root/etc/config/tvws.config $(1)/etc/config/tvws

	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/
	$(CP) $(PKG_BUILD_DIR)/usr/lib/lua/luci/* $(1)/usr/lib/lua/luci/

endef

$(eval $(call BuildPackage,$(PKG_NAME)))

# call BuildPackage - OpenWrt buildroot signature

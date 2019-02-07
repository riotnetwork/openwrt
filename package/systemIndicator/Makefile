#
# This software is licensed under the Public Domain.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=systemIndicator
PKG_VERSION:=0.1
PKG_RELEASE:=1

# see https://spdx.org/licenses/
PKG_LICENSE:=CC0-1.0

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/systemIndicator
	SECTION:=utils
	# Should this package be selected by default?
	#DEFAULT:=y
	CATEGORY:=Utilities
	TITLE:=sqiuggle system control interface
	# Feature FOO also needs libsodium:
	MAINTAINER:= Foo Bar <foo@example.com>
	URL:=https://www.example.com
	SUBMENU:=LED Control
endef

define Package/systemIndicator/description
	Some example Programm called example2
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	#$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
# Nothing to do here for us.
# By default example2/src/Makefile will be used.
endef

define Build/Compile
	CFLAGS="$(TARGET_CFLAGS)" CPPFLAGS="$(TARGET_CPPFLAGS)" $(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS)
endef

define Package/systemIndicator/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/bin/

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/systemIndicator $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/systemIndicator $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/usr/bin/checknetwork $(1)/usr/bin/
endef

$(eval $(call BuildPackage,systemIndicator))

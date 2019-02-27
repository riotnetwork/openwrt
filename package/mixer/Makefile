#
# This software is licensed under the Public Domain.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mixer
PKG_VERSION:=0.1
PKG_RELEASE:=1

# see https://spdx.org/licenses/
PKG_LICENSE:=CC0-1.0

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/mixer
	SECTION:=utils
	# Should this package be selected by default?
	#DEFAULT:=y
	CATEGORY:=Utilities
	TITLE:=RF2071 Mixer interface application
	# Feature FOO also needs libsodium:
	MAINTAINER:= Daniel de Kock <daniel@riot.network>
	URL:=https://www.riot.network
	SUBMENU:=RF mixer
endef

define Package/mixer/description
	utillity to configure a SPI attached RF2071 Mixer
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	
endef

define Build/Configure
# Nothing to do here for us.
# By default example2/src/Makefile will be used.
endef

define Build/Compile
	CFLAGS="$(TARGET_CFLAGS)" CPPFLAGS="$(TARGET_CPPFLAGS)" $(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS)
endef

define Package/mixer/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mixer $(1)/usr/sbin/

	$(INSTALL_BIN) ./files/usr/sbin/UHFChan.sh $(1)/usr/sbin/tvws

	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/etc/init.d/mixer.init $(1)/etc/init.d/mixer

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/mixer.config $(1)/etc/config/mixer
endef

$(eval $(call BuildPackage,mixer))

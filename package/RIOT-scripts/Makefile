#
# Copyright (c) 2013 Arduino LLC. All right reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=RIOT-scripts
PKG_VERSION:=1.3
PKG_RELEASE:=2

include $(INCLUDE_DIR)/package.mk

define Package/RIOT-scripts
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=Custom RIOT scripts
  DEPENDS:=+kmod-loop +kmod-spi-gpio-custom +kmod-i2c-gpio-custom +gnupg +libiwinfo-lua +kmod-usb-acm +kmod-usb-ehci
endef

define Build/Compile
	# NOOP
endef

define Package/RIOT-scripts/install
	$(INSTALL_DIR) $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d

	$(INSTALL_BIN) ./files/usr/bin/blink-start $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/blink-stop $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/boot-complete-notify $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/update-file-available $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/run-sysupgrade $(1)/usr/bin/
#	$(INSTALL_BIN) ./files/usr/bin/wifi-live-or-reset $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/wifi-reset-and-reboot $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/reset-to-factory-anyway $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/pretty-wifi-info.lua $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/upgrade-all $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/extract-built-date $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/extract-built-date-from-sysupgrade-image $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/get-gpio-conf $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/u-boot-upgrade $(1)/usr/bin



	$(INSTALL_BIN) ./files/etc/init.d/rename-wifi-if-access-point $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/generate_new_gpg_key $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/enable-rngd $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/handle_wifi_reset $(1)/etc/init.d
#	$(INSTALL_BIN) ./files/etc/init.d/rngd-turn-off $(1)/etc/init.d

	$(INSTALL_BIN) ./files/etc/init.d/create-serial-bus-gpio $(1)/etc/init.d
#	$(INSTALL_BIN) ./files/etc/init.d/firstconfig $(1)/etc/init.d
#	$(INSTALL_BIN) ./files/etc/init.d/internet-check-service $(1)/etc/init.d
endef

$(eval $(call BuildPackage,RIOT-scripts))


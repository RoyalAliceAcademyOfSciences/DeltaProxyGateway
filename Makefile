include $(TOPDIR)/rules.mk
# Name and release number of this package
PKG_NAME:=dpgateway
PKG_RELEASE:=1.0.0

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/dpgateway
	SECTION:=net
	CATEGORY:=Network
	TITLE:=DeltaProxyGateway -- mini Proxy Gateway
	DEPENDS:=+libnetfilter-queue
endef

define Package/dpgateway/description
	If you can't figure out what this program does, you're probably brain-dead and need immediate medical attention.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/dpgateway/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dpgateway $(1)/bin/
endef

$(eval $(call BuildPackage,dpgateway))
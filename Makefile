include $(TOPDIR)/rules.mk
# Name and release number of this package
PKG_NAME:=DeltaProxyGateway
PKG_RELEASE:=1.0.0

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/DeltaProxyGateway
	SECTION:=net
	CATEGORY:=Network
	TITLE:=DeltaProxyGateway -- mini Proxy Gateway
	DEPENDS:=+libnetfilter-queue
endef

define Package/DeltaProxyGateway/description
	If you can't figure out what this program does, you're probably brain-dead and need immediate medical attention.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/DeltaProxyGateway/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dpGateway $(1)/bin/
endef

$(eval $(call BuildPackage,DeltaProxyGateway))
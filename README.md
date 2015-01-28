# DeltaProxyGateway
3.客户端编译要麻烦些，需要有编译openwrt的ipk知识，
自己先下载openwrt sdk的源码库，再更新package，保证有以下依赖的源码
libmnl libnfnetlink libnetfilter-queue 

pushd package
git clone https://github.com/RoyalAliceAcademyOfS ... ateway.git
popd

4.编写两个Makefile
第一个package/DeltaProxyGateway/src/Makefile 内容如下：

dpGateway: dpGateway.o
$(CC) $(LDFLAGS) dpGateway.o -o dpGateway -lnfnetlink -lnetfilter_queue
dpGateway.o: dpGateway.c
$(CC) $(CFLAGS) -c dpGateway.c
# remove object files and executable when user executes "make clean"
clean:
rm *.o dpGateway

第二个package/DeltaProxyGateway/Makefile 内容如下：
include $(TOPDIR)/rules.mk
# Name and release number of this package
PKG_NAME:=DeltaProxyGateway
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/DeltaProxyGateway
SECTION:=net
CATEGORY:=Network
TITLE:=DeltaProxyGateway -- mini Proxy Gateway
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


5.进入openwrt sdk主目录，选择要编译的程序和依赖：
执行：make menuconfig
选择：kmod-nfnetlink kmod-nfnetlink-queue libnfnetlink libnetfilter-queue libmnl DeltaProxyGateway
保存后，执行：make V=99 package/DeltaProxyGateway/compile

6.生成的ipk在 sdk/bin/芯片/packages 里面，下载到目标机器上安装即可
opkg install libmnl
Installing libmnl (1.0.1-1) to root...
Configuring libmnl.
opkg install libnetfilter-queue
Installing libnetfilter-queue (1.0.0-1) to root...
Installing libnfnetlink (1.0.0-2) to root...
Installing kmod-nfnetlink-queue (3.3.8-1) to root...
Installing kmod-nfnetlink (3.3.8-1) to root...
Configuring kmod-nfnetlink.
Configuring libnfnetlink.
Configuring kmod-nfnetlink-queue.
Configuring libnetfilter-queue.
opkg install DeltaProxyGateway
Installing DeltaProxyGateway to root...
Configuring DeltaProxyGateway.

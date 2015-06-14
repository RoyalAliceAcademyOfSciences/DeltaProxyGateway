# DeltaProxyGateway
客户端编译要麻烦些，需要有编译openwrt的ipk知识，自己先下载openwrt sdk的源码库，再更新package，保证有以下依赖的源码
```
libnetfilter-queue 
```
进入openwrt sdk主目录
```
  $ ./scripts/feeds update -a
  $ ./scripts/feeds install libnetfilter-queue
  $ pushd package
  $ git clone https://github.com/RoyalAliceAcademyOfSciences/DeltaProxyGateway.git
  $ popd
  $ make menuconfig
```  
选择：kmod-nfnetlink kmod-nfnetlink-queue libnfnetlink libnetfilter-queue libmnl DeltaProxyGateway

保存后，执行：
```
  $ make V=99 package/DeltaProxyGateway/compile
```
生成的ipk在 sdk/bin/芯片/packages 里面，下载到目标机器上安装即可
```
  $opkg install libmnl
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
```

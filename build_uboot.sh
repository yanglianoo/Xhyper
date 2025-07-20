#!/bin/bash

if [ -d "./u-boot" ]; then
    rm -rf ./u-boot
fi

git clone --depth=1 https://github.com/u-boot/u-boot.git

cd u-boot
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- qemu_arm64_defconfig

# 借助sed命令修改配置文件，把预启动命令从原本的 "usb start" 替换成 "bootm 0x40200000 - ${fdtcontroladdr}"。
# 这一修改的目的是让 U-Boot 启动时直接从内存地址0x40200000加载内核镜像。
# Set preboot
sed -i 's/CONFIG_PREBOOT="usb start"/CONFIG_PREBOOT="bootm 0x40200000 - ${fdtcontroladdr}"/g' "./.config"
make CROSS_COMPILE=aarch64-linux-gnu- -j4
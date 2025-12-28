#!/bin/bash
CPU="cortex-a72"
QCPU="cortex-a72"
QEMU="qemu-system-aarch64"
GIC_VERSION=3
MACHINE="virt,gic-version=$GIC_VERSION,virtualization=on"
NCPU=2
QEMUOPTS="-cpu $QCPU -machine $MACHINE -smp $NCPU -m 256M -nographic \
          -bios ./u-boot/u-boot.bin \
          -device loader,file=./build/X-Hyper_Uimage,addr=0x40200000,force-raw=on"

if [ $# -ne 0 ];then
    echo "debug mode ..."
    $QEMU $QEMUOPTS 
else
    echo "run mode ..."
    $QEMU $QEMUOPTS
fi
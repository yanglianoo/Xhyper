#!/bin/bash

load_addr=0x40200000
entry_point=0x40200000
output_name=X-Hyper_Uimage
image_name=X-Hyper

cd guest 
make clean
make 

cd ..
make 
cd build
# mkimage
mkimage -A arm64 -O linux -C none -a $load_addr -e $entry_point -d ${image_name} ${output_name}

# -A arm64：指定架构为 ARM64。
# -O linux：指定操作系统为 Linux。
# -C none：不压缩内核（若内核已压缩，可改为-C gzip）。
# -a $load_addr：设置加载地址为 0x40200000。
# -e $entry_point：设置入口点地址为 0x40200000。
# -d ${image_name}：输入文件为X-Hyper（编译生成的内核）。
# ${output_name}：输出 UImage 文件名为X-Hyper_Uimage。
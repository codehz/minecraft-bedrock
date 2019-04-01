#!/bin/bash
set -exuo pipefail
pacman -Syu --needed --noconfirm wget unzip gcc
wget https://minecraft.azureedge.net/bin-linux/bedrock-server-$CORE_VERSION.zip -O /tmp/bedrock.zip
unzip /tmp/bedrock.zip -d ./server
gcc -o -Os ./server/launcher /tmp/launcher.c -leditline -lpthread
strip ./server/launcher
/packager.sh ./server/bedrock_server .
/packager.sh ./server/launcher .
rm -rf data
mkdir -p data ./server/worlds
mv ./server/libCrypto.so ./usr/lib
echo '[]' >./server/ops.json
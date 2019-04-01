#!/bin/bash
set -exuo pipefail
pacman -Syu --needed --noconfirm base-devel wget unzip gcc git
(
  cd /tmp
  git clone https://github.com/troglobit/editline
  cd editline
  ./autogen.sh
  ./configure
  make
  make install
)
wget https://minecraft.azureedge.net/bin-linux/bedrock-server-$CORE_VERSION.zip -O /tmp/bedrock.zip
unzip /tmp/bedrock.zip -d ./server
gcc -Os -o ./server/launcher /tmp/launcher.c -leditline -lpthread
strip ./server/launcher
/packager.sh ./server/bedrock_server .
cp /usr{/local/lib/libeditline,/lib/libpthread}.so.* ./usr/lib
rm -rf data
mkdir -p data ./server/worlds
mv ./server/libCrypto.so ./usr/lib
echo '[]' >./server/ops.json
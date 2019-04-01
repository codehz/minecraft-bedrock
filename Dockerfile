FROM codehz/archlinux as builder

WORKDIR /data
ARG CORE_VERSION="1.10.0.7"
RUN pacman -Syu --needed --noconfirm wget unzip tcc
RUN wget https://minecraft.azureedge.net/bin-linux/bedrock-server-$CORE_VERSION.zip -O /tmp/bedrock.zip
RUN unzip /tmp/bedrock.zip -d ./server
ADD launcher.c /tmp/launcher.c
RUN tcc -o ./server/launcher /tmp/launcher.c
RUN /packager.sh ./server/bedrock_server . && rm -rf data && mkdir -p data ./server/worlds && echo '[]' > ./server/ops.json

FROM scratch

COPY --from=builder /data /
WORKDIR /server
CMD ["/server/launcher"]

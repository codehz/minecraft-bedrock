FROM codehz/archlinux as builder

WORKDIR /data
ARG CORE_VERSION="1.10.0.7"
ADD . /tmp
RUN /tmp/build.sh

FROM scratch

COPY --from=builder /data /
WORKDIR /server
ENV DISABLE_READLINE=1
CMD ["/server/launcher"]

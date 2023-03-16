#!/usr/bin/env bash

if [ "$EUID" -ne 0 ]; then
  echo "This command must run as root" >&2
  exit 1
fi

if [ -z "$TARGET" ]; then
  export TARGET=linux-amd64
fi

case "${TARGET}" in

  openwrt-*)
    curl -sSL https://github.com/finwo/pmlag/releases/download/edge/pmlag-${TARGET}.tar.gz -o - | \
      gzip -d | \
      tar xv --strip-components=1 --directory=/
    ;;

  linux-*)
    echo -n "Downloading tarball..."
    curl -sSL https://github.com/finwo/pmlag/releases/download/edge/pmlag-${TARGET}.tar.gz -o - | \
      gzip -d | \
      tar x
    echo "done"
    echo "Installing binaries..."
    cd pmlag-${TARGET}
    make install
    if [ -d /etc/init.d ]; then
      echo "Installing openrc service..."
      make svc_openrc
    fi
    cd ..
    rm -rf pmlag-${TARGET}
    ;;

  *)
    echo "Unknown target type '${TARGET}'" >&2
    exit 1
    ;;
esac

echo "Finished"

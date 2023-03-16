#!/usr/bin/env

TARGET=linux-amd64

echo -n "Downloading tarball..."
curl -sSL https://github.com/finwo/pmlag/releases/download/edge/pmlag-${TARGET}.tar.gz -o - \
  gzip -d \
  tar x
echo "done"

echo "Installing binaries..."
cd pmlag
make install

if [ -d /etc/init.d ]; then
  echo "Installing openrc service..."
  make svc_openrc
fi

echo "Finished"

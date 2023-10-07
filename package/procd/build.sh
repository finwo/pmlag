#!/usr/bin/env bash

set -ex

NAM=pmlag-procd-${TARGET}

cd $(dirname $0)/../../
mkdir -p ${NAM}/usr/bin
mkdir -p ${NAM}/usr/share/man/man1
mkdir -p ${NAM}/etc/init.d
install pmlag   ${NAM}/usr/bin
install pmlag.1 ${NAM}/usr/share/man/man1
cp -rT package/common/files ${NAM}/
cp -rT package/procd/files ${NAM}/
DESTDIR=/usr envsubst '${DESTDIR}' < package/procd/files/etc/init.d/pmlag > ${NAM}/etc/init.d/pmlag
tar c ${NAM} | gzip -9 > ${NAM}.tar.gz
rm -rf ${NAM}

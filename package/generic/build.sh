#!/usr/bin/env bash

set -ex

NAM=pmlag-generic-${TARGET}

cd $(dirname $0)/../../
mkdir -p ${NAM}
cp Makefile.pkg ${NAM}/Makefile
cp pmlag        ${NAM}/pmlag
cp pmlag.1      ${NAM}/pmlag.1
mkdir -p ${NAM}/svc
cp -rT package/common/files ${NAM}/svc/common
cp -rT package/openrc/files ${NAM}/svc/openrc
cp -rT package/procd/files  ${NAM}/svc/procd
tar c ${NAM} | gzip -9 > ${NAM}.tar.gz
rm -rf ${NAM}

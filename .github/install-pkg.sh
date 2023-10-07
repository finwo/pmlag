#!/usr/bin/env bash

install_cmd=""
os="unknown"

# Basic OS detection
if command -v apt-get &>/dev/null; then
  # Debian or it's derivatives
  install_cmd="apt-get install -yq"
  os="debian"
elif command -v apk &>/dev/null; then
  # Alpine
  install_cmd="apk add"
  os="alpine"
elif command -v xbps-install &>/dev/null; then
  # Void
  install_cmd="xbps-install -Sy"
  os="void"
fi

# Catch unkown OS
if [ "unknown" == "$os" ]; then
  echo "Unknown OS" >&2
  exit 1;
fi

# Catch alternative packages
declare -A pkg_alt
# pkg_alt["void_pandoc"]="panini"
if [ -z "${pkg_alt[${os}_${1}]}" ]; then
  pkg=$1
else
  pkg="${pkg_alt[${os}_${1}]}"
fi

${install_cmd} ${pkg}

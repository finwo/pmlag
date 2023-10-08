#!/usr/bin/env sh

install_cmd=""
os="unknown"

# Basic OS detection
if command -v apt-get &>/dev/null; then
  # Debian or it's derivatives
  install_cmd="sudo apt-get install -yq"
  os="debian"
elif command -v apk &>/dev/null; then
  # Alpine
  install_cmd="sudo apk add"
  os="alpine"
elif command -v xbps-install &>/dev/null; then
  # Void
  install_cmd="sudo xbps-install -Sy"
  os="void"
fi

# Catch unkown OS
if [ "unknown" == "$os" ]; then
  echo "Unknown OS" >&2
  exit 1;
fi

pkg=${1}
case "${os}_${1}" in
  # void_pandoc) pkg="panini" ;;
  *) # Intentionally empty
    ;;
esac

${install_cmd} ${pkg}

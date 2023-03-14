#!/usr/bin/env sh

while inotifywait --recursive --event modify src; do
  npm start
done

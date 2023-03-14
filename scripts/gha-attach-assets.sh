#!/usr/bin/env bash

pwd

      # - run: 'curl -L -X POST -H "Accept: application/vnd.github+json" -H "Authorization: Bearer ${GITHUB_TOKEN}" -H "X-GitHub-Api-Version: 2022-11-28" -H "Content-Type: application/octet-stream" https://uploads.github.com/repos/${GITHUB_REPOSITORY}/tags/${GITHUB_REF/refs\/tags\//}/assets?name=pmlag-linux-amd64.tar.gz --data-binary "@package/pmlag-linux-amd64.tar.gz"'

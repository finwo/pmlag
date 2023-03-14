#!/usr/bin/env bash

pwd

        # 'curl -L -X POST -H "Accept: application/vnd.github+json" -H "Authorization: Bearer ${GITHUB_TOKEN}" -H "X-GitHub-Api-Version: 2022-11-28" https://api.github.com/repos/${GITHUB_REPOSITORY}/releases --data "{\\"tag_name\\":\\${GITHUB_REF/refs\/tags\//}"\\"}"'

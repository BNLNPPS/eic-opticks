#!/usr/bin/env bash

set -euo pipefail

repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
compose_env="${repo_root}/.env.compose"
user_name=${USER:-$(id -un)}
checkout_name=$(basename -- "${repo_root}")

cp "${repo_root}/.devcontainer/.env.defaults" "${compose_env}"
printf '\nCOMPOSE_PROJECT_NAME=simphony-%s-%s\n' ${user_name} ${checkout_name} >> ${compose_env}
if [[ -f "${repo_root}/.env.local" ]]; then
  printf '\n' >> "${compose_env}"
  cat "${repo_root}/.env.local" >> "${compose_env}"
fi

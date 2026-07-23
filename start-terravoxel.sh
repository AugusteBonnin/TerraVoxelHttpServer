#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_PATH="${ROOT_DIR}/build/TerraVoxelHttpServer"
LOG_FILE="${ROOT_DIR}/logs/terravoxel.log"
CACHE_ROOT="${ROOT_DIR}/cache"
HTTP_PORT="${TERRAVOXEL_HTTP_PORT:-8080}"

mkdir -p "${ROOT_DIR}/logs" "${CACHE_ROOT}"

export TERRAVOXEL_HTTP_PORT="${HTTP_PORT}"
export TERRAVOXEL_CACHE_ROOT="${CACHE_ROOT}"

if [ -f "${ROOT_DIR}/.env" ]; then
  set -a
  # shellcheck disable=SC1091
  source "${ROOT_DIR}/.env"
  set +a
fi

if [ ! -x "${BIN_PATH}" ]; then
  echo "Binaire introuvable : ${BIN_PATH}" >&2
  echo "Compilez d'abord le projet avec cmake --build build" >&2
  exit 1
fi

echo "Démarrage TerraVoxelHttpServer sur le port ${HTTP_PORT}" >&2
exec >>"${LOG_FILE}" 2>&1
exec "${BIN_PATH}"

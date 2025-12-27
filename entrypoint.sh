#!/usr/bin/env bash
set -euo pipefail

# ThemisDB Docker Entrypoint Script
# Following PostgreSQL best practices for Docker containers

# Configuration paths
CONFIG_TEMPLATE="/etc/themis/config.json"
TARGET_CONFIG="${THEMIS_CONFIG_PATH:-/etc/themis/config.json}"
DATA_DIR="${THEMIS_DATA_DIR:-/var/lib/themisdb}"
ROCKSDB_PATH="${THEMIS_ROCKSDB_PATH:-${DATA_DIR}/data}"
VECTOR_INDEX_PATH="${THEMIS_VECTOR_INDEX_PATH:-${DATA_DIR}/vector_indexes}"
LOG_DIR="${THEMIS_LOG_DIR:-/var/log/themis}"

# Server configuration
PORT="${THEMIS_PORT:-18765}"
HOST="${THEMIS_HOST:-0.0.0.0}"
WORKER_THREADS="${THEMIS_WORKER_THREADS:-8}"

# Storage configuration
MEMTABLE_SIZE_MB="${THEMIS_MEMTABLE_SIZE_MB:-256}"
BLOCK_CACHE_SIZE_MB="${THEMIS_BLOCK_CACHE_SIZE_MB:-1024}"

# Feature flags
ENABLE_TRACING="${THEMIS_ENABLE_TRACING:-false}"
ENABLE_SEMANTIC_CACHE="${THEMIS_ENABLE_SEMANTIC_CACHE:-true}"
ENABLE_LLM_STORE="${THEMIS_ENABLE_LLM_STORE:-true}"
ENABLE_CDC="${THEMIS_ENABLE_CDC:-true}"
ENABLE_TIMESERIES="${THEMIS_ENABLE_TIMESERIES:-true}"

# Telemetry configuration
OTLP_ENDPOINT="${THEMIS_OTLP_ENDPOINT:-http://localhost:4318}"
SERVICE_NAME="${THEMIS_SERVICE_NAME:-themis-server}"

echo "[entrypoint] Starting ThemisDB..."
echo "[entrypoint] Data directory: ${DATA_DIR}"
echo "[entrypoint] Config path: ${TARGET_CONFIG}"
echo "[entrypoint] Server: ${HOST}:${PORT}"

# Ensure persistent directories exist with proper ownership
if ! mkdir -p "${DATA_DIR}" "${ROCKSDB_PATH}" "${VECTOR_INDEX_PATH}" "${LOG_DIR}"; then
  echo "[entrypoint] Error: Failed to create data directories" >&2
  exit 1
fi

if ! chmod 755 "${DATA_DIR}" "${ROCKSDB_PATH}" "${VECTOR_INDEX_PATH}" "${LOG_DIR}"; then
  echo "[entrypoint] Warning: Failed to set permissions on data directories" >&2
fi

# Set ownership to themis user if running as root
if [ "$(id -u)" = "0" ]; then
  if id -u themis >/dev/null 2>&1; then
    echo "[entrypoint] Setting ownership to themis user..."
    if ! chown -R themis:themis "${DATA_DIR}" "${LOG_DIR}" 2>/dev/null; then
      echo "[entrypoint] Warning: Failed to change ownership (may not be mounted with correct permissions)" >&2
    fi
  fi
fi

# Generate or update configuration file with environment variables
if [ ! -f "$TARGET_CONFIG" ] || [ "${THEMIS_FORCE_CONFIG_GENERATION:-false}" = "true" ]; then
  echo "[entrypoint] Generating configuration from environment variables..."
  mkdir -p "$(dirname "$TARGET_CONFIG")"
  
  # Create config using jq if available, otherwise use template
  if command -v jq >/dev/null 2>&1; then
    cat > "$TARGET_CONFIG" <<EOF
{
  "storage": {
    "rocksdb_path": "${ROCKSDB_PATH}",
    "memtable_size_mb": ${MEMTABLE_SIZE_MB},
    "block_cache_size_mb": ${BLOCK_CACHE_SIZE_MB},
    "enable_blobdb": true,
    "compression": {
      "default": "lz4",
      "bottommost": "zstd"
    }
  },
  "server": {
    "host": "${HOST}",
    "port": ${PORT},
    "worker_threads": ${WORKER_THREADS}
  },
  "vector_index": {
    "object_name": "documents",
    "dimension": 10,
    "metric": "COSINE",
    "engine": "hnsw",
    "hnsw_m": 16,
    "hnsw_ef_construction": 200,
    "ef_search": 64,
    "use_gpu": false,
    "auto_save": true,
    "save_path": "${VECTOR_INDEX_PATH}",
    "save_on_shutdown": true,
    "load_on_startup": true
  },
  "tracing": {
    "enabled": ${ENABLE_TRACING},
    "service_name": "${SERVICE_NAME}",
    "otlp_endpoint": "${OTLP_ENDPOINT}"
  },
  "features": {
    "semantic_cache": ${ENABLE_SEMANTIC_CACHE},
    "llm_store": ${ENABLE_LLM_STORE},
    "cdc": ${ENABLE_CDC},
    "timeseries": ${ENABLE_TIMESERIES}
  },
  "sse": {
    "max_events_per_second": 0
  }
}
EOF
    echo "[entrypoint] Configuration generated successfully"
  else
    echo "[entrypoint] jq not found; using minimal config" >&2
    cat > "$TARGET_CONFIG" <<EOF
{
  "server": {"host": "${HOST}", "port": ${PORT}, "worker_threads": ${WORKER_THREADS}},
  "storage": {"rocksdb_path": "${ROCKSDB_PATH}", "memtable_size_mb": ${MEMTABLE_SIZE_MB}, "block_cache_size_mb": ${BLOCK_CACHE_SIZE_MB}},
  "vector_index": {"save_path": "${VECTOR_INDEX_PATH}"}
}
EOF
  fi
fi

# Display configuration summary
echo "[entrypoint] Configuration summary:"
if command -v jq >/dev/null 2>&1; then
  cat "$TARGET_CONFIG" | jq -r '
    "  Server: \(.server.host):\(.server.port)",
    "  Workers: \(.server.worker_threads)",
    "  RocksDB: \(.storage.rocksdb_path)",
    "  Vectors: \(.vector_index.save_path // "not configured")"
  ' || true
else
  echo "  Port: ${PORT}"
  echo "  Data: ${ROCKSDB_PATH}"
fi

# Change to data directory
cd "${DATA_DIR}" || true

# Execute themis_server as themis user if running as root
if [ "$(id -u)" = "0" ]; then
  if command -v gosu >/dev/null 2>&1; then
    echo "[entrypoint] Switching to themis user (using gosu)..."
    exec gosu themis /usr/local/bin/themis_server --config "$TARGET_CONFIG" "$@"
  elif command -v runuser >/dev/null 2>&1 && id -u themis >/dev/null 2>&1; then
    echo "[entrypoint] Switching to themis user (using runuser)..."
    exec runuser -u themis -- /usr/local/bin/themis_server --config "$TARGET_CONFIG" "$@"
  elif command -v su >/dev/null 2>&1 && id -u themis >/dev/null 2>&1; then
    echo "[entrypoint] Switching to themis user (using su)..."
    exec su -s /bin/sh themis -c "exec /usr/local/bin/themis_server --config '$TARGET_CONFIG' \"\$@\"" -- "$@"
  else
    echo "[entrypoint] Warning: cannot drop privileges to 'themis' user. Running as root." >&2
    exec /usr/local/bin/themis_server --config "$TARGET_CONFIG" "$@"
  fi
else
  echo "[entrypoint] Running as user: $(id -un)"
  exec /usr/local/bin/themis_server --config "$TARGET_CONFIG" "$@"
fi

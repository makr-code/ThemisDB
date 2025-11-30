#!/usr/bin/env bash
set -euo pipefail

CONFIG_TEMPLATE="/usr/local/share/themis/config.qnap.json"
TARGET_CONFIG="${THEMIS_CONFIG_PATH:-/etc/themis/config.json}"
PORT="${THEMIS_PORT:-18765}"

# Ensure persistent directories exist with permissive ownership
mkdir -p /data /data/themis_server /data/vector_indexes /var/log/themis || true
chmod 0775 /data /data/themis_server /data/vector_indexes /var/log/themis || true

# Try to set ownership to the non-root runtime user (uid 999)
if id -u themis >/dev/null 2>&1; then
  chown -R themis:root /data /data/themis_server /data/vector_indexes /var/log/themis 2>/dev/null || true
fi

# Ensure we run from data directory so relative paths map to /data
mkdir -p /data
chown -R themis:root /data 2>/dev/null || true
cd /data || true

# If no config is mounted, seed from template
if [ ! -f "$TARGET_CONFIG" ]; then
  echo "[entrypoint] No config at $TARGET_CONFIG, seeding from template"
  mkdir -p "$(dirname "$TARGET_CONFIG")"
  if [ -f "$CONFIG_TEMPLATE" ]; then
    cp "$CONFIG_TEMPLATE" "$TARGET_CONFIG"
  else
    echo "[entrypoint] Warning: config template $CONFIG_TEMPLATE not found; creating minimal config" >&2
    echo '{"server": {"port": 18765}, "storage": {"rocksdb_path": "/data/themis_server"}}' > "$TARGET_CONFIG"
  fi
fi

# Rewrite JSON config to reflect chosen port and data paths (if jq available)
TMP_CFG="${TARGET_CONFIG}.tmp"
if command -v jq >/dev/null 2>&1; then
  jq \
    --argjson port "$PORT" \
    '.server.port = ($port|tonumber)
     | .storage.rocksdb_path = "/data/themis_server"
     | .vector_index.save_path = "/data/vector_indexes"' \
    "$TARGET_CONFIG" > "$TMP_CFG" && mv -f "$TMP_CFG" "$TARGET_CONFIG"
else
  echo "[entrypoint] jq not found; skipping config rewrite" >&2
fi

echo "[entrypoint] Using config: $TARGET_CONFIG"
if command -v jq >/dev/null 2>&1; then
  cat "$TARGET_CONFIG" | jq '.server, .storage.rocksdb_path, .vector_index.save_path' || true
else
  echo "[entrypoint] (no jq) --- begin config ---"
  sed -n '1,120p' "$TARGET_CONFIG" || true
  echo "[entrypoint] (no jq) --- end config ---"
fi

# Try to drop to non-root `themis` user if possible. Prefer `runuser` or `su`.
if command -v runuser >/dev/null 2>&1 && id -u themis >/dev/null 2>&1; then
  exec runuser -u themis -- /usr/local/bin/themis_server --config "$TARGET_CONFIG"
elif command -v su >/dev/null 2>&1 && id -u themis >/dev/null 2>&1; then
  exec su -s /bin/sh themis -c "/usr/local/bin/themis_server --config '$TARGET_CONFIG'"
else
  echo "[entrypoint] Warning: cannot drop privileges to 'themis' (no runuser/su or user missing). Running as current user." >&2
  exec /usr/local/bin/themis_server --config "$TARGET_CONFIG"
fi

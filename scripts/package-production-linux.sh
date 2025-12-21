#!/bin/bash
# Linux Package Builder (Production/Dev) for ThemisDB v1.3.0
set -e

VERSION="1.3.0"
DEV=false
INCLUDE_TESTS=""
INCLUDE_BENCHMARKS=""
BUILD_DIR="/mnt/c/VCC/themis/build-wsl"
RELEASE_DIR="/mnt/c/VCC/themis/release"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --version)
      VERSION="$2"; shift 2 ;;
    --dev)
      DEV=true; shift ;;
    --include-tests)
      INCLUDE_TESTS=true; shift ;;
    --include-benchmarks)
      INCLUDE_BENCHMARKS=true; shift ;;
    *)
      echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

if [[ -z "$INCLUDE_TESTS" ]]; then INCLUDE_TESTS=$DEV; fi
if [[ -z "$INCLUDE_BENCHMARKS" ]]; then INCLUDE_BENCHMARKS=$DEV; fi

SUFFIX=$([[ $DEV == true ]] && echo "dev" || echo "production")
STAGE="themisdb-${VERSION}-linux-${SUFFIX}"

echo "═══════════════════════════════════════════════════════════════"
echo "  ThemisDB v${VERSION} - Linux ${SUFFIX^} Package"
echo "═══════════════════════════════════════════════════════════════"

cd "$BUILD_DIR"

# Clean
rm -rf "$STAGE"

# Create directory structure
echo ""
echo "[1/6] Erstelle Verzeichnisstruktur..."
mkdir -p "$STAGE"/{bin,config/processors,data/{db,wal,backups},docs/api,examples/api,logs,models,plugins,scripts/{admin,monitoring},tests/performance,tools,tools/dev-libs,tools/dev-include}
echo "  + Directory structure created"

# Copy Binaries
echo ""
echo "[2/6] Kopiere Binaries..."
if [ -f "$BUILD_DIR/themis_server" ]; then
  cp "$BUILD_DIR/themis_server" "$STAGE/bin/"
  echo "  + themis_server"
else
  echo "  WARNING: themis_server not found!"
fi

# Copy shared libraries
find "$BUILD_DIR" -maxdepth 1 -name "lib*.so*" -exec cp {} "$STAGE/bin/" \; 2>/dev/null || true

# Optional: Tests & Benchmarks
if [[ "$INCLUDE_TESTS" == "true" ]]; then
  if [ -f "$BUILD_DIR/themis_tests" ]; then
    cp "$BUILD_DIR/themis_tests" "$STAGE/tests/"
    echo "  + themis_tests"
  fi
  find "$BUILD_DIR" -maxdepth 1 -type f -name "*test*.so*" -exec cp {} "$STAGE/tests/" \; 2>/dev/null || true

  # Bundle gtest/gmock headers and libs for dev usage
  VCPKG_LIB="$BUILD_DIR/vcpkg_installed/x64-linux/lib"
  VCPKG_INC="$BUILD_DIR/vcpkg_installed/x64-linux/include"
  for lib in libgtest.a libgmock.a libgtest_main.a libgmock_main.a; do
    if [ -f "$VCPKG_LIB/$lib" ]; then
      cp "$VCPKG_LIB/$lib" "$STAGE/tools/dev-libs/" 2>/dev/null || true
    fi
  done
  if [ -d "$VCPKG_INC/gtest" ]; then
    cp -r "$VCPKG_INC/gtest" "$STAGE/tools/dev-include/" 2>/dev/null || true
  fi
fi

if [[ "$INCLUDE_BENCHMARKS" == "true" ]]; then
  find "$BUILD_DIR" -maxdepth 1 -type f -name "bench_*" -exec cp {} "$STAGE/tests/performance/" \; 2>/dev/null || true
  if [ -f "$BUILD_DIR/themis_benchmarks" ]; then
    cp "$BUILD_DIR/themis_benchmarks" "$STAGE/tests/performance/"
    echo "  + themis_benchmarks"
  fi

  VCPKG_LIB="$BUILD_DIR/vcpkg_installed/x64-linux/lib"
  VCPKG_INC="$BUILD_DIR/vcpkg_installed/x64-linux/include"
  for lib in libbenchmark.a libbenchmark_main.a; do
    if [ -f "$VCPKG_LIB/$lib" ]; then
      cp "$VCPKG_LIB/$lib" "$STAGE/tools/dev-libs/" 2>/dev/null || true
    fi
  done
  if [ -d "$VCPKG_INC/benchmark" ]; then
    cp -r "$VCPKG_INC/benchmark" "$STAGE/tools/dev-include/" 2>/dev/null || true
  fi
fi

# Create Config Files
echo ""
echo "[3/6] Erstelle Konfigurationen..."
cat > "$STAGE/config/config.json" << 'EOF'
{
  "server": {
    "host": "0.0.0.0",
    "port": 8765,
    "threads": 4,
    "max_connections": 1000
  },
  "database": {
    "path": "./data/db",
    "wal_path": "./data/wal",
    "backup_path": "./data/backups"
  },
  "logging": {
    "level": "info",
    "file": "./logs/themis.log",
    "max_size_mb": 100,
    "max_files": 10
  }
}
EOF

cat > "$STAGE/config/llm_config.yaml" << 'EOF'
llm:
  enabled: true
  plugin: llamacpp
  model:
    path: ./models/mistral-7b-instruct-v0.2.Q4_K_M.gguf
    n_gpu_layers: 32
    n_ctx: 4096
EOF
echo "  + config.json, llm_config.yaml"

# Create Admin Scripts
echo ""
echo "[4/6] Erstelle Admin-Tools..."
cat > "$STAGE/scripts/admin/backup.sh" << 'EOF'
#!/bin/bash
BACKUP_DIR="./data/backups/backup-$(date +%Y%m%d-%H%M%S)"
echo "Creating backup to: $BACKUP_DIR"
mkdir -p "$BACKUP_DIR"
cp -r ./data/db/* "$BACKUP_DIR/"
echo "Backup completed!"
EOF

cat > "$STAGE/scripts/admin/restore.sh" << 'EOF'
#!/bin/bash
if [ -z "$1" ]; then
  echo "Usage: ./restore.sh <backup-path>"
  exit 1
fi
echo "Restoring from: $1"
systemctl stop themisdb 2>/dev/null || true
cp -r "$1"/* ./data/db/
systemctl start themisdb 2>/dev/null || true
echo "Restore completed!"
EOF

cat > "$STAGE/scripts/monitoring/health-check.sh" << 'EOF'
#!/bin/bash
STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8765/health)
if [ "$STATUS" -eq 200 ]; then
  echo "ThemisDB is healthy"
  exit 0
else
  echo "Health check failed"
  exit 1
fi
EOF

cat > "$STAGE/tools/themisdb.service" << 'EOF'
[Unit]
Description=ThemisDB Multi-Model Database
After=network.target

[Service]
Type=simple
User=themisdb
WorkingDirectory=/opt/themisdb
ExecStart=/opt/themisdb/bin/themis_server --config /opt/themisdb/config/config.json
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

chmod +x "$STAGE/scripts/admin/"*.sh "$STAGE/scripts/monitoring/"*.sh
echo "  + backup.sh, restore.sh, health-check.sh, systemd service"

# Copy Documentation
echo ""
echo "[5/6] Kopiere Dokumentation..."
cp /mnt/c/VCC/themis/{README.md,LICENSE,CHANGELOG.md} "$STAGE/docs/" 2>/dev/null || true

cat > "$STAGE/INSTALL.md" << 'EOF'
# ThemisDB v1.3.0 - Linux Production Installation

## Quick Start

```bash
tar -xzf themisdb-v1.3.0-linux-x64-production.tar.gz
cd themisdb-1.3.0-linux
./bin/themis_server --config config/config.json
```

## Systemd Installation

```bash
sudo cp -r themisdb-1.3.0-linux /opt/themisdb
sudo cp /opt/themisdb/tools/themisdb.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable themisdb
sudo systemctl start themisdb
```

## Directory Structure
bin/       - Executables
config/    - Configuration
data/      - Database storage
scripts/   - Admin scripts

## Admin Tools
./scripts/admin/backup.sh              # Backup
./scripts/admin/restore.sh <path>      # Restore
./scripts/monitoring/health-check.sh   # Health check
EOF
echo "  + Documentation copied"

# Create Tarball
echo ""
echo "[6/6] Erstelle Tarball..."
TAR_NAME="themisdb-v${VERSION}-linux-x64-${SUFFIX}.tar.gz"
tar -czf "$TAR_NAME" "$STAGE"
mv "$TAR_NAME" "$RELEASE_DIR/"

SIZE=$(du -h "$RELEASE_DIR/$TAR_NAME" | cut -f1)
echo "  + Package: $SIZE"

# Checksum
HASH=$(sha256sum "$RELEASE_DIR/$TAR_NAME" | awk '{print $1}')
SUM_FILE=$([[ $DEV == true ]] && echo "SHA256SUMS_dev.txt" || echo "SHA256SUMS_production.txt")
echo "$HASH  $TAR_NAME" > "$RELEASE_DIR/$SUM_FILE"
echo "  + SHA256: ${HASH:0:16}..."

# Cleanup
rm -rf "$STAGE"

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  Linux Package Ready: $TAR_NAME"
echo "═══════════════════════════════════════════════════════════════"

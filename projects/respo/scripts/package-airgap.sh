#!/bin/bash
# Package RESPO for air-gapped deployment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="${1:-$PROJECT_DIR/dist}"

mkdir -p "$OUTPUT_DIR/images"

echo "[INFO] Building RESPO API image..."
cd "$PROJECT_DIR"
docker build -t respo-api:latest -f docker/Dockerfile.api .

echo "[INFO] Saving Docker images..."
docker save respo-api:latest | gzip > "$OUTPUT_DIR/images/respo-api.tar.gz"

echo "[INFO] Copying configuration..."
cp -r "$PROJECT_DIR/docker" "$OUTPUT_DIR/"
cp "$PROJECT_DIR/.env.example" "$OUTPUT_DIR/.env"
cp -r "$PROJECT_DIR/scripts" "$OUTPUT_DIR/"

echo "[INFO] Air-gapped package ready in: $OUTPUT_DIR"

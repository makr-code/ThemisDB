#!/bin/bash
# ============================================================================
# Stop ThemisDB LoRA Framework Docker Environment
# ============================================================================
# Usage:
#   ./stop.sh           # Stop all services
#   ./stop.sh --clean   # Stop and remove volumes
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCKER_DIR="$(dirname "$SCRIPT_DIR")"

CLEAN_VOLUMES="${1}"

echo "============================================"
echo "Stopping ThemisDB LoRA Framework"
echo "============================================"
echo ""

cd "$DOCKER_DIR"

# Stop services
echo "Stopping services..."
docker-compose -f docker-compose.yml -f docker-compose.dev.yml -f docker-compose.test.yml down

if [ "$CLEAN_VOLUMES" = "--clean" ]; then
  echo ""
  echo "Removing volumes..."
  docker-compose -f docker-compose.yml down -v
  echo "✓ Volumes removed"
fi

echo ""
echo "============================================"
echo "Services Stopped Successfully!"
echo "============================================"
echo ""

if [ "$CLEAN_VOLUMES" != "--clean" ]; then
  echo "Data volumes preserved."
  echo "To remove volumes, run: $0 --clean"
  echo ""
fi

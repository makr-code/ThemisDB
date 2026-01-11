#!/bin/bash
# ============================================================================
# Reset All Volumes
# ============================================================================
# WARNING: This will delete all data in Docker volumes
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCKER_DIR="$(dirname "$SCRIPT_DIR")"

echo "============================================"
echo "WARNING: Reset All Docker Volumes"
echo "============================================"
echo ""
echo "This will DELETE all data including:"
echo "  - ThemisDB data and logs"
echo "  - Prometheus metrics"
echo "  - Grafana dashboards and settings"
echo "  - vcpkg cache"
echo ""
read -p "Are you sure? (type 'yes' to confirm): " CONFIRM

if [ "$CONFIRM" != "yes" ]; then
  echo "Aborted."
  exit 0
fi

cd "$DOCKER_DIR"

echo ""
echo "Stopping all services..."
docker-compose -f docker-compose.yml -f docker-compose.dev.yml -f docker-compose.test.yml down

echo ""
echo "Removing volumes..."
docker-compose down -v

echo ""
echo "Removing orphaned volumes..."
docker volume prune -f

echo ""
echo "============================================"
echo "✓ All Volumes Reset Successfully!"
echo "============================================"
echo ""
echo "Start fresh environment with:"
echo "  ./scripts/start.sh"
echo ""

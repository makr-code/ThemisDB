#!/bin/bash
# ============================================================================
# Start ThemisDB LoRA Framework Docker Environment
# ============================================================================
# Usage:
#   ./start.sh          # Start in production mode
#   ./start.sh dev      # Start in development mode
#   ./start.sh test     # Start in test mode
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCKER_DIR="$(dirname "$SCRIPT_DIR")"

MODE="${1:-prod}"

echo "============================================"
echo "ThemisDB LoRA Framework - Docker Compose"
echo "============================================"
echo ""

cd "$DOCKER_DIR"

case "$MODE" in
  prod|production)
    echo "Starting in PRODUCTION mode..."
    docker-compose up -d
    ;;
  
  dev|development)
    echo "Starting in DEVELOPMENT mode..."
    docker-compose -f docker-compose.yml -f docker-compose.dev.yml up -d
    ;;
  
  test|testing)
    echo "Starting in TEST mode..."
    docker-compose -f docker-compose.yml -f docker-compose.test.yml up --abort-on-container-exit
    ;;
  
  *)
    echo "Error: Unknown mode '$MODE'"
    echo ""
    echo "Usage: $0 [prod|dev|test]"
    echo "  prod  - Production mode (default)"
    echo "  dev   - Development mode with hot-reload"
    echo "  test  - Test mode with automated tests"
    exit 1
    ;;
esac

if [ "$MODE" != "test" ]; then
  echo ""
  echo "============================================"
  echo "Services Started Successfully!"
  echo "============================================"
  echo ""
  echo "Access URLs:"
  echo "  ThemisDB:   http://localhost:8529"
  echo "  Prometheus: http://localhost:9091"
  echo "  Grafana:    http://localhost:3000 (admin/admin)"
  echo ""
  echo "View logs:"
  echo "  docker-compose logs -f"
  echo ""
  echo "Stop services:"
  echo "  ./scripts/stop.sh"
  echo ""
fi

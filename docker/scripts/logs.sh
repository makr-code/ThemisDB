#!/bin/bash
# ============================================================================
# View Service Logs
# ============================================================================
# Usage:
#   ./logs.sh              # View all logs
#   ./logs.sh themisdb     # View specific service logs
#   ./logs.sh -f           # Follow logs
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCKER_DIR="$(dirname "$SCRIPT_DIR")"

cd "$DOCKER_DIR"

if [ -z "$1" ]; then
  echo "Viewing all service logs..."
  docker-compose logs --tail=100
elif [ "$1" = "-f" ]; then
  echo "Following all service logs..."
  docker-compose logs -f
else
  echo "Viewing logs for service: $1"
  docker-compose logs --tail=100 -f "$1"
fi

#!/bin/bash
# RESPO Deployment Script
# Usage: ./deploy.sh [dev|prod|airgap]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

ENV="${1:-dev}"

echo "[INFO] Deploying RESPO in $ENV mode..."

case "$ENV" in
    dev)
        cd "$PROJECT_DIR/docker"
        docker compose up -d
        ;;
    prod)
        cd "$PROJECT_DIR/docker"
        docker compose -f docker-compose.yml -f docker-compose.prod.yml up -d
        ;;
    airgap)
        cd "$PROJECT_DIR/docker"
        docker compose -f docker-compose.yml -f docker-compose.airgap.yml up -d
        ;;
    *)
        echo "Usage: $0 [dev|prod|airgap]"
        exit 1
        ;;
esac

echo ""
echo "RESPO is starting!"
echo "  API:  http://localhost:8080"
echo "  Docs: http://localhost:8080/docs"

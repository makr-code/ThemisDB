#!/bin/bash
# ThemisDB Hyperscaler - Quick Docker Compose Startup
# Uses docker compose to start 10-instance sharding cluster

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=================================="
echo "ThemisDB Hyperscaler Cluster"
echo "10-Instance Sharding Test (RAID 1/2/5)"
echo "=================================="
echo ""

# Check if Docker Desktop is running
if ! docker ps > /dev/null 2>&1; then
    echo "ERROR: Docker Desktop is not running."
    echo "Please start Docker Desktop and try again."
    exit 1
fi

echo "✓ Docker Desktop is running"
echo ""

# Create required directories
echo "Setting up directories..."
mkdir -p "$PROJECT_DIR/docker/logs/shard-0"
mkdir -p "$PROJECT_DIR/docker/logs/shard-1"
mkdir -p "$PROJECT_DIR/docker/logs/shard-2"
mkdir -p "$PROJECT_DIR/docker/logs/shard-3"
mkdir -p "$PROJECT_DIR/docker/logs/shard-4"
mkdir -p "$PROJECT_DIR/docker/logs/shard-5"
mkdir -p "$PROJECT_DIR/docker/logs/shard-6"
mkdir -p "$PROJECT_DIR/docker/logs/shard-7"
mkdir -p "$PROJECT_DIR/docker/logs/shard-8"
mkdir -p "$PROJECT_DIR/docker/logs/shard-9"
mkdir -p "$PROJECT_DIR/docker/logs/nginx"
mkdir -p "$PROJECT_DIR/docker/nginx/ssl"
mkdir -p "$PROJECT_DIR/docker/grafana/provisioning"
mkdir -p "$PROJECT_DIR/docker/grafana/dashboards"

echo "✓ Directories created"
echo ""

# Build minimal Docker image (since full build failed)
echo "Creating Docker image (using lite version without complex builds)..."
docker build \
  -f "$PROJECT_DIR/docker/hyperscaler/Dockerfile.lite" \
  -t themisdb-hyperscaler:latest \
  --build-arg THEMIS_VERSION=1.3.4-hyperscaler \
  "$PROJECT_DIR"

echo "✓ Docker image created"
echo ""

# Start services
echo "Starting 10-instance cluster with docker-compose..."
cd "$PROJECT_DIR/docker/hyperscaler"

docker-compose -f docker-compose.hyperscaler-sharding.yml up -d

echo ""
echo "=================================="
echo "Cluster Started!"
echo "=================================="
echo ""
echo "Services running:"
echo "  RAID 1 Group (Shards 0-2):"
echo "    - Shard 0 (primary):   http://localhost:8080"
echo "    - Shard 1 (replica):   http://localhost:8081"
echo "    - Shard 2 (replica):   http://localhost:8082"
echo ""
echo "  RAID 2 Group (Shards 3-5):"
echo "    - Shard 3 (primary):   http://localhost:8083"
echo "    - Shard 4 (replica):   http://localhost:8084"
echo "    - Shard 5 (replica):   http://localhost:8085"
echo ""
echo "  RAID 5 Group (Shards 6-9):"
echo "    - Shard 6 (primary):   http://localhost:8086"
echo "    - Shard 7 (replica):   http://localhost:8087"
echo "    - Shard 8 (replica):   http://localhost:8088"
echo "    - Shard 9 (replica):   http://localhost:8089"
echo ""
echo "Load Balancer:"
echo "  - Nginx (all shards):   http://localhost"
echo "  - RAID 1 endpoint:      http://localhost/raid1/"
echo "  - RAID 2 endpoint:      http://localhost/raid2/"
echo "  - RAID 5 endpoint:      http://localhost/raid5/"
echo ""
echo "Monitoring:"
echo "  - Prometheus:           http://localhost:9090"
echo "  - Grafana:              http://localhost:3000 (admin/admin)"
echo ""
echo "Management commands:"
echo "  View logs:       docker-compose -f docker/hyperscaler/docker-compose.hyperscaler-sharding.yml logs -f [service]"
echo "  Stop cluster:    docker-compose -f docker/hyperscaler/docker-compose.hyperscaler-sharding.yml down"
echo "  Restart shard:   docker-compose -f docker/hyperscaler/docker-compose.hyperscaler-sharding.yml restart themis-shard-X"
echo "  View status:     docker-compose -f docker/hyperscaler/docker-compose.hyperscaler-sharding.yml ps"
echo ""

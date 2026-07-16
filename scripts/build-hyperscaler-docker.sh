#!/bin/bash
# ThemisDB Hyperscaler Edition - Docker Build & Sharding Test Setup Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "=================================="
echo "ThemisDB Hyperscaler Docker Build"
echo "=================================="
echo ""

# Build Docker image
echo "Building Docker image themisdb-hyperscaler:latest..."
docker build \
  -f docker/hyperscaler/Dockerfile \
  -t themisdb-hyperscaler:latest \
  --build-arg THEMIS_VERSION=1.3.4-hyperscaler \
  --build-arg ENABLE_LLM=ON \
  --build-arg LLAMA_GIT_REF=master \
  .

echo ""
echo "Docker image built successfully!"
echo ""

# Create required directories
echo "Creating required directories..."
mkdir -p docker/logs/{shard-{0..9},nginx}
mkdir -p docker/nginx/ssl
mkdir -p docker/grafana/{provisioning,dashboards}

echo ""
echo "=================================="
echo "Setup Complete!"
echo "=================================="
echo ""
echo "To start the 10-shard cluster:"
echo "  cd docker"
echo "  docker-compose -f docker/hyperscaler/docker-compose.hyperscaler-sharding.yml up -d"
echo ""
echo "Access points:"
echo "  - Load Balancer:    http://localhost"
echo "  - RAID 1 Group:     http://localhost/raid1/"
echo "  - RAID 2 Group:     http://localhost/raid2/"
echo "  - RAID 5 Group:     http://localhost/raid5/"
echo "  - Shard 0:          http://localhost:8080"
echo "  - Shard 1-9:        http://localhost:8081-8089"
echo "  - Prometheus:       http://localhost:9090"
echo "  - Grafana:          http://localhost:3000 (admin/admin)"
echo ""
echo "To view logs:"
echo "  docker-compose -f docker/hyperscaler/docker-compose.hyperscaler-sharding.yml logs -f [service]"
echo ""
echo "To stop the cluster:"
echo "  docker-compose -f docker/hyperscaler/docker-compose.hyperscaler-sharding.yml down"
echo ""

#!/usr/bin/env bash
# Quick rebuild script for QNAP deployment (Linux/macOS)
# Ensures compatibility with older GLIBC/GLIBCXX by using Ubuntu 20.04 base

set -e

TAG="${1:-latest}"
REGISTRY="${2:-}"

echo "==> Building ThemisDB for QNAP (x64-linux, Ubuntu 20.04 base)"

docker build \
    --build-arg VCPKG_TRIPLET=x64-linux \
    -t "themis:${TAG}" \
    -f Dockerfile \
    .

echo "==> Build successful: themis:${TAG}"

if [ -n "$REGISTRY" ]; then
    REMOTE_TAG="${REGISTRY}/themis:${TAG}"
    echo "==> Tagging for registry: ${REMOTE_TAG}"
    docker tag "themis:${TAG}" "${REMOTE_TAG}"
    
    echo "==> Pushing to registry..."
    docker push "${REMOTE_TAG}"
    echo "==> Push successful: ${REMOTE_TAG}"
fi

echo ""
echo "==> Next steps for QNAP deployment:"
echo "1. Save image: docker save themis:${TAG} | gzip > themis-${TAG}.tar.gz"
echo "2. Transfer to QNAP via SCP/SFTP"
echo "3. Load on QNAP: docker load < themis-${TAG}.tar.gz"
echo "4. Deploy with: docker-compose -f docker-compose.qnap.yml up -d"
echo ""
echo "See QNAP_DEPLOYMENT.md for detailed instructions."

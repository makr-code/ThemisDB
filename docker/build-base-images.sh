#!/bin/bash
# ============================================================================
# Build Base Images for ThemisDB
# ============================================================================
# Builds and pushes base images for faster builds
#
# Usage:
#   ./build-base-images.sh [REGISTRY] [PUSH]
#
# Examples:
#   ./build-base-images.sh themisdb           # Build only
#   ./build-base-images.sh themisdb push      # Build and push
#   ./build-base-images.sh ghcr.io/themisdb push

set -e

REGISTRY="${1:-themisdb}"
PUSH_FLAG="${2}"
VCPKG_VERSION="2024.10.21"
THEMIS_VERSION="v1.4.0"

echo "============================================"
echo "ThemisDB Base Images Builder"
echo "============================================"
echo "Registry:      ${REGISTRY}"
echo "vcpkg Version: ${VCPKG_VERSION}"
echo "ThemisDB Ver:  ${THEMIS_VERSION}"
echo "Push:          ${PUSH_FLAG:-no}"
echo "============================================"
echo ""

# Check buildx
if ! docker buildx version > /dev/null 2>&1; then
    echo "ERROR: docker buildx not found"
    exit 1
fi

# Create/use builder
if ! docker buildx inspect themis-base-builder > /dev/null 2>&1; then
    docker buildx create --name themis-base-builder --use
fi
docker buildx use themis-base-builder

# ============================================================================
# 1. Build vcpkg-base
# ============================================================================
echo ">>> Building vcpkg-base:${VCPKG_VERSION}"
docker buildx build \
    --platform linux/amd64,linux/arm64 \
    -f docker/Dockerfile.vcpkg-base \
    -t "${REGISTRY}/vcpkg-base:${VCPKG_VERSION}" \
    -t "${REGISTRY}/vcpkg-base:latest" \
    ${PUSH_FLAG:+--push} \
    ${PUSH_FLAG:---load} \
    .

echo "✅ vcpkg-base built"
echo ""

# ============================================================================
# 2. Build vcpkg-deps (all editions)
# ============================================================================
for edition in community enterprise hyperscaler; do
    echo ">>> Building vcpkg-deps:${edition}-${THEMIS_VERSION}"
    docker buildx build \
        --platform linux/amd64,linux/arm64 \
        --build-arg VCPKG_BASE_VERSION="${VCPKG_VERSION}" \
        --target "${edition}" \
        -f docker/Dockerfile.vcpkg-deps \
        -t "${REGISTRY}/vcpkg-deps:${edition}-${THEMIS_VERSION}" \
        -t "${REGISTRY}/vcpkg-deps:${edition}" \
        ${PUSH_FLAG:+--push} \
        ${PUSH_FLAG:---load} \
        .
    
    echo "✅ vcpkg-deps:${edition} built"
    echo ""
done

# ============================================================================
# 3. Build llama-base
# ============================================================================
echo ">>> Building llama-base:latest"
docker buildx build \
    --platform linux/amd64,linux/arm64 \
    --build-arg VCPKG_BASE_VERSION="${VCPKG_VERSION}" \
    --build-arg LLAMA_GIT_REF=master \
    -f docker/Dockerfile.llama-base \
    -t "${REGISTRY}/llama-base:latest" \
    -t "${REGISTRY}/llama-base:$(date +%Y%m%d)" \
    ${PUSH_FLAG:+--push} \
    ${PUSH_FLAG:---load} \
    .

echo "✅ llama-base built"
echo ""

# ============================================================================
# Summary
# ============================================================================
echo "============================================"
echo "✅ All base images built successfully!"
echo "============================================"
echo ""
echo "Base Images:"
echo "  ${REGISTRY}/vcpkg-base:${VCPKG_VERSION}"
echo "  ${REGISTRY}/vcpkg-deps:community-${THEMIS_VERSION}"
echo "  ${REGISTRY}/vcpkg-deps:enterprise-${THEMIS_VERSION}"
echo "  ${REGISTRY}/vcpkg-deps:hyperscaler-${THEMIS_VERSION}"
echo "  ${REGISTRY}/llama-base:latest"
echo ""

if [ "$PUSH_FLAG" = "push" ]; then
    echo "✅ Images pushed to registry"
else
    echo "ℹ️  Images built locally (not pushed)"
    echo "   To push: $0 ${REGISTRY} push"
fi

echo ""
echo "Next steps:"
echo "  1. Update Dockerfile.unified to use these base images"
echo "  2. Run: ./docker/build-all-editions.sh ${THEMIS_VERSION#v} ${REGISTRY}"

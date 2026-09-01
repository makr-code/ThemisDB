#!/usr/bin/env bash
set -euo pipefail

#
# ThemisDB Docker Image Build & Push Script
# Builds Docker images and optionally pushes to Docker Hub
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
RELEASE_DIR="${REPO_ROOT}/release"

# Configuration
TAG="${1:-$(cat "$REPO_ROOT/VERSION" 2>/dev/null || echo "")}"
MULTIARCH_OCI_PATH="${RELEASE_DIR}/themisdb-${TAG}-multiarch.oci.tar"
PLATFORMS="${PLATFORMS:-linux/amd64,linux/arm64}"
DOCKERFILE="${DOCKERFILE:-Dockerfile.unified}"
PUSH="${PUSH:-false}"
NO_CACHE="${NO_CACHE:-false}"
BUILD_BINARY="${BUILD_BINARY:-false}"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Validation
if [ -z "$TAG" ]; then
    echo -e "${RED}ERROR: Version tag required. Set TAG or ensure VERSION file exists.${NC}"
    exit 1
fi

echo -e "${CYAN}╔════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║     ThemisDB Docker Build Script      ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════╝${NC}"
echo ""
echo -e "${GREEN}Configuration:${NC}"
echo "  Tag:        $TAG"
echo "  Platforms:  $PLATFORMS"
echo "  Dockerfile: $DOCKERFILE"
echo "  Push:       $([ "$PUSH" = "true" ] && echo "YES" || echo "NO")"
echo "  Build Binary: $([ "$BUILD_BINARY" = "true" ] && echo "YES" || echo "NO")"
echo ""

# Step 1: Build binary if requested
if [ "$BUILD_BINARY" = "true" ]; then
    echo -e "${YELLOW}════════ Building themis_server binary ════════${NC}"

    cd "$REPO_ROOT"

    echo -e "${CYAN}Configuring CMake...${NC}"
    cmake --preset linux-release \
        -DTHEMIS_BUILD_TESTS=OFF \
        -DTHEMIS_BUILD_BENCHMARKS=OFF

    echo -e "${CYAN}Building themis_server...${NC}"
    cmake --build --preset linux-release --target themis_server

    # Copy binary to release directory
    mkdir -p "$RELEASE_DIR"
    BINARY_PATH="${REPO_ROOT}/build-gcc-linux-release/themis_server"
    if [ -f "$BINARY_PATH" ]; then
        echo -e "${CYAN}Copying binary to $RELEASE_DIR...${NC}"
        cp "$BINARY_PATH" "${RELEASE_DIR}/themis_server"
        chmod +x "${RELEASE_DIR}/themis_server"
    fi
fi

# Step 2: Check Docker installation
echo ""
echo -e "${YELLOW}════════ Checking Docker installation ════════${NC}"
if ! command -v docker &> /dev/null; then
    echo -e "${RED}ERROR: Docker not found or not installed${NC}"
    exit 1
fi
docker --version | sed "s/^/${GREEN}/" | sed "s/$/${NC}/"

# Step 3: Build Docker image
echo ""
echo -e "${YELLOW}════════ Building Docker image ════════${NC}"

IMAGE_TAG="themisdb/themisdb:$TAG"
IMAGE_TAG_LATEST="themisdb/themisdb:latest"

COMMON_BUILD_ARGS=(
    "--build-arg" "VERSION=$TAG"
)

for var in HTTP_PROXY HTTPS_PROXY ALL_PROXY NO_PROXY VCPKG_ENABLE_ONLINE INCLUDE_TINYLLAMA TINYLLAMA_FORCE_DOWNLOAD TINYLLAMA_HF_URL; do
    val="${!var:-}"
    if [ -n "$val" ]; then
        COMMON_BUILD_ARGS+=("--build-arg" "${var}=${val}")
    fi
done

# For multi-arch builds, use buildx
if [[ "$PLATFORMS" == *","* ]]; then
    echo -e "${CYAN}Building multi-architecture image...${NC}"
    if ! docker buildx ls 2>/dev/null | grep -q themis-builder; then
        echo -e "${CYAN}Setting up buildx builder...${NC}"
        docker buildx create --name themis-builder --use
    fi
    
    BUILD_ARGS=(
        "buildx" "build"
        "--builder" "themis-builder"
        "-f" "${REPO_ROOT}/docker/$DOCKERFILE"
        "-t" "$IMAGE_TAG"
        "-t" "$IMAGE_TAG_LATEST"
        "--platform" "$PLATFORMS"
    )
    BUILD_ARGS+=("${COMMON_BUILD_ARGS[@]}")
    if [ "$NO_CACHE" = "true" ]; then
        BUILD_ARGS+=("--no-cache")
    fi
    
    if [ "$PUSH" = "true" ]; then
        BUILD_ARGS+=("--push")
        echo -e "${GREEN}  [Push enabled - image will be pushed after build]${NC}"
    else
        mkdir -p "${RELEASE_DIR}"
        rm -f "${MULTIARCH_OCI_PATH}"
        BUILD_ARGS+=("--output=type=oci,dest=${MULTIARCH_OCI_PATH}")
        echo -e "${YELLOW}  [Push disabled - exporting multi-arch OCI archive to ${MULTIARCH_OCI_PATH}]${NC}"
    fi
    
    docker "${BUILD_ARGS[@]}" "${REPO_ROOT}"
else
    # Single-arch build with standard docker build
    BUILD_ARGS=(
        "build"
        "-f" "${REPO_ROOT}/docker/$DOCKERFILE"
        "-t" "$IMAGE_TAG"
        "-t" "$IMAGE_TAG_LATEST"
    )
    BUILD_ARGS+=("${COMMON_BUILD_ARGS[@]}")
    
    if [ "$NO_CACHE" = "true" ]; then
        BUILD_ARGS+=("--no-cache")
    fi
    
    docker "${BUILD_ARGS[@]}" "${REPO_ROOT}"
    
    echo -e "${GREEN}Image built successfully: $IMAGE_TAG${NC}"
    
    # Step 5: Push to Docker Hub (if requested)
    if [ "$PUSH" = "true" ]; then
        echo ""
        echo -e "${YELLOW}════════ Pushing to Docker Hub ════════${NC}"
        
        # Check Docker login
        if ! docker info 2>/dev/null | grep -q "Username"; then
            echo -e "${CYAN}Docker not logged in. Running: docker login${NC}"
            docker login
        fi
        
        # Push image and tags
        echo -e "${CYAN}Pushing $IMAGE_TAG...${NC}"
        docker push "$IMAGE_TAG"
        
        echo -e "${CYAN}Pushing $IMAGE_TAG_LATEST...${NC}"
        docker push "$IMAGE_TAG_LATEST"
        
        echo -e "${GREEN}✅ Images pushed successfully!${NC}"
    fi
fi

# Step 6: Summary
echo ""
echo -e "${GREEN}╔════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║    Docker Build Completed Successfully ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════╝${NC}"
echo ""
echo -e "${CYAN}Image Tags:${NC}"
echo "  • $IMAGE_TAG"
  echo "  • $IMAGE_TAG_LATEST"
echo ""
echo -e "${CYAN}Next Steps:${NC}"
if [ "$PUSH" = "true" ]; then
    echo "  ✅ Image has been pushed to Docker Hub"
    echo "  → Pull with: docker pull $IMAGE_TAG"
else
    echo "  → Run locally: docker run -d -p 18765:18765 -v themisdb_data:/data $IMAGE_TAG"
    echo "  → Push to registry: docker push $IMAGE_TAG"
fi
echo ""

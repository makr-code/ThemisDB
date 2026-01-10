#!/bin/bash
# ============================================================================
# ThemisDB Docker Build Script - All Editions
# ============================================================================
# Builds all ThemisDB editions with optimized caching
#
# Usage:
#   ./build-all-editions.sh [VERSION] [REGISTRY] [PLATFORM]
#
# Examples:
#   ./build-all-editions.sh 1.4.0 themisdb/themisdb linux/amd64
#   ./build-all-editions.sh 1.4.0 ghcr.io/themisdb/themisdb "linux/amd64,linux/arm64"
#   ./build-all-editions.sh latest themisdb/themisdb linux/amd64 --push

set -e

# ============================================================================
# Configuration
# ============================================================================
VERSION="${1:-1.4.0}"
REGISTRY="${2:-themisdb/themisdb}"
PLATFORM="${3:-linux/amd64}"
PUSH_FLAG="${4}"

# Edition definitions
declare -A EDITIONS=(
    ["minimal"]="MINIMAL:OFF"
    ["community"]="COMMUNITY:OFF"
    ["enterprise"]="ENTERPRISE:OFF"
    ["hyperscaler"]="HYPERSCALER:ON"
)

# ============================================================================
# Setup
# ============================================================================
echo "============================================"
echo "ThemisDB Docker Build - All Editions"
echo "============================================"
echo "Version:  ${VERSION}"
echo "Registry: ${REGISTRY}"
echo "Platform: ${PLATFORM}"
echo "Push:     ${PUSH_FLAG:-no}"
echo "Note:     Using local llama.cpp from ./llama.cpp/"
echo "============================================"
echo ""

# Check if buildx is available
if ! docker buildx version > /dev/null 2>&1; then
    echo "ERROR: docker buildx not found. Please install Docker BuildKit."
    exit 1
fi

# Create builder instance if not exists
if ! docker buildx inspect themis-builder > /dev/null 2>&1; then
    echo "Creating buildx builder instance..."
    docker buildx create --name themis-builder --use
fi

docker buildx use themis-builder

# ============================================================================
# Build function
# ============================================================================
build_edition() {
    local edition_name="$1"
    local edition_config="$2"
    
    IFS=':' read -r edition_value llm_enabled <<< "$edition_config"
    
    echo ""
    echo "========================================"
    echo "Building: ${edition_name} edition"
    echo "Edition:  ${edition_value}"
    echo "LLM:      ${llm_enabled}"
    echo "========================================"
    
    local tags=(
        "${REGISTRY}:${VERSION}-${edition_name}"
        "${REGISTRY}:${edition_name}"
    )
    
    # Community edition also gets 'latest' tag
    if [ "$edition_name" = "community" ]; then
        tags+=("${REGISTRY}:latest")
    fi
    
    # Build tag arguments
    local tag_args=()
    for tag in "${tags[@]}"; do
        tag_args+=(-t "$tag")
    done
    
    # Cache configuration
    local cache_from="type=registry,ref=${REGISTRY}:cache-${edition_name}"
    local cache_to="type=registry,ref=${REGISTRY}:cache-${edition_name},mode=max"
    
    # Build command
    local build_args=(
        buildx build
        --platform "${PLATFORM}"
        --build-arg "THEMIS_EDITION=${edition_value}"
        --build-arg "ENABLE_LLM=${llm_enabled}"
        --build-arg "THEMIS_VERSION=${VERSION}"
        --cache-from "${cache_from}"
        --cache-to "${cache_to}"
        "${tag_args[@]}"
        -f docker/Dockerfile.unified
    )
    
    # Add push flag if specified
    if [ -n "$PUSH_FLAG" ]; then
        build_args+=(--push)
    else
        build_args+=(--load)
    fi
    
    build_args+=(.)
    
    echo "Command: docker ${build_args[*]}"
    echo ""
    
    if docker "${build_args[@]}"; then
        echo "✅ Successfully built ${edition_name} edition"
    else
        echo "❌ Failed to build ${edition_name} edition"
        return 1
    fi
}

# ============================================================================
# Build all editions
# ============================================================================
failed_editions=()

for edition_name in "${!EDITIONS[@]}"; do
    if ! build_edition "$edition_name" "${EDITIONS[$edition_name]}"; then
        failed_editions+=("$edition_name")
    fi
done

# ============================================================================
# Summary
# ============================================================================
echo ""
echo "============================================"
echo "Build Summary"
echo "============================================"

if [ ${#failed_editions[@]} -eq 0 ]; then
    echo "✅ All editions built successfully!"
    echo ""
    echo "Available images:"
    for edition_name in "${!EDITIONS[@]}"; do
        echo "  - ${REGISTRY}:${VERSION}-${edition_name}"
        echo "  - ${REGISTRY}:${edition_name}"
    done
    echo "  - ${REGISTRY}:latest (community)"
    
    echo ""
    echo "Run with:"
    echo "  docker run -d -p 8080:8080 -p 18765:18765 ${REGISTRY}:community"
    echo "  docker run -d -p 8080:8080 -p 18765:18765 ${REGISTRY}:enterprise"
    echo "  docker run -d -p 8080:8080 -p 18765:18765 ${REGISTRY}:hyperscaler"
    
    exit 0
else
    echo "❌ Failed editions: ${failed_editions[*]}"
    exit 1
fi

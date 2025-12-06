#!/usr/bin/env bash
#
# Unified Multi-Arch Docker Build for ThemisDB (Docker Desktop / Offline)
#
# Builds all Docker image variants for ThemisDB using Docker Desktop with buildx.
# This script is designed for local/offline builds without requiring internet access
# during the build process (vcpkg dependencies are cached).
#
# Supported Build Variants:
# - Standard (Ubuntu 22.04): linux/amd64, linux/arm64
# - QNAP (Ubuntu 20.04): linux/amd64 (GLIBC 2.31 compatibility)
# - Raspberry Pi (ARM64): linux/arm64
#
# Usage:
#   ./docker-build-multiarch.sh [options]
#
# Options:
#   -v, --version VERSION   Version tag (default: from VERSION file or 1.0.0)
#   -r, --registry REGISTRY Docker registry prefix (default: themisdb)
#   -b, --build VARIANT     Build variant: all, standard, qnap, rpi (default: all)
#   -p, --platform PLATFORM Override platform for standard build
#   --push                  Push images to registry
#   --no-cache              Disable Docker build cache
#   -h, --help              Show this help message
#
# Examples:
#   # Build all variants locally
#   ./docker-build-multiarch.sh
#
#   # Build only QNAP variant
#   ./docker-build-multiarch.sh -b qnap
#
#   # Build and push to registry
#   ./docker-build-multiarch.sh --push
#

set -euo pipefail

# =============================================================================
# Configuration
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION=""
REGISTRY="themisdb"
VARIANT="all"
PLATFORM=""
PUSH=false
NO_CACHE=false
BUILDER_NAME="themis-multiarch"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# Build results
declare -A BUILD_RESULTS

# =============================================================================
# Helper Functions
# =============================================================================

print_header() {
    echo ""
    echo -e "${BLUE}======================================================================${NC}"
    echo -e "${BLUE} $1${NC}"
    echo -e "${BLUE}======================================================================${NC}"
    echo ""
}

print_step() {
    echo -e "${CYAN}► $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_failure() {
    echo -e "${RED}✗ $1${NC}"
}

show_help() {
    head -40 "$0" | tail -35 | sed 's/^#//' | sed 's/^ //'
    exit 0
}

# =============================================================================
# Argument Parsing
# =============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--version)
            VERSION="$2"
            shift 2
            ;;
        -r|--registry)
            REGISTRY="$2"
            shift 2
            ;;
        -b|--build)
            VARIANT="$2"
            shift 2
            ;;
        -p|--platform)
            PLATFORM="$2"
            shift 2
            ;;
        --push)
            PUSH=true
            shift
            ;;
        --no-cache)
            NO_CACHE=true
            shift
            ;;
        -h|--help)
            show_help
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            ;;
    esac
done

# Read version from VERSION file if not specified
if [[ -z "$VERSION" ]]; then
    if [[ -f "$SCRIPT_DIR/VERSION" ]]; then
        VERSION=$(cat "$SCRIPT_DIR/VERSION" | tr -d '[:space:]')
    else
        VERSION="1.0.0"
    fi
fi

# Validate variant
case $VARIANT in
    all|standard|qnap|rpi) ;;
    *)
        echo "Invalid variant: $VARIANT"
        echo "Valid options: all, standard, qnap, rpi"
        exit 1
        ;;
esac

# =============================================================================
# Docker Buildx Setup
# =============================================================================

check_docker_buildx() {
    print_step "Checking Docker buildx..."
    
    if ! docker buildx version &>/dev/null; then
        print_failure "Docker buildx is not available"
        echo "Please ensure Docker Desktop is installed with buildx support."
        return 1
    fi
    
    local version
    version=$(docker buildx version | head -1)
    print_success "Docker buildx available: $version"
    return 0
}

init_multiarch_builder() {
    print_step "Initializing multi-arch builder: $BUILDER_NAME"
    
    if ! docker buildx ls 2>&1 | grep -q "$BUILDER_NAME"; then
        echo "  Creating new builder instance..."
        docker buildx create --name "$BUILDER_NAME" --use \
            --platform linux/amd64,linux/arm64,linux/arm/v7
    else
        echo "  Using existing builder instance..."
        docker buildx use "$BUILDER_NAME"
    fi
    
    # Bootstrap the builder
    docker buildx inspect --bootstrap &>/dev/null || true
    
    print_success "Multi-arch builder ready"
    return 0
}

# =============================================================================
# Build Functions
# =============================================================================

build_docker_image() {
    local name="$1"
    local dockerfile="$2"
    local platforms="$3"
    shift 3
    local tags=("$@")
    
    print_header "Building: $name"
    
    echo -e "  Dockerfile: ${WHITE}$dockerfile${NC}"
    echo -e "  Platforms:  ${WHITE}$platforms${NC}"
    echo "  Tags:"
    for tag in "${tags[@]}"; do
        echo -e "    - ${WHITE}$tag${NC}"
    done
    echo ""
    
    # Build command arguments
    local cmd_args=("buildx" "build")
    
    # Add cache options
    if [[ "$NO_CACHE" == true ]]; then
        cmd_args+=("--no-cache")
    fi
    
    # Add platform
    cmd_args+=("--platform" "$platforms")
    
    # Add tags
    for tag in "${tags[@]}"; do
        cmd_args+=("-t" "$tag")
    done
    
    # Add dockerfile
    cmd_args+=("-f" "$dockerfile")
    
    # Progress output
    cmd_args+=("--progress" "plain")
    
    # Push or load
    if [[ "$PUSH" == true ]]; then
        cmd_args+=("--push")
    else
        # For multi-platform builds without push, we can only use --load for single platform
        if [[ "$platforms" != *","* ]]; then
            cmd_args+=("--load")
        else
            print_warning "Multi-platform build without push - images will be in buildx cache only"
            echo "  Use --push to push to registry, or build single platform with -p"
        fi
    fi
    
    # Context
    cmd_args+=(".")
    
    # Execute build
    print_step "Starting build..."
    local start_time
    start_time=$(date +%s)
    
    if docker "${cmd_args[@]}"; then
        local end_time
        end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_success "Build completed in $(printf '%02d:%02d' $((duration/60)) $((duration%60)))"
        return 0
    else
        print_failure "Build failed"
        return 1
    fi
}

build_standard() {
    local platforms="${PLATFORM:-linux/amd64,linux/arm64}"
    local tags=(
        "$REGISTRY/themisdb:$VERSION"
        "$REGISTRY/themisdb:latest"
    )
    
    # Add build args to command
    local old_cmd_args
    build_docker_image \
        "Standard Multi-Arch (Ubuntu 22.04)" \
        "Dockerfile" \
        "$platforms" \
        "${tags[@]}"
}

build_qnap() {
    local tags=(
        "$REGISTRY/themisdb:$VERSION-qnap"
        "$REGISTRY/themisdb:qnap"
    )
    
    build_docker_image \
        "QNAP (Ubuntu 20.04, x64)" \
        "Dockerfile.qnap" \
        "linux/amd64" \
        "${tags[@]}"
}

build_rpi() {
    local tags=(
        "$REGISTRY/themisdb:$VERSION-rpi"
        "$REGISTRY/themisdb:rpi"
    )
    
    build_docker_image \
        "Raspberry Pi (ARM64)" \
        "Dockerfile" \
        "linux/arm64" \
        "${tags[@]}"
}

# =============================================================================
# Main Execution
# =============================================================================

print_header "ThemisDB Multi-Arch Docker Build"

echo -e "Version:   ${WHITE}$VERSION${NC}"
echo -e "Registry:  ${WHITE}$REGISTRY${NC}"
echo -e "Variant:   ${WHITE}$VARIANT${NC}"
echo -e "Push:      ${WHITE}$PUSH${NC}"
echo -e "No Cache:  ${WHITE}$NO_CACHE${NC}"

# Check prerequisites
check_docker_buildx || exit 1
init_multiarch_builder || exit 1

# Execute builds based on variant selection
start_time=$(date +%s)

case $VARIANT in
    all)
        build_standard && BUILD_RESULTS[standard]=0 || BUILD_RESULTS[standard]=1
        build_qnap && BUILD_RESULTS[qnap]=0 || BUILD_RESULTS[qnap]=1
        build_rpi && BUILD_RESULTS[rpi]=0 || BUILD_RESULTS[rpi]=1
        ;;
    standard)
        build_standard && BUILD_RESULTS[standard]=0 || BUILD_RESULTS[standard]=1
        ;;
    qnap)
        build_qnap && BUILD_RESULTS[qnap]=0 || BUILD_RESULTS[qnap]=1
        ;;
    rpi)
        build_rpi && BUILD_RESULTS[rpi]=0 || BUILD_RESULTS[rpi]=1
        ;;
esac

end_time=$(date +%s)
total_duration=$((end_time - start_time))

# =============================================================================
# Summary
# =============================================================================

print_header "Build Summary"

echo "Total Duration: $(printf '%02d:%02d:%02d' $((total_duration/3600)) $(((total_duration%3600)/60)) $((total_duration%60)))"
echo ""

echo "Build Results:"
success_count=0
fail_count=0

for key in "${!BUILD_RESULTS[@]}"; do
    if [[ ${BUILD_RESULTS[$key]} -eq 0 ]]; then
        echo -e "  ${GREEN}$key : ✓ Success${NC}"
        ((success_count++))
    else
        echo -e "  ${RED}$key : ✗ Failed${NC}"
        ((fail_count++))
    fi
done

echo ""

if [[ $fail_count -eq 0 ]]; then
    print_success "All $success_count build(s) completed successfully!"
    
    echo ""
    echo "Built Images:"
    
    if [[ -v BUILD_RESULTS[standard] ]]; then
        echo -e "  ${WHITE}$REGISTRY/themisdb:$VERSION (amd64, arm64)${NC}"
        echo -e "  ${WHITE}$REGISTRY/themisdb:latest${NC}"
    fi
    if [[ -v BUILD_RESULTS[qnap] ]]; then
        echo -e "  ${WHITE}$REGISTRY/themisdb:$VERSION-qnap (amd64)${NC}"
        echo -e "  ${WHITE}$REGISTRY/themisdb:qnap${NC}"
    fi
    if [[ -v BUILD_RESULTS[rpi] ]]; then
        echo -e "  ${WHITE}$REGISTRY/themisdb:$VERSION-rpi (arm64)${NC}"
        echo -e "  ${WHITE}$REGISTRY/themisdb:rpi${NC}"
    fi
    
    if [[ "$PUSH" != true ]]; then
        echo ""
        echo -e "${YELLOW}To push images to registry:${NC}"
        echo "  ./docker-build-multiarch.sh -b $VARIANT --push"
    fi
    
    echo ""
    echo -e "${YELLOW}To test locally:${NC}"
    echo "  docker run --rm -p 18765:18765 $REGISTRY/themisdb:latest"
    
    exit 0
else
    print_failure "$fail_count of $((success_count + fail_count)) build(s) failed"
    exit 1
fi

#!/usr/bin/env bash
#
# Docker Build for ThemisDB (Hybrid Pre-built Binary)
#
# Builds Docker images for ThemisDB using pre-built binaries.
# Uses standard 'docker build' (no buildx required).
#
# This hybrid approach provides:
# - Fast build times (seconds instead of minutes)
# - Small image sizes (~100-200 MB)
# - 100% offline capability
#
# Usage:
#   ./docker-build.sh [options]
#
# Options:
#   -v, --version VERSION   Version tag (default: from VERSION file or 1.0.0)
#   -r, --registry REGISTRY Docker registry prefix (default: themisdb)
#   -b, --variant VARIANT   Build variant: standard, qnap (default: standard)
#   --binary PATH           Path to pre-built binary (default: build/themis_server)
#   --build-binary          Build binary before creating Docker image
#   --push                  Push images to registry
#   --no-cache              Disable Docker build cache
#   -h, --help              Show this help message
#
# Examples:
#   # Build Docker image with existing binary
#   ./docker-build.sh
#
#   # Build binary first, then create Docker image
#   ./docker-build.sh --build-binary
#
#   # Build QNAP variant
#   ./docker-build.sh -b qnap
#
#   # Build and push to registry
#   ./docker-build.sh --push
#

set -euo pipefail

# =============================================================================
# Configuration
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION=""
REGISTRY="themisdb"
VARIANT="standard"
BINARY_PATH="build/themis_server"
BUILD_BINARY=false
PUSH=false
NO_CACHE=false

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m'

# =============================================================================
# Helper Functions
# =============================================================================

print_header() {
    echo ""
    echo -e "${BLUE}============================================================${NC}"
    echo -e "${BLUE} $1${NC}"
    echo -e "${BLUE}============================================================${NC}"
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
    head -45 "$0" | tail -40 | sed 's/^#//' | sed 's/^ //'
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
        -b|--variant)
            VARIANT="$2"
            shift 2
            ;;
        --binary)
            BINARY_PATH="$2"
            shift 2
            ;;
        --build-binary)
            BUILD_BINARY=true
            shift
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
    standard|qnap) ;;
    *)
        echo "Invalid variant: $VARIANT"
        echo "Valid options: standard, qnap"
        exit 1
        ;;
esac

# =============================================================================
# Build Binary (optional)
# =============================================================================

build_binary() {
    print_header "Building Binary"
    
    local build_dir="${HOME}/themis-build-release"
    
    print_step "Checking build directory..."
    if [[ ! -d "$build_dir" ]]; then
        print_warning "Build directory not found: $build_dir"
        echo "Please set up the build directory first:"
        echo "  mkdir -p $build_dir"
        echo "  cd $build_dir"
        echo "  cmake -S $SCRIPT_DIR -B . -DCMAKE_BUILD_TYPE=Release -DTHEMIS_STATIC_BUILD=ON"
        return 1
    fi
    print_success "Build directory exists"
    
    print_step "Building themis_server (monolithic/static)..."
    cd "$build_dir"
    cmake --build . --target themis_server -j"$(nproc)"
    
    if [[ $? -ne 0 ]]; then
        print_failure "Build failed"
        return 1
    fi
    
    print_step "Copying binary..."
    mkdir -p "${SCRIPT_DIR}/build"
    cp "${build_dir}/themis_server" "${SCRIPT_DIR}/build/"
    
    print_success "Binary built and copied successfully"
    cd "${SCRIPT_DIR}"
    return 0
}

# =============================================================================
# Docker Build
# =============================================================================

build_docker_image() {
    local dockerfile="$1"
    shift
    local tags=("$@")
    
    print_header "Building Docker Image"
    
    echo -e "  Dockerfile: ${WHITE}$dockerfile${NC}"
    echo -e "  Binary:     ${WHITE}$BINARY_PATH${NC}"
    echo "  Tags:"
    for tag in "${tags[@]}"; do
        echo -e "    - ${WHITE}$tag${NC}"
    done
    echo ""
    
    # Check binary exists
    if [[ ! -f "$BINARY_PATH" ]]; then
        print_failure "Binary not found at: $BINARY_PATH"
        echo ""
        echo -e "${YELLOW}Please build the binary first:${NC}"
        echo "  Option 1: ./docker-build.sh --build-binary"
        echo "  Option 2: Build manually and copy to build/"
        return 1
    fi
    
    print_success "Binary found: $BINARY_PATH"
    
    # Build command
    local cmd_args=("build")
    
    if [[ "$NO_CACHE" == true ]]; then
        cmd_args+=("--no-cache")
    fi
    
    for tag in "${tags[@]}"; do
        cmd_args+=("-t" "$tag")
    done
    
    cmd_args+=("-f" "$dockerfile" ".")
    
    print_step "Starting Docker build..."
    local start_time
    start_time=$(date +%s)
    
    if docker "${cmd_args[@]}"; then
        local end_time
        end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_success "Build completed in $(printf '%02d:%02d' $((duration/60)) $((duration%60)))"
        
        # Push if requested
        if [[ "$PUSH" == true ]]; then
            print_step "Pushing images to registry..."
            for tag in "${tags[@]}"; do
                echo "  Pushing: $tag"
                docker push "$tag"
                if [[ $? -ne 0 ]]; then
                    print_failure "Failed to push: $tag"
                    return 1
                fi
            done
            print_success "All images pushed successfully"
        fi
        
        return 0
    else
        print_failure "Build failed"
        return 1
    fi
}

# =============================================================================
# Main Execution
# =============================================================================

print_header "ThemisDB Docker Build (Hybrid Pre-built Binary)"

echo -e "Version:      ${WHITE}$VERSION${NC}"
echo -e "Registry:     ${WHITE}$REGISTRY${NC}"
echo -e "Variant:      ${WHITE}$VARIANT${NC}"
echo -e "Binary Path:  ${WHITE}$BINARY_PATH${NC}"
echo -e "Build Binary: ${WHITE}$BUILD_BINARY${NC}"
echo -e "Push:         ${WHITE}$PUSH${NC}"

# Check Docker
print_step "Checking Docker..."
if ! docker version --format '{{.Server.Version}}' &>/dev/null; then
    print_failure "Docker is not running"
    exit 1
fi
docker_version=$(docker version --format '{{.Server.Version}}')
print_success "Docker available: $docker_version"

# Build binary if requested
if [[ "$BUILD_BINARY" == true ]]; then
    build_binary || exit 1
fi

# Determine tags based on variant
declare -a tags
dockerfile="Dockerfile.simple"

case $VARIANT in
    standard)
        tags=(
            "$REGISTRY/themisdb:$VERSION"
            "$REGISTRY/themisdb:latest"
        )
        ;;
    qnap)
        tags=(
            "$REGISTRY/themisdb:$VERSION-qnap"
            "$REGISTRY/themisdb:qnap"
        )
        ;;
esac

# Execute build
start_time=$(date +%s)
build_docker_image "$dockerfile" "${tags[@]}"
success=$?
end_time=$(date +%s)
total_duration=$((end_time - start_time))

# =============================================================================
# Summary
# =============================================================================

print_header "Build Summary"

echo "Total Duration: $(printf '%02d:%02d' $((total_duration/60)) $((total_duration%60)))"
echo ""

if [[ $success -eq 0 ]]; then
    print_success "Build completed successfully!"
    
    echo ""
    echo "Built Images:"
    for tag in "${tags[@]}"; do
        echo -e "  ${WHITE}$tag${NC}"
    done
    
    echo ""
    echo -e "${YELLOW}To test locally:${NC}"
    echo "  docker run --rm -p 18765:18765 ${tags[0]}"
    
    echo ""
    echo -e "${YELLOW}To run with data volume:${NC}"
    echo "  docker run -d -p 18765:18765 -v \$(pwd)/data:/data ${tags[0]}"
    
    if [[ "$PUSH" != true ]]; then
        echo ""
        echo -e "${YELLOW}To push to registry:${NC}"
        echo "  ./docker-build.sh -b $VARIANT --push"
    fi
    
    exit 0
else
    print_failure "Build failed"
    exit 1
fi

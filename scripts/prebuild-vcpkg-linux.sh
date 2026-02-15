#!/usr/bin/env bash
# =============================================================================
# ThemisDB vcpkg Prebuild Cache Generator for Linux/WSL
# =============================================================================
# Purpose: Generate prebuilt vcpkg packages for x64-linux in both debug and
#          release configurations, ready to be mounted in Docker containers
#
# Usage:
#   ./scripts/prebuild-vcpkg-linux.sh [debug|release|both]
#
# Output: prebuilt-cache/x64-linux/{debug,release}/vcpkg_installed/
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PREBUILT_CACHE_DIR="${PROJECT_ROOT}/prebuilt-cache"
TRIPLET="${VCPKG_TARGET_TRIPLET:-x64-linux}"
BUILD_TYPE="${1:-both}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

show_usage() {
    cat <<EOF
Usage: $0 [debug|release|both]

Generate prebuilt vcpkg packages for Docker mounting.

Options:
  debug    - Generate debug build packages only
  release  - Generate release build packages only (default)
  both     - Generate both debug and release packages

Environment Variables:
  VCPKG_ROOT              - Path to vcpkg installation (default: ./vcpkg)
  VCPKG_TARGET_TRIPLET    - Target triplet (default: x64-linux)
  THEMIS_EDITION          - ThemisDB edition (default: COMMUNITY)

Examples:
  # Generate release packages only (fastest)
  $0 release

  # Generate both debug and release packages
  $0 both

  # Generate with specific edition
  THEMIS_EDITION=MINIMAL $0 release

Output:
  prebuilt-cache/x64-linux/debug/vcpkg_installed/
  prebuilt-cache/x64-linux/release/vcpkg_installed/

EOF
}

# Check if help requested
if [[ "${1:-}" == "-h" ]] || [[ "${1:-}" == "--help" ]]; then
    show_usage
    exit 0
fi

# Validate build type
if [[ "$BUILD_TYPE" != "debug" ]] && [[ "$BUILD_TYPE" != "release" ]] && [[ "$BUILD_TYPE" != "both" ]]; then
    log_error "Invalid build type: $BUILD_TYPE"
    show_usage
    exit 1
fi

# Check for vcpkg
if [[ -z "${VCPKG_ROOT:-}" ]]; then
    if [[ -d "${PROJECT_ROOT}/vcpkg" ]]; then
        export VCPKG_ROOT="${PROJECT_ROOT}/vcpkg"
    else
        log_error "VCPKG_ROOT not set and ./vcpkg not found"
        log_info "Please set VCPKG_ROOT or clone vcpkg to ./vcpkg"
        exit 1
    fi
fi

if [[ ! -f "${VCPKG_ROOT}/vcpkg" ]]; then
    log_error "vcpkg executable not found at: ${VCPKG_ROOT}/vcpkg"
    log_info "Please bootstrap vcpkg: cd ${VCPKG_ROOT} && ./bootstrap-vcpkg.sh"
    exit 1
fi

log_info "================================================================"
log_info "ThemisDB vcpkg Prebuild Cache Generator"
log_info "================================================================"
log_info "Project Root:    $PROJECT_ROOT"
log_info "vcpkg Root:      $VCPKG_ROOT"
log_info "Target Triplet:  $TRIPLET"
log_info "Build Type:      $BUILD_TYPE"
log_info "Edition:         ${THEMIS_EDITION:-COMMUNITY}"
log_info "Output:          $PREBUILT_CACHE_DIR/$TRIPLET/"
log_info "================================================================"

# Function to build packages for a specific configuration
build_packages() {
    local config="$1"  # debug or release
    local cmake_build_type
    
    if [[ "$config" == "debug" ]]; then
        cmake_build_type="Debug"
    else
        cmake_build_type="Release"
    fi
    
    log_info ""
    log_info "Building $config packages for $TRIPLET..."
    log_info "----------------------------------------------------------------"
    
    # Create temporary build directory
    local temp_build_dir="${PROJECT_ROOT}/build-prebuild-${config}"
    mkdir -p "$temp_build_dir"
    
    # Create output directory
    local output_dir="${PREBUILT_CACHE_DIR}/${TRIPLET}/${config}"
    mkdir -p "$output_dir"
    
    # Configure CMake to install packages
    log_info "Configuring CMake for $config build..."
    cd "$temp_build_dir"
    
    cmake "${PROJECT_ROOT}" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE="$cmake_build_type" \
        -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
        -DVCPKG_INSTALLED_DIR="${temp_build_dir}/vcpkg_installed" \
        -DTHEMIS_EDITION="${THEMIS_EDITION:-COMMUNITY}" \
        -DTHEMIS_BUILD_TESTS=OFF \
        -DTHEMIS_BUILD_BENCHMARKS=OFF \
        || { log_error "CMake configuration failed for $config"; return 1; }
    
    log_info "Installing vcpkg packages..."
    
    # vcpkg install will populate vcpkg_installed directory
    "${VCPKG_ROOT}/vcpkg" install \
        --triplet="$TRIPLET" \
        --x-manifest-root="${PROJECT_ROOT}" \
        --x-install-root="${temp_build_dir}/vcpkg_installed" \
        --clean-after-build \
        || { log_error "vcpkg install failed for $config"; return 1; }
    
    # Copy installed packages to output directory
    log_info "Copying packages to prebuilt cache..."
    if [[ -d "${temp_build_dir}/vcpkg_installed/${TRIPLET}" ]]; then
        rm -rf "${output_dir}/vcpkg_installed"
        cp -r "${temp_build_dir}/vcpkg_installed" "$output_dir/"
        
        # Calculate size
        local size=$(du -sh "${output_dir}/vcpkg_installed" | cut -f1)
        local lib_count=$(find "${output_dir}/vcpkg_installed/${TRIPLET}/lib" -name '*.a' -o -name '*.so' 2>/dev/null | wc -l)
        
        log_success "$config packages ready!"
        log_info "  Location: ${output_dir}/vcpkg_installed"
        log_info "  Size: $size"
        log_info "  Libraries: $lib_count"
    else
        log_error "vcpkg_installed directory not found after build"
        return 1
    fi
    
    # Clean up temporary build directory (keep vcpkg_installed)
    log_info "Cleaning up temporary files..."
    cd "$PROJECT_ROOT"
    find "$temp_build_dir" -mindepth 1 -maxdepth 1 ! -name vcpkg_installed -exec rm -rf {} + 2>/dev/null || true
    
    log_success "$config prebuild complete!"
    return 0
}

# Main execution
START_TIME=$(date +%s)

if [[ "$BUILD_TYPE" == "both" ]] || [[ "$BUILD_TYPE" == "release" ]]; then
    build_packages "release" || exit 1
fi

if [[ "$BUILD_TYPE" == "both" ]] || [[ "$BUILD_TYPE" == "debug" ]]; then
    build_packages "debug" || exit 1
fi

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

log_info ""
log_info "================================================================"
log_success "Prebuild generation complete!"
log_info "================================================================"
log_info "Duration: ${DURATION}s"
log_info "Output: ${PREBUILT_CACHE_DIR}/${TRIPLET}/"
log_info ""
log_info "Next steps:"
log_info "  1. Use in Docker:"
log_info "     docker build --build-context prebuilt=${PREBUILT_CACHE_DIR}/${TRIPLET}/release ..."
log_info ""
log_info "  2. Or mount in docker-compose:"
log_info "     volumes:"
log_info "       - ${PREBUILT_CACHE_DIR}/${TRIPLET}/release/vcpkg_installed:/vcpkg-prebuilt:ro"
log_info ""
log_info "  3. Share with team (archive):"
log_info "     tar czf vcpkg-${TRIPLET}-release.tar.gz -C ${PREBUILT_CACHE_DIR}/${TRIPLET}/release vcpkg_installed"
log_info "================================================================"

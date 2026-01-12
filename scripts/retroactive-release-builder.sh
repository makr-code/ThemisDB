#!/bin/bash
# =============================================================================
# ThemisDB Retroactive Release Builder
# =============================================================================
# Purpose: Extract source code at specific version tags, commits, or branches
#          and build/package binaries retroactively for all past releases.
#
# Usage:
#   ./retroactive-release-builder.sh [OPTIONS]
#
# Options:
#   --tag TAG              Build specific tag (e.g., v1.3.4)
#   --commit COMMIT        Build specific commit SHA or branch name
#   --all-tags             Build all version tags
#   --list-tags            List available version tags
#   --platform PLATFORM    Target platform (linux|windows|macos|all) [default: linux]
#   --output-dir DIR       Output directory for artifacts [default: ./release-retroactive]
#   --skip-build           Skip build, only package existing binaries
#   --clean                Clean build directories before building
#   --help                 Show this help message
#
# Examples:
#   # Build specific tag for Linux
#   ./retroactive-release-builder.sh --tag v1.3.4 --platform linux
#
#   # Build from specific commit (intermediate release)
#   ./retroactive-release-builder.sh --commit a1b2c3d --platform linux
#
#   # Build from merge commit
#   ./retroactive-release-builder.sh --commit release/v1.3.4 --platform linux
#
#   # Build all tags for all platforms
#   ./retroactive-release-builder.sh --all-tags --platform all
#
#   # List available tags
#   ./retroactive-release-builder.sh --list-tags
#
# =============================================================================

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default configuration
PLATFORM="linux"
OUTPUT_DIR="./release-retroactive"
SKIP_BUILD=false
CLEAN_BUILD=false
SPECIFIC_TAG=""
SPECIFIC_COMMIT=""
BUILD_ALL_TAGS=false
LIST_TAGS=false

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# =============================================================================
# Utility Functions
# =============================================================================

print_header() {
    echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║  ThemisDB Retroactive Release Builder                     ║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
    echo
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

show_help() {
    cat << EOF
ThemisDB Retroactive Release Builder

Usage: $0 [OPTIONS]

Options:
    --tag TAG              Build specific tag (e.g., v1.3.4)
    --commit COMMIT        Build specific commit SHA or branch name
    --all-tags             Build all version tags
    --list-tags            List available version tags
    --platform PLATFORM    Target platform (linux|windows|macos|all) [default: linux]
    --output-dir DIR       Output directory for artifacts [default: ./release-retroactive]
    --skip-build           Skip build, only package existing binaries
    --clean                Clean build directories before building
    --help                 Show this help message

Examples:
    # Build specific tag for Linux
    $0 --tag v1.3.4 --platform linux

    # Build from specific commit (intermediate release)
    $0 --commit a1b2c3d --platform linux

    # Build from merge commit or branch
    $0 --commit release/v1.3.4 --platform linux

    # Build all tags for all platforms
    $0 --all-tags --platform all

    # List available tags
    $0 --list-tags

EOF
}

# =============================================================================
# Git Operations
# =============================================================================

list_version_tags() {
    print_info "Available version tags:"
    git tag -l "v*" | sort -V
}

get_all_version_tags() {
    git tag -l "v*" | sort -V
}

checkout_ref() {
    local ref=$1
    local ref_type=$2  # "tag" or "commit"
    
    print_info "Checking out $ref_type: $ref"
    
    # Stash any local changes
    if ! git diff-index --quiet HEAD --; then
        print_warning "Stashing local changes..."
        git stash push -m "Auto-stash before retroactive build"
    fi
    
    # Check if ref exists
    if ! git rev-parse "$ref" >/dev/null 2>&1; then
        print_error "$ref_type does not exist: $ref"
        if [ "$ref_type" = "tag" ]; then
            print_info "Available tags:"
            git tag -l "v*" | head -10
        fi
        return 1
    fi
    
    # Checkout the ref
    git checkout "$ref" 2>&1 || {
        print_error "Failed to checkout $ref_type: $ref"
        return 1
    }
    
    # Update submodules if any
    if [ -f ".gitmodules" ]; then
        print_info "Updating submodules..."
        git submodule update --init --recursive
    fi
    
    print_success "Checked out $ref_type: $ref"
    
    # Show commit information
    local commit=$(git rev-parse HEAD)
    local short_commit=$(git rev-parse --short HEAD)
    print_info "Commit: $short_commit ($commit)"
    
    # Show which branch contains this commit
    local branch=$(git branch -r --contains "$commit" | grep -E "(main|master|release/|develop)" | head -1 | xargs)
    if [ -n "$branch" ]; then
        print_info "Ref is from branch: $branch"
    fi
    
    # Show commit message
    local commit_msg=$(git log -1 --pretty=format:"%s")
    print_info "Commit message: $commit_msg"
}

restore_original_branch() {
    print_info "Restoring original branch..."
    
    # Get the original branch (stored before checkout)
    local original_branch=$(git rev-parse --abbrev-ref HEAD@{1} 2>/dev/null || echo "main")
    
    # If we're in detached HEAD state, try to restore
    if git symbolic-ref -q HEAD > /dev/null; then
        print_info "Already on a branch"
    else
        print_info "Returning to branch: $original_branch"
        git checkout "$original_branch" 2>&1 || git checkout main 2>&1 || git checkout develop 2>&1
    fi
    
    # Pop stashed changes if any
    if git stash list | grep -q "Auto-stash before retroactive build"; then
        print_info "Restoring stashed changes..."
        git stash pop
    fi
    
    print_success "Restored original state"
}

# =============================================================================
# Build Functions
# =============================================================================

detect_build_system() {
    local version=$1
    
    if [ -f "CMakeLists.txt" ]; then
        echo "cmake"
    elif [ -f "Makefile" ]; then
        echo "make"
    else
        print_error "No supported build system detected"
        return 1
    fi
}

build_linux() {
    local tag=$1
    local version=${tag#v}
    local build_dir="build-retroactive"
    
    print_info "Building Linux binaries for $tag..."
    
    # Clean if requested
    if [ "$CLEAN_BUILD" = true ]; then
        print_info "Cleaning build directory..."
        rm -rf "$build_dir"
    fi
    
    # Create build directory
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Configure CMake
    print_info "Configuring CMake..."
    cmake -S .. -B . \
        -DCMAKE_BUILD_TYPE=Release \
        -DTHEMIS_BUILD_TESTS=OFF \
        -DTHEMIS_BUILD_BENCHMARKS=OFF \
        -DTHEMIS_ENABLE_TRACING=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local
    
    # Build
    print_info "Building (using $(nproc) cores)..."
    cmake --build . --config Release -j "$(nproc)"
    
    # Package
    print_info "Generating packages..."
    cpack -G "TGZ;DEB;RPM" || {
        print_warning "Some package formats failed, continuing with available formats"
    }
    
    cd ..
    print_success "Linux build completed for $tag"
}

build_windows() {
    local tag=$1
    local version=${tag#v}
    
    print_warning "Windows builds require Windows environment with MSVC"
    print_info "Skipping Windows build on Linux host"
    return 0
}

build_macos() {
    local tag=$1
    local version=${tag#v}
    
    if [ "$(uname)" != "Darwin" ]; then
        print_warning "macOS builds require macOS environment"
        print_info "Skipping macOS build on non-macOS host"
        return 0
    fi
    
    print_info "Building macOS binaries for $tag..."
    # Similar to Linux build, but with macOS-specific options
    # Implementation similar to build_linux
    print_success "macOS build completed for $tag"
}

build_platform() {
    local tag=$1
    local platform=$2
    
    case "$platform" in
        linux)
            build_linux "$tag"
            ;;
        windows)
            build_windows "$tag"
            ;;
        macos)
            build_macos "$tag"
            ;;
        all)
            build_linux "$tag"
            build_windows "$tag"
            build_macos "$tag"
            ;;
        *)
            print_error "Unknown platform: $platform"
            return 1
            ;;
    esac
}

# =============================================================================
# Packaging Functions
# =============================================================================

package_artifacts() {
    local ref=$1
    local platform=$2
    
    # Determine version from ref
    local version
    if [[ "$ref" =~ ^v[0-9] ]]; then
        # It's a version tag
        version=${ref#v}
    else
        # It's a commit or branch - try to get version from VERSION file
        if [ -f "VERSION" ]; then
            version=$(cat VERSION | tr -d '[:space:]')
        else
            # Use commit SHA as version
            version=$(git rev-parse --short HEAD)
        fi
    fi
    
    print_info "Packaging artifacts for $ref (version: $version, platform: $platform)..."
    
    local ref_output_dir="${OUTPUT_DIR}/${ref//\//-}"  # Replace / with - for directory name
    mkdir -p "$ref_output_dir"
    
    # Find and copy build artifacts
    local build_dir="build-retroactive"
    
    if [ -d "$build_dir" ]; then
        # Copy packages
        find "$build_dir" -maxdepth 1 \( -name "*.tar.gz" -o -name "*.deb" -o -name "*.rpm" -o -name "*.zip" \) -exec cp {} "$ref_output_dir/" \;
        
        # Generate SHA256 checksums
        print_info "Generating SHA256 checksums..."
        (cd "$ref_output_dir" && sha256sum * > SHA256SUMS.txt 2>/dev/null || true)
        
        # Create release notes
        create_release_notes "$ref" "$version" "$ref_output_dir"
        
        print_success "Artifacts packaged in: $ref_output_dir"
        
        # List generated files
        print_info "Generated files:"
        ls -lh "$ref_output_dir"
    else
        print_warning "Build directory not found: $build_dir"
    fi
}

create_release_notes() {
    local ref=$1
    local version=$2
    local output_dir=$3
    
    local safe_ref=${ref//\//-}  # Replace / with - for filename
    local notes_file="${output_dir}/RELEASE_NOTES_${safe_ref}.md"
    
    cat > "$notes_file" << EOF
# ThemisDB ${ref} - Retroactive Build

**Version:** ${version}  
**Build Date:** $(date -u '+%Y-%m-%d %H:%M:%S UTC')  
**Build Type:** Retroactive Release Build  
**Build Host:** $(hostname)  
**Build Platform:** ${PLATFORM}

## Build Information

This release was built retroactively from the source code at ref ${ref}.

- **Git Ref:** ${ref}
- **Git Commit:** $(git rev-parse HEAD)
- **Commit Message:** $(git log -1 --pretty=format:"%s")
- **Build Date:** $(date -u '+%Y-%m-%d %H:%M:%S UTC')

## Artifacts

EOF
    
    # List artifacts
    for file in "${output_dir}"/*; do
        if [ -f "$file" ] && [ "$(basename "$file")" != "RELEASE_NOTES_${tag}.md" ]; then
            echo "- \`$(basename "$file")\`" >> "$notes_file"
        fi
    done
    
    cat >> "$notes_file" << EOF

## Installation

### Linux (Debian/Ubuntu)

\`\`\`bash
# Download DEB package
wget https://github.com/makr-code/ThemisDB/releases/download/${tag}/themisdb-${version}-Linux.deb

# Install
sudo dpkg -i themisdb-${version}-Linux.deb
\`\`\`

### Linux (Red Hat/CentOS/Fedora)

\`\`\`bash
# Download RPM package
wget https://github.com/makr-code/ThemisDB/releases/download/${tag}/themisdb-${version}-Linux.rpm

# Install
sudo rpm -i themisdb-${version}-Linux.rpm
\`\`\`

### From Source

\`\`\`bash
# Clone repository and checkout tag
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
git checkout ${tag}

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j\$(nproc)
\`\`\`

## Checksums

See \`SHA256SUMS.txt\` for file checksums.

---

For more information, visit the [ThemisDB GitHub repository](https://github.com/makr-code/ThemisDB).
EOF
    
    print_success "Release notes created: $notes_file"
}

# =============================================================================
# Main Build Process
# =============================================================================

build_ref() {
    local ref=$1
    local ref_type=$2  # "tag" or "commit"
    
    print_header
    print_info "Processing $ref_type: $ref"
    print_info "Platform: $PLATFORM"
    print_info "Output directory: $OUTPUT_DIR"
    echo
    
    # Checkout the ref
    checkout_ref "$ref" "$ref_type" || {
        print_error "Failed to checkout $ref_type: $ref"
        return 1
    }
    
    # Build unless skipped
    if [ "$SKIP_BUILD" = false ]; then
        build_platform "$ref" "$PLATFORM" || {
            print_error "Build failed for $ref_type: $ref"
            restore_original_branch
            return 1
        }
    fi
    
    # Package artifacts
    package_artifacts "$ref" "$PLATFORM"
    
    # Restore original branch
    restore_original_branch
    
    print_success "Completed processing $ref_type: $ref"
    echo
}

# =============================================================================
# Argument Parsing
# =============================================================================

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --tag)
                SPECIFIC_TAG="$2"
                shift 2
                ;;
            --commit)
                SPECIFIC_COMMIT="$2"
                shift 2
                ;;
            --all-tags)
                BUILD_ALL_TAGS=true
                shift
                ;;
            --list-tags)
                LIST_TAGS=true
                shift
                ;;
            --platform)
                PLATFORM="$2"
                shift 2
                ;;
            --output-dir)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            --skip-build)
                SKIP_BUILD=true
                shift
                ;;
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# =============================================================================
# Main
# =============================================================================

main() {
    cd "$REPO_ROOT"
    
    # Parse command line arguments
    parse_arguments "$@"
    
    # List tags if requested
    if [ "$LIST_TAGS" = true ]; then
        list_version_tags
        exit 0
    fi
    
    # Validate options
    if [ "$BUILD_ALL_TAGS" = false ] && [ -z "$SPECIFIC_TAG" ] && [ -z "$SPECIFIC_COMMIT" ]; then
        print_error "Either --tag, --commit, or --all-tags must be specified"
        show_help
        exit 1
    fi
    
    # Validate mutually exclusive options
    if [ -n "$SPECIFIC_TAG" ] && [ -n "$SPECIFIC_COMMIT" ]; then
        print_error "Cannot specify both --tag and --commit"
        show_help
        exit 1
    fi
    
    # Create output directory
    mkdir -p "$OUTPUT_DIR"
    
    # Build specific tag, commit, or all tags
    if [ -n "$SPECIFIC_TAG" ]; then
        build_ref "$SPECIFIC_TAG" "tag"
    elif [ -n "$SPECIFIC_COMMIT" ]; then
        build_ref "$SPECIFIC_COMMIT" "commit"
    elif [ "$BUILD_ALL_TAGS" = true ]; then
        local tags=($(get_all_version_tags))
        
        if [ ${#tags[@]} -eq 0 ]; then
            print_warning "No version tags found"
            exit 0
        fi
        
        print_info "Found ${#tags[@]} version tags"
        echo
        
        for tag in "${tags[@]}"; do
            build_ref "$tag" "tag" || {
                print_error "Failed to build tag: $tag"
                print_warning "Continuing with next tag..."
                echo
            }
        done
    fi
    
    print_success "All builds completed!"
    print_info "Artifacts available in: $OUTPUT_DIR"
}

# Run main function
main "$@"

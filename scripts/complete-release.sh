#!/bin/bash
# =============================================================================
# ThemisDB Complete Release Script (Git Flow Compatible)
# =============================================================================
# This script automates the complete release process following Git Flow:
#   develop → release/vX.X.X → main (+ tag) → retroactive build
#
# Usage:
#   ./complete-release.sh <version> [OPTIONS]
#
# Options:
#   --skip-build      Skip retroactive build after tagging
#   --skip-merge-back Skip merge back to develop
#   --dry-run         Show what would be done without executing
#
# Example:
#   ./complete-release.sh 1.5.0
#   ./complete-release.sh 1.5.0 --skip-build
#
# =============================================================================

set -e

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║  ThemisDB Complete Release (Git Flow)                     ║${NC}"
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

# Parse arguments
VERSION=$1
SKIP_BUILD=false
SKIP_MERGE_BACK=false
DRY_RUN=false

shift || true

while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --skip-merge-back)
            SKIP_MERGE_BACK=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [ -z "$VERSION" ]; then
    print_error "Version is required"
    echo "Usage: $0 <version> [OPTIONS]"
    echo "Example: $0 1.5.0"
    exit 1
fi

print_header
print_info "Version: $VERSION"
print_info "Skip Build: $SKIP_BUILD"
print_info "Skip Merge Back: $SKIP_MERGE_BACK"
print_info "Dry Run: $DRY_RUN"
echo

# Validate version format
if ! [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9-]+)?$ ]]; then
    print_error "Invalid version format: $VERSION"
    print_info "Expected format: X.Y.Z or X.Y.Z-prerelease"
    exit 1
fi

# Function to run command
run_cmd() {
    if [ "$DRY_RUN" = true ]; then
        print_warning "[DRY RUN] Would execute: $*"
    else
        "$@"
    fi
}

# Step 1: Ensure we're on develop and up-to-date
print_info "Step 1: Checking out develop branch..."
run_cmd git checkout develop
run_cmd git pull origin develop
print_success "On develop branch and up-to-date"

# Step 2: Create release branch
RELEASE_BRANCH="release/v$VERSION"
print_info "Step 2: Creating release branch: $RELEASE_BRANCH"
run_cmd git checkout -b "$RELEASE_BRANCH"
print_success "Release branch created"

# Step 3: Update VERSION file
print_info "Step 3: Updating VERSION file..."
if [ "$DRY_RUN" = false ]; then
    echo "$VERSION" > VERSION
    git add VERSION
    git commit -m "chore: Bump version to $VERSION"
fi
print_success "VERSION file updated"

# Step 4: Run tests (optional but recommended)
print_info "Step 4: Running tests..."
print_warning "Skipping tests - run manually if needed"
# Uncomment to enable:
# run_cmd cmake -B build -DCMAKE_BUILD_TYPE=Release -DTHEMIS_BUILD_TESTS=ON
# run_cmd cmake --build build -j$(nproc)
# run_cmd cd build && ctest --output-on-failure

# Step 5: Merge to main
print_info "Step 5: Merging release branch to main..."
run_cmd git checkout main
run_cmd git pull origin main
run_cmd git merge --no-ff "$RELEASE_BRANCH" -m "Merge $RELEASE_BRANCH into main"
print_success "Merged to main"

# Step 6: Create tag
TAG="v$VERSION"
print_info "Step 6: Creating tag: $TAG"
run_cmd git tag -a "$TAG" -m "Release $TAG"
print_success "Tag created"

# Step 7: Push main and tag
print_info "Step 7: Pushing main and tag to origin..."
run_cmd git push origin main
run_cmd git push origin "$TAG"
print_success "Pushed to origin"

# Step 8: Merge back to develop
if [ "$SKIP_MERGE_BACK" = false ]; then
    print_info "Step 8: Merging release branch back to develop..."
    run_cmd git checkout develop
    run_cmd git merge --no-ff "$RELEASE_BRANCH" -m "Merge $RELEASE_BRANCH back into develop"
    run_cmd git push origin develop
    print_success "Merged back to develop"
else
    print_warning "Step 8: Skipping merge back to develop"
fi

# Step 9: Delete release branch
print_info "Step 9: Deleting release branch..."
run_cmd git branch -d "$RELEASE_BRANCH"
print_success "Release branch deleted"

# Step 10: Build binaries retroactively
if [ "$SKIP_BUILD" = false ]; then
    print_info "Step 10: Building binaries retroactively..."
    
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    
    if [ "$DRY_RUN" = false ]; then
        "$SCRIPT_DIR/retroactive-release-builder.sh" --tag "$TAG" --clean
    else
        print_warning "[DRY RUN] Would execute: retroactive-release-builder.sh --tag $TAG --clean"
    fi
    
    print_success "Binaries built"
    
    if [ "$DRY_RUN" = false ]; then
        print_info "Artifacts location: release-retroactive/$TAG/"
        ls -lh "release-retroactive/$TAG/" 2>/dev/null || true
    fi
else
    print_warning "Step 10: Skipping retroactive build"
fi

# Summary
echo
print_success "═══════════════════════════════════════════════════════"
print_success "  Release $VERSION completed successfully!"
print_success "═══════════════════════════════════════════════════════"
echo
print_info "Summary:"
echo "  • Release branch: $RELEASE_BRANCH"
echo "  • Tag: $TAG"
echo "  • Main branch: Updated"
echo "  • Develop branch: Updated"
if [ "$SKIP_BUILD" = false ]; then
    echo "  • Binaries: release-retroactive/$TAG/"
fi
echo
print_info "Next steps:"
echo "  1. Verify GitHub Actions workflows completed successfully"
echo "  2. Test the release artifacts"
echo "  3. Update CHANGELOG.md if needed"
echo "  4. Announce the release"
echo

if [ "$DRY_RUN" = true ]; then
    print_warning "This was a DRY RUN - no changes were made"
fi

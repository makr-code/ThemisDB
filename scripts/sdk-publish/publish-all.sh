#!/bin/bash
# ThemisDB SDK Publishing Master Script
# Publishes all SDK packages to their respective registries
#
# Usage:
#   ./publish-all.sh [--dry-run] [--version VERSION] [--skip REGISTRY]
#
# Environment Variables Required:
#   NPM_TOKEN        - npm access token
#   PYPI_TOKEN       - PyPI API token
#   NUGET_API_KEY    - NuGet API key
#   MAVEN_USERNAME   - Maven Central username
#   MAVEN_PASSWORD   - Maven Central password
#   CARGO_TOKEN      - Crates.io API token

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Defaults
DRY_RUN=false
VERSION=""
SKIP_REGISTRIES=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --version)
            VERSION="$2"
            shift 2
            ;;
        --skip)
            SKIP_REGISTRIES="$SKIP_REGISTRIES $2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--dry-run] [--version VERSION] [--skip REGISTRY]"
            echo ""
            echo "Options:"
            echo "  --dry-run           Run without actually publishing"
            echo "  --version VERSION   Version to publish (reads from VERSION file if not specified)"
            echo "  --skip REGISTRY     Skip specific registry (npm, pypi, nuget, maven, cargo, go, swift)"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Get version
if [[ -z "$VERSION" ]]; then
    if [[ -f "$ROOT_DIR/VERSION" ]]; then
        VERSION=$(cat "$ROOT_DIR/VERSION")
    else
        echo -e "${RED}Error: No version specified and VERSION file not found${NC}"
        exit 1
    fi
fi

echo -e "${BLUE}â•”â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•—${NC}"
echo -e "${BLUE}â•‘           ThemisDB SDK Publishing v${VERSION}                    â•‘${NC}"
echo -e "${BLUE}â•šâ•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•${NC}"
echo ""

if [[ "$DRY_RUN" == "true" ]]; then
    echo -e "${YELLOW}âš ï¸  DRY RUN MODE - No packages will be published${NC}"
    echo ""
fi

# Track results
declare -A RESULTS

# Function to check if registry should be skipped
should_skip() {
    local registry="$1"
    if [[ "$SKIP_REGISTRIES" == *"$registry"* ]]; then
        return 0
    fi
    return 1
}

# Function to run publish script
publish_registry() {
    local name="$1"
    local script="$2"
    local registry="$3"
    
    if should_skip "$registry"; then
        echo -e "${YELLOW}â­ï¸  Skipping $name${NC}"
        RESULTS["$name"]="SKIPPED"
        return
    fi
    
    echo -e "${BLUE}â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”${NC}"
    echo -e "${BLUE}ðŸ“¦ Publishing $name...${NC}"
    echo -e "${BLUE}â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”â”${NC}"
    
    local args="--version $VERSION"
    if [[ "$DRY_RUN" == "true" ]]; then
        args="$args --dry-run"
    fi
    
    if [[ -f "$script" ]]; then
        if bash "$script" $args; then
            RESULTS["$name"]="SUCCESS"
            echo -e "${GREEN}âœ… $name published successfully${NC}"
        else
            RESULTS["$name"]="FAILED"
            echo -e "${RED}âŒ $name publishing failed${NC}"
        fi
    else
        RESULTS["$name"]="SCRIPT_NOT_FOUND"
        echo -e "${YELLOW}âš ï¸  Script not found: $script${NC}"
    fi
    echo ""
}

# Publish to each registry
publish_registry "JavaScript/TypeScript (NPM)" "$SCRIPT_DIR/publish-npm.sh" "npm"
publish_registry "Python (PyPI)" "$SCRIPT_DIR/publish-pypi.sh" "pypi"
publish_registry "C# (NuGet)" "$SCRIPT_DIR/publish-nuget.sh" "nuget"
publish_registry "Java (Maven Central)" "$SCRIPT_DIR/publish-maven.sh" "maven"
publish_registry "Rust (Crates.io)" "$SCRIPT_DIR/publish-crates.sh" "cargo"
publish_registry "Go (go.dev)" "$SCRIPT_DIR/publish-go.sh" "go"
publish_registry "Swift (Swift PM)" "$SCRIPT_DIR/publish-swift.sh" "swift"

# Summary
echo -e "${BLUE}â•”â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•—${NC}"
echo -e "${BLUE}â•‘                    Publishing Summary                       â•‘${NC}"
echo -e "${BLUE}â•šâ•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•${NC}"
echo ""

success_count=0
failed_count=0
skipped_count=0

for registry in "${!RESULTS[@]}"; do
    result="${RESULTS[$registry]}"
    case "$result" in
        SUCCESS)
            echo -e "  ${GREEN}âœ… $registry${NC}"
            ((success_count++))
            ;;
        FAILED)
            echo -e "  ${RED}âŒ $registry${NC}"
            ((failed_count++))
            ;;
        SKIPPED)
            echo -e "  ${YELLOW}â­ï¸  $registry (skipped)${NC}"
            ((skipped_count++))
            ;;
        *)
            echo -e "  ${YELLOW}âš ï¸  $registry ($result)${NC}"
            ((skipped_count++))
            ;;
    esac
done

echo ""
echo -e "Total: ${GREEN}$success_count success${NC}, ${RED}$failed_count failed${NC}, ${YELLOW}$skipped_count skipped${NC}"

if [[ $failed_count -gt 0 ]]; then
    exit 1
fi

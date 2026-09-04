#!/usr/bin/env bash
# version-release-type-manager.sh
# Manages atomic read/write operations on VERSION and RELEASE_TYPE files.
# Ensures consistency between version and release type across the repository.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VERSION_FILE="${REPO_ROOT}/VERSION"
RELEASE_TYPE_FILE="${REPO_ROOT}/RELEASE_TYPE"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Allowed release types
VALID_RELEASE_TYPES=("nightly" "alpha" "beta" "rc" "stable")

# Helper: check if value is in array
contains_element() {
    local element="$1"
    shift
    for item in "$@"; do
        [[ "$item" == "$element" ]] && return 0
    done
    return 1
}

# Command: read version
cmd_read_version() {
    if [[ ! -f "$VERSION_FILE" ]]; then
        echo "ERROR: VERSION file not found: $VERSION_FILE" >&2
        exit 1
    fi
    cat "$VERSION_FILE" | tr -d '[:space:]'
}

# Command: read release type
cmd_read_release_type() {
    if [[ ! -f "$RELEASE_TYPE_FILE" ]]; then
        echo "ERROR: RELEASE_TYPE file not found: $RELEASE_TYPE_FILE" >&2
        exit 1
    fi
    cat "$RELEASE_TYPE_FILE" | tr -d '[:space:]'
}

# Command: read both as JSON
cmd_read_json() {
    local version
    local release_type
    
    version=$(cmd_read_version)
    release_type=$(cmd_read_release_type)
    
    cat <<EOF
{
  "version": "$version",
  "release_type": "$release_type"
}
EOF
}

# Command: write version (atomic with temp + mv)
cmd_write_version() {
    local new_version="$1"
    
    if [[ -z "$new_version" ]]; then
        echo "ERROR: Version cannot be empty" >&2
        exit 1
    fi
    
    # Simple SemVer validation: X.Y.Z or X.Y.Z-prerelease
    if ! [[ "$new_version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9.]+)?$ ]]; then
        echo "ERROR: Invalid version format: $new_version (expected X.Y.Z or X.Y.Z-prerelease)" >&2
        exit 1
    fi
    
    local temp_file
    temp_file=$(mktemp)
    trap "rm -f '$temp_file'" EXIT
    
    echo "$new_version" > "$temp_file"
    mv "$temp_file" "$VERSION_FILE"
    
    echo -e "${GREEN}✓ VERSION updated to: $new_version${NC}"
}

# Command: write release type (atomic)
cmd_write_release_type() {
    local new_type="$1"
    
    if [[ -z "$new_type" ]]; then
        echo "ERROR: Release type cannot be empty" >&2
        exit 1
    fi
    
    if ! contains_element "$new_type" "${VALID_RELEASE_TYPES[@]}"; then
        echo "ERROR: Invalid release type: $new_type" >&2
        echo "       Valid types: ${VALID_RELEASE_TYPES[*]}" >&2
        exit 1
    fi
    
    local temp_file
    temp_file=$(mktemp)
    trap "rm -f '$temp_file'" EXIT
    
    echo "$new_type" > "$temp_file"
    mv "$temp_file" "$RELEASE_TYPE_FILE"
    
    echo -e "${GREEN}✓ RELEASE_TYPE updated to: $new_type${NC}"
}

# Command: write both atomically
cmd_write_both() {
    local new_version="$1"
    local new_type="$2"
    
    # Validate both first
    if ! [[ "$new_version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9.]+)?$ ]]; then
        echo "ERROR: Invalid version format: $new_version" >&2
        exit 1
    fi
    
    if ! contains_element "$new_type" "${VALID_RELEASE_TYPES[@]}"; then
        echo "ERROR: Invalid release type: $new_type" >&2
        exit 1
    fi
    
    # Write both
    cmd_write_version "$new_version"
    cmd_write_release_type "$new_type"
}

# Command: bump version
cmd_bump_version() {
    local bump_type="$1"  # major, minor, patch
    local current_version
    
    current_version=$(cmd_read_version)
    
    # Parse current version
    if [[ ! "$current_version" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+) ]]; then
        echo "ERROR: Cannot parse current version: $current_version" >&2
        exit 1
    fi
    
    local major="${BASH_REMATCH[1]}"
    local minor="${BASH_REMATCH[2]}"
    local patch="${BASH_REMATCH[3]}"
    
    case "$bump_type" in
        major)
            ((major++))
            minor=0
            patch=0
            ;;
        minor)
            ((minor++))
            patch=0
            ;;
        patch)
            ((patch++))
            ;;
        *)
            echo "ERROR: Invalid bump type: $bump_type (expected major, minor, or patch)" >&2
            exit 1
            ;;
    esac
    
    local new_version="${major}.${minor}.${patch}"
    cmd_write_version "$new_version"
    echo "$new_version"
}

# Command: generate nightly version
cmd_generate_nightly_version() {
    local base_version
    local date_suffix
    local run_number="${1:--}"
    
    base_version=$(cmd_read_version | sed 's/-.*$//')  # Remove any pre-release suffix
    date_suffix=$(date +%Y%m%d)
    
    # Format: v2.4.0-nightly.20260904 (or with run number: v2.4.0-nightly.20260904.5)
    if [[ "$run_number" != "-" ]]; then
        echo "${base_version}-nightly.${date_suffix}.${run_number}"
    else
        echo "${base_version}-nightly.${date_suffix}"
    fi
}

# Main
main() {
    local cmd="${1:-}"
    
    case "$cmd" in
        read-version)
            cmd_read_version
            ;;
        read-release-type)
            cmd_read_release_type
            ;;
        read-json)
            cmd_read_json
            ;;
        write-version)
            cmd_write_version "${2:-}"
            ;;
        write-release-type)
            cmd_write_release_type "${2:-}"
            ;;
        write-both)
            cmd_write_both "${2:-}" "${3:-}"
            ;;
        bump-version)
            cmd_bump_version "${2:-}"
            ;;
        generate-nightly-version)
            cmd_generate_nightly_version "${2:-}"
            ;;
        *)
            cat <<EOF
Usage: $0 <command> [args]

Commands:
  read-version                     Read current version
  read-release-type                Read current release type
  read-json                        Read both as JSON
  write-version <version>          Update VERSION file
  write-release-type <type>        Update RELEASE_TYPE file
  write-both <version> <type>      Update both files atomically
  bump-version <major|minor|patch> Bump version and echo new version
  generate-nightly-version [run#]  Generate nightly version string

Valid release types: ${VALID_RELEASE_TYPES[*]}

Examples:
  $0 read-version
  $0 write-version 2.4.1
  $0 write-release-type stable
  $0 bump-version minor
  $0 generate-nightly-version 5
EOF
            exit 1
            ;;
    esac
}

main "$@"

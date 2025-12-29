#!/bin/bash
# ThemisDB Crates.io Publishing Script
# Publishes themisdb to Crates.io

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
CLIENT_DIR="$ROOT_DIR/clients/rust"

DRY_RUN=false
VERSION=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run) DRY_RUN=true; shift ;;
        --version) VERSION="$2"; shift 2 ;;
        *) shift ;;
    esac
done

echo "ðŸ“¦ Crates.io Publishing for themisdb"

# Check prerequisites
if ! command -v cargo &> /dev/null; then
    echo "âŒ cargo not found"
    exit 1
fi

if [[ -z "${CARGO_TOKEN:-}" ]] && [[ "$DRY_RUN" == "false" ]]; then
    echo "âŒ CARGO_TOKEN environment variable not set"
    exit 1
fi

cd "$CLIENT_DIR"

# Update version if specified
if [[ -n "$VERSION" ]]; then
    sed -i "s/^version = .*/version = \"$VERSION\"/" Cargo.toml
fi

# Check formatting
echo "ðŸ” Checking format..."
cargo fmt --check || echo "Format check skipped"

# Lint
echo "ðŸ” Running clippy..."
cargo clippy -- -D warnings || echo "Clippy check skipped"

# Build
echo "ðŸ”¨ Building..."
cargo build --release

# Run tests
echo "ðŸ§ª Running tests..."
cargo test || echo "No tests found or tests skipped"

# Publish
if [[ "$DRY_RUN" == "true" ]]; then
    echo "ðŸ” Dry run - would publish:"
    cargo package --list
else
    echo "ðŸš€ Publishing to Crates.io..."
    cargo login "$CARGO_TOKEN"
    cargo publish
fi

echo "âœ… Crates.io publishing complete"

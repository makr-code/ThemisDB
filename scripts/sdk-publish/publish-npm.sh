#!/bin/bash
# ThemisDB NPM Publishing Script
# Publishes @themisdb/client to npm registry

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
CLIENT_DIR="$ROOT_DIR/clients/javascript"

DRY_RUN=false
VERSION=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run) DRY_RUN=true; shift ;;
        --version) VERSION="$2"; shift 2 ;;
        *) shift ;;
    esac
done

echo "ðŸ“¦ NPM Publishing for @themisdb/client"

# Check prerequisites
if ! command -v npm &> /dev/null; then
    echo "âŒ npm not found"
    exit 1
fi

if [[ -z "${NPM_TOKEN:-}" ]] && [[ "$DRY_RUN" == "false" ]]; then
    echo "âŒ NPM_TOKEN environment variable not set"
    exit 1
fi

cd "$CLIENT_DIR"

# Update version if specified
if [[ -n "$VERSION" ]]; then
    npm version "$VERSION" --no-git-tag-version --allow-same-version
fi

# Install dependencies
echo "ðŸ“¥ Installing dependencies..."
npm ci

# Build
echo "ðŸ”¨ Building..."
npm run build

# Run tests
echo "ðŸ§ª Running tests..."
npm test

# Publish
if [[ "$DRY_RUN" == "true" ]]; then
    echo "ðŸ” Dry run - would publish:"
    npm pack --dry-run
else
    echo "ðŸš€ Publishing to npm..."
    echo "//registry.npmjs.org/:_authToken=${NPM_TOKEN}" > ~/.npmrc
    npm publish --access public
fi

echo "âœ… NPM publishing complete"

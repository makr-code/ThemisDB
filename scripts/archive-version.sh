#!/bin/bash
# ThemisDB Source Code Archive Script
# Usage: ./archive-version.sh <version> <commit-sha> [edition] [artifact-kind] [arch]
# Example: ./archive-version.sh 1.0.0 60e901590e4b2e5990877b3c0f49cdcd2bb1f992 community sourcecode x64

set -e

# Check arguments
if [ "$#" -lt 2 ] || [ "$#" -gt 5 ]; then
    echo "Usage: $0 <version> <commit-sha> [edition] [artifact-kind] [arch]"
    echo "Example: $0 1.0.0 60e901590e4b2e5990877b3c0f49cdcd2bb1f992 community sourcecode x64"
    exit 1
fi

VERSION="$1"
COMMIT_SHA="$2"
EDITION="${3:-community}"
ARTIFACT_KIND="${4:-sourcecode}"
ARCH="${5:-x64}"

# Validate version format
if ! [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9.-]+)?$ ]]; then
    echo "Error: Invalid version format. Expected: X.Y.Z or X.Y.Z-suffix"
    exit 1
fi

# Validate commit exists
if ! git cat-file -e "$COMMIT_SHA^{commit}" 2>/dev/null; then
    echo "Error: Commit $COMMIT_SHA does not exist"
    exit 1
fi

# Validate release naming tokens
if ! [[ "$EDITION" =~ ^[a-z0-9-]+$ ]]; then
    echo "Error: Invalid edition '$EDITION'. Expected lowercase token (e.g., community, enterprise)."
    exit 1
fi

if ! [[ "$ARTIFACT_KIND" =~ ^(sourcecode|binary)$ ]]; then
    echo "Error: Invalid artifact kind '$ARTIFACT_KIND'. Expected: sourcecode or binary"
    exit 1
fi

if ! [[ "$ARCH" =~ ^(arm|x86|x64)$ ]]; then
    echo "Error: Invalid arch '$ARCH'. Expected one of: arm, x86, x64"
    exit 1
fi

OUTPUT_FILE="themisdb-${VERSION}-${EDITION}-${ARTIFACT_KIND}-${ARCH}.zip"
CHECKSUM_FILE="${OUTPUT_FILE}.sha256"

echo "Creating source archive for ThemisDB v${VERSION}"
echo "Commit: ${COMMIT_SHA}"
echo "Edition: ${EDITION}"
echo "Kind: ${ARTIFACT_KIND}"
echo "Arch: ${ARCH}"
echo "Output: ${OUTPUT_FILE}"
echo ""

# Create the archive
# Note: git archive excludes .git automatically
git archive --format=zip \
  --output="${OUTPUT_FILE}" \
  --prefix="themisdb-${VERSION}/" \
  "${COMMIT_SHA}" \
  -- . \
  ':!external/' \
  ':!vcpkg/' \
  ':!llama.cpp/' \
  ':!.git/'

if [ ! -f "${OUTPUT_FILE}" ]; then
    echo "Error: Failed to create archive"
    exit 1
fi

# Generate SHA256 checksum
echo "Generating SHA256 checksum..."
sha256sum "${OUTPUT_FILE}" > "${CHECKSUM_FILE}"

# Display results
FILESIZE=$(du -h "${OUTPUT_FILE}" | cut -f1)
CHECKSUM=$(cut -d' ' -f1 "${CHECKSUM_FILE}")

echo ""
echo "✓ Archive created successfully!"
echo "  File: ${OUTPUT_FILE}"
echo "  Size: ${FILESIZE}"
echo "  SHA256: ${CHECKSUM}"
echo ""
echo "Checksum saved to: ${CHECKSUM_FILE}"
echo ""
echo "Next steps:"
echo "  1. Upload ${OUTPUT_FILE} to GitHub Release"
echo "  2. Include the SHA256 checksum in release notes:"
echo "     ${CHECKSUM}"

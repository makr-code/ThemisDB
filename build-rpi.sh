#!/usr/bin/env bash
# Build Raspberry Pi ARM64 Docker image and extract binary
# Requires Docker with buildx support

set -euo pipefail

VERSION="${VERSION:-1.0.0}"
PLATFORM="${PLATFORM:-linux/arm64}"
PUSH="${PUSH:-false}"

echo "=== ThemisDB Raspberry Pi ARM64 Build ==="
echo ""
echo "Version: $VERSION"
echo "Platform: $PLATFORM"
echo ""

# Check for buildx
echo "Checking Docker buildx..."
if ! docker buildx version &>/dev/null; then
    echo "Error: Docker buildx not available"
    echo "Please install Docker with buildx support"
    exit 1
fi

# Create builder if needed
BUILDER_NAME="themis-multiarch"
if ! docker buildx ls | grep -q "$BUILDER_NAME"; then
    echo "Creating multiarch builder..."
    docker buildx create --name "$BUILDER_NAME" --use \
        --platform linux/amd64,linux/arm64,linux/arm/v7
else
    echo "Using existing builder: $BUILDER_NAME"
    docker buildx use "$BUILDER_NAME"
fi

# Build ARM64 image
echo ""
echo "Building ARM64 Docker image..."
echo "(This may take 30-60 minutes on first build)"
echo ""

IMAGE_NAME="themisdb/themisdb:${VERSION}-rpi"
IMAGE_LATEST="themisdb/themisdb:rpi"

BUILD_ARGS=(
    "buildx" "build"
    "--platform" "$PLATFORM"
    "--build-arg" "TARGETARCH=arm64"
    "--build-arg" "VCPKG_TRIPLET=arm64-linux"
    "--progress" "plain"
    "-t" "$IMAGE_NAME"
    "-t" "$IMAGE_LATEST"
    "-f" "Dockerfile"
)

if [ "$PUSH" = "true" ]; then
    BUILD_ARGS+=("--push")
else
    BUILD_ARGS+=("--load")
fi

BUILD_ARGS+=(".")

docker "${BUILD_ARGS[@]}"

echo ""
echo "ARM64 image built successfully: $IMAGE_NAME"

# Extract binary if not pushing
if [ "$PUSH" != "true" ]; then
    echo ""
    echo "Extracting binary from ARM64 image..."
    
    # Create temp container
    CONTAINER_ID=$(docker create --platform "$PLATFORM" "$IMAGE_NAME")
    
    # Extract binary
    OUTPUT_DIR="release/themisdb-${VERSION}-rpi-arm64"
    mkdir -p "$OUTPUT_DIR"
    
    docker cp "${CONTAINER_ID}:/usr/local/bin/themis_server" "$OUTPUT_DIR/themis_server_rpi_arm64"
    docker rm "$CONTAINER_ID" >/dev/null
    
    # Copy additional files
    cp LICENSE "$OUTPUT_DIR/"
    cp README.md "$OUTPUT_DIR/"
    cp -r config "$OUTPUT_DIR/"
    
    # Create INSTALL.txt
    cat > "$OUTPUT_DIR/INSTALL.txt" <<EOF
ThemisDB v${VERSION} - Raspberry Pi ARM64

Installation:
1. Ensure Raspberry Pi OS Bullseye/Bookworm (64-bit)
2. Install dependencies:
   sudo apt-get update
   sudo apt-get install libssl3 libcurl4 libyaml-cpp0.7

3. Make binary executable:
   chmod +x themis_server_rpi_arm64

4. Run:
   ./themis_server_rpi_arm64

Server listens on http://localhost:18765

System Requirements:
- Raspberry Pi 4/5 (recommended: 4GB+ RAM)
- Raspberry Pi OS 64-bit (Bullseye or Bookworm)
- 2GB+ free disk space
EOF
    
    # Create SHA256SUMS.txt
    cd "$OUTPUT_DIR"
    sha256sum themis_server_rpi_arm64 > SHA256SUMS.txt
    cd - >/dev/null
    
    # Create ZIP package
    echo ""
    echo "Creating release package..."
    ZIP_PATH="release/themisdb-${VERSION}-rpi-arm64.zip"
    (cd "$OUTPUT_DIR" && zip -r "../../${ZIP_PATH}" .)
    
    # Generate ZIP checksum
    sha256sum "$ZIP_PATH" > "${ZIP_PATH}.sha256"
    
    echo ""
    echo "=== Build Complete ==="
    echo ""
    echo "Package: $ZIP_PATH"
    echo "SHA256: $(cat ${ZIP_PATH}.sha256)"
    echo ""
    echo "Docker image: $IMAGE_NAME"
    echo ""
    
else
    echo ""
    echo "Image pushed to registry: $IMAGE_NAME"
    echo ""
fi

echo "To run on Raspberry Pi:"
echo "  docker run -d -p 18765:18765 -v /path/to/data:/data $IMAGE_NAME"
echo ""

#!/usr/bin/env bash
# setup-vcpkg-offline.sh
# ThemisDB vcpkg Offline Cache Setup (Linux/macOS)
# Downloads all dependencies for offline builds

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
GRAY='\033[0;37m'
NC='\033[0m' # No Color

# Configuration
TRIPLETS=("x64-linux" "arm64-linux")
SKIP_BOOTSTRAP=false
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --triplet)
            TRIPLETS=("$2")
            shift 2
            ;;
        --skip-bootstrap)
            SKIP_BOOTSTRAP=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
VCPKG_ROOT="$ROOT_DIR/vcpkg"
VCPKG_EXE="$VCPKG_ROOT/vcpkg"
DOWNLOADS_DIR="$VCPKG_ROOT/downloads"

echo -e "${CYAN}🚀 ThemisDB vcpkg Offline Cache Setup${NC}"
echo -e "${CYAN}═══════════════════════════════════════${NC}\n"

# 1. vcpkg Bootstrap
if [ "$SKIP_BOOTSTRAP" = false ]; then
    echo -e "${YELLOW}📦 Step 1/4: vcpkg Bootstrap${NC}"
    
    if [ ! -f "$VCPKG_EXE" ]; then
        echo -e "${GRAY}  ⚙️  Bootstrapping vcpkg...${NC}"
        cd "$VCPKG_ROOT"
        ./bootstrap-vcpkg.sh -disableMetrics
        cd "$ROOT_DIR"
        echo -e "${GREEN}  ✅ vcpkg bootstrapped${NC}"
    else
        echo -e "${GREEN}  ✅ vcpkg already bootstrapped${NC}"
    fi
else
    echo -e "${GRAY}📦 Step 1/4: vcpkg Bootstrap (skipped)${NC}"
fi

# 2. vcpkg Update
echo -e "\n${YELLOW}📦 Step 2/4: vcpkg Repository Update${NC}"
echo -e "${GRAY}  ⚙️  Pulling latest vcpkg registry...${NC}"

cd "$VCPKG_ROOT"
if git pull > /dev/null 2>&1; then
    echo -e "${GREEN}  ✅ vcpkg registry updated${NC}"
else
    echo -e "${YELLOW}  ⚠️  vcpkg update failed (continuing anyway)${NC}"
fi
cd "$ROOT_DIR"

# 3. Download Dependencies
echo -e "\n${YELLOW}📦 Step 3/4: Download Source Archives${NC}"

# Dependencies from vcpkg.json
CORE_DEPS=(
    "openssl"
    "rocksdb[lz4,zstd]"
    "simdjson"
    "tbb"
    "arrow[parquet,compute]"
    "hnswlib"
    "gtest"
    "benchmark"
    "boost-asio"
    "boost-beast"
    "spdlog"
    "nlohmann-json"
    "opentelemetry-cpp[otlp-http]"
    "curl"
    "yaml-cpp"
    "zstd"
    "mimalloc"
)

OPTIONAL_DEPS=(
    "faiss"      # GPU feature
    "grpc"       # RPC feature
    "protobuf"   # RPC feature
)

ALL_DEPS=("${CORE_DEPS[@]}" "${OPTIONAL_DEPS[@]}")

for triplet in "${TRIPLETS[@]}"; do
    echo -e "${CYAN}  📥 Downloading for triplet: $triplet${NC}"
    
    for dep in "${ALL_DEPS[@]}"; do
        if [ "$VERBOSE" = true ]; then
            echo -e "${GRAY}    - $dep${NC}"
        fi
        
        if ! "$VCPKG_EXE" x-download "$dep" --triplet "$triplet" > /dev/null 2>&1; then
            if [ "$VERBOSE" = true ]; then
                echo -e "${YELLOW}      ⚠️  Warning: $dep download failed${NC}"
            fi
        fi
    done
    
    echo -e "${GREEN}  ✅ Triplet $triplet complete${NC}"
done

# 4. Verify Cache
echo -e "\n${YELLOW}📦 Step 4/4: Verify Cache${NC}"

if [ -d "$DOWNLOADS_DIR" ]; then
    ARCHIVE_COUNT=$(find "$DOWNLOADS_DIR" -type f | wc -l | tr -d ' ')
    CACHE_SIZE=$(du -sh "$DOWNLOADS_DIR" | cut -f1)
    
    echo -e "${GREEN}  ✅ Cache ready:${NC}"
    echo -e "${GRAY}     - Archives: $ARCHIVE_COUNT${NC}"
    echo -e "${GRAY}     - Size: $CACHE_SIZE${NC}"
    echo -e "${GRAY}     - Location: $DOWNLOADS_DIR${NC}"
else
    echo -e "${YELLOW}  ⚠️  Warning: downloads/ directory not found${NC}"
fi

echo -e "\n${CYAN}═══════════════════════════════════════${NC}"
echo -e "${GREEN}✅ vcpkg Offline Cache Setup Complete!${NC}"
echo -e "\n${CYAN}💡 Next Steps:${NC}"
echo -e "${GRAY}   1. Build: cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake${NC}"
echo -e "${GRAY}   2. Compile: cmake --build build -j\$(nproc)${NC}"
echo -e "\n${GRAY}📚 Docs: docs/deployment/VCPKG_OFFLINE_STRATEGY.md${NC}\n"

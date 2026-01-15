#!/bin/bash
# Simple standalone test for signature verifier
# Compiles and runs a minimal test without full CMake build

set -e

echo "=== Building Standalone Signature Verifier Test ==="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Compile signature_verifier.cpp and a minimal test
g++ -std=c++17 -I../include -I/usr/include \
    -o test_signature_simple \
    test_signature_simple.cpp \
    ../src/llm/security/signature_verifier.cpp \
    -lssl -lcrypto -lspdlog -lpthread \
    -DTHEMIS_TEST_BUILD=1

echo ""
echo "=== Running Standalone Test ==="
./test_signature_simple

echo ""
echo "=== Test Complete ==="

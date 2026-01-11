#!/bin/bash
# ============================================================================
# Run Tests in Docker Environment
# ============================================================================
# Runs LoRA framework tests in isolated Docker containers
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCKER_DIR="$(dirname "$SCRIPT_DIR")"

echo "============================================"
echo "ThemisDB LoRA Framework - Test Runner"
echo "============================================"
echo ""

cd "$DOCKER_DIR"

# Start test environment
echo "Starting test environment..."
docker-compose -f docker-compose.yml -f docker-compose.test.yml up --build --abort-on-container-exit

TEST_EXIT_CODE=$?

# Cleanup
echo ""
echo "Cleaning up test environment..."
docker-compose -f docker-compose.yml -f docker-compose.test.yml down

echo ""
if [ $TEST_EXIT_CODE -eq 0 ]; then
  echo "============================================"
  echo "✓ All Tests Passed!"
  echo "============================================"
else
  echo "============================================"
  echo "✗ Tests Failed (exit code: $TEST_EXIT_CODE)"
  echo "============================================"
fi

exit $TEST_EXIT_CODE

#!/bin/bash
# Manual test script for Error API endpoints
# Run this after starting the ThemisDB server on localhost:8080

set -e

BASE_URL="${BASE_URL:-http://localhost:8080}"
echo "Testing Error API endpoints at ${BASE_URL}"
echo "================================================"
echo

# Test 1: List all errors
echo "Test 1: GET /api/v1/errors - List all registered errors"
echo "--------------------------------------------------------"
curl -s "${BASE_URL}/api/v1/errors" | jq '.' || echo "Failed"
echo
echo

# Test 2: Filter by category
echo "Test 2: GET /api/v1/errors?category=LLM - Filter by LLM category"
echo "----------------------------------------------------------------"
curl -s "${BASE_URL}/api/v1/errors?category=LLM" | jq '.' || echo "Failed"
echo
echo

# Test 3: Get specific error by code
echo "Test 3: GET /api/v1/errors/2000 - Get specific error details"
echo "------------------------------------------------------------"
curl -s "${BASE_URL}/api/v1/errors/2000" | jq '.' || echo "Failed"
echo
echo

# Test 4: Get non-existent error (should return 404)
echo "Test 4: GET /api/v1/errors/9999 - Non-existent error (expect 404)"
echo "------------------------------------------------------------------"
curl -s -w "\nHTTP Status: %{http_code}\n" "${BASE_URL}/api/v1/errors/9999" | jq '.' || echo "Failed"
echo
echo

# Test 5: List all categories
echo "Test 5: GET /api/v1/errors/categories - List all error categories"
echo "-------------------------------------------------------------------"
curl -s "${BASE_URL}/api/v1/errors/categories" | jq '.' || echo "Failed"
echo
echo

# Test 6: Search errors by keyword
echo "Test 6: GET /api/v1/errors/search?q=gpu - Search errors by keyword"
echo "--------------------------------------------------------------------"
curl -s "${BASE_URL}/api/v1/errors/search?q=gpu" | jq '.' || echo "Failed"
echo
echo

# Test 7: Search without query parameter (should return 400)
echo "Test 7: GET /api/v1/errors/search - Missing query (expect 400)"
echo "---------------------------------------------------------------"
curl -s -w "\nHTTP Status: %{http_code}\n" "${BASE_URL}/api/v1/errors/search" | jq '.' || echo "Failed"
echo
echo

echo "================================================"
echo "All manual tests completed!"
echo "================================================"

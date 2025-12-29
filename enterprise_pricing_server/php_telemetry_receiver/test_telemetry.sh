#!/bin/bash
# Test script for PHP telemetry receiver

# Configuration
BASE_URL="http://localhost/telemetry.php"  # Change for production
LICENSE_KEY="THEMIS-ENT-1234-TEST2"
INSTANCE_ID="test-instance-$(date +%s)"

echo "=== ThemisDB PHP Telemetry Receiver Test ==="
echo ""

# Test 1: Get API info
echo "Test 1: Get API Info"
curl -s "$BASE_URL" | jq .
echo ""

# Test 2: Send heartbeat
echo "Test 2: Send Heartbeat"
curl -s -X POST "$BASE_URL?action=heartbeat" \
  -H "Content-Type: application/json" \
  -d "{
    \"instance_id\": \"$INSTANCE_ID\",
    \"license_key\": \"$LICENSE_KEY\",
    \"metrics\": {
        \"nodes\": 5,
        \"cores\": 160,
        \"storage_tb\": 10.5,
        \"uptime_seconds\": 7200,
        \"query_count_24h\": 250000
    },
    \"server_info\": {
        \"hostname\": \"test-server\",
        \"version\": \"2.1.0\",
        \"location\": \"eu-west-1\"
    }
  }" | jq .
echo ""

# Test 3: Get statistics
echo "Test 3: Get Statistics"
curl -s "$BASE_URL?action=statistics" | jq .
echo ""

# Test 4: Get license instances
echo "Test 4: Get License Instances"
curl -s "$BASE_URL?action=license_instances&key=$LICENSE_KEY" | jq .
echo ""

# Test 5: Get instance details
echo "Test 5: Get Instance Details"
curl -s "$BASE_URL?action=instance&id=$INSTANCE_ID" | jq .
echo ""

# Test 6: Rate limiting (should fail)
echo "Test 6: Rate Limiting (should fail with 429)"
curl -s -X POST "$BASE_URL?action=heartbeat" \
  -H "Content-Type: application/json" \
  -d "{
    \"instance_id\": \"$INSTANCE_ID\",
    \"license_key\": \"$LICENSE_KEY\",
    \"metrics\": {
        \"nodes\": 5,
        \"cores\": 160,
        \"storage_tb\": 10.5,
        \"uptime_seconds\": 7200,
        \"query_count_24h\": 250000
    }
  }" | jq .
echo ""

echo "=== Tests Complete ==="

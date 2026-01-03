#!/bin/bash
# ThemisDB Multi-Shard RAID Benchmark Execution Script
# Usage: ./run_benchmark.sh <scenario> <raid_level> <num_shards>

set -e

SCENARIO=${1:-S1}
RAID_LEVEL=${2:-RAID10}
NUM_SHARDS=${3:-6}
DURATION_HOURS=${4:-4}

echo "============================================================================"
echo "ThemisDB Multi-Shard RAID Benchmark"
echo "============================================================================"
echo "Scenario: $SCENARIO"
echo "RAID Level: $RAID_LEVEL"
echo "Shards: $NUM_SHARDS"
echo "Duration: $DURATION_HOURS hours"
echo "============================================================================"
echo ""

# Determine workload type based on scenario
case $SCENARIO in
  S1|S2|S3)
    WORKLOAD_TYPE="OLTP"
    TARGET_QPS=10000
    DATA_SIZE_GB=100
    ;;
  S4)
    WORKLOAD_TYPE="Mixed"
    TARGET_QPS=30000
    DATA_SIZE_GB=500
    DURATION_HOURS=12
    ;;
  S5)
    WORKLOAD_TYPE="OLAP"
    TARGET_QPS=500
    DATA_SIZE_GB=1000
    DURATION_HOURS=18
    ;;
  S6)
    WORKLOAD_TYPE="TimeSeries"
    TARGET_QPS=50000
    DATA_SIZE_GB=1000
    DURATION_HOURS=24
    ;;
  S7)
    WORKLOAD_TYPE="VectorSearch"
    TARGET_QPS=5000
    DATA_SIZE_GB=500
    DURATION_HOURS=10
    ;;
  S8)
    WORKLOAD_TYPE="Mixed"
    TARGET_QPS=30000
    DATA_SIZE_GB=500
    DURATION_HOURS=16
    ;;
  *)
    echo "Unknown scenario: $SCENARIO"
    exit 1
    ;;
esac

# Export environment variables
export SCENARIO
export RAID_LEVEL
export NUM_SHARDS
export WORKLOAD_TYPE
export TARGET_QPS
export DATA_SIZE_GB
export DURATION_HOURS
export CLUSTER_NAME="themis-benchmark-${SCENARIO}"

# Determine Docker Compose profile
if [ $NUM_SHARDS -eq 3 ]; then
  PROFILE=""
elif [ $NUM_SHARDS -eq 6 ]; then
  PROFILE="--profile 6-shards"
elif [ $NUM_SHARDS -eq 12 ]; then
  PROFILE="--profile 12-shards"
elif [ $NUM_SHARDS -eq 24 ]; then
  PROFILE="--profile 24-shards"
else
  echo "Unsupported shard count: $NUM_SHARDS"
  exit 1
fi

echo "Step 1: Stopping any existing cluster..."
docker-compose -f docker-compose.multi-shard-raid.yml down -v 2>/dev/null || true

echo ""
echo "Step 2: Starting ThemisDB cluster ($NUM_SHARDS shards, $RAID_LEVEL)..."
docker-compose -f docker-compose.multi-shard-raid.yml up -d $PROFILE

echo ""
echo "Step 3: Waiting for cluster to be healthy (60 seconds)..."
sleep 60

echo ""
echo "Step 4: Loading test data ($DATA_SIZE_GB GB, ~$((DATA_SIZE_GB * 50000)) documents)..."
export DOCUMENTS_TOTAL=$((DATA_SIZE_GB * 50000))
docker-compose -f docker-compose.multi-shard-raid.yml --profile data-load up data-loader

echo ""
echo "Step 5: Starting benchmark workload ($DURATION_HOURS hours)..."
docker-compose -f docker-compose.multi-shard-raid.yml --profile benchmark up benchmark-controller

echo ""
echo "============================================================================"
echo "Benchmark completed!"
echo "Results saved to: ./results/${SCENARIO}_${RAID_LEVEL}_*.json"
echo "============================================================================"
echo ""
echo "Next steps:"
echo "  - View Grafana dashboard: http://localhost:3000 (admin/admin)"
echo "  - View Prometheus: http://localhost:9090"
echo "  - Analyze results: python analyze_results.py ./results/${SCENARIO}_${RAID_LEVEL}_*.json"
echo ""
echo "To stop the cluster:"
echo "  docker-compose -f docker-compose.multi-shard-raid.yml down"
echo ""

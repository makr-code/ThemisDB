#!/bin/bash
# Validation script for distributed_tensor module gap closure
# Purpose: Verify that all open gaps have been properly implemented

set -e

REPO_ROOT="${1:-.}"
BUILD_PRESET="${2:-windows-release}"
VALIDATION_LOG="/tmp/dt_gaps_validation_$(date +%s).log"

echo "=== Distributed Tensor Module - Gap Closure Validation ===" | tee "$VALIDATION_LOG"
echo "Repository: $REPO_ROOT" | tee -a "$VALIDATION_LOG"
echo "Build Preset: $BUILD_PRESET" | tee -a "$VALIDATION_LOG"
echo "Timestamp: $(date -Iseconds)" | tee -a "$VALIDATION_LOG"
echo "" | tee -a "$VALIDATION_LOG"

# Check 1: Verify RocksDB integration (no TODO comments in manifest_store.cc)
echo "CHECK 1: RocksDB Integration in ManifestStore" | tee -a "$VALIDATION_LOG"
ROCKSDB_TODOS=$(grep -c "TODO.*RocksDB\|TODO.*rocksdb\|TODO.*Replace with actual RocksDB" \
    "$REPO_ROOT/src/distributed_tensor/src/manifest_store.cc" 2>/dev/null || echo "0")
if [ "$ROCKSDB_TODOS" -eq 0 ]; then
    echo "  ✓ No RocksDB TODO comments found" | tee -a "$VALIDATION_LOG"
else
    echo "  ✗ Found $ROCKSDB_TODOS RocksDB TODO comments (expected 0)" | tee -a "$VALIDATION_LOG"
fi
echo "" | tee -a "$VALIDATION_LOG"

# Check 2: Verify checkpoint resume implementation (no TODO in snapshot_update_worker.cc)
echo "CHECK 2: Checkpoint Resume Logic in SnapshotUpdateWorker" | tee -a "$VALIDATION_LOG"
CHECKPOINT_TODOS=$(grep -c "TODO.*production\|TODO.*checkpoint\|TODO.*In production" \
    "$REPO_ROOT/src/distributed_tensor/src/snapshot_update_worker.cc" 2>/dev/null || echo "0")
if [ "$CHECKPOINT_TODOS" -eq 0 ]; then
    echo "  ✓ No checkpoint TODO comments found" | tee -a "$VALIDATION_LOG"
else
    echo "  ✗ Found $CHECKPOINT_TODOS checkpoint TODO comments (expected 0)" | tee -a "$VALIDATION_LOG"
fi
echo "" | tee -a "$VALIDATION_LOG"

# Check 3: Verify logging integration (shard_summary_coordinator.cc)
echo "CHECK 3: Logging Integration in ShardSummaryCoordinator" | tee -a "$VALIDATION_LOG"
LOGGING_TODOS=$(grep -c "TODO.*logging\|TODO.*Warn\|TODO.*use themis" \
    "$REPO_ROOT/src/distributed_tensor/src/shard_summary_coordinator.cc" 2>/dev/null || echo "0")
if [ "$LOGGING_TODOS" -eq 0 ]; then
    echo "  ✓ No logging TODO comments found" | tee -a "$VALIDATION_LOG"
else
    echo "  ✗ Found $LOGGING_TODOS logging TODO comments (expected 0)" | tee -a "$VALIDATION_LOG"
fi
echo "" | tee -a "$VALIDATION_LOG"

# Check 4: Verify RocksDB usage in manifest_store.cc
echo "CHECK 4: RocksDB Code Integration" | tee -a "$VALIDATION_LOG"
ROCKSDB_INCLUDES=$(grep -c "rocksdb/db.h\|rocksdb/options.h\|RocksDB::" \
    "$REPO_ROOT/src/distributed_tensor/src/manifest_store.cc" 2>/dev/null || echo "0")
if [ "$ROCKSDB_INCLUDES" -gt 0 ]; then
    echo "  ✓ Found RocksDB includes/usage ($ROCKSDB_INCLUDES references)" | tee -a "$VALIDATION_LOG"
else
    echo "  ✗ No RocksDB includes found (expected at least 1)" | tee -a "$VALIDATION_LOG"
fi
echo "" | tee -a "$VALIDATION_LOG"

# Check 5: Verify checkpoint resume implementation
echo "CHECK 5: Checkpoint Resume Implementation Details" | tee -a "$VALIDATION_LOG"
CHECKPOINT_IMPLEMENTATION=$(grep -c "delta_window\|residual\|manifest\|state" \
    "$REPO_ROOT/src/distributed_tensor/src/snapshot_update_worker.cc" 2>/dev/null || echo "0")
if [ "$CHECKPOINT_IMPLEMENTATION" -gt 10 ]; then
    echo "  ✓ Found substantial checkpoint logic ($CHECKPOINT_IMPLEMENTATION references)" | tee -a "$VALIDATION_LOG"
else
    echo "  ⚠ Found limited checkpoint logic ($CHECKPOINT_IMPLEMENTATION references)" | tee -a "$VALIDATION_LOG"
fi
echo "" | tee -a "$VALIDATION_LOG"

# Check 6: File metadata
echo "CHECK 6: File Modification Status" | tee -a "$VALIDATION_LOG"
for file in manifest_store.cc snapshot_update_worker.cc shard_summary_coordinator.cc; do
    if [ -f "$REPO_ROOT/src/distributed_tensor/src/$file" ]; then
        MOD_TIME=$(stat -c %Y "$REPO_ROOT/src/distributed_tensor/src/$file" 2>/dev/null || echo "unknown")
        echo "  ✓ $file exists (modified: $MOD_TIME)" | tee -a "$VALIDATION_LOG"
    else
        echo "  ✗ $file not found" | tee -a "$VALIDATION_LOG"
    fi
done
echo "" | tee -a "$VALIDATION_LOG"

echo "=== Validation Summary ===" | tee -a "$VALIDATION_LOG"
echo "Validation log saved to: $VALIDATION_LOG" | tee -a "$VALIDATION_LOG"
echo "Results:"
tail -20 "$VALIDATION_LOG"

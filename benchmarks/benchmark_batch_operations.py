#!/usr/bin/env python3
"""
Batch Operations Performance Benchmark

This script benchmarks different batch operation modes to demonstrate
the performance improvements in ThemisDB v1.5.0.

Requirements:
    pip install requests

Usage:
    python benchmark_batch_operations.py
"""

import requests
import json
import time
import sys
from typing import List, Dict

# Configuration
THEMISDB_URL = "http://localhost:8529"
NUM_ITEMS = 10000
BATCH_SIZES = [100, 500, 1000, 5000]
DURABILITY_MODES = ["sync", "async"]  # "no_sync" not tested for safety


def generate_test_data(count: int) -> List[Dict]:
    """Generate test entities."""
    return [
        {
            "id": i,
            "name": f"User {i}",
            "email": f"user{i}@example.com",
            "age": 20 + (i % 50),
            "status": "active" if i % 2 == 0 else "pending"
        }
        for i in range(count)
    ]


def batch_insert(entities: List[Dict], batch_size: int, durability: str) -> Dict:
    """
    Perform batch insert with specified configuration.
    
    Returns:
        dict: Statistics including throughput and latency
    """
    operations = [
        {
            "op": "put",
            "key": f"bench_users:{entity['id']}",
            "blob": json.dumps(entity)
        }
        for entity in entities
    ]
    
    total_batches = 0
    total_time = 0.0
    total_succeeded = 0
    total_failed = 0
    
    for i in range(0, len(operations), batch_size):
        batch = operations[i:i+batch_size]
        
        start = time.time()
        try:
            response = requests.post(
                f"{THEMISDB_URL}/entities/batch",
                json={
                    "operations": batch,
                    "options": {
                        "durability": durability
                    }
                },
                timeout=30
            )
            response.raise_for_status()
            result = response.json()
            
            total_batches += 1
            total_time += (time.time() - start)
            total_succeeded += result.get("succeeded", 0)
            total_failed += result.get("failed", 0)
            
        except Exception as e:
            print(f"  ❌ Error in batch {i//batch_size + 1}: {e}")
            total_failed += len(batch)
    
    return {
        "total_items": len(operations),
        "total_batches": total_batches,
        "total_succeeded": total_succeeded,
        "total_failed": total_failed,
        "total_time_sec": total_time,
        "avg_throughput": total_succeeded / total_time if total_time > 0 else 0,
        "avg_latency_ms": (total_time / total_batches * 1000) if total_batches > 0 else 0
    }


def cleanup_test_data():
    """Clean up test data."""
    print("\n🧹 Cleaning up test data...")
    try:
        # This would require a range delete endpoint or batch delete
        # For now, we'll skip cleanup in the benchmark
        pass
    except Exception as e:
        print(f"  ⚠️  Cleanup warning: {e}")


def print_results(results: Dict[str, Dict]):
    """Print benchmark results in a formatted table."""
    print("\n" + "="*80)
    print("📊 Batch Operations Performance Benchmark Results")
    print("="*80)
    print(f"Items per test: {NUM_ITEMS:,}")
    print(f"ThemisDB URL: {THEMISDB_URL}")
    print("="*80)
    
    # Print header
    print(f"\n{'Mode':<10} {'Batch Size':<12} {'Throughput':<20} {'Latency':<15} {'Success Rate'}")
    print("-"*80)
    
    # Print results
    for key, stats in sorted(results.items()):
        mode, batch_size = key.split("_")
        throughput = f"{stats['avg_throughput']:.1f} items/sec"
        latency = f"{stats['avg_latency_ms']:.1f} ms"
        success_rate = f"{stats['total_succeeded']/stats['total_items']*100:.1f}%"
        
        print(f"{mode:<10} {batch_size:<12} {throughput:<20} {latency:<15} {success_rate}")
    
    # Print comparison
    print("\n" + "="*80)
    print("Performance Comparison (vs sync mode, batch_size=100)")
    print("="*80)
    
    baseline_key = "sync_100"
    if baseline_key in results:
        baseline_throughput = results[baseline_key]['avg_throughput']
        
        for key, stats in sorted(results.items()):
            if key == baseline_key:
                continue
            
            speedup = stats['avg_throughput'] / baseline_throughput
            mode, batch_size = key.split("_")
            print(f"{mode:<10} (batch={batch_size:<5}): {speedup:6.2f}x faster")
    
    print("="*80)


def check_server():
    """Check if ThemisDB server is accessible."""
    try:
        response = requests.get(f"{THEMISDB_URL}/_admin/version", timeout=5)
        if response.status_code == 200:
            print(f"✅ ThemisDB server accessible at {THEMISDB_URL}")
            return True
    except Exception as e:
        print(f"❌ Cannot connect to ThemisDB at {THEMISDB_URL}")
        print(f"   Error: {e}")
        print("\n💡 Make sure ThemisDB is running:")
        print("   ./themisdb --config themisdb.conf")
        return False


def main():
    """Run the benchmark."""
    print("="*80)
    print("ThemisDB Batch Operations Performance Benchmark")
    print("="*80)
    
    # Check server
    if not check_server():
        sys.exit(1)
    
    # Generate test data
    print(f"\n📝 Generating {NUM_ITEMS:,} test entities...")
    entities = generate_test_data(NUM_ITEMS)
    print(f"   ✅ Generated {len(entities):,} entities")
    
    # Run benchmarks
    results = {}
    total_tests = len(BATCH_SIZES) * len(DURABILITY_MODES)
    current_test = 0
    
    for durability in DURABILITY_MODES:
        for batch_size in BATCH_SIZES:
            current_test += 1
            key = f"{durability}_{batch_size}"
            
            print(f"\n[{current_test}/{total_tests}] Testing: {durability} mode, batch_size={batch_size}")
            print(f"  ⏳ Inserting {NUM_ITEMS:,} items...")
            
            stats = batch_insert(entities, batch_size, durability)
            results[key] = stats
            
            print(f"  ✅ Completed in {stats['total_time_sec']:.2f}s")
            print(f"  ⚡ Throughput: {stats['avg_throughput']:.1f} items/sec")
            print(f"  ⏱️  Avg latency: {stats['avg_latency_ms']:.1f} ms/batch")
            
            # Small delay between tests
            time.sleep(1)
    
    # Print results
    print_results(results)
    
    # Cleanup
    cleanup_test_data()
    
    print("\n✅ Benchmark complete!")
    print("\n💡 Tips:")
    print("   - Use async mode for production (2-5x faster than sync)")
    print("   - Batch size of 500-1000 is optimal for most workloads")
    print("   - See docs/knowledge-base/BATCH_OPERATIONS_GUIDE.md for more details")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  Benchmark interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Benchmark failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

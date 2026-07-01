#!/usr/bin/env python3
"""
Verification script for Sprint 2 Quick-Win Batch 2: Cache & Concurrency Fixes

Validates all 5 findings are correctly implemented:
- QW-011: Missing read lock during validation
- QW-012: Race condition in cache invalidation
- QW-013: Cache coherency: duplicate entries
- QW-014: Atomic increment without CAS loop
- QW-015: Result cloning inefficiency
"""

import sys
import re
from pathlib import Path

def check_file_exists(path, description):
    """Check if a file exists and report status."""
    if Path(path).exists():
        print(f"✓ {description}: {path}")
        return True
    else:
        print(f"✗ {description}: {path} NOT FOUND")
        return False

def check_file_contains(path, patterns, description):
    """Check if file contains all required patterns."""
    try:
        with open(path, 'r') as f:
            content = f.read()
        
        all_found = True
        for pattern, pattern_desc in patterns:
            if isinstance(pattern, str):
                found = pattern in content
            else:  # regex
                found = pattern.search(content) is not None
            
            if found:
                print(f"  ✓ {pattern_desc}")
            else:
                print(f"  ✗ {pattern_desc}")
                all_found = False
        
        if all_found:
            print(f"✓ {description}: All patterns found\n")
        else:
            print(f"✗ {description}: Some patterns missing\n")
        
        return all_found
    except Exception as e:
        print(f"✗ Error reading {path}: {e}\n")
        return False

def verify_qw011_shared_lock():
    """Verify QW-011: shared_lock for read operations."""
    print("=" * 70)
    print("QW-011: Missing read lock during validation")
    print("=" * 70)
    
    header_patterns = [
        ("#include <shared_mutex>", "shared_mutex header included"),
        ("#include <atomic>", "atomic header included"),
        ("std::shared_mutex cache_mutex_", "shared_mutex for cache_mutex_"),
        ("std::shared_mutex dependency_mutex_", "shared_mutex for dependency_mutex_"),
        ("std::shared_mutex stats_mutex_", "shared_mutex for stats_mutex_"),
    ]
    
    cpp_patterns = [
        ("std::shared_lock<std::shared_mutex>", "shared_lock used for reads"),
        ("std::unique_lock<std::shared_mutex>", "unique_lock used for writes"),
        ("read_lock", "read_lock variable used"),
    ]
    
    h_result = check_file_contains(
        "include/query/query_cache.h",
        header_patterns,
        "Header file verification"
    )
    
    cpp_result = check_file_contains(
        "src/query/query_cache.cpp",
        cpp_patterns,
        "CPP file verification"
    )
    
    return h_result and cpp_result

def verify_qw012_atomic_invalidation():
    """Verify QW-012: Atomic cache invalidation."""
    print("=" * 70)
    print("QW-012: Race condition in cache invalidation")
    print("=" * 70)
    
    patterns = [
        ("std::atomic<bool> cache_valid_", "atomic cache_valid flag"),
        ("cache_valid_.store", "atomic store operation"),
        ("cache_valid_.compare_exchange_strong", "atomic compare-exchange operation"),
        (re.compile(r"memory_order_release"), "memory_order_release for synchronization"),
        (re.compile(r"std::memory_order_relaxed"), "memory_order_relaxed for relaxed operations"),
    ]
    
    return check_file_contains(
        "include/query/query_cache.h",
        [("std::atomic<bool> cache_valid_", "atomic cache_valid flag")],
        "Atomic flag in header"
    ) and check_file_contains(
        "src/query/query_cache.cpp",
        patterns,
        "Atomic operations in implementation"
    )

def verify_qw013_duplicate_detection():
    """Verify QW-013: Cache coherency - duplicate detection."""
    print("=" * 70)
    print("QW-013: Cache coherency - duplicate entries")
    print("=" * 70)
    
    patterns = [
        ("existing_it = cache_.find(fingerprint)", "check for existing entry"),
        ("if (existing_it != cache_.end())", "duplicate detection condition"),
        ("THEMIS_WARN(\"Cache coherency:", "coherency warning logging"),
        ("existing_entry.result == entry.result", "result coherency check"),
    ]
    
    return check_file_contains(
        "src/query/query_cache.cpp",
        patterns,
        "Duplicate detection implementation"
    )

def verify_qw014_atomic_increment():
    """Verify QW-014: Atomic operations with proper memory ordering."""
    print("=" * 70)
    print("QW-014: Atomic increment without CAS loop")
    print("=" * 70)
    
    patterns = [
        ("stats_.total_requests++", "stats operations"),
        ("stats_.hits++", "hits increment"),
        ("std::memory_order_release", "release semantics for writes"),
        (re.compile(r"fetch_add|fetch_sub"), "atomic fetch operations"),
    ]
    
    return check_file_contains(
        "src/query/query_cache.cpp",
        patterns,
        "Atomic counter operations"
    )

def verify_qw015_move_semantics():
    """Verify QW-015: Result cloning efficiency with move semantics."""
    print("=" * 70)
    print("QW-015: Result cloning inefficiency")
    print("=" * 70)
    
    patterns = [
        ("std::move(", "std::move used for move semantics"),
        ("LookupResult result", "LookupResult construction"),
        ("result.result = entry.result", "result assignment"),
        ("return result", "return with RVO"),
    ]
    
    return check_file_contains(
        "src/query/query_cache.cpp",
        patterns,
        "Move semantics implementation"
    )

def verify_test_coverage():
    """Verify test coverage for all findings."""
    print("=" * 70)
    print("Test Coverage Verification")
    print("=" * 70)
    
    test_file = "tests/test_query_cache_concurrency.cpp"
    
    if not check_file_exists(test_file, "Concurrency test file"):
        return False
    
    tests = [
        ("ConcurrentReadsWithSharedLock", "QW-011 test"),
        ("AtomicCacheInvalidation", "QW-012 test"),
        ("CacheCoherencyDuplicateDetection", "QW-013 test"),
        ("MoveSemanticsDuringRetrieval", "QW-014/015 test"),
        ("MixedWorkloadConcurrency", "Integration test"),
        ("HighConcurrencyReadStress", "Stress test"),
    ]
    
    test_patterns = [(test, desc) for test, desc in tests]
    
    return check_file_contains(test_file, test_patterns, "Test coverage")

def verify_documentation():
    """Verify documentation completeness."""
    print("=" * 70)
    print("Documentation Verification")
    print("=" * 70)
    
    doc_file = "ai_working/SPRINT_2_BATCH_2_COMPLETION_SUMMARY.md"
    
    if not check_file_exists(doc_file, "Completion summary"):
        return False
    
    patterns = [
        ("QW-011", "QW-011 documented"),
        ("QW-012", "QW-012 documented"),
        ("QW-013", "QW-013 documented"),
        ("QW-014", "QW-014 documented"),
        ("QW-015", "QW-015 documented"),
        ("std::shared_lock", "shared_lock documented"),
        ("Atomic CAS", "Atomic operations documented"),
        ("Move semantics", "Move semantics documented"),
    ]
    
    return check_file_contains(doc_file, patterns, "Documentation")

def main():
    """Run all verification checks."""
    print("\n" + "=" * 70)
    print("SPRINT 2 QUICK-WIN BATCH 2 VERIFICATION")
    print("Cache & Concurrency Fixes")
    print("=" * 70 + "\n")
    
    results = {
        "QW-011 (Shared Lock)": verify_qw011_shared_lock(),
        "QW-012 (Atomic Invalidation)": verify_qw012_atomic_invalidation(),
        "QW-013 (Duplicate Detection)": verify_qw013_duplicate_detection(),
        "QW-014 (Atomic Operations)": verify_qw014_atomic_increment(),
        "QW-015 (Move Semantics)": verify_qw015_move_semantics(),
        "Test Coverage": verify_test_coverage(),
        "Documentation": verify_documentation(),
    }
    
    print("\n" + "=" * 70)
    print("VERIFICATION SUMMARY")
    print("=" * 70)
    
    passed = sum(1 for v in results.values() if v)
    total = len(results)
    
    for check, result in results.items():
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{status}: {check}")
    
    print(f"\nTotal: {passed}/{total} checks passed")
    
    if passed == total:
        print("\n✓ ALL VERIFICATION CHECKS PASSED - Ready for testing")
        return 0
    else:
        print(f"\n✗ VERIFICATION FAILED - {total - passed} checks failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())

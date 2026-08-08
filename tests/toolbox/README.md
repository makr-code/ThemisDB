# tests/toolbox - Toolbox Module Tests

## Overview

This directory contains comprehensive test coverage for the toolbox module, organized into 4 focused test targets covering contract validation, ingestion workflows, performance gates, and component primitives.

- **Total Test Cases:** 96+ tests
- **Test Framework:** Google Test (GTest)
- **Build Target:** `test_toolbox_*` (multiple specialized targets)
- **Estimated Runtime:** ~30-60 seconds (full suite on typical hardware)

---

## Test Targets

### 1. test_toolbox_contract_hardening_focused

**File:** `test_toolbox_contract_hardening_focused.cpp`

**Purpose:** Validate builder, registry, and bridge API contracts with fail-fast semantics and null-check hardening.

**Key Test Cases:**
- `BuilderFailFastOnNullDependencies` - builder rejects null ContentManager
- `BuilderFailFastOnNullTextExtractor` - builder rejects null TextExtractor
- `BridgeNullChecksOnInvalidWriter` - bridge null-checks graph/vector writers
- `BridgeExplicitErrorMessagesOnFailure` - bridge logs descriptive errors
- `RegistryInitializationGuard` - registry throws if accessed before initialize()
- `RegistryResetDuringActive` - registry handles concurrent reset safely

**Run Command:**
```bash
# Run focused contract/hardening tests
ctest --preset linux-release -R test_toolbox_contract_hardening_focused -VV

# Or directly
./build/linux-release/tests/toolbox/test_toolbox_contract_hardening_focused

# Run specific test case
./build/linux-release/tests/toolbox/test_toolbox_contract_hardening_focused --gtest_filter="*BuilderFailFast*"
```

**Expected Results:** All tests PASS; 0 failures

### 2. test_toolbox_ingestion

**File:** `test_toolbox_ingestion.cpp`

**Purpose:** Test real-world ingestion workflows including extraction, bridge enrichment, and registry behavior.

**Key Test Cases:**
- `IngestSimpleContent` - basic ingestion workflow
- `IngestMixedContent` - handle mixed text/binary content
- `BridgeEnrichmentWorkflow` - end-to-end content enrichment
- `EmptyExtractionHandling` - graceful handling of empty results
- `ConcurrentIngestion` - thread-safe extraction under load

**Run Command:**
```bash
# Run ingestion workflow tests
ctest --preset linux-release -R test_toolbox_ingestion -VV

# Or directly
./build/linux-release/tests/toolbox/test_toolbox_ingestion

# Run specific fixture
./build/linux-release/tests/toolbox/test_toolbox_ingestion --gtest_filter="*MixedContent*"
```

**Expected Results:** All tests PASS; 0 failures

### 3. test_toolbox_phase5

**File:** `test_toolbox_phase5.cpp`

**Purpose:** Performance gate validation and stress testing (Phase 5 hardening).

**Key Test Cases (Performance Gates):**
- `GATE-TBX-P1: ExtractEntities_Throughput` - ≥100K ops/s
- `GATE-TBX-P2: ExtractEntitySet_Latency` - p95 ≤ 50ms
- `GATE-TBX-P3: TextNormalization_Latency` - p95 ≤ 10ms
- `GATE-TBX-P4: LanguageDetection_Latency` - p95 ≤ 15ms
- `GATE-TBX-P5: ContentFingerprinting_Throughput` - ≥1M ops/s
- `GATE-TBX-P6: BridgeEnrichment_Latency` - p95 ≤ 100ms

**Key Test Cases (Stress Fixtures):**
- `HighConcurrencyFixture` - 8 threads × 100 ops/thread (800 total operations)
- `MixedContentFixture` - varied text, binary, empty content
- `DegradedPathFixture` - soft-fail behavior under error conditions
- `LongRunFixture` - sustained operations over time

**Run Command:**
```bash
# Run all Phase 5 tests (gates + stress)
ctest --preset linux-release -R test_toolbox_phase5 -VV

# Or directly
./build/linux-release/tests/toolbox/test_toolbox_phase5

# Run only performance gates
./build/linux-release/tests/toolbox/test_toolbox_phase5 --gtest_filter="*Gate*"

# Run only stress tests
./build/linux-release/tests/toolbox/test_toolbox_phase5 --gtest_filter="*Fixture*"

# Run specific gate (e.g., P1 throughput)
./build/linux-release/tests/toolbox/test_toolbox_phase5 --gtest_filter="*GATE_TBX_P1*"
```

**Expected Results:**
- All gates PASS (performance thresholds met)
- All stress fixtures complete without deadlocks
- No data races (if run with thread sanitizer)

### 4. test_toolbox_primitives

**File:** `test_toolbox_primitives.cpp`

**Purpose:** Unit tests for helper components (text chunker, normalizer, language detector, quality scorer, fingerprinter).

**Key Test Cases:**

**Text Chunker:**
- `ChunkingPreservesSemantics` - chunks maintain word boundaries
- `ChunkingRespectsMaxSize` - output chunks ≤ max_chunk_size
- `ChunkingHandlesEmptyInput` - graceful empty input handling

**Text Normalizer:**
- `NormalizationConsistency` - same input → same output
- `NormalizationHandlesEncodingErrors` - fallback to UTF-8
- `NormalizationPreservesMeaning` - semantic preservation after normalization

**Language Detector:**
- `DetectorIdentifiesCommonLanguages` - English, German, etc.
- `DetectorReturnsConfidenceScores` - confidence 0.0-1.0
- `DetectorHandlesUnknownLanguages` - fallback behavior

**Text Quality Scorer:**
- `QualityMetricsInBounds` - scores 0.0-1.0
- `QualityScoresCorrelateWithReadability` - higher scores for readable text
- `QualityHandlesEdgeCases` - empty text, malformed input

**Content Fingerprinter:**
- `FingerprintingConsistency` - same input → same fingerprint
- `FingerprintingDetectsDuplicates` - identical content → same hash
- `FingerprintingCollisionRate` - extremely low collision probability

**Run Command:**
```bash
# Run all primitive component tests
ctest --preset linux-release -R test_toolbox_primitives -VV

# Or directly
./build/linux-release/tests/toolbox/test_toolbox_primitives

# Run specific component (e.g., text chunker)
./build/linux-release/tests/toolbox/test_toolbox_primitives --gtest_filter="*Chunk*"

# Run specific component (e.g., language detector)
./build/linux-release/tests/toolbox/test_toolbox_primitives --gtest_filter="*Detector*"
```

**Expected Results:** All tests PASS; edge cases properly handled

---

## Running All Toolbox Tests

### Run All Tests (Recommended)

```bash
# Configure release build
cmake --preset linux-release

# Build all toolbox test targets
cmake --build --preset linux-release --target test_toolbox_contract_hardening_focused test_toolbox_ingestion test_toolbox_phase5 test_toolbox_primitives

# Run all tests via ctest
ctest --preset linux-release -L toolbox --output-on-failure

# Or run via ctest with verbose output
ctest --preset linux-release -L toolbox -VV
```

### Run Specific Test Target

```bash
# Example: Run only contract hardening tests
ctest --preset linux-release -R test_toolbox_contract_hardening_focused -VV

# Example: Run contract + ingestion tests
ctest --preset linux-release -R "test_toolbox_(contract_hardening|ingestion)" -VV
```

### Run Specific Test Case

```bash
# Run a single test by name
./build/linux-release/tests/toolbox/test_toolbox_primitives --gtest_filter="ChunkingPreservesSemantics"

# Run tests matching pattern
./build/linux-release/tests/toolbox/test_toolbox_primitives --gtest_filter="*Chunking*"

# Run tests from specific fixture
./build/linux-release/tests/toolbox/test_toolbox_phase5 --gtest_filter="HighConcurrencyFixture.*"
```

### Test Output and Reporting

```bash
# Run with XML output (for CI/CD integration)
ctest --preset linux-release -L toolbox --output-on-failure \
  -DCTEST_OUTPUT_FORMAT=xml

# Run with JSON output
./build/linux-release/tests/toolbox/test_toolbox_primitives --gtest_output=json:test_results.json

# Generate test coverage report (if built with --coverage)
cmake --build --preset linux-release --target gcov
# Results in: build/linux-release/coverage/
```

---

## Test Data

### Standard Test Text Samples

**kShortText (~100 chars):**
```
"The quick brown fox jumps over the lazy dog. This sentence contains every letter of the English alphabet."
```

**kMediumText (~1KB):**
```
Lorem ipsum dolor sit amet, consectetur adipiscing elit... (1000+ characters)
```

**MakeMediumText() Factory:**
```cpp
std::string MakeMediumText(size_t size_bytes = 1024) {
  // Generates semantically meaningful text of specified size
  // Used for consistent benchmark data
}
```

### Test Fixtures

- **HighConcurrencyFixture:** 8 concurrent threads, 800 operations total
- **MixedContentFixture:** UTF-8, binary, empty, malformed inputs
- **DegradedPathFixture:** Soft-fail error conditions
- **LongRunFixture:** 100K+ sustained operations

---

## Incident Taxonomy Coverage (IT-13..IT-20)

Tests IT-13 through IT-20 cover comprehensive edge cases per the unified incident taxonomy:

| Test | Incident Code | Scenario | Expected Behavior |
|------|---------------|----------|-------------------|
| IT-13 | EX-EMPTY | No text extracted | Empty result recorded in metrics |
| IT-14 | EX-FAILED | Extraction processor error | Error incremented in `toolbox_extraction_failures_total` |
| IT-15 | BR-NO-TEXT | ContentManager couldn't extract | Soft-fail with empty result |
| IT-16 | BR-WRITER | Graph/vector write failure | Logged; `toolbox_bridge_failures_total` incremented |
| IT-17 | REG-NOT-INIT | Registry accessed before initialize() | Exception thrown immediately |
| IT-18 | REG-DOUBLE | initialize() called twice | Warning logged; previous state preserved |
| IT-19 | HLP-EMPTY | Empty text passed to helper | Default result returned |
| IT-20 | HLP-ENCODING | Unsupported text encoding | Fallback to UTF-8; error counter incremented |

---

## Recommended Testing Workflow

### Local Development (Rapid Iteration)

```bash
# 1. Configure once
cmake --preset linux-release

# 2. Build one test at a time
cmake --build --preset linux-release --target test_toolbox_primitives

# 3. Run focused test
./build/linux-release/tests/toolbox/test_toolbox_primitives --gtest_filter="*ChunkingPreservesSemantics"

# 4. Debug failures with verbose output
./build/linux-release/tests/toolbox/test_toolbox_primitives --gtest_filter="*ChunkingPreservesSemantics" --gtest_repeat=5
```

### Pre-Commit Validation

```bash
# Run contract hardening + ingestion (fast validation)
ctest --preset linux-release -R "test_toolbox_(contract_hardening|ingestion)" -VV
```

### Pre-Release Validation

```bash
# Run all toolbox tests including Phase 5 gates
ctest --preset linux-release -L toolbox -VV --output-on-failure

# Verify performance gates pass
ctest --preset linux-release -R test_toolbox_phase5 -VV

# Run with thread sanitizer (if available)
ctest --preset linux-release-asan -L toolbox -VV
```

---

## Environment and Dependencies

- **GTest 1.10+:** Required for all tests
- **C++17:** Required compiler features
- **fmt, spdlog, nlohmann-json:** For logging and configuration
- **Threading:** Tests use std::thread for concurrency validation

---

## Links and References

- [ARCHITECTURE.md](ARCHITECTURE.md) - Module 3-plane model and data flows
- [ROADMAP.md](ROADMAP.md) - Phase 1-6 status, implementation progress
- [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) - Gate thresholds, baselines
- [SECURITY.md](SECURITY.md) - Incident taxonomy (4 execution planes)
- [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md) - Deployment and operational guidance

---

## Troubleshooting

### Test Compilation Failures

**Issue:** `Cannot find <gtest/gtest.h>`

**Solution:** Ensure GTest is installed:
```bash
sudo apt-get install libgtest-dev
cd /usr/src/gtest && cmake . && make && sudo make install
```

### Test Execution Failures

**Issue:** `Registry not initialized` or `REG-NOT-INIT`

**Solution:** Test fixtures should call `ToolboxRegistry::instance().reset()` in setUp().

**Issue:** `Bridge writer failed` or `BR-WRITER`

**Solution:** Mock graph/vector writers are required; see `test_toolbox_ingestion.cpp` for setup examples.

### Performance Gate Timeouts

**Issue:** Test takes > 60s to complete

**Solution:** Run on dedicated hardware; check system load with `top`. Phase 5 gates may take 30-60s on typical hardware.

---

## Status

- **Last Updated:** 2026-08-07
- **Test Coverage:** 96+ tests across 4 targets
- **Performance Gates:** GATE-TBX-P1..P6 all certified
- **Stress Fixtures:** HighConcurrency, MixedContent, DegradedPath, LongRun all operational

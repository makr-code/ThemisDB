# VECTOR_SEARCH DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\vector_search\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\vector_search\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 2
- Compounds: 9
- Classes/Structs: 1
- Namespaces: 2
- File Compounds: 2

## Namespaces
- @133306050373163275064377246177363323300164302251
- std::chrono_literals

## Types
### Classes
- FlatVectorIndex

### Structs
- none

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 10

### FlatVectorIndex

#### `FlatVectorIndex(std::size_t dim)`
- Source: `tests/vector_search/test_vector_search_highcardinality_stress.cpp`:60
- Brief: n/a
- Parameters:
  - `dim` (std::size_t): n/a

#### `bool insert(uint64_t id, Vec vec)`
- Source: `tests/vector_search/test_vector_search_highcardinality_stress.cpp`:62
- Brief: n/a
- Parameters:
  - `id` (uint64_t): n/a
  - `vec` (Vec): n/a

#### `std::vector< uint64_t > knn(const Vec &q, std::size_t k) const`
- Source: `tests/vector_search/test_vector_search_highcardinality_stress.cpp`:73
- Brief: n/a
- Parameters:
  - `q` (const Vec &): n/a
  - `k` (std::size_t): n/a

#### `std::size_t size() const`
- Source: `tests/vector_search/test_vector_search_highcardinality_stress.cpp`:93
- Brief: n/a
- Parameters: none

### bench_vector_search_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp`:240
- Brief: n/a
- Parameters: none

### test_vector_search_highcardinality_stress.cpp

#### `TEST(WaveD_HighCardinalityStress, ConcurrentHNSWBuildAndQuery)`
- Source: `tests/vector_search/test_vector_search_highcardinality_stress.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_HighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentHNSWBuildAndQuery): n/a

#### `TEST(WaveD_HighCardinalityStress, DimensionalityStressTest)`
- Source: `tests/vector_search/test_vector_search_highcardinality_stress.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_HighCardinalityStress): n/a
  - `<unnamed>` (DimensionalityStressTest): n/a

#### `TEST(WaveD_HighCardinalityStress, HighCardinalityVectorInsert)`
- Source: `tests/vector_search/test_vector_search_highcardinality_stress.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_HighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalityVectorInsert): n/a

#### `float l2sq(const Vec &a, const Vec &b)`
- Source: `tests/vector_search/test_vector_search_highcardinality_stress.cpp`:48
- Brief: n/a
- Parameters:
  - `a` (const Vec &): n/a
  - `b` (const Vec &): n/a

#### `Vec makeVec(std::size_t dim, uint64_t seed)`
- Source: `tests/vector_search/test_vector_search_highcardinality_stress.cpp`:40
- Brief: n/a
- Parameters:
  - `dim` (std::size_t): n/a
  - `seed` (uint64_t): n/a


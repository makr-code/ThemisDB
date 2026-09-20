# RETRIEVAL DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\retrieval\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\retrieval\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 8
- Compounds: 42
- Classes/Structs: 15
- Namespaces: 8
- File Compounds: 8

## Namespaces
- @101144354060153315320173045250372172213366376103
- std::chrono_literals
- testing
- themis
- themis::retrieval
- themis::retrieval::@172047165322366117002102055362132114162376037037
- themis::retrieval::IntegrityHelper
- themis::retrieval::testing

## Types
### Classes
- HybridRetrieverParity
- StubRoutingTable
- StubShard
- themis::retrieval::LoRAManifestStore
- themis::retrieval::testing::HybridRetrieverEngine
- themis::retrieval::testing::HybridRetrieverParityContractTest
- themis::retrieval::testing::HybridRetrieverParityTest
- themis::retrieval::testing::MockANNRetriever
- themis::retrieval::testing::MockExactRetriever

### Structs
- themis::retrieval::AdapterUsagePolicy
- themis::retrieval::ArtifactIntegrity
- themis::retrieval::LoRAPackage
- themis::retrieval::LoRAPackageProvenance
- themis::retrieval::PortableAdapterProduct
- themis::retrieval::testing::RetrievalResult

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 114

### StubRoutingTable

#### `StubRoutingTable(int num_shards)`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:40
- Brief: n/a
- Parameters:
  - `num_shards` (int): n/a

#### `void corrupt(const std::string &key, int bad_shard)`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:46
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `bad_shard` (int): n/a

#### `void heal()`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:51
- Brief: n/a
- Parameters: none

#### `int numShards() const`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:63
- Brief: n/a
- Parameters: none

#### `int route(const std::string &key) const`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:42
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `int routeWithOverride(const std::string &key) const`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:56
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

### StubShard

#### `bool contains(const std::string &key) const`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:79
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `void insert(const std::string &key)`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:74
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::size_t size() const`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:84
- Brief: n/a
- Parameters: none

### bench_retrieval_dedicated_gates.cpp

#### `BENCHMARK(BM_RT_BM_01_SingleShardRouting) -> Name("RT-BM-01/SingleShardExactRouting")`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RT_BM_01_SingleShardRouting): n/a

#### `BENCHMARK(BM_RT_BM_02_MultiShardRouting) -> Name("RT-BM-02/MultiShardRouting16") ->Threads(8)`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RT_BM_02_MultiShardRouting): n/a

#### `BENCHMARK(BM_RT_BM_03_CpuBaseline) -> Name("RT-BM-03/CpuBaselineThroughput")`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RT_BM_03_CpuBaseline): n/a

#### `BENCHMARK(BM_RT_BM_03_GpuAdvisoryThroughput) -> Name("RT-BM-03/GpuAdvisoryPathThroughput")`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RT_BM_03_GpuAdvisoryThroughput): n/a

#### `BENCHMARK(BM_RT_BM_04_HighCardinalityRouting) -> Name("RT-BM-04/HighCardinalityRouting")`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RT_BM_04_HighCardinalityRouting): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:140
- Brief: n/a
- Parameters: none

#### `void BM_RT_BM_01_SingleShardRouting(benchmark::State &state)`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:42
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RT_BM_02_MultiShardRouting(benchmark::State &state)`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:60
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RT_BM_03_CpuBaseline(benchmark::State &state)`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:102
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RT_BM_03_GpuAdvisoryThroughput(benchmark::State &state)`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:91
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RT_BM_04_HighCardinalityRouting(benchmark::State &state)`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:116
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `int routeKey(const std::string &key, int num_shards)`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:35
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `num_shards` (int): n/a

#### `bool stubCpuQuery(const std::vector< float > &vec)`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:85
- Brief: n/a
- Parameters:
  - `vec` (const std::vector< float > &): n/a

#### `bool stubGpuQuery(const std::vector< float > &vec)`
- Source: `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp`:78
- Brief: n/a
- Parameters:
  - `vec` (const std::vector< float > &): n/a

### test_retrieval_highcardinality_stress.cpp

#### `TEST(RetrievalStress, ConcurrentFailoverStress)`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrievalStress): n/a
  - `<unnamed>` (ConcurrentFailoverStress): n/a

#### `TEST(RetrievalStress, HighCardinalityShardRouting)`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrievalStress): n/a
  - `<unnamed>` (HighCardinalityShardRouting): n/a

#### `TEST(RetrievalStress, MultiShardExactRoutingStress)`
- Source: `tests/retrieval/test_retrieval_highcardinality_stress.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrievalStress): n/a
  - `<unnamed>` (MultiShardExactRoutingStress): n/a

### test_retrieval_llm_reranking_focused.cpp

#### `TEST(RetrievalLlmRerankingContract, ListByStatus)`
- Source: `tests/retrieval/test_retrieval_llm_reranking_focused.cpp`:24
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrievalLlmRerankingContract): n/a
  - `<unnamed>` (ListByStatus): n/a

#### `TEST(RetrievalLlmRerankingContract, StoreAndLoadById)`
- Source: `tests/retrieval/test_retrieval_llm_reranking_focused.cpp`:12
- Brief: n/a
- Parameters:
  - `<unnamed>` (RetrievalLlmRerankingContract): n/a
  - `<unnamed>` (StoreAndLoadById): n/a

### test_retrieval_lora_trust_boundary_focused.cpp

#### `TEST(LoRATrustBoundaryTest, LTB01a_CorrectChecksumAccepted)`
- Source: `tests/retrieval/test_retrieval_lora_trust_boundary_focused.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRATrustBoundaryTest): n/a
  - `<unnamed>` (LTB01a_CorrectChecksumAccepted): n/a
- Details: TestLTB-01a: Adapter with matching checksum → verifyChecksum returns true.

#### `TEST(LoRATrustBoundaryTest, LTB01b_FullCheckAdapterPipelineAccepted)`
- Source: `tests/retrieval/test_retrieval_lora_trust_boundary_focused.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRATrustBoundaryTest): n/a
  - `<unnamed>` (LTB01b_FullCheckAdapterPipelineAccepted): n/a
- Details: TestLTB-01b: Full checkAdapter pipeline with correct checksum → is_valid.

#### `TEST(LoRATrustBoundaryTest, LTB02a_TamperedDataRejectedByVerifyChecksum)`
- Source: `tests/retrieval/test_retrieval_lora_trust_boundary_focused.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRATrustBoundaryTest): n/a
  - `<unnamed>` (LTB02a_TamperedDataRejectedByVerifyChecksum): n/a
- Details: TestLTB-02a: verifyChecksum with tampered data → returns false.

#### `TEST(LoRATrustBoundaryTest, LTB02b_FullCheckAdapterRejectedOnMismatch)`
- Source: `tests/retrieval/test_retrieval_lora_trust_boundary_focused.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRATrustBoundaryTest): n/a
  - `<unnamed>` (LTB02b_FullCheckAdapterRejectedOnMismatch): n/a
- Details: TestLTB-02b: checkAdapter with mismatched checksum in metadata → is_valid=false.

#### `TEST(LoRATrustBoundaryTest, LTB02c_EmptyDataWithChecksumRejected)`
- Source: `tests/retrieval/test_retrieval_lora_trust_boundary_focused.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRATrustBoundaryTest): n/a
  - `<unnamed>` (LTB02c_EmptyDataWithChecksumRejected): n/a
- Details: TestLTB-02c: Empty adapter data with non-empty checksum → rejected.

#### `TEST(LoRATrustBoundaryTest, LTB03a_UnknownSignerRejectedUnderStrictPolicy)`
- Source: `tests/retrieval/test_retrieval_lora_trust_boundary_focused.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRATrustBoundaryTest): n/a
  - `<unnamed>` (LTB03a_UnknownSignerRejectedUnderStrictPolicy): n/a
- Details: TestLTB-03a: STRICT trust registry + unknown signer → adapter rejected. Governance requirement: an adapter from an unrecognised signer must NOT be loaded under a strict trust policy.

#### `TEST(LoRATrustBoundaryTest, LTB03b_UnknownSignerAcceptedWithWarningUnderLenientPolicy)`
- Source: `tests/retrieval/test_retrieval_lora_trust_boundary_focused.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRATrustBoundaryTest): n/a
  - `<unnamed>` (LTB03b_UnknownSignerAcceptedWithWarningUnderLenientPolicy): n/a
- Details: TestLTB-03b: LENIENT trust registry + unknown signer → loaded with warning. Documented behaviour: lenient policy accepts unknown signers but MUST emit a warning so operators can detect and remediate. Governance note: lenient mode must only be used in development environments. Production deployments must use STRICT mode (see ROADMAP.md §LoRA Trust).

#### `TEST(LoRATrustBoundaryTest, LTB03c_KnownSignerAcceptedWithoutWarningUnderStrictPolicy)`
- Source: `tests/retrieval/test_retrieval_lora_trust_boundary_focused.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRATrustBoundaryTest): n/a
  - `<unnamed>` (LTB03c_KnownSignerAcceptedWithoutWarningUnderStrictPolicy): n/a
- Details: TestLTB-03c: Known signer under STRICT policy → no warning, accepted. Confirms the positive path: registered signers load without warnings under the strict policy.

#### `TEST(LoRATrustBoundaryTest, LTB03d_UnknownSignerStopsBeforeChecksumValidation)`
- Source: `tests/retrieval/test_retrieval_lora_trust_boundary_focused.cpp`:282
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRATrustBoundaryTest): n/a
  - `<unnamed>` (LTB03d_UnknownSignerStopsBeforeChecksumValidation): n/a
- Details: TestLTB-03d: End-to-end load simulation — unknown signer under strict policy prevents the adapter from reaching checkAdapter. This test models the complete trust chain: Check signer trust (registry gate). Only proceed to checksum/checkAdapter when signer is trusted.

### themis::retrieval::AdapterUsagePolicy

#### `AdapterUsagePolicy from_json(const json &j)`
- Source: `src/retrieval/include/lora_package.h`:113
- Brief: From json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: contains(), at().

#### `json to_json() const`
- Source: `src/retrieval/include/lora_package.h`:111
- Brief: n/a
- Parameters: none

### themis::retrieval::ArtifactIntegrity

#### `ArtifactIntegrity from_json(const json &j)`
- Source: `src/retrieval/include/lora_package.h`:146
- Brief: From json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: contains(), at().

#### `json to_json() const`
- Source: `src/retrieval/include/lora_package.h`:145
- Brief: n/a
- Parameters: none

### themis::retrieval::IntegrityHelper

#### `std::string sha256Hex(const std::string &input)`
- Source: `src/retrieval/include/lora_package.h`:281
- Brief: Sha256 Hex.
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. Calls: data(), size().

#### `std::string sha256Hex(const uint8_t *data, size_t size)`
- Source: `src/retrieval/include/lora_package.h`:279
- Brief: Sha256 Hex.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value. Calls: update(), bytesToHex(), finalise().

#### `bool verifyHash(const std::string &input, const std::string &expected_hex)`
- Source: `src/retrieval/include/lora_package.h`:286
- Brief: Verify Hash.
- Parameters:
  - `input` (const std::string &): Input parameter.
  - `expected_hex` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: input Input parameter. expected_hex Input parameter. True when the operation succeeds. Calls: sha256Hex().

#### `bool verifyHash(const uint8_t *data, size_t size, const std::string &expected_hex)`
- Source: `src/retrieval/include/lora_package.h`:283
- Brief: Verify Hash.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
  - `expected_hex` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: data Input parameter. size Input parameter. expected_hex Input parameter. True when the operation succeeds. Calls: sha256Hex().

### themis::retrieval::LoRAManifestStore

#### `LoRAManifestStore()=default`
- Source: `src/retrieval/include/lora_package.h`:302
- Brief: n/a
- Parameters: none

#### `LoRAManifestStore(LoRAManifestStore &&)=default`
- Source: `src/retrieval/include/lora_package.h`:308
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAManifestStore &&): n/a

#### `LoRAManifestStore(const LoRAManifestStore &)=delete`
- Source: `src/retrieval/include/lora_package.h`:306
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LoRAManifestStore &): n/a

#### `bool deletePackage(const std::string &package_id)`
- Source: `src/retrieval/include/lora_package.h`:334
- Brief: Delete Package.
- Parameters:
  - `package_id` (const std::string &): Identifier of the package.
- Return: True when the operation succeeds.
- Details: package_id Identifier of the package. True when the operation succeeds. package_id Identifier of the package. True when the operation succeeds. Calls: lk(), erase().

#### `bool deleteProduct(const std::string &product_id)`
- Source: `src/retrieval/include/lora_package.h`:357
- Brief: Delete Product.
- Parameters:
  - `product_id` (const std::string &): Identifier of the product.
- Return: True when the operation succeeds.
- Details: product_id Identifier of the product. True when the operation succeeds. product_id Identifier of the product. True when the operation succeeds. Calls: lk(), erase().

#### `json exportPackages() const`
- Source: `src/retrieval/include/lora_package.h`:375
- Brief: n/a
- Parameters: none

#### `json exportProducts() const`
- Source: `src/retrieval/include/lora_package.h`:384
- Brief: n/a
- Parameters: none

#### `size_t importPackages(const json &j)`
- Source: `src/retrieval/include/lora_package.h`:382
- Brief: Import Packages.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: is_array(), LoRAPackage::from_json(), lk(), std::move().

#### `size_t importProducts(const json &j)`
- Source: `src/retrieval/include/lora_package.h`:391
- Brief: Import Products.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: is_array(), PortableAdapterProduct::from_json(), lk(), std::move().

#### `std::vector< std::string > listPackageIds() const`
- Source: `src/retrieval/include/lora_package.h`:336
- Brief: n/a
- Parameters: none

#### `std::vector< LoRAPackage > listPackagesByStatus(LoRAPackageStatus status) const`
- Source: `src/retrieval/include/lora_package.h`:338
- Brief: n/a
- Parameters:
  - `status` (LoRAPackageStatus): n/a

#### `std::vector< std::string > listProductIds() const`
- Source: `src/retrieval/include/lora_package.h`:359
- Brief: n/a
- Parameters: none

#### `std::vector< PortableAdapterProduct > listProductsByPackage(const std::string &package_id) const`
- Source: `src/retrieval/include/lora_package.h`:361
- Brief: n/a
- Parameters:
  - `package_id` (const std::string &): n/a

#### `std::vector< PortableAdapterProduct > listProductsByStatus(AdapterProductStatus status) const`
- Source: `src/retrieval/include/lora_package.h`:364
- Brief: n/a
- Parameters:
  - `status` (AdapterProductStatus): n/a

#### `std::optional< LoRAPackage > loadPackage(const std::string &package_id) const`
- Source: `src/retrieval/include/lora_package.h`:326
- Brief: n/a
- Parameters:
  - `package_id` (const std::string &): n/a

#### `std::optional< PortableAdapterProduct > loadProduct(const std::string &product_id) const`
- Source: `src/retrieval/include/lora_package.h`:349
- Brief: n/a
- Parameters:
  - `product_id` (const std::string &): n/a

#### `LoRAManifestStore & operator=(LoRAManifestStore &&)=default`
- Source: `src/retrieval/include/lora_package.h`:309
- Brief: n/a
- Parameters:
  - `<unnamed>` (LoRAManifestStore &&): n/a

#### `LoRAManifestStore & operator=(const LoRAManifestStore &)=delete`
- Source: `src/retrieval/include/lora_package.h`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LoRAManifestStore &): n/a

#### `size_t packageCount() const`
- Source: `src/retrieval/include/lora_package.h`:395
- Brief: n/a
- Parameters: none

#### `size_t productCount() const`
- Source: `src/retrieval/include/lora_package.h`:397
- Brief: n/a
- Parameters: none

#### `void setSignatureVerifier(SignatureVerifier verifier)`
- Source: `src/retrieval/include/lora_package.h`:316
- Brief: ── Configuration ─────────────────────────────────────────────────────
- Parameters:
  - `verifier` (SignatureVerifier): Input parameter.
- Details: Set Signature Verifier. verifier Input parameter. verifier Input parameter. Calls: lk(), std::move().

#### `bool storePackage(const LoRAPackage &pkg)`
- Source: `src/retrieval/include/lora_package.h`:324
- Brief: ── LoRAPackage CRUD ──────────────────────────────────────────────────
- Parameters:
  - `pkg` (const LoRAPackage &): Input parameter.
- Return: True when the operation succeeds.
- Details: ── LoRAPackage CRUD ────────────────────────────────────────────────────────── pkg Input parameter. True when the operation succeeds. pkg Input parameter. True when the operation succeeds. Calls: empty(), lk().

#### `bool storeProduct(const PortableAdapterProduct &product)`
- Source: `src/retrieval/include/lora_package.h`:347
- Brief: ── PortableAdapterProduct CRUD ───────────────────────────────────────
- Parameters:
  - `product` (const PortableAdapterProduct &): Input parameter.
- Return: True when the operation succeeds.
- Details: ── PortableAdapterProduct CRUD ─────────────────────────────────────────────── product Input parameter. True when the operation succeeds. product Input parameter. True when the operation succeeds. Calls: empty(), lk().

#### `bool verifyPackageIntegrity(const std::string &package_id) const`
- Source: `src/retrieval/include/lora_package.h`:369
- Brief: n/a
- Parameters:
  - `package_id` (const std::string &): n/a

#### `bool verifyProductIntegrity(const std::string &product_id) const`
- Source: `src/retrieval/include/lora_package.h`:371
- Brief: n/a
- Parameters:
  - `product_id` (const std::string &): n/a

#### `~LoRAManifestStore()=default`
- Source: `src/retrieval/include/lora_package.h`:303
- Brief: n/a
- Parameters: none

### themis::retrieval::LoRAPackage

#### `void computeManifestHash()`
- Source: `src/retrieval/include/lora_package.h`:197
- Brief: ── Helpers ──────────────────────────────────────────────────────────
- Parameters: none
- Details: Compute Manifest Hash. Calls: to_json(), dump(), IntegrityHelper::sha256Hex().

#### `LoRAPackage from_json(const json &j)`
- Source: `src/retrieval/include/lora_package.h`:191
- Brief: From json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: j Input parameter. Return value. std::invalid_argument if an error occurs. Calls: contains(), at(), statusFromString().

#### `LoRAPackageStatus statusFromString(const std::string &s)`
- Source: `src/retrieval/include/lora_package.h`:208
- Brief: Status From String.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: s Input parameter. Return value. s Input parameter. Return value. std::invalid_argument if an error occurs. Implements statusFromString without additional internal calls.

#### `std::string statusToString() const`
- Source: `src/retrieval/include/lora_package.h`:201
- Brief: n/a
- Parameters: none

#### `bool supportsArchitecture(const std::string &arch) const`
- Source: `src/retrieval/include/lora_package.h`:199
- Brief: n/a
- Parameters:
  - `arch` (const std::string &): n/a

#### `json to_json() const`
- Source: `src/retrieval/include/lora_package.h`:189
- Brief: n/a
- Parameters: none

### themis::retrieval::LoRAPackageProvenance

#### `LoRAPackageProvenance from_json(const json &j)`
- Source: `src/retrieval/include/lora_package.h`:130
- Brief: From json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: contains(), at().

#### `json to_json() const`
- Source: `src/retrieval/include/lora_package.h`:129
- Brief: n/a
- Parameters: none

### themis::retrieval::PortableAdapterProduct

#### `void computeManifestHash()`
- Source: `src/retrieval/include/lora_package.h`:261
- Brief: ── Helpers ──────────────────────────────────────────────────────────
- Parameters: none
- Details: Compute Manifest Hash. Calls: to_json(), dump(), IntegrityHelper::sha256Hex().

#### `PortableAdapterProduct from_json(const json &j)`
- Source: `src/retrieval/include/lora_package.h`:255
- Brief: From json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: j Input parameter. Return value. std::invalid_argument if an error occurs. Calls: contains(), at(), statusFromString().

#### `AdapterProductStatus statusFromString(const std::string &s)`
- Source: `src/retrieval/include/lora_package.h`:270
- Brief: Status From String.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: s Input parameter. Return value. s Input parameter. Return value. std::invalid_argument if an error occurs. Implements statusFromString without additional internal calls.

#### `std::string statusToString() const`
- Source: `src/retrieval/include/lora_package.h`:263
- Brief: n/a
- Parameters: none

#### `json to_json() const`
- Source: `src/retrieval/include/lora_package.h`:253
- Brief: n/a
- Parameters: none

### themis::retrieval::testing

#### `TEST_F(HybridRetrieverParityContractTest, VerifyParityContract)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:561
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridRetrieverParityContractTest): n/a
  - `<unnamed>` (VerifyParityContract): n/a

#### `TEST_F(HybridRetrieverParityTest, HYB01_ExactMatchBypassANN)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:331
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridRetrieverParityTest): n/a
  - `<unnamed>` (HYB01_ExactMatchBypassANN): n/a
- Details: HYB-01: Exact match found, bypass ANN

#### `TEST_F(HybridRetrieverParityTest, HYB02_NoExactMatchUseANN)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:352
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridRetrieverParityTest): n/a
  - `<unnamed>` (HYB02_NoExactMatchUseANN): n/a
- Details: HYB-02: No exact match, use ANN

#### `TEST_F(HybridRetrieverParityTest, HYB03_MixedDatasetParityValidation)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridRetrieverParityTest): n/a
  - `<unnamed>` (HYB03_MixedDatasetParityValidation): n/a
- Details: HYB-03: Mixed dataset - exact + ANN candidates

#### `TEST_F(HybridRetrieverParityTest, HYB04_HighCardinalityExactCandidates)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridRetrieverParityTest): n/a
  - `<unnamed>` (HYB04_HighCardinalityExactCandidates): n/a
- Details: HYB-04: High-cardinality exact candidates

#### `TEST_F(HybridRetrieverParityTest, HYB05_EmptyExactResultsFallback)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:418
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridRetrieverParityTest): n/a
  - `<unnamed>` (HYB05_EmptyExactResultsFallback): n/a
- Details: HYB-05: Empty exact results, ANN fallback

#### `TEST_F(HybridRetrieverParityTest, HYB06_ConcurrentQuerieThreadSafety)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:437
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridRetrieverParityTest): n/a
  - `<unnamed>` (HYB06_ConcurrentQuerieThreadSafety): n/a
- Details: HYB-06: Concurrent exact + ANN queries (thread-safety)

#### `TEST_F(HybridRetrieverParityTest, HYB07_LatencyComparisonExactVsANN)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:477
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridRetrieverParityTest): n/a
  - `<unnamed>` (HYB07_LatencyComparisonExactVsANN): n/a
- Details: HYB-07: Latency comparison (exact vs ANN)

#### `TEST_F(HybridRetrieverParityTest, HYB08_EdgeCasesFailClosedBehavior)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:519
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridRetrieverParityTest): n/a
  - `<unnamed>` (HYB08_EdgeCasesFailClosedBehavior): n/a
- Details: HYB-08: Edge case - NULL/empty/malformed inputs

### themis::retrieval::testing::HybridRetrieverEngine

#### `HybridRetrieverEngine()`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:190
- Brief: n/a
- Parameters: none

#### `void addDocument(const std::string &id, const std::string &content, const std::vector< float > &vector)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:233
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `content` (const std::string &): n/a
  - `vector` (const std::vector< float > &): n/a

#### `void clearData()`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:240
- Brief: n/a
- Parameters: none

#### `size_t getDocumentCount() const`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:245
- Brief: n/a
- Parameters: none

#### `RetrievalMode getMode() const`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:231
- Brief: n/a
- Parameters: none

#### `std::vector< RetrievalResult > retrieve(const std::string &query, const std::vector< float > &query_vector, size_t top_k=10)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:198
- Brief: n/a
- Parameters:
  - `query` (const std::string &): n/a
  - `query_vector` (const std::vector< float > &): n/a
  - `top_k` (size_t): n/a
- Details: Hybrid retrieve: exact-first with fallback

#### `void setMode(RetrievalMode mode)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:230
- Brief: n/a
- Parameters:
  - `mode` (RetrievalMode): n/a

### themis::retrieval::testing::HybridRetrieverParityTest

#### `HybridRetrieverParityTest()`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:263
- Brief: n/a
- Parameters: none

#### `double computeSpearmanCorrelation(const std::vector< float > &scores1, const std::vector< float > &scores2)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:272
- Brief: n/a
- Parameters:
  - `scores1` (const std::vector< float > &): n/a
  - `scores2` (const std::vector< float > &): n/a
- Details: Compute Spearman rank correlation between two score sequences Returns correlation coefficient in [-1, 1]

### themis::retrieval::testing::MockANNRetriever

#### `MockANNRetriever()`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:109
- Brief: n/a
- Parameters: none

#### `void addVector(const std::string &id, const std::vector< float > &vector)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:147
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `vector` (const std::vector< float > &): n/a

#### `void clearVectors()`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:151
- Brief: n/a
- Parameters: none

#### `float computeDistance(const std::vector< float > &a, const std::vector< float > &b)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:163
- Brief: n/a
- Parameters:
  - `a` (const std::vector< float > &): n/a
  - `b` (const std::vector< float > &): n/a

#### `std::vector< RetrievalResult > retrieveANN(const std::vector< float > &query_vector, size_t top_k=10)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:115
- Brief: n/a
- Parameters:
  - `query_vector` (const std::vector< float > &): n/a
  - `top_k` (size_t): n/a
- Details: Perform ANN-based retrieval Simulates approximate nearest neighbor search

#### `size_t vectorCount() const`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:155
- Brief: n/a
- Parameters: none

### themis::retrieval::testing::MockExactRetriever

#### `MockExactRetriever()`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:54
- Brief: n/a
- Parameters: none

#### `void addDocument(const std::string &id, const std::string &content)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:85
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `content` (const std::string &): n/a

#### `void clearDocuments()`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:89
- Brief: n/a
- Parameters: none

#### `size_t documentCount() const`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:93
- Brief: n/a
- Parameters: none

#### `std::vector< RetrievalResult > retrieveExact(const std::string &query, size_t top_k=10)`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:59
- Brief: n/a
- Parameters:
  - `query` (const std::string &): n/a
  - `top_k` (size_t): n/a
- Details: Perform exact match retrieval

### themis::retrieval::testing::RetrievalResult

#### `bool operator==(const RetrievalResult &other) const`
- Source: `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`:43
- Brief: n/a
- Parameters:
  - `other` (const RetrievalResult &): n/a


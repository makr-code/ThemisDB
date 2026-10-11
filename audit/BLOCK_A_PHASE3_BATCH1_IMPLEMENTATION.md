---
Author: "Copilot Task Agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "Block A - Phase 3 Batch 1 Implementation Ready"
---

# Block A: Immediate Hands-On Implementation (Phase 3 Batch 1)

## Overview

Block A implements 80-90 production tests across 4 minimal modules:
1. **distributed_tensor** (20-25 tests)
2. **llama_cpp** (20-25 tests)
3. **stable_diffusion** (20-25 tests)
4. **execution** (20-25 tests)

All tests follow CTest quality standard:
- **Function-level (40-60%):** Core function behavior
- **Module-level (15-20%):** Integration within module
- **Cross-module (8-15%):** Dependency validation
- **Happy-path (10-15%):** Normal operation
- **Error/Failure (8-12%):** Error handling
- **Edge-case (5-8%):** Boundary conditions

**Timeline:** 2-3 weeks, 1 developer + 1 reviewer
**All tests tagged:** `focused` + CTest categories

---

## Module 1: distributed_tensor (20-25 tests)

### Module Overview

**Location:** `src/distributed_tensor/`
**Current Tests:** 1 (test_tensor_operations_basic.cpp)
**LOC:** ~4,200 lines
**Required Tests (1 per 500 LOC):** 8-9 minimum

### Test Breakdown by CTest Category

#### Function-Level (40-60%): 10-12 tests

```cpp
// tests/distributed_tensor/test_function_tensor_allocation.cpp
#include <gtest/gtest.h>
#include "distributed_tensor/tensor.h"
#include "distributed_tensor/allocation.h"

namespace themis::tests {

class TensorAllocationTest : public ::testing::Test {
  // Tests core allocation functions
protected:
  void SetUp() override {
    context = CreateTestContext();
  }
};

// F001: Allocate basic tensor (happy-path)
TEST_F(TensorAllocationTest, AllocateBasicTensor) {
  auto tensor = Allocate(Shape{10, 20, 30}, DataType::Float32);
  EXPECT_EQ(tensor->shape(), Shape{10, 20, 30});
  EXPECT_EQ(tensor->dtype(), DataType::Float32);
  EXPECT_FALSE(tensor->is_empty());
}

// F002: Allocate with custom device (function + device control)
TEST_F(TensorAllocationTest, AllocateOnDevice) {
  auto device = GetTestDevice();
  auto tensor = Allocate(Shape{100, 100}, DataType::Float32, device);
  EXPECT_EQ(tensor->device(), device);
}

// F003: Allocate zero-size tensor (edge-case)
TEST_F(TensorAllocationTest, AllocateZeroSize) {
  auto tensor = Allocate(Shape{0, 0}, DataType::Float32);
  EXPECT_TRUE(tensor->is_empty());
  EXPECT_EQ(tensor->num_elements(), 0);
}

// F004: Allocation OOM handling (error-path)
TEST_F(TensorAllocationTest, AllocationOOM) {
  EXPECT_THROW(
    Allocate(Shape{1000000, 1000000}, DataType::Float32),
    OutOfMemoryError
  );
}

// F005: Deallocate tensor (cleanup)
TEST_F(TensorAllocationTest, DeallocateTensor) {
  auto tensor = Allocate(Shape{10, 10}, DataType::Float32);
  auto ptr = tensor.get();
  tensor.reset();
  // Verify memory is reclaimed (check via memory profiler)
  EXPECT_LE(GetMemoryUsage(), 1024 * 1024);  // <1MB after cleanup
}

// F006-F012: Additional function tests...
// - SetTensorValue (happy-path, edge-cases)
// - GetTensorValue (valid index, out-of-bounds)
// - ReshapeTensor (valid reshape, invalid reshape)
// - ConcatTensors (different sizes, same dtype, different dtype)
// - etc. (total 10-12 tests)

} // namespace themis::tests
```

#### Module-Level (15-20%): 4-5 tests

```cpp
// tests/distributed_tensor/test_module_tensor_operations.cpp
#include <gtest/gtest.h>
#include "distributed_tensor/tensor_ops.h"

namespace themis::tests {

class TensorOperationsModuleTest : public ::testing::Test {
  // Tests module integration (allocation + operations + cleanup)
protected:
  void SetUp() override {
    session = CreateTestSession();
  }
};

// M001: Full tensor lifecycle (allocate → operate → deallocate)
TEST_F(TensorOperationsModuleTest, TensorLifecycle) {
  auto tensor = Allocate(Shape{100, 100}, DataType::Float32);
  
  // Initialize
  FillTensor(tensor, 3.14f);
  
  // Operate
  auto result = Multiply(tensor, 2.0f);
  
  // Verify
  float val = GetTensorValue(result, Index{0, 0});
  EXPECT_FLOAT_EQ(val, 6.28f);
}

// M002: Multi-tensor operations (tensor + tensor)
TEST_F(TensorOperationsModuleTest, MatrixMultiplication) {
  auto a = CreateTensor(Shape{10, 20}, DataType::Float32);
  auto b = CreateTensor(Shape{20, 30}, DataType::Float32);
  
  auto result = MatMul(a, b);
  EXPECT_EQ(result->shape(), Shape{10, 30});
}

// M003: Reduction operations (sum, mean, max)
TEST_F(TensorOperationsModuleTest, TensorReductions) {
  auto tensor = CreateTensor(Shape{100, 100}, DataType::Float32);
  FillSequence(tensor, 1.0f, 10000.0f);
  
  auto sum = ReduceSum(tensor);
  EXPECT_NEAR(sum, 50005000.0f, 1000.0f);
}

// M004: Distributed operations (shard + aggregate)
TEST_F(TensorOperationsModuleTest, DistributedReduce) {
  auto tensor = CreateTensor(Shape{1000, 1000}, DataType::Float32);
  auto shards = ShardTensor(tensor, num_shards=4);
  
  // Reduce each shard
  std::vector<float> shard_sums;
  for (auto& shard : shards) {
    shard_sums.push_back(ReduceSum(shard));
  }
  
  // Aggregate
  float total_sum = 0.0f;
  for (auto s : shard_sums) total_sum += s;
  
  EXPECT_NEAR(total_sum, ReduceSum(tensor), 100.0f);
}

// M005: Session state management (module cleanup)
TEST_F(TensorOperationsModuleTest, SessionCleanup) {
  {
    auto session = CreateTestSession();
    auto tensor = Allocate(Shape{1000, 1000}, DataType::Float32);
    // Tensor should be deallocated when session ends
  }
  EXPECT_LE(GetSessionMemoryUsage(), 1024);  // <1KB after session cleanup
}

} // namespace themis::tests
```

#### Cross-Module (8-15%): 2-3 tests

```cpp
// tests/distributed_tensor/test_cross_module_server_integration.cpp
#include <gtest/gtest.h>
#include "distributed_tensor/tensor.h"
#include "server/server.h"
#include "server/rpc_handler.h"

namespace themis::tests {

class TensorServerIntegrationTest : public ::testing::Test {
  // Tests tensor with server RPC layer
protected:
  void SetUp() override {
    server = StartTestServer();
  }
};

// X001: Tensor transfer via RPC
TEST_F(TensorServerIntegrationTest, SerializeTensorOverRPC) {
  auto tensor = CreateTensor(Shape{50, 50}, DataType::Float32);
  FillRandom(tensor);
  
  // Serialize for RPC
  auto serialized = SerializeTensor(tensor);
  
  // Deserialize on server
  auto rpc_result = server->HandleTensorRequest(serialized);
  auto received_tensor = DeserializeTensor(rpc_result);
  
  EXPECT_EQ(received_tensor->shape(), tensor->shape());
  EXPECT_TENSORS_EQUAL(received_tensor, tensor);
}

// X002: Tensor storage in database
TEST_F(TensorServerIntegrationTest, StoreTensorInDB) {
  auto tensor = CreateTensor(Shape{100, 100}, DataType::Float32);
  
  // Store tensor
  std::string tensor_id = server->StoreTensor(tensor);
  
  // Retrieve tensor
  auto retrieved = server->GetTensor(tensor_id);
  EXPECT_TENSORS_EQUAL(retrieved, tensor);
}

// X003: Tensor streaming from LLM module
TEST_F(TensorServerIntegrationTest, LLMTensorOutput) {
  // Simulate LLM generating embeddings (tensor output)
  auto llm_response = server->CallLLMModule("get_embeddings");
  
  auto tensor = llm_response.GetTensor("embeddings");
  EXPECT_FALSE(tensor->is_empty());
  EXPECT_EQ(tensor->dtype(), DataType::Float32);
}

} // namespace themis::tests
```

#### Happy-Path (10-15%): 2-3 tests

```cpp
// tests/distributed_tensor/test_happy_path_tensor_workflows.cpp
#include <gtest/gtest.h>
#include "distributed_tensor/tensor.h"

namespace themis::tests {

class TensorHappyPathTest : public ::testing::Test {};

// H001: Normal forward pass (neural network)
TEST_F(TensorHappyPathTest, ForwardPassWorkflow) {
  // Input
  auto x = CreateTensor(Shape{32, 784}, DataType::Float32);  // 32 samples, 784 features
  
  // Layer 1: Linear + ReLU
  auto w1 = CreateTensor(Shape{784, 128}, DataType::Float32);
  auto z1 = MatMul(x, w1);
  auto a1 = ReLU(z1);
  
  // Layer 2: Linear + Softmax
  auto w2 = CreateTensor(Shape{128, 10}, DataType::Float32);
  auto z2 = MatMul(a1, w2);
  auto pred = Softmax(z2);
  
  EXPECT_EQ(pred->shape(), Shape{32, 10});
}

// H002: Data loading pipeline
TEST_F(TensorHappyPathTest, DataPipelineWorkflow) {
  // Load batch
  auto batch = LoadBatch("test_data", batch_size=32);
  
  // Preprocess
  auto normalized = Normalize(batch);
  
  // Augment
  auto augmented = RandomRotate(normalized, max_angle=30);
  
  // Use in training
  auto predictions = InferencePass(augmented);
  
  EXPECT_EQ(predictions->shape()[0], 32);  // Batch size preserved
}

// H003: Distributed computation
TEST_F(TensorHappyPathTest, DistributedComputeWorkflow) {
  auto global_tensor = CreateTensor(Shape{1000, 1000}, DataType::Float32);
  
  // Distribute
  auto shards = ShardTensor(global_tensor, num_shards=4);
  
  // Compute on each shard
  std::vector<Tensor> results;
  for (auto& shard : shards) {
    results.push_back(Multiply(shard, 2.0f));
  }
  
  // Aggregate
  auto aggregated = ConcatTensors(results);
  
  EXPECT_EQ(aggregated->shape()[0], 1000);
}

} // namespace themis::tests
```

#### Error/Failure (8-12%): 2 tests

```cpp
// tests/distributed_tensor/test_error_handling_tensor.cpp
#include <gtest/gtest.h>
#include "distributed_tensor/tensor.h"

namespace themis::tests {

class TensorErrorHandlingTest : public ::testing::Test {};

// E001: Shape mismatch detection
TEST_F(TensorErrorHandlingTest, ShapeMismatchDetection) {
  auto a = CreateTensor(Shape{10, 20}, DataType::Float32);
  auto b = CreateTensor(Shape{30, 40}, DataType::Float32);
  
  // MatMul requires matching dimensions
  EXPECT_THROW(
    MatMul(a, b),
    ShapeMismatchError
  );
}

// E002: dtype mismatch handling
TEST_F(TensorErrorHandlingTest, DtypeMismatchHandling) {
  auto f32_tensor = CreateTensor(Shape{10, 10}, DataType::Float32);
  auto int32_tensor = CreateTensor(Shape{10, 10}, DataType::Int32);
  
  EXPECT_THROW(
    MatMul(f32_tensor, int32_tensor),
    DataTypeMismatchError
  );
}

} // namespace themis::tests
```

#### Edge-Case (5-8%): 1-2 tests

```cpp
// tests/distributed_tensor/test_edge_cases_tensor.cpp
#include <gtest/gtest.h>
#include "distributed_tensor/tensor.h"

namespace themis::tests {

class TensorEdgeCaseTest : public ::testing::Test {};

// E001: Very large tensors (>1GB)
TEST_F(TensorEdgeCaseTest, VeryLargeTensor) {
  SKIP_IF_NOT_ENOUGH_MEMORY(2 * 1024 * 1024 * 1024);  // Skip if <2GB available
  
  auto large = CreateTensor(Shape{100000, 100000}, DataType::Float32);
  EXPECT_FALSE(large->is_empty());
  
  // Partial operation (not full matrix multiply)
  auto result = ReduceSum(large);
  EXPECT_GT(result, 0.0f);
}

// E002: Reshape with -1 dimension (automatic calculation)
TEST_F(TensorEdgeCaseTest, ReshapeAutomaticDimension) {
  auto tensor = CreateTensor(Shape{100, 100}, DataType::Float32);
  
  // Reshape to (100, ?, 1) - ? should be 100
  auto reshaped = Reshape(tensor, Shape{100, -1, 1});
  
  EXPECT_EQ(reshaped->shape(), Shape{100, 100, 1});
}

} // namespace themis::tests
```

### Summary: distributed_tensor CMakeLists.txt

```cmake
# tests/distributed_tensor/CMakeLists.txt

set(DISTRIBUTED_TENSOR_TESTS
  test_function_tensor_allocation.cpp
  test_module_tensor_operations.cpp
  test_cross_module_server_integration.cpp
  test_happy_path_tensor_workflows.cpp
  test_error_handling_tensor.cpp
  test_edge_cases_tensor.cpp
)

foreach(test_file ${DISTRIBUTED_TENSOR_TESTS})
  string(REPLACE ".cpp" "" test_name ${test_file})
  add_executable(${test_name} ${test_file})
  target_link_libraries(${test_name} gtest gtest_main themis_distributed_tensor)
endforeach()

# Register all tests with labels
set_tests_properties(test_function_tensor_allocation PROPERTIES
  LABELS "focused;function;distributed_tensor;wave7"
)
set_tests_properties(test_module_tensor_operations PROPERTIES
  LABELS "focused;module;distributed_tensor;wave7"
)
# ... register all others ...

# Aggregate target for Phase 3 Batch 1
set_property(GLOBAL APPEND PROPERTY THEMIS_FOCUSED_TARGETS
  test_function_tensor_allocation
  test_module_tensor_operations
  test_cross_module_server_integration
  test_happy_path_tensor_workflows
  test_error_handling_tensor
  test_edge_cases_tensor
)
```

---

## Modules 2-4: Quick Implementation Checklist

### llama_cpp (20-25 tests)

**Test Categories:**
- Function (10-12): Model loading, tokenization, inference, memory management
- Module (4-5): Session lifecycle, batch processing, caching
- Cross-module (2-3): Server RPC, storage integration, metrics export
- Happy-path (2-3): Full inference pipeline, batched inference
- Error (2): Invalid model format, OOM during inference
- Edge (1-2): Very long context (8K+ tokens), concurrent inference

**Key Tests:**
- `test_function_llama_model_loading.cpp` (4 tests: load, unload, reload, error)
- `test_module_llama_inference.cpp` (4 tests: forward pass, batching, caching, cleanup)
- `test_cross_module_llama_metrics.cpp` (3 tests: RPC, DB storage, metrics)
- `test_happy_path_llama_workflows.cpp` (3 tests: chat, completion, embeddings)
- `test_error_handling_llama.cpp` (2 tests: invalid model, OOM)
- `test_edge_cases_llama.cpp` (2 tests: long context, concurrent)

### stable_diffusion (20-25 tests)

**Test Categories:**
- Function (10-12): Model loading, encoding, generation, decoding
- Module (4-5): Pipeline orchestration, noise scheduling, sampling
- Cross-module (2-3): Server RPC, GPU resource allocation, result storage
- Happy-path (2-3): Text-to-image generation, image-to-image, upscaling
- Error (2): Invalid prompts, CUDA OOM, invalid dimensions
- Edge (1-2): Very long prompts (100+ tokens), extreme aspect ratios

**Key Tests:**
- `test_function_diffusion_model_loading.cpp`
- `test_module_diffusion_pipeline.cpp`
- `test_cross_module_diffusion_gpu_allocation.cpp`
- `test_happy_path_diffusion_workflows.cpp` (text2img, img2img, upscaling)
- `test_error_handling_diffusion.cpp`
- `test_edge_cases_diffusion.cpp`

### execution (20-25 tests)

**Test Categories:**
- Function (10-12): Query parsing, plan generation, execution, result collection
- Module (4-5): Transaction management, error recovery, resource cleanup
- Cross-module (2-3): Server integration, storage backend, consistency
- Happy-path (2-3): OLTP, OLAP, mixed workloads
- Error (2): Invalid SQL, resource exhaustion, timeout
- Edge (1-2): Complex joins, large result sets

**Key Tests:**
- `test_function_execution_query_parsing.cpp`
- `test_module_execution_plan.cpp`
- `test_cross_module_execution_storage.cpp`
- `test_happy_path_execution_queries.cpp`
- `test_error_handling_execution.cpp`
- `test_edge_cases_execution.cpp`

---

## Implementation Workflow (Developer)

### Day 1-2: Setup & distributed_tensor

```bash
# 1. Create test directory structure
mkdir -p tests/distributed_tensor

# 2. Draft all test files (copy templates from guide)
cp TEMPLATES/test_function_*.cpp tests/distributed_tensor/
cp TEMPLATES/test_module_*.cpp tests/distributed_tensor/
# ... etc

# 3. Implement function-level tests (10-12 tests)
# Edit: tests/distributed_tensor/test_function_tensor_allocation.cpp
# Add fixtures, helper functions, test cases

# 4. Build & run locally
cd build
ctest --tests-regex "distributed_tensor" -v
```

### Day 3-5: Module + Cross-Module tests

```bash
# 5. Implement module-level tests (4-5 tests)
# Edit: tests/distributed_tensor/test_module_tensor_operations.cpp

# 6. Implement cross-module tests (2-3 tests)
# Edit: tests/distributed_tensor/test_cross_module_server_integration.cpp

# 7. Rebuild & test
cmake --build build
ctest --tests-regex "distributed_tensor" -v
```

### Day 6-7: Happy-Path + Error + Edge Cases

```bash
# 8. Implement happy-path (2-3 tests)
# 9. Implement error handling (2 tests)
# 10. Implement edge cases (1-2 tests)

# 11. Final build & full test run
ctest --tests-regex "distributed_tensor" -v --output-on-failure
```

### Day 8-14: llama_cpp, stable_diffusion, execution (same pattern)

---

## Testing Before Commit

```bash
# 1. Local build + test
mkdir -p /tmp/phase3-batch1-build
cmake -S . -B /tmp/phase3-batch1-build \
  -DTHEMIS_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/phase3-batch1-build --parallel 8

# 2. Run focused tests only
ctest --test-dir /tmp/phase3-batch1-build \
  --label-regex "focused" \
  -v --output-on-failure

# 3. Check test count
ctest --test-dir /tmp/phase3-batch1-build \
  --label-regex "focused" --count

# 4. Run with coverage (if Block C CMakeLists.txt integration done)
ctest --test-dir /tmp/phase3-batch1-build \
  --label-regex "focused" \
  -v --output-on-failure

# 5. Verify CMake registration
grep -r "THEMIS_FOCUSED_TARGETS" tests/
```

---

## Success Metrics

- ✅ 20-25 tests per module (distributed_tensor, llama_cpp, stable_diffusion, execution)
- ✅ All tests labeled: `focused` + CTest category (function, module, cross-module, happy, error, edge)
- ✅ All tests pass locally (ctest --label-regex "focused" passes 100%)
- ✅ CMake registration complete (aggregate target builds)
- ✅ No build warnings or errors
- ✅ Test execution time <5 seconds per test (fast feedback)
- ✅ Code review: 2-3 reviewer approval per module
- ✅ PR gates pass: gate-pr-core.yml (release_critical subset)
- ✅ Baseline coverage established (coverage-report-nightly.yml integration)

---

## Estimated Timeline

| Module | Tests | Function Tests | Module Tests | Cross-Module Tests | Happy | Error | Edge | Est. Hours | Reality Check |
|--------|-------|---|---|---|---|---|---|---|---|
| distributed_tensor | 20-25 | 10-12 | 4-5 | 2-3 | 2-3 | 2 | 1-2 | 12-15 | ✓ Reasonable |
| llama_cpp | 20-25 | 10-12 | 4-5 | 2-3 | 2-3 | 2 | 1-2 | 12-15 | ✓ Reasonable |
| stable_diffusion | 20-25 | 10-12 | 4-5 | 2-3 | 2-3 | 2 | 1-2 | 12-15 | ✓ Reasonable |
| execution | 20-25 | 10-12 | 4-5 | 2-3 | 2-3 | 2 | 1-2 | 12-15 | ✓ Reasonable |
| CMake registration + integration | — | — | — | — | — | — | — | 4-6 | ✓ Reasonable |
| Code review + iteration | — | — | — | — | — | — | — | 8-12 | ✓ Reality check |
| **TOTAL** | **80-90** | **40-48** | **16-20** | **8-12** | **8-12** | **8** | **4-8** | **60-90 hours** | **2-3 weeks** |

---

## Next Steps After Block A

1. ✅ Phase 3 Batch 1: 80-90 tests (distributed_tensor, llama_cpp, stable_diffusion, execution)
2. → **Phase 3 Batch 2:** Sparse Tier A modules (Server, Sharding, Storage, Query, Transaction, Auth)
   - 200-300 tests with `release_critical`, `security/*`, `governance/*`, `performance` labels
   - ~4-6 weeks implementation

3. → **Phase 3 Batch 3:** Roadmap TODOs → tests (rag module)
   - 50-100 tests with `governance/roadmap` labels
   - ~2-3 weeks implementation

4. → **Phase 3 Batch 4:** Integration/E2E/Soak/Chaos tests
   - 30-50 tests with `integration/*`, `compliance/*`, `regression/*` labels
   - ~2-3 weeks implementation

5. → **Phase 4:** CI/CD infrastructure finalization
   - Coverage gate enforcement
   - Nightly reporting dashboard
   - Badge + metrics publication
   - ~1-2 weeks setup

**Total Project Duration: ~3-4 months to 100% test coverage (810-950 tests)**


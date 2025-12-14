# LLM Code Translator Implementation

This directory contains the **basic framework implementation** of the LLM-based code translation and execution system.

## 🏗️ Architecture

The implementation consists of three core layers:

### 1. **vLLM Communication Layer** (`vllm_client.h/cpp`)
- REST API client for vLLM server
- Handles HTTP communication with local vLLM instance
- Supports single and multi-sample generation
- Health checking and error handling

### 2. **Execution Plan Layer** (`execution_plan.h/cpp`)
- Platform-independent JSON representation of database operations
- 8 operation types: QUERY, AGGREGATE, TRANSFORM, JOIN, GRAPH_TRAVERSE, VECTOR_SEARCH, TIME_SERIES, MUTATION
- Validation and serialization
- Supports filters, aggregations, and complex parameters

### 3. **Translation Layer** (`prompt_to_plan.h/cpp`)
- Translates natural language → Execution Plans via LLM
- Few-shot learning with examples
- Schema context integration
- Multi-sample generation for neural approaches

## 📋 Prerequisites

### Running vLLM Server

```bash
# Install vLLM
pip install vllm

# Start vLLM server with CodeGen model
python -m vllm.entrypoints.openai.api_server \
    --model Salesforce/codegen-16B-multi \
    --host 0.0.0.0 \
    --port 8000
```

### Build Dependencies

- **C++20 compiler** (GCC 10+, Clang 12+, MSVC 19.28+)
- **CMake 3.20+**
- **libcurl** (for HTTP requests)
- **nlohmann_json** (for JSON parsing)

## 🛠️ Building

### Option 1: As Part of ThemisDB

```bash
cd /path/to/ThemisDB
mkdir build && cd build
cmake ..
make llm_code_translator
```

### Option 2: Standalone

```bash
cd projects/llm-code-translator
mkdir build && cd build
cmake ..
make
```

## 🚀 Usage

### Basic Example

```cpp
#include "vllm_client.h"
#include "prompt_to_plan.h"

using namespace themis::llm_translator;

// 1. Configure vLLM client
VLLMConfig config;
config.base_url = "http://localhost:8000";
config.model_name = "codegen-16B";

auto vllm_client = std::make_shared<VLLMClient>(config);

// 2. Create translator
PromptToPlanTranslator translator(vllm_client);

// 3. Translate prompt to execution plan
auto plan = translator.translate(
    "Find all sensors with temperature > 50°C in last 24 hours"
);

// 4. Use the plan (execute, JIT compile, or convert to assembly)
std::cout << plan.toJson().dump(2) << std::endl;
```

### Multi-Sample Generation (AlphaCode-inspired)

```cpp
// Generate multiple plan candidates
PromptToPlanConfig config;
config.use_multi_sample = true;
config.num_samples = 10;

translator.setConfig(config);

auto plans = translator.translateMultiple(
    "Show average temperature per sensor"
);

// Select best plan (performance prediction, validation, etc.)
for (const auto& plan : plans) {
    if (plan.validate()) {
        // Evaluate and select...
    }
}
```

## 📊 Execution Plan Format

```json
{
  "operation": 1,
  "datasource": "sensor_readings",
  "filters": [
    {"field": "timestamp", "op": ">=", "value": "-24h"},
    {"field": "temperature", "op": ">", "value": 50}
  ],
  "group_by": ["sensor_id"],
  "aggregations": [
    {"function": "AVG", "field": "temperature", "alias": "avg_temp"}
  ],
  "metadata": {
    "original_prompt": "Show average temperature...",
    "llm_model_used": "codegen-16B",
    "generation_time_ms": 234,
    "confidence_score": 0.95
  }
}
```

## 🔧 Components

### VLLMClient

- **Purpose**: Communicate with vLLM server via REST API
- **Methods**:
  - `generate()` - Single completion
  - `generateMultiple()` - Multi-sample generation
  - `healthCheck()` - Server availability

### ExecutionPlan

- **Purpose**: Platform-independent operation representation
- **Features**:
  - JSON serialization/deserialization
  - Validation (correctness, security)
  - Metadata tracking

### PromptToPlanTranslator

- **Purpose**: Natural language → Execution Plan
- **Features**:
  - Few-shot learning
  - Schema context integration
  - Multi-sample generation
  - Retry logic

## 🎯 Next Steps

This basic framework implements:
- ✅ vLLM REST API communication
- ✅ Execution plan data structures
- ✅ Prompt-to-plan translation
- ✅ Multi-sample generation

**To be implemented:**
- [ ] Direct execution engine (interpret plans)
- [ ] JIT compiler (plan → native code)
- [ ] Assembly generator (plan → .asm)
- [ ] Neural optimization (select best plan)
- [ ] Code improvement engine
- [ ] Integration with ThemisDB query engine

## 📝 Example Output

```
=== LLM Code Translator - Basic Example ===

Checking vLLM server health... ✓ Server is healthy

Query: Find all sensors with temperature > 50°C
------------------------------------------------------------
Execution Plan:
{
  "operation": 0,
  "datasource": "sensor_readings",
  "filters": [
    {"field": "temperature", "op": ">", "value": 50}
  ]
}

Metadata:
  - Model: codegen-16B
  - Generation Time: 156 ms
  - Confidence: 95%
  - Valid: Yes
```

## 🔒 Security

- All plans are validated before execution
- Only predefined operations allowed
- Input sanitization
- Resource limits enforced

## 📚 References

- [vLLM Documentation](https://docs.vllm.ai/)
- [CodeGen Model](https://github.com/salesforce/CodeGen)
- [Project Documentation](../docs/)

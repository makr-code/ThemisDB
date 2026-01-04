# P1: Enterprise Features Implementation

**Priorität:** P1 - WICHTIG  
**Zeitrahmen:** 4-6 Wochen (Sprint 2-3)  
**Status:** 🟡 Geplant

---

## 🎯 Zielsetzung

Implementation der Enterprise-Features für Production-Deployments: Timestamp Authority, LLM Production Validator, Multi-Node Shard RPC, und LLM Inference Engine Verbesserungen.

---

## 📊 Feature-Übersicht

| Feature | Priorität | Aufwand | Status | Impact |
|---------|-----------|---------|--------|--------|
| **Timestamp Authority (RFC 3161)** | P1 | 3 Tage | 🔴 TODO | eIDAS Compliance |
| **LLM Production Validator** | P1 | 2 Tage | 🔴 TODO | Monitoring |
| **Shard RPC Client Multi-Node** | P1 | 1 Woche | 🔴 TODO | Cluster-Mode |
| **LLM Inference Engine** | P1 | 1 Woche | 🔴 TODO | Performance |
| **Grafana Metrics (LLM)** | P1 | 2 Tage | 🔴 TODO | Observability |

**Gesamt:** 3-4 Wochen Development + 1-2 Wochen Testing

---

## 1️⃣ Timestamp Authority (RFC 3161)

### Aktuelle Situation

**Dateien:**
- `src/security/timestamp_authority.cpp` (Stub - 100 Zeilen)
- `src/security/timestamp_authority_openssl.cpp` (Real - geplant)

**Aktueller Code (Stub):**
```cpp
// Minimal stub implementation for TimestampAuthority.
TimestampResult TimestampAuthority::createTimestamp(const std::vector<uint8_t>& data) {
    TimestampResult res;
    res.success = true;
    res.timestamp_token = base64_encode(data);
    res.timestamp_rfc3161 = getCurrentISO8601Timestamp();
    res.serial_number = "STUB-SERIAL";
    res.tsa_name = "STUB-TSA";
    return res;
}
```

### Use Cases

1. **Qualifizierte elektronische Zeitstempel** (eIDAS Art. 42)
   - Rechtssichere Dokumenten-Zeitstempel
   - Langzeitarchivierung

2. **Audit-Compliance**
   - Unveränderbare Zeitstempel für Audit-Logs
   - DSGVO Art. 30 Verarbeitungsverzeichnis

3. **Code-Signing**
   - Zeitstempel für Software-Releases
   - Zertifikats-Gültigkeit über Ablaufdatum hinaus

### Implementation

**Neue Datei:** `src/security/timestamp_authority_openssl.cpp`

```cpp
#include "security/timestamp_authority.h"
#include <openssl/ts.h>
#include <openssl/err.h>
#include <curl/curl.h>

class TimestampAuthorityImpl {
public:
    TimestampAuthorityImpl(const TSAConfig& config) : config_(config) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    
    ~TimestampAuthorityImpl() {
        curl_global_cleanup();
    }
    
    TimestampResult createTimestamp(const std::vector<uint8_t>& data) {
        TimestampResult result;
        
        // 1. Create timestamp request
        TS_REQ* request = createTimestampRequest(data);
        if (!request) {
            result.success = false;
            result.error_message = "Failed to create timestamp request";
            return result;
        }
        
        // 2. Serialize request
        unsigned char* request_data = nullptr;
        int request_len = i2d_TS_REQ(request, &request_data);
        
        // 3. Send to TSA server
        std::vector<uint8_t> response_data;
        bool sent = sendToTSA(request_data, request_len, response_data);
        OPENSSL_free(request_data);
        TS_REQ_free(request);
        
        if (!sent) {
            result.success = false;
            result.error_message = "Failed to send request to TSA";
            return result;
        }
        
        // 4. Parse response
        const unsigned char* p = response_data.data();
        TS_RESP* response = d2i_TS_RESP(nullptr, &p, response_data.size());
        if (!response) {
            result.success = false;
            result.error_message = "Failed to parse TSA response";
            return result;
        }
        
        // 5. Verify response
        if (!verifyTimestampResponse(response, data)) {
            result.success = false;
            result.error_message = "Timestamp verification failed";
            TS_RESP_free(response);
            return result;
        }
        
        // 6. Extract timestamp token
        TS_TST_INFO* tst_info = TS_RESP_get_tst_info(response);
        if (!tst_info) {
            result.success = false;
            result.error_message = "Failed to extract timestamp info";
            TS_RESP_free(response);
            return result;
        }
        
        // 7. Fill result
        result.success = true;
        result.timestamp_rfc3161 = extractTimestamp(tst_info);
        result.serial_number = extractSerialNumber(tst_info);
        result.tsa_name = extractTSAName(tst_info);
        result.timestamp_token = base64_encode(response_data);
        
        TS_RESP_free(response);
        return result;
    }

private:
    TS_REQ* createTimestampRequest(const std::vector<uint8_t>& data) {
        // Create SHA256 hash of data
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(data.data(), data.size(), hash);
        
        // Create request
        TS_REQ* req = TS_REQ_new();
        TS_REQ_set_version(req, 1);
        
        // Set message imprint
        TS_MSG_IMPRINT* imprint = TS_MSG_IMPRINT_new();
        TS_MSG_IMPRINT_set_algo(imprint, OBJ_nid2obj(NID_sha256));
        ASN1_OCTET_STRING_set(TS_MSG_IMPRINT_get_msg(imprint), hash, SHA256_DIGEST_LENGTH);
        TS_REQ_set_msg_imprint(req, imprint);
        TS_MSG_IMPRINT_free(imprint);
        
        // Request certificate in response
        TS_REQ_set_cert_req(req, 1);
        
        // Set nonce (random)
        ASN1_INTEGER* nonce = ASN1_INTEGER_new();
        ASN1_INTEGER_set(nonce, rand());
        TS_REQ_set_nonce(req, nonce);
        ASN1_INTEGER_free(nonce);
        
        return req;
    }
    
    bool sendToTSA(const unsigned char* data, int len, std::vector<uint8_t>& response) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/timestamp-query");
        
        std::string response_str;
        curl_easy_setopt(curl, CURLOPT_URL, config_.tsa_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_str);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        CURLcode res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) return false;
        
        response.assign(response_str.begin(), response_str.end());
        return true;
    }
    
    bool verifyTimestampResponse(TS_RESP* response, const std::vector<uint8_t>& data) {
        // Check status
        TS_STATUS_INFO* status = TS_RESP_get_status_info(response);
        ASN1_INTEGER* status_int = TS_STATUS_INFO_get0_status(status);
        long status_val = ASN1_INTEGER_get(status_int);
        
        if (status_val != 0 && status_val != 1) {
            return false;  // Not granted or granted with modifications
        }
        
        // Verify hash matches
        TS_TST_INFO* tst_info = TS_RESP_get_tst_info(response);
        TS_MSG_IMPRINT* imprint = TS_TST_INFO_get_msg_imprint(tst_info);
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(data.data(), data.size(), hash);
        
        ASN1_OCTET_STRING* msg_hash = TS_MSG_IMPRINT_get_msg(imprint);
        if (memcmp(hash, msg_hash->data, SHA256_DIGEST_LENGTH) != 0) {
            return false;
        }
        
        return true;
    }
    
    std::string extractTimestamp(TS_TST_INFO* tst_info) {
        ASN1_GENERALIZEDTIME* gen_time = TS_TST_INFO_get_time(tst_info);
        BIO* bio = BIO_new(BIO_s_mem());
        ASN1_GENERALIZEDTIME_print(bio, gen_time);
        
        char* time_str = nullptr;
        long len = BIO_get_mem_data(bio, &time_str);
        std::string result(time_str, len);
        BIO_free(bio);
        
        return result;
    }
    
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
    TSAConfig config_;
};
```

**Konfiguration:**

```yaml
# config/security.yaml
timestamp_authority:
  enabled: true
  tsa_url: "http://timestamp.digicert.com"
  # Alternative TSAs:
  # - "http://zeitstempel.dfn.de"  # DFN-Verein (Germany)
  # - "https://freetsa.org/tsr"     # FreeTSA
  timeout_seconds: 30
  retry_attempts: 3
```

**Aufwand:** 3 Tage

---

## 2️⃣ LLM Production Validator

### Aktuelle Situation

**Datei:** `src/llm/production_validator.cpp`

**Aktueller Code:**
```cpp
ProductionMetrics ProductionValidator::benchmarkInference(const std::string& model_id) {
    ProductionMetrics result;
    result.model_id = model_id;
    result.passed = true;
    
    // For now, placeholder
    result.latency_p50_ms = 100.0;
    result.latency_p95_ms = 250.0;
    result.throughput_tokens_per_sec = 1200.0;  // Placeholder
    
    return result;
}
```

### Use Cases

1. **Model Validation** vor Deployment
2. **Performance Monitoring** in Production
3. **SLA Compliance** Checking
4. **Capacity Planning**

### Implementation

```cpp
class ProductionValidator {
public:
    ProductionMetrics benchmarkInference(const std::string& model_id) {
        ProductionMetrics metrics;
        metrics.model_id = model_id;
        
        // 1. Load model
        auto plugin = loadModel(model_id);
        if (!plugin) {
            metrics.passed = false;
            metrics.error_message = "Failed to load model";
            return metrics;
        }
        
        // 2. Run benchmark suite
        std::vector<double> latencies;
        int total_tokens = 0;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Benchmark: 100 requests with varying lengths
        for (int i = 0; i < 100; i++) {
            std::string prompt = generateBenchmarkPrompt(i % 10);
            
            auto req_start = std::chrono::high_resolution_clock::now();
            auto response = plugin->generate({
                .prompt = prompt,
                .max_tokens = 50,
                .temperature = 0.7
            });
            auto req_end = std::chrono::high_resolution_clock::now();
            
            double latency = std::chrono::duration<double, std::milli>(req_end - req_start).count();
            latencies.push_back(latency);
            total_tokens += response.completion_tokens;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double total_time_s = std::chrono::duration<double>(end_time - start_time).count();
        
        // 3. Calculate metrics
        std::sort(latencies.begin(), latencies.end());
        metrics.latency_p50_ms = latencies[latencies.size() / 2];
        metrics.latency_p95_ms = latencies[static_cast<size_t>(latencies.size() * 0.95)];
        metrics.latency_p99_ms = latencies[static_cast<size_t>(latencies.size() * 0.99)];
        metrics.throughput_tokens_per_sec = total_tokens / total_time_s;
        
        // 4. Quality checks
        metrics.passed = true;
        if (metrics.latency_p95_ms > 5000.0) {  // 5 seconds
            metrics.passed = false;
            metrics.warnings.push_back("P95 latency exceeds 5 seconds");
        }
        if (metrics.throughput_tokens_per_sec < 10.0) {
            metrics.passed = false;
            metrics.warnings.push_back("Throughput too low");
        }
        
        // 5. Memory usage
        metrics.memory_used_mb = measureMemoryUsage();
        
        return metrics;
    }
    
    bool validateQuality(const std::string& model_id) {
        // Run standard test suite
        std::vector<QualityTest> tests = {
            {"math", "What is 2+2?", {"4", "four"}},
            {"knowledge", "What is the capital of France?", {"Paris"}},
            {"reasoning", "If John is taller than Mary, and Mary is taller than Sue, who is the shortest?", {"Sue"}},
        };
        
        auto plugin = loadModel(model_id);
        int passed = 0;
        
        for (const auto& test : tests) {
            auto response = plugin->generate({.prompt = test.prompt, .max_tokens = 50});
            
            bool correct = false;
            for (const auto& expected : test.expected_answers) {
                if (response.text.find(expected) != std::string::npos) {
                    correct = true;
                    break;
                }
            }
            
            if (correct) passed++;
        }
        
        return (passed >= tests.size() * 0.8);  // 80% threshold
    }

private:
    std::string generateBenchmarkPrompt(int variant) {
        static const std::vector<std::string> prompts = {
            "Explain quantum computing in simple terms.",
            "Write a haiku about databases.",
            "What are the benefits of ACID transactions?",
            // ... more variants
        };
        return prompts[variant % prompts.size()];
    }
    
    size_t measureMemoryUsage() {
        // Platform-specific memory measurement
#ifdef __linux__
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.find("VmRSS:") == 0) {
                size_t kb = std::stoul(line.substr(6));
                return kb / 1024;  // MB
            }
        }
#endif
        return 0;
    }
};
```

**Aufwand:** 2 Tage

---

## 3️⃣ Shard RPC Client - Multi-Node Support

### Aktuelle Situation

**Datei:** `src/sharding/shard_rpc_client.cpp`

**Aktueller Code:**
```cpp
// For now, using in-process simulation
bool ShardRPCClient::connect(const std::string& endpoint) {
    if (endpoint.find("localhost") != std::string::npos) {
        // v1.3.0: In-process simulation for single-node deployments
        is_in_process_ = true;
        connected_ = true;
        return true;
    }
    
    // TODO: Real gRPC connection for multi-node
    return false;
}
```

### Use Cases

1. **Multi-Node Cluster** Deployments
2. **Horizontal Scaling**
3. **Geographic Distribution**
4. **Fault Tolerance**

### Implementation

**Proto Definition:** `proto/shard_rpc.proto`

```protobuf
syntax = "proto3";
package themis.shard;

service ShardRPC {
    rpc Query(QueryRequest) returns (QueryResponse);
    rpc Write(WriteRequest) returns (WriteResponse);
    rpc Healthcheck(HealthcheckRequest) returns (HealthcheckResponse);
}

message QueryRequest {
    string shard_id = 1;
    string query = 2;
    map<string, string> params = 3;
}

message QueryResponse {
    bool success = 1;
    bytes data = 2;
    string error = 3;
}

// ... more messages
```

**Implementation:**

```cpp
class ShardRPCClient {
public:
    bool connect(const std::string& endpoint) {
        // Parse endpoint
        std::string host, port;
        if (!parseEndpoint(endpoint, host, port)) {
            return false;
        }
        
        // Check if in-process
        if (host == "localhost" && is_single_node_) {
            is_in_process_ = true;
            connected_ = true;
            return true;
        }
        
        // Create gRPC channel
        grpc::ChannelArguments args;
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 30000);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);
        args.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);
        
        channel_ = grpc::CreateCustomChannel(
            endpoint,
            grpc::InsecureChannelCredentials(),  // TODO: mTLS
            args
        );
        
        stub_ = ShardRPC::NewStub(channel_);
        
        // Test connection
        if (!healthcheck()) {
            channel_.reset();
            stub_.reset();
            return false;
        }
        
        connected_ = true;
        return true;
    }
    
    QueryResult query(const std::string& shard_id, const std::string& query_str) {
        if (is_in_process_) {
            return queryInProcess(shard_id, query_str);
        }
        
        // gRPC call with retry
        int attempts = 0;
        while (attempts < max_retries_) {
            grpc::ClientContext context;
            context.set_deadline(std::chrono::system_clock::now() + timeout_);
            
            QueryRequest request;
            request.set_shard_id(shard_id);
            request.set_query(query_str);
            
            QueryResponse response;
            grpc::Status status = stub_->Query(&context, request, &response);
            
            if (status.ok()) {
                return QueryResult{
                    .success = response.success(),
                    .data = std::vector<uint8_t>(response.data().begin(), response.data().end()),
                    .error = response.error()
                };
            }
            
            // Retry logic
            if (status.error_code() == grpc::StatusCode::UNAVAILABLE ||
                status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
                attempts++;
                std::this_thread::sleep_for(retry_delay_ * attempts);
                continue;
            }
            
            // Non-retryable error
            return QueryResult{
                .success = false,
                .error = status.error_message()
            };
        }
        
        return QueryResult{
            .success = false,
            .error = "Max retries exceeded"
        };
    }
    
private:
    bool healthcheck() {
        grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
        
        HealthcheckRequest request;
        HealthcheckResponse response;
        
        grpc::Status status = stub_->Healthcheck(&context, request, &response);
        return status.ok() && response.healthy();
    }
    
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<ShardRPC::Stub> stub_;
    bool is_in_process_ = false;
    bool is_single_node_ = false;
    std::chrono::seconds timeout_{30};
    int max_retries_ = 3;
    std::chrono::milliseconds retry_delay_{100};
};
```

**Aufwand:** 1 Woche

---

## 4️⃣ LLM Inference Engine Improvements

### Features

1. **Context Caching** - KV-Cache Reuse
2. **Batch Processing** - Multiple requests parallel
3. **Request Queuing** - Priority-basierte Queue
4. **Load Balancing** - Multi-Model Support

**Aufwand:** 1 Woche

---

## 5️⃣ Grafana Metrics für LLM

### Implementation

```cpp
class GrafanaLLMMetrics {
public:
    void recordInference(const std::string& model_id, double latency_ms, int tokens) {
        // Prometheus metrics
        inference_latency_
            .labels({{"model", model_id}})
            .observe(latency_ms);
        
        tokens_generated_
            .labels({{"model", model_id}})
            .inc(tokens);
        
        inference_count_
            .labels({{"model", model_id}})
            .inc();
    }
    
    void recordError(const std::string& model_id, const std::string& error_type) {
        error_count_
            .labels({{"model", model_id}, {"type", error_type}})
            .inc();
    }

private:
    prometheus::Histogram& inference_latency_;
    prometheus::Counter& tokens_generated_;
    prometheus::Counter& inference_count_;
    prometheus::Counter& error_count_;
};
```

**Aufwand:** 2 Tage

---

## 📋 Implementation Timeline

### Sprint 2 (Woche 3-4)

#### Woche 3
- [ ] **Tag 1-3:** Timestamp Authority Implementation
  - RFC 3161 Client
  - OpenSSL Integration
  - TSA Server Communication
  - Unit Tests

- [ ] **Tag 4-5:** LLM Production Validator
  - Benchmark Suite
  - Quality Tests
  - Memory Monitoring

#### Woche 4
- [ ] **Tag 1-3:** Shard RPC Client (Multi-Node) - Teil 1
  - Proto Definitions
  - gRPC Channel Setup
  - Basic Query/Write Operations

- [ ] **Tag 4-5:** Shard RPC Client - Teil 2
  - Retry Logic
  - Healthcheck
  - Connection Pooling

### Sprint 3 (Woche 5-6)

#### Woche 5
- [ ] **Tag 1-5:** LLM Inference Engine
  - Context Caching
  - Batch Processing
  - Request Queue
  - Load Balancer

#### Woche 6
- [ ] **Tag 1-2:** Grafana Metrics
  - Prometheus Integration
  - Dashboard Templates

- [ ] **Tag 3-5:** Integration Testing
  - End-to-End Tests
  - Performance Tests
  - Load Tests

---

## 🧪 Testing Strategy

### Unit Tests
- Timestamp Authority: RFC 3161 Response Parsing
- Production Validator: Metrics Calculation
- Shard RPC: Retry Logic, Healthcheck
- Inference Engine: Context Caching, Batching

### Integration Tests
- TSA with Real Server (FreeTSA)
- Multi-Node Cluster (3+ nodes)
- LLM Inference unter Last

### Performance Tests
- Shard RPC Latency < 50ms
- LLM Throughput > 100 tokens/s
- TSA Response < 2s

---

## 📊 Success Metrics

| Feature | Metric | Target | Status |
|---------|--------|--------|--------|
| Timestamp Authority | TSA Response Time | < 2s | 🔴 TODO |
| LLM Validator | Benchmark Completion | < 2 min | 🔴 TODO |
| Shard RPC | Multi-Node Latency | < 50ms | 🔴 TODO |
| Inference Engine | Context Hit Rate | > 80% | 🔴 TODO |
| Grafana Metrics | Dashboard Completeness | 100% | 🔴 TODO |

---

## 🔗 Dependencies

- **P0 (LLaMA.cpp Plugin)** muss abgeschlossen sein
- OpenSSL für TSA
- gRPC für Shard RPC
- Prometheus für Metrics

---

**Erstellt:** 4. Januar 2026  
**Start:** 13. Januar 2026 (nach P0)  
**Ende:** 14. Februar 2026  
**Status:** 🟡 Geplant

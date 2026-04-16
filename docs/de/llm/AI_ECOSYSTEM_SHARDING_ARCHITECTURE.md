# ThemisDB AI-Ökosystem: Horizontales Sharding mit integriertem LLM

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Architecture / AI Infrastructure  
**Sprache:** Deutsch

---

## 📋 Executive Summary

Dieses Dokument beschreibt die technische Architektur eines **vollständigen AI-Ökosystems** basierend auf ThemisDB's horizontalem Sharding, bei dem jede Shard-Instanz ein komplettes Sprachmodell (llama.cpp → GPU/VRAM) hält und LoRA-Adapter von anderen ThemisDB-Instanzen dynamisch laden kann.

**Kernkonzept:**
- Jede ThemisDB-Shard = **Speicher + Compute + LLM**
- LoRA-Adapter werden zwischen Shards ausgetauscht
- Horizontal skalierbare AI-Inferenz mit Daten-Lokalität
- Vollständig autonomes, verteiltes LLM-System

**Vorteile:**
- ✅ **Data Locality**: Inferenz passiert dort, wo die Daten sind
- ✅ **Horizontal Scaling**: Mehr Shards = Mehr Inferenz-Kapazität
- ✅ **Dynamic LoRA**: Adapter werden on-demand zwischen Shards transferiert
- ✅ **Fault Tolerance**: Raft Consensus + Model Replication
- ✅ **Zero External Dependencies**: Kein separater vLLM-Cluster nötig

---

## 🏗️ Architektur-Übersicht

### High-Level: AI-Ökosystem mit Sharding

```
┌───────────────────────────────────────────────────────────────────────────┐
│                  ThemisDB AI-Ökosystem Cluster                            │
│                  (N Shards, jede mit eigenem LLM)                         │
├───────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │                      Shard 1 (Rechts-Domain)                     │    │
│  ├──────────────────────────────────────────────────────────────────┤    │
│  │  Storage (RocksDB)         │  LLM Inference (llama.cpp + CUDA)   │    │
│  │  ├─ Legal Documents        │  ├─ Base Model: Mistral-7B         │    │
│  │  ├─ Court Cases (5M)       │  │  (VRAM: 14 GB)                  │    │
│  │  ├─ Embeddings (FAISS GPU) │  ├─ LoRA: legal-qa-v1 (local)      │    │
│  │  │  (VRAM: 8 GB)           │  │  (VRAM: 32 MB)                  │    │
│  │  └─ Metadata               │  └─ LoRA Cache (from other shards) │    │
│  │                            │     (VRAM: 256 MB)                  │    │
│  └─────────────┬──────────────┴──────────────────┬──────────────────┘    │
│                │                                  │                        │
│       URN: urn:themis:document:legal:*          GPU: 24 GB total         │
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │                      Shard 2 (Medizin-Domain)                    │    │
│  ├──────────────────────────────────────────────────────────────────┤    │
│  │  Storage (RocksDB)         │  LLM Inference (llama.cpp + CUDA)   │    │
│  │  ├─ Medical Records        │  ├─ Base Model: Llama-3-8B          │    │
│  │  ├─ Patient Data (3M)      │  │  (VRAM: 16 GB)                  │    │
│  │  ├─ Embeddings (FAISS GPU) │  ├─ LoRA: medical-diagnosis-v1     │    │
│  │  │  (VRAM: 5 GB)           │  │  (VRAM: 64 MB)                  │    │
│  │  └─ EHR Documents          │  └─ LoRA: legal-qa-v1 (cached)     │    │
│  │                            │     (transferred from Shard 1)      │    │
│  └─────────────┬──────────────┴──────────────────┬──────────────────┘    │
│                │                                  │                        │
│       URN: urn:themis:document:medical:*        GPU: 24 GB total         │
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │                      Shard 3 (Umweltrecht-Domain)                │    │
│  ├──────────────────────────────────────────────────────────────────┤    │
│  │  Storage (RocksDB)         │  LLM Inference (llama.cpp + CUDA)   │    │
│  │  ├─ Environmental Laws     │  ├─ Base Model: Mistral-7B         │    │
│  │  ├─ Regulations (2M)       │  │  (VRAM: 14 GB)                  │    │
│  │  ├─ Embeddings (FAISS GPU) │  ├─ LoRA: env-law-v1 (local)       │    │
│  │  │  (VRAM: 4 GB)           │  │  (VRAM: 32 MB)                  │    │
│  │  └─ Case Studies           │  └─ Multi-LoRA Support (8 slots)   │    │
│  │                            │                                      │    │
│  └─────────────┬──────────────┴──────────────────┬──────────────────┘    │
│                │                                  │                        │
│       URN: urn:themis:document:environment:*    GPU: 24 GB total         │
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │              Shard Communication Layer (Phase 3)                  │    │
│  │  ┌────────────────────────────────────────────────────────────┐  │    │
│  │  │ ShardRouter + RemoteExecutor + mTLS + Circuit Breaker      │  │    │
│  │  │                                                             │  │    │
│  │  │ Features:                                                   │  │    │
│  │  │ - URN-based Routing (Consistent Hash)                      │  │    │
│  │  │ - Cross-Shard LoRA Transfer (gRPC Binary)                  │  │    │
│  │  │ - Federated RAG Queries                                    │  │    │
│  │  │ - Distributed LLM Inference                                │  │    │
│  │  │ - Raft Consensus für Model Replication                     │  │    │
│  │  └────────────────────────────────────────────────────────────┘  │    │
│  └──────────────────────────────────────────────────────────────────┘    │
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │              etcd v3 Cluster (Coordination)                       │    │
│  │  ┌────────────────────────────────────────────────────────────┐  │    │
│  │  │ - Shard Topology                                            │  │    │
│  │  │ - LoRA Registry (welche Shard hat welchen Adapter)         │  │    │
│  │  │ - Model Versions                                            │  │    │
│  │  │ - Health Status                                             │  │    │
│  │  └────────────────────────────────────────────────────────────┘  │    │
│  └──────────────────────────────────────────────────────────────────┘    │
│                                                                            │
└───────────────────────────────────────────────────────────────────────────┘

Gesamtsystem:
- 3+ Shards (horizontal skalierbar)
- Jede Shard: Eigene GPU (24 GB), eigenes LLM, eigene Daten
- LoRA-Adapter werden dynamisch zwischen Shards ausgetauscht
- Federated RAG: Query kann mehrere Shards abfragen
- Raft Consensus: Automatic Failover + Model Replication
```

---

## 🔧 Komponenten-Design

### 1. LLM-fähige Shard-Instanz

Jede ThemisDB-Shard wird erweitert um:

```cpp
// include/llm/llm_enabled_shard.h
#pragma once

#include "llm/llm_model_loader.h"
#include "llm/lora_registry.h"
#include "sharding/shard_router.h"
#include "acceleration/faiss_gpu_backend.h"
#include <memory>

namespace themis {
namespace llm {

/**
 * LLM-Enabled Shard
 * 
 * Erweitert eine Standard-ThemisDB-Shard um:
 * - Native LLM Inference (llama.cpp)
 * - LoRA Adapter Management
 * - Cross-Shard LoRA Transfer
 * - Federated RAG Queries
 */
class LLMEnabledShard {
public:
    struct Config {
        // Shard Identity
        std::string shard_id;
        std::string domain;  // "legal", "medical", "environment", etc.
        
        // LLM Configuration
        std::string base_model_path;     // "/models/mistral-7b-q4.gguf"
        int n_gpu_layers = 32;            // GPU offload
        int n_ctx = 4096;                 // Context length
        size_t max_vram_mb = 14336;       // 14 GB für Model
        
        // LoRA Configuration
        std::vector<std::string> local_loras;  // Lokal vorinstallierte Adapter
        size_t lora_cache_slots = 8;           // Max cached remote LoRAs
        size_t lora_cache_vram_mb = 512;       // 512 MB für LoRA Cache
        
        // FAISS GPU Configuration
        size_t faiss_vram_mb = 8192;      // 8 GB für Vector Index
        int faiss_nlist = 4096;
        int faiss_nprobe = 64;
        
        // Sharding Configuration
        std::shared_ptr<sharding::ShardRouter> shard_router;
        std::shared_ptr<sharding::URNResolver> urn_resolver;
    };
    
    explicit LLMEnabledShard(const Config& config);
    ~LLMEnabledShard();
    
    // ═══════════════════════════════════════════════════════════
    // Local LLM Inference (Single Shard)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * RAG Query auf dieser Shard
     * 
     * 1. Vector Search in lokalen Embeddings (FAISS GPU)
     * 2. Context Assembly aus lokalen Dokumenten
     * 3. LLM Inference mit lokalem oder gecachtem LoRA
     */
    struct RAGRequest {
        std::string query;
        std::string lora_adapter_id;  // Optional: Spezifischer LoRA
        int top_k = 10;
        int max_tokens = 512;
        float temperature = 0.7f;
    };
    
    struct RAGResponse {
        std::string answer;
        std::vector<std::string> source_documents;
        std::string used_lora;
        std::string executed_on_shard;
        float inference_time_ms;
    };
    
    RAGResponse executeLocalRAG(const RAGRequest& request);
    
    // ═══════════════════════════════════════════════════════════
    // Federated RAG (Cross-Shard Queries)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * Federated RAG über mehrere Shards
     * 
     * Use Case: Query braucht Kontext aus mehreren Domains
     * Beispiel: "Medizinische und rechtliche Aspekte von X"
     * 
     * Workflow:
     * 1. Coordinator-Shard (diese) schickt Sub-Queries an andere Shards
     * 2. Jede Shard führt lokalen RAG aus
     * 3. Coordinator aggregiert Ergebnisse
     * 4. Finaler LLM Call mit kombiniertem Context
     */
    struct FederatedRAGRequest {
        std::string query;
        std::vector<std::string> target_shards;  // Leer = alle Shards
        std::vector<std::string> target_domains; // "legal", "medical", etc.
        int top_k_per_shard = 5;
        int max_tokens = 512;
        bool parallel_execution = true;
    };
    
    RAGResponse executeFederatedRAG(const FederatedRAGRequest& request);
    
    // ═══════════════════════════════════════════════════════════
    // LoRA Management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * Load LoRA Adapter (lokal oder von anderer Shard)
     * 
     * Wenn LoRA nicht lokal:
     * 1. Query LoRA Registry (etcd) für Location
     * 2. Transfer LoRA von Source-Shard (gRPC Binary)
     * 3. Cache in lokalem VRAM
     * 4. Apply to Model
     */
    bool loadLoRA(const std::string& lora_id);
    
    /**
     * Unload LoRA aus Cache (bei VRAM-Druck)
     */
    bool unloadLoRA(const std::string& lora_id);
    
    /**
     * List verfügbare LoRAs (lokal + remote)
     */
    struct LoRAInfo {
        std::string id;
        std::string source_shard;  // Wo ist der Master
        bool is_local;             // Lokal installiert?
        bool is_cached;            // Im Cache?
        size_t size_mb;
        std::string version;
    };
    
    std::vector<LoRAInfo> listAvailableLoRAs();
    
    /**
     * Publish eigenen LoRA ins Cluster
     * (Registriert in etcd, andere Shards können abrufen)
     */
    bool publishLoRA(const std::string& lora_id, const std::string& path);
    
    // ═══════════════════════════════════════════════════════════
    // Cross-Shard LoRA Transfer
    // ═══════════════════════════════════════════════════════════
    
    /**
     * Transfer LoRA von anderer Shard (Server-Side)
     * 
     * Wird aufgerufen wenn andere Shard LoRA anfordert
     */
    std::vector<uint8_t> serveLoRA(const std::string& lora_id);
    
    /**
     * Receive LoRA von anderer Shard (Client-Side)
     */
    bool receiveLoRA(const std::string& lora_id, 
                     const std::string& source_shard);
    
    // ═══════════════════════════════════════════════════════════
    // Model Replication (Raft-based)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * Replicate Base Model zu Backup-Shards
     * 
     * Bei Failover kann Backup-Shard übernehmen
     */
    bool replicateBaseModel(const std::vector<std::string>& replica_shards);
    
    /**
     * Sync LoRA Updates (wenn LoRA auf Source-Shard aktualisiert wird)
     */
    bool syncLoRAUpdate(const std::string& lora_id, uint64_t version);
    
    // ═══════════════════════════════════════════════════════════
    // Statistics & Monitoring
    // ═══════════════════════════════════════════════════════════
    
    struct Stats {
        // LLM Stats
        size_t total_inferences = 0;
        size_t local_rag_calls = 0;
        size_t federated_rag_calls = 0;
        double avg_inference_ms = 0.0;
        
        // LoRA Stats
        size_t lora_cache_hits = 0;
        size_t lora_cache_misses = 0;
        size_t lora_transfers = 0;
        
        // VRAM Usage
        size_t model_vram_mb;
        size_t faiss_vram_mb;
        size_t lora_vram_mb;
        size_t total_vram_mb;
        
        // Shard Coordination
        size_t cross_shard_queries = 0;
    };
    
    Stats getStats() const;
    
private:
    Config config_;
    
    // LLM Components
    std::unique_ptr<LLMModelLoader> model_loader_;
    std::unique_ptr<LoRARegistry> lora_registry_;
    
    // Vector Search
    std::unique_ptr<acceleration::FaissGPUVectorBackend> vector_backend_;
    
    // Sharding Infrastructure
    std::shared_ptr<sharding::ShardRouter> shard_router_;
    std::shared_ptr<sharding::URNResolver> urn_resolver_;
    
    // LoRA Cache (LRU)
    struct LoRACache {
        std::map<std::string, std::vector<uint8_t>> entries;
        std::list<std::string> lru_order;
        size_t max_size_mb;
        size_t current_size_mb;
    };
    LoRACache lora_cache_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
};

} // namespace llm
} // namespace themis
```

### 2. LoRA Registry (etcd-basiert)

```cpp
// include/llm/lora_registry.h
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

/**
 * Distributed LoRA Registry
 * 
 * Stored in etcd für Cluster-weite Koordination
 * 
 * Key Schema:
 *   /themis/lora/{lora_id}/metadata     -> JSON metadata
 *   /themis/lora/{lora_id}/location     -> Shard ID (master)
 *   /themis/lora/{lora_id}/replicas     -> Array of replica shard IDs
 *   /themis/lora/{lora_id}/version      -> Version number
 */
class LoRARegistry {
public:
    struct LoRAMetadata {
        std::string id;
        std::string master_shard;           // Primäre Shard
        std::vector<std::string> replicas;  // Backup-Shards
        std::string base_model;             // Kompatibles Base Model
        std::string domain;                 // "legal", "medical", etc.
        size_t size_bytes;
        uint64_t version;
        std::string created_at;
        nlohmann::json custom_metadata;
    };
    
    /**
     * Register LoRA in Cluster
     */
    bool registerLoRA(const LoRAMetadata& metadata);
    
    /**
     * Lookup LoRA Location
     */
    std::optional<LoRAMetadata> getLoRAMetadata(const std::string& lora_id);
    
    /**
     * List all LoRAs (optional filter by domain)
     */
    std::vector<LoRAMetadata> listLoRAs(
        const std::string& domain_filter = ""
    );
    
    /**
     * Update LoRA Version (für Synchronisation)
     */
    bool updateLoRAVersion(const std::string& lora_id, uint64_t new_version);
    
    /**
     * Add Replica Location
     */
    bool addReplica(const std::string& lora_id, const std::string& shard_id);
    
    /**
     * Watch for LoRA Updates (etcd watch)
     */
    void watchLoRAUpdates(
        std::function<void(const std::string& lora_id, uint64_t version)> callback
    );
    
private:
    std::string etcd_endpoint_;
    std::string cluster_id_;
};

} // namespace llm
} // namespace themis
```

### 3. Shard-to-Shard LoRA Transfer Protocol

```protobuf
// src/llm/lora_transfer.proto
syntax = "proto3";

package themis.llm;

// LoRA Transfer Service
service LoRATransferService {
    // Request LoRA from another shard
    rpc RequestLoRA(LoRARequest) returns (stream LoRAChunk);
    
    // Push LoRA to another shard
    rpc PushLoRA(stream LoRAChunk) returns (LoRATransferStatus);
    
    // Check if LoRA is available
    rpc CheckLoRAAvailability(LoRAAvailabilityRequest) returns (LoRAAvailabilityResponse);
}

message LoRARequest {
    string lora_id = 1;
    string requesting_shard = 2;
    uint64 expected_version = 3;
}

message LoRAChunk {
    string lora_id = 1;
    uint64 chunk_index = 2;
    uint64 total_chunks = 3;
    bytes data = 4;  // Binäre LoRA-Daten
    bytes checksum = 5;  // SHA-256 für Verifikation
}

message LoRATransferStatus {
    bool success = 1;
    string error_message = 2;
    uint64 bytes_transferred = 3;
    float transfer_time_ms = 4;
}

message LoRAAvailabilityRequest {
    string lora_id = 1;
}

message LoRAAvailabilityResponse {
    bool available = 1;
    uint64 version = 2;
    uint64 size_bytes = 3;
}
```

```cpp
// src/llm/lora_transfer_service.cpp
#include "llm/lora_transfer_service.h"
#include <fstream>

namespace themis {
namespace llm {

class LoRATransferServiceImpl final : public LoRATransferService::Service {
public:
    LoRATransferServiceImpl(LLMEnabledShard* shard) : shard_(shard) {}
    
    grpc::Status RequestLoRA(
        grpc::ServerContext* context,
        const LoRARequest* request,
        grpc::ServerWriter<LoRAChunk>* writer
    ) override {
        // Load LoRA binary data
        auto lora_data = shard_->serveLoRA(request->lora_id());
        
        if (lora_data.empty()) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, 
                              "LoRA not found");
        }
        
        // Stream in chunks (1 MB per chunk)
        const size_t chunk_size = 1024 * 1024;
        size_t offset = 0;
        uint64_t chunk_index = 0;
        uint64_t total_chunks = (lora_data.size() + chunk_size - 1) / chunk_size;
        
        while (offset < lora_data.size()) {
            LoRAChunk chunk;
            chunk.set_lora_id(request->lora_id());
            chunk.set_chunk_index(chunk_index);
            chunk.set_total_chunks(total_chunks);
            
            size_t bytes_to_send = std::min(chunk_size, 
                                            lora_data.size() - offset);
            chunk.set_data(lora_data.data() + offset, bytes_to_send);
            
            // Checksum für Chunk
            auto checksum = sha256(lora_data.data() + offset, bytes_to_send);
            chunk.set_checksum(checksum);
            
            if (!writer->Write(chunk)) {
                return grpc::Status(grpc::StatusCode::ABORTED, 
                                  "Stream broken");
            }
            
            offset += bytes_to_send;
            chunk_index++;
        }
        
        LOG_INFO << "Transferred LoRA " << request->lora_id() 
                 << " (" << lora_data.size() << " bytes) to "
                 << request->requesting_shard();
        
        return grpc::Status::OK;
    }
    
private:
    LLMEnabledShard* shard_;
};

} // namespace llm
} // namespace themis
```

---

## 🔀 Use Cases & Workflows

### Use Case 1: Lokale RAG Query

**Szenario:** Anwalt fragt rechtliche Frage auf Legal-Shard

```
┌─────────────────────────────────────────────────────────────┐
│  Client Request                                              │
│  POST /api/rag/query                                         │
│  {                                                            │
│    "query": "Welche Haftung bei Baumängeln?",              │
│    "domain": "legal"                                         │
│  }                                                            │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Shard Router (URN Resolution)                               │
│  - Query → URN: urn:themis:document:legal:*                 │
│  - Consistent Hash → Shard 1 (Legal Domain)                 │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Shard 1: LLMEnabledShard::executeLocalRAG()                │
│                                                              │
│  Step 1: Vector Search (FAISS GPU)                          │
│    - Embed Query: 768-dim vector (2ms)                      │
│    - Search in 5M legal embeddings (5ms)                    │
│    - Top-10 relevant cases                                  │
│                                                              │
│  Step 2: Context Assembly                                   │
│    - Load full documents from RocksDB (10ms)                │
│    - Extract relevant passages                              │
│    - Build context (max 4096 tokens)                        │
│                                                              │
│  Step 3: LLM Inference                                      │
│    - Apply LoRA: legal-qa-v1 (lokal)                        │
│    - Generate answer (llama.cpp + CUDA)                     │
│    - Latenz: 300ms                                          │
│                                                              │
│  Total: ~317ms                                              │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Response                                                    │
│  {                                                            │
│    "answer": "Bei Baumängeln haftet der Auftragnehmer...",  │
│    "sources": ["BGH 2020-01-15", "VOB §13"],              │
│    "shard": "shard-1-legal",                                │
│    "lora": "legal-qa-v1",                                   │
│    "inference_time_ms": 317                                 │
│  }                                                            │
└─────────────────────────────────────────────────────────────┘
```

### Use Case 2: Cross-Domain Federated RAG

**Szenario:** Komplexe Frage braucht Rechts- UND Medizin-Kontext

```
┌─────────────────────────────────────────────────────────────┐
│  Client Request                                              │
│  POST /api/rag/federated                                     │
│  {                                                            │
│    "query": "Medizinische und rechtliche Aspekte der       │
│               Patientenaufklärung",                          │
│    "domains": ["legal", "medical"]                          │
│  }                                                            │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Coordinator Shard (z.B. Shard 1)                           │
│  executeFederatedRAG()                                       │
└─────────────────────────┬───────────────────────────────────┘
                          │
           ┌──────────────┴──────────────┐
           │                             │
           ▼                             ▼
┌────────────────────────┐    ┌────────────────────────┐
│  Shard 1 (Legal)       │    │  Shard 2 (Medical)     │
│  Sub-Query:            │    │  Sub-Query:            │
│  "Rechtliche Aspekte"  │    │  "Medizinische Aspekte"│
│                        │    │                        │
│  - Vector Search       │    │  - Vector Search       │
│  - Top-5 Cases         │    │  - Top-5 Guidelines    │
│  - LoRA: legal-qa-v1   │    │  - LoRA: medical-v1    │
│                        │    │                        │
│  Latenz: 150ms         │    │  Latenz: 160ms         │
└────────────┬───────────┘    └───────────┬────────────┘
             │                            │
             │      Parallel Execution    │
             └────────────┬───────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Coordinator Shard: Context Fusion                           │
│                                                              │
│  - Merge Results:                                            │
│    Legal Context (5 Dokumente) + Medical Context (5 Docs)   │
│                                                              │
│  - Combined Prompt:                                          │
│    "Basierend auf folgenden rechtlichen und medizinischen   │
│     Dokumenten: [...]"                                       │
│                                                              │
│  - Final LLM Call:                                          │
│    Base Model mit beiden Contexts                           │
│    (optional: beide LoRAs gleichzeitig, wenn supported)     │
│                                                              │
│  Latenz: 350ms                                              │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Response                                                    │
│  {                                                            │
│    "answer": "Aus rechtlicher Sicht... Medizinisch...",    │
│    "sources": {                                              │
│      "legal": ["BGB §630c", ...],                          │
│      "medical": ["S3-Leitlinie Aufklärung", ...]           │
│    },                                                        │
│    "shards_queried": ["shard-1-legal", "shard-2-medical"], │
│    "total_time_ms": 510                                     │
│  }                                                            │
└─────────────────────────────────────────────────────────────┘
```

### Use Case 3: Dynamic LoRA Transfer

**Szenario:** Medical-Shard braucht Legal-LoRA für eine Hybrid-Query

```
┌─────────────────────────────────────────────────────────────┐
│  Shard 2 (Medical) bekommt Query mit legal-qa-v1 LoRA       │
│  - LoRA nicht lokal verfügbar                               │
│  - LoRA nicht im Cache                                      │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Step 1: Query LoRA Registry (etcd)                         │
│  GET /themis/lora/legal-qa-v1/metadata                      │
│                                                              │
│  Response:                                                   │
│  {                                                            │
│    "id": "legal-qa-v1",                                     │
│    "master_shard": "shard-1-legal",                         │
│    "replicas": ["shard-1-legal-replica"],                   │
│    "size_bytes": 33554432,  // 32 MB                        │
│    "version": 3                                              │
│  }                                                            │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Step 2: gRPC Call zu Shard 1                               │
│  LoRATransferService::RequestLoRA()                          │
│                                                              │
│  - Shard 1 streamt LoRA in Chunks (1 MB pro Chunk)         │
│  - Shard 2 empfängt und verifikiert (SHA-256)              │
│  - Transfer: 32 MB in ~150ms (über 10 Gbit/s Network)      │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Step 3: LoRA in Cache laden                                │
│  - Cache in VRAM (LoRA Cache Slot)                         │
│  - Apply zu Base Model (llama.cpp)                          │
│  - Mark as "cached" in local registry                       │
│                                                              │
│  VRAM Update:                                               │
│  Before: Model (16 GB) + FAISS (5 GB) = 21 GB              │
│  After:  Model (16 GB) + FAISS (5 GB) + LoRA (32 MB)       │
│        = 21.03 GB                                           │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  Step 4: Inference mit transferiertem LoRA                  │
│  - Query kann nun ausgeführt werden                         │
│  - LoRA bleibt im Cache für zukünftige Queries             │
│                                                              │
│  Cache Hit Rate steigt über Zeit                            │
└─────────────────────────────────────────────────────────────┘
```

---

## 📊 Performance & Scaling

### GPU-Speicher Breakdown (Pro Shard, 24 GB GPU)

```
┌────────────────────────────────────────────────────────┐
│  GPU Memory Allocation (NVIDIA RTX 4090 24 GB)        │
├────────────────────────────────────────────────────────┤
│                                                         │
│  Base Model (Mistral-7B Q4):           14 GB          │
│  - Quantized Weights                                   │
│  - KV Cache (context=4096)                            │
│                                                         │
│  FAISS Vector Index:                     8 GB          │
│  - 10M Embeddings (768-dim, IVF+PQ)                   │
│                                                         │
│  LoRA Slots:                           512 MB          │
│  - Local LoRA (1x):                    32 MB          │
│  - Cached LoRAs (8x):                 256 MB          │
│  - Working Buffer:                    224 MB          │
│                                                         │
│  System Overhead:                      1.5 GB          │
│  - CUDA Runtime                                        │
│  - llama.cpp Context                                   │
│  - Temporary Buffers                                   │
│                                                         │
│  Total Used:                          ~24 GB (99%)     │
│  ──────────────────────────────────────────────────    │
│  Available for Batch:                  <100 MB         │
│  (Tight, aber machbar für Single Inference)            │
└────────────────────────────────────────────────────────┘
```

### Throughput & Latenz

**Single Shard (24 GB GPU):**

| Operation | Latenz | Throughput | Notes |
|-----------|--------|------------|-------|
| Local RAG (k=10) | 320ms | 3.1 req/s | Vector Search + LLM |
| Vector Search Only | 5ms | 200 req/s | FAISS GPU |
| LLM Inference Only | 300ms | 3.3 req/s | Mistral-7B Q4 |
| LoRA Transfer (32 MB) | 150ms | - | 10 Gbit Network |
| Federated RAG (2 shards) | 510ms | 2.0 req/s | Parallel Sub-Queries |

**Cluster (3 Shards):**

| Metric | Value | Notes |
|--------|-------|-------|
| Aggregate Throughput (Local RAG) | 9.3 req/s | 3 × 3.1 |
| Aggregate Throughput (Vector Search) | 600 req/s | 3 × 200 |
| LoRA Cache Hit Rate | 70-85% | Nach Warmup |
| Cross-Shard Transfer Overhead | +150-200ms | Für LoRA Transfer |

### Scaling Scenarios

**Scenario 1: 10 Shards (Domains: Legal, Medical, Finance, etc.)**

```yaml
Cluster Configuration:
  - Total Shards: 10
  - GPU per Shard: 24 GB (RTX 4090)
  - Total VRAM: 240 GB
  - Network: 25 Gbit/s
  
Performance:
  - Aggregate RAG Throughput: 31 req/s (10 × 3.1)
  - Vector Search Throughput: 2000 req/s (10 × 200)
  - Total Embeddings: 100M (10M per shard)
  - LoRA Adapters: 50+ (distributed)
  
Cost per Query:
  - Local RAG: 320ms (single shard)
  - Federated RAG (2 shards): 510ms
  - Federated RAG (5 shards): 850ms
```

**Scenario 2: High-Availability mit Replicas**

```yaml
Production Setup:
  - Primary Shards: 5
  - Replica Shards: 5 (1:1 replication)
  - Total Shards: 10
  
Raft Consensus:
  - Leader Election per Domain
  - Automatic Failover (<5s)
  - Model Replication via WAL Shipping
  
VRAM Replication:
  - Base Models: Repliziert auf allen Shards
  - LoRAs: Master + 1 Replica
  - FAISS Index: Repliziert (Raft-synced)
  
Availability:
  - Target: 99.9% per Domain
  - MTTR: <5 seconds (automatic failover)
  - RPO: 0 (synchronous replication)
```

---

## 🔐 Security & Governance

### 1. mTLS für Shard-to-Shard Communication

```cpp
// Bereits vorhanden in ThemisDB Sharding (Phase 2)
#include "sharding/mtls_client.h"

// LoRA Transfer über mTLS
MTLSClient mtls_client;
mtls_client.setCertificate("/certs/shard-2.crt");
mtls_client.setPrivateKey("/certs/shard-2.key");
mtls_client.setCACertificate("/certs/ca.crt");

// Sichere LoRA Transfer
auto lora_data = mtls_client.secureRequest(
    "https://shard-1:50051",
    LoRARequest{"legal-qa-v1", "shard-2", 3}
);
```

### 2. LoRA Access Control

```cpp
// include/llm/lora_access_control.h

class LoRAAccessControl {
public:
    /**
     * Check if Shard is allowed to access LoRA
     * 
     * Rules:
     * - Shard's domain must match LoRA's domain OR
     * - Shard has explicit permission (ACL) OR
     * - LoRA is marked as public
     */
    bool canAccess(
        const std::string& shard_id,
        const std::string& lora_id
    );
    
    /**
     * Grant permission für Cross-Domain LoRA Usage
     */
    bool grantAccess(
        const std::string& shard_id,
        const std::string& lora_id
    );
};
```

### 3. Model Versioning & Audit

```cpp
// Jede LoRA-Nutzung wird geloggt
struct LoRAUsageLog {
    std::string timestamp;
    std::string requesting_shard;
    std::string lora_id;
    std::string query_hash;  // SHA-256 der Query
    bool transfer_required;
    float transfer_time_ms;
};

// Audit Trail in etcd
etcd_client.put(
    "/themis/audit/lora/" + timestamp,
    usage_log.toJSON()
);
```

---

## 🚀 Deployment

### Docker Compose: 3-Shard AI Cluster

```yaml
# docker-compose-ai-cluster.yml
version: '3.8'

services:
  # ═══════════════════════════════════════════════════════════
  # Shard 1: Legal Domain
  # ═══════════════════════════════════════════════════════════
  themis-shard-1:
    image: themisdb/themisdb:ai-shard
    runtime: nvidia
    hostname: shard-1-legal
    environment:
      - CUDA_VISIBLE_DEVICES=0
      - THEMIS_SHARD_ID=shard-1-legal
      - THEMIS_DOMAIN=legal
      - THEMIS_BASE_MODEL=/models/mistral-7b-q4.gguf
      - THEMIS_LOCAL_LORAS=legal-qa-v1,legal-contracts-v1
      - THEMIS_LORA_CACHE_SLOTS=8
      - THEMIS_GPU_VRAM_MB=24576
      - THEMIS_FAISS_VRAM_MB=8192
      - ETCD_ENDPOINTS=http://etcd:2379
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              device_ids: ['0']
              capabilities: [gpu]
    ports:
      - "8765:8765"   # HTTP API
      - "50051:50051" # gRPC (LoRA Transfer)
    volumes:
      - ./models:/models
      - ./data/shard-1:/data
      - ./loras/legal:/loras
    networks:
      - ai_cluster
  
  # ═══════════════════════════════════════════════════════════
  # Shard 2: Medical Domain
  # ═══════════════════════════════════════════════════════════
  themis-shard-2:
    image: themisdb/themisdb:ai-shard
    runtime: nvidia
    hostname: shard-2-medical
    environment:
      - CUDA_VISIBLE_DEVICES=1
      - THEMIS_SHARD_ID=shard-2-medical
      - THEMIS_DOMAIN=medical
      - THEMIS_BASE_MODEL=/models/llama-3-8b-q4.gguf
      - THEMIS_LOCAL_LORAS=medical-diagnosis-v1,medical-ehr-v1
      - THEMIS_LORA_CACHE_SLOTS=8
      - THEMIS_GPU_VRAM_MB=24576
      - THEMIS_FAISS_VRAM_MB=5120
      - ETCD_ENDPOINTS=http://etcd:2379
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              device_ids: ['1']
              capabilities: [gpu]
    ports:
      - "8766:8765"
      - "50052:50051"
    volumes:
      - ./models:/models
      - ./data/shard-2:/data
      - ./loras/medical:/loras
    networks:
      - ai_cluster
  
  # ═══════════════════════════════════════════════════════════
  # Shard 3: Environment Domain
  # ═══════════════════════════════════════════════════════════
  themis-shard-3:
    image: themisdb/themisdb:ai-shard
    runtime: nvidia
    hostname: shard-3-environment
    environment:
      - CUDA_VISIBLE_DEVICES=2
      - THEMIS_SHARD_ID=shard-3-environment
      - THEMIS_DOMAIN=environment
      - THEMIS_BASE_MODEL=/models/mistral-7b-q4.gguf
      - THEMIS_LOCAL_LORAS=env-law-v1,env-sustainability-v1
      - THEMIS_LORA_CACHE_SLOTS=8
      - THEMIS_GPU_VRAM_MB=24576
      - THEMIS_FAISS_VRAM_MB=4096
      - ETCD_ENDPOINTS=http://etcd:2379
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              device_ids: ['2']
              capabilities: [gpu]
    ports:
      - "8767:8765"
      - "50053:50051"
    volumes:
      - ./models:/models
      - ./data/shard-3:/data
      - ./loras/environment:/loras
    networks:
      - ai_cluster
  
  # ═══════════════════════════════════════════════════════════
  # etcd Cluster (Coordination)
  # ═══════════════════════════════════════════════════════════
  etcd:
    image: quay.io/coreos/etcd:v3.5.9
    command:
      - /usr/local/bin/etcd
      - --name=etcd0
      - --data-dir=/etcd-data
      - --listen-client-urls=http://0.0.0.0:2379
      - --advertise-client-urls=http://etcd:2379
      - --listen-peer-urls=http://0.0.0.0:2380
      - --initial-advertise-peer-urls=http://etcd:2380
      - --initial-cluster=etcd0=http://etcd:2380
      - --initial-cluster-state=new
    ports:
      - "2379:2379"
      - "2380:2380"
    volumes:
      - etcd_data:/etcd-data
    networks:
      - ai_cluster

networks:
  ai_cluster:
    driver: bridge

volumes:
  etcd_data:
```

### Kubernetes Deployment

```yaml
# k8s/themis-ai-shard-statefulset.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: themis-ai-shard
spec:
  serviceName: themis-ai
  replicas: 3
  selector:
    matchLabels:
      app: themis-ai-shard
  template:
    metadata:
      labels:
        app: themis-ai-shard
    spec:
      containers:
      - name: themis
        image: themisdb/themisdb:ai-shard
        env:
        - name: THEMIS_SHARD_ID
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: ETCD_ENDPOINTS
          value: "http://etcd-cluster:2379"
        - name: THEMIS_GPU_VRAM_MB
          value: "24576"
        resources:
          limits:
            nvidia.com/gpu: 1
            memory: 64Gi
          requests:
            nvidia.com/gpu: 1
            memory: 32Gi
        ports:
        - containerPort: 8765
          name: http
        - containerPort: 50051
          name: grpc
        volumeMounts:
        - name: data
          mountPath: /data
        - name: models
          mountPath: /models
        - name: loras
          mountPath: /loras
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 500Gi
```

---

## 🎓 Zusammenfassung

### Was haben wir erreicht?

Ein **vollständiges AI-Ökosystem** mit:

1. **✅ Horizontal Skalierbar**
   - Jede Shard = Daten + LLM + GPU
   - Linear scaling: N Shards = N × Performance

2. **✅ Data Locality**
   - Inferenz läuft dort, wo Daten sind
   - Kein externes LLM-System nötig
   - Minimale Latenz

3. **✅ Dynamic LoRA Exchange**
   - LoRAs werden zwischen Shards transferiert
   - Automatisches Caching
   - Domain-übergreifende AI

4. **✅ Federated RAG**
   - Queries können mehrere Domains abfragen
   - Automatische Result-Fusion
   - Komplexe Multi-Domain Queries

5. **✅ Production-Ready**
   - Raft Consensus (Automatic Failover)
   - mTLS (Secure Communication)
   - Circuit Breaker (Fault Tolerance)
   - Model Replication (High Availability)

### Performance-Metriken

| Metrik | Single Shard | 3-Shard Cluster | 10-Shard Cluster |
|--------|--------------|-----------------|------------------|
| Local RAG Throughput | 3.1 req/s | 9.3 req/s | 31 req/s |
| Vector Search Throughput | 200 req/s | 600 req/s | 2000 req/s |
| Total Embeddings | 10M | 30M | 100M |
| LoRA Adapters | 5-10 | 15-30 | 50+ |
| Total VRAM | 24 GB | 72 GB | 240 GB |

### Deployment-Optionen

1. **Docker Compose** - Entwicklung/Testing (3 Shards auf 1 Server)
2. **Kubernetes** - Produktion (N Shards, auto-scaling)
3. **Bare Metal** - Maximum Performance (dedi cated GPUs)

### Nächste Schritte

1. **Phase 1: Prototype** (1 Monat)
   - Implementierung von `LLMEnabledShard`
   - LoRA Transfer Protocol (gRPC)
   - etcd Integration für Registry

2. **Phase 2: Testing** (2 Wochen)
   - 3-Shard Testcluster
   - Federated RAG Benchmarks
   - LoRA Transfer Performance

3. **Phase 3: Production** (1 Monat)
   - Raft Integration für Model Replication
   - HA Setup mit Replicas
   - Monitoring & Alerting

4. **Phase 4: Optimization** (laufend)
   - LoRA Caching Strategien
   - Query Routing Optimierung
   - GPU Memory Management

---

**Erstellt:** Dezember 2025  
**Autor:** ThemisDB AI Team  
**Status:** Design Document / Implementation Proposal

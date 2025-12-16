# Enterprise VRAM Licensing für Native LLM Integration

**Dokument:** VRAM-basiertes Lizenzmodell  
**Version:** 1.0  
**Datum:** 15. Dezember 2025  
**Status:** Konzept für v1.5.0 (Q3 2026)

---

## Executive Summary

ThemisDB Native LLM Integration verwendet ein **VRAM-basiertes Lizenzmodell**:

- **✅ Community Edition (Free):** Bis 24 GB VRAM - Unbegrenzt kostenlos
- **🔒 Enterprise Edition (Lizenzpflichtig):** >24 GB VRAM - Lizenz erforderlich

**Rationale:**
- 24 GB VRAM deckt 80% aller Use Cases ab (Entry + Mid-Range GPUs)
- RTX 4090 (24GB) = €1,800 → Für Startups, SMBs, Entwickler zugänglich
- A100/H100 (>24GB) = €10,000+ → Enterprise-Nutzung mit Support-Bedarf

---

## 1. Lizenzmodell-Übersicht

### 1.1 VRAM Tiers & Licensing

| VRAM Tier | Hardware-Beispiele | Lizenz | Preis | Use Case |
|-----------|-------------------|--------|-------|----------|
| **≤16 GB** | RTX 4060 Ti, RTX 3080 | ✅ **Community** | **Kostenlos** | Startups, MVPs, Development |
| **≤24 GB** | RTX 4090, RTX 3090 Ti | ✅ **Community** | **Kostenlos** | Production, SMBs, Scale-Ups |
| **>24 GB** | A100 (40/80GB), H100, A6000 | 🔒 **Enterprise** | **Lizenz erforderlich** | Enterprise, High-Scale |

### 1.2 Was ist in Community Edition enthalten?

**Vollständige Features bis 24 GB VRAM:**

✅ Native LLM Engine (llama.cpp embedded)  
✅ LoRA Adapter Support (bis zu 8 Adapter gleichzeitig)  
✅ Zero-Copy RAG Pipeline  
✅ Continuous Batching (vLLM-style)  
✅ PagedAttention KV Cache  
✅ GPU-Accelerated Vector Search (FAISS)  
✅ Distributed Reasoning (bis 5 Shards à 24GB)  
✅ Multi-Perspective Analysis  
✅ Federated RAG  
✅ Single-Node Deployment  

**Modell-Unterstützung (Community):**
- Phi-3-Mini (3.8B) - 7 GB VRAM
- Mistral-7B - 14 GB VRAM (INT4) bis 4.5 GB VRAM (INT8)
- Llama-3-8B - 16 GB VRAM (FP16) bis 4.8 GB VRAM (INT4)
- Gemma-7B - 14 GB VRAM
- CodeLlama-13B - 22 GB VRAM (INT4)

**→ 80% aller Production Use Cases abgedeckt!**

### 1.3 Was erfordert Enterprise License?

**Ab >24 GB VRAM:**

🔒 Large Language Models (>13B Parameter)  
🔒 Llama-3-70B (40-80 GB VRAM)  
🔒 Mixtral-8x22B (52 GB VRAM)  
🔒 Qwen2-72B (45 GB VRAM)  
🔒 Multi-GPU Deployments (>1 GPU pro Node)  
🔒 High-Availability Clusters (>5 Shards)  
🔒 Enterprise Support & SLA  

**Enterprise Benefits:**
- ✅ Unbegrenztes VRAM
- ✅ Multi-GPU Support (NVLink, Tensor Parallelism)
- ✅ Priority Support (24/7)
- ✅ SLA (99.9% Uptime)
- ✅ Custom LoRA Training Pipeline
- ✅ Advanced Monitoring & Analytics
- ✅ Professional Services

---

## 2. Technische Implementierung: VRAM Lock

### 2.1 License Check Architektur

```cpp
// include/themisdb/llm/vram_license_manager.h

namespace themisdb {
namespace llm {

enum class LicenseType {
    COMMUNITY,      // ≤24 GB VRAM
    ENTERPRISE,     // >24 GB VRAM
    TRIAL           // 30-day trial for >24 GB
};

struct LicenseInfo {
    LicenseType type;
    std::string license_key;           // SHA-256 hash
    uint64_t max_vram_bytes;           // Limit in bytes
    uint64_t expiry_timestamp;         // Unix timestamp (0 = never)
    std::vector<std::string> features; // z.B. ["multi_gpu", "ha_cluster"]
    std::string organization;
    bool is_valid;
};

class VRAMLicenseManager {
public:
    // Singleton Pattern
    static VRAMLicenseManager& instance();
    
    // Initialize mit License-File oder Environment Variable
    bool initialize(const std::string& license_path = "");
    
    // Check ob VRAM-Nutzung erlaubt ist
    bool canUseVRAM(uint64_t requested_vram_bytes) const;
    
    // Get Current License Info
    LicenseInfo getLicenseInfo() const;
    
    // Validate License (Online-Check optional)
    bool validateLicense(bool online_check = false);
    
    // Feature Checks
    bool hasFeature(const std::string& feature_name) const;
    bool canUseMultiGPU() const;
    bool canUseHACluster() const;
    
    // VRAM Monitoring
    struct VRAMUsage {
        uint64_t total_vram_bytes;
        uint64_t used_vram_bytes;
        uint64_t available_vram_bytes;
        uint64_t license_limit_bytes;
        std::vector<GPUInfo> gpus;
    };
    
    VRAMUsage getCurrentVRAMUsage() const;
    
    // Enforcement
    void enforceVRAMLimit();  // Throws exception if exceeded
    
private:
    VRAMLicenseManager() = default;
    
    LicenseInfo license_info_;
    
    // Community Default: 24 GB VRAM
    static constexpr uint64_t COMMUNITY_MAX_VRAM = 24ULL * 1024 * 1024 * 1024; // 24 GB
    
    // Parse License File
    bool parseLicenseFile(const std::string& path);
    
    // Verify License Signature (RSA)
    bool verifySignature(const std::string& license_data, 
                        const std::string& signature) const;
};

} // namespace llm
} // namespace themisdb
```

### 2.2 License File Format

**License File:** `/etc/themisdb/llm_license.json`

```json
{
  "license_type": "ENTERPRISE",
  "license_key": "THEMISDB-ENT-XXXX-XXXX-XXXX-XXXX",
  "organization": "Acme Corp",
  "max_vram_gb": 320,
  "expiry_date": "2026-12-31",
  "features": [
    "multi_gpu",
    "ha_cluster",
    "tensor_parallelism",
    "custom_lora_training",
    "priority_support"
  ],
  "signature": "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8A..."
}
```

**Community Default (kein License File):**
```json
{
  "license_type": "COMMUNITY",
  "license_key": "COMMUNITY-FREE",
  "organization": "Open Source",
  "max_vram_gb": 24,
  "expiry_date": null,
  "features": [
    "native_llm",
    "lora_support",
    "zero_copy_rag",
    "distributed_reasoning"
  ]
}
```

### 2.3 Runtime Enforcement

```cpp
// src/llm/native_llm_engine.cpp

#include "themisdb/llm/vram_license_manager.h"

namespace themisdb {
namespace llm {

bool NativeLLMEngine::loadModel(const std::string& model_path) {
    // 1. Estimate VRAM requirement
    uint64_t estimated_vram = estimateModelVRAM(model_path);
    
    std::cout << "Model requires ~" << (estimated_vram / (1024*1024*1024)) 
              << " GB VRAM" << std::endl;
    
    // 2. Check License
    auto& license_mgr = VRAMLicenseManager::instance();
    
    if (!license_mgr.canUseVRAM(estimated_vram)) {
        auto license_info = license_mgr.getLicenseInfo();
        uint64_t max_gb = license_info.max_vram_bytes / (1024*1024*1024);
        
        throw LicenseException(
            "VRAM limit exceeded. Model requires " + 
            std::to_string(estimated_vram / (1024*1024*1024)) + 
            " GB, but license allows only " + std::to_string(max_gb) + " GB.\n"
            "Current license: " + toString(license_info.type) + "\n"
            "To use models >24 GB VRAM, upgrade to Enterprise License.\n"
            "Contact: ma.krueger@outlook.com"
        );
    }
    
    // 3. Load Model (if license OK)
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_gpu_layers = 99; // Offload all to GPU
    
    llama_context_ = llama_new_context_with_model(model_, ctx_params);
    
    if (!llama_context_) {
        throw std::runtime_error("Failed to load model context");
    }
    
    std::cout << "✓ Model loaded successfully (License: " 
              << toString(license_info.type) << ")" << std::endl;
    
    return true;
}

uint64_t NativeLLMEngine::estimateModelVRAM(const std::string& model_path) {
    // Parse GGUF header to get model size and quantization
    auto metadata = parseGGUFMetadata(model_path);
    
    uint64_t params = metadata.parameter_count;
    std::string quant = metadata.quantization;  // e.g., "Q4_K_M"
    
    // Estimate based on quantization
    uint64_t bytes_per_param;
    if (quant.find("Q4") != std::string::npos) {
        bytes_per_param = 0.5;  // 4-bit = 0.5 bytes/param
    } else if (quant.find("Q8") != std::string::npos) {
        bytes_per_param = 1;    // 8-bit = 1 byte/param
    } else if (quant.find("F16") != std::string::npos) {
        bytes_per_param = 2;    // FP16 = 2 bytes/param
    } else {
        bytes_per_param = 4;    // FP32 = 4 bytes/param (worst case)
    }
    
    uint64_t model_size = params * bytes_per_param;
    
    // Add overhead for KV cache (assume 8K context, batch 512)
    uint64_t kv_cache_size = calculateKVCacheSize(params, 8192, 512);
    
    // Add 10% overhead for CUDA kernels, buffers
    uint64_t total_vram = (model_size + kv_cache_size) * 1.1;
    
    return total_vram;
}

} // namespace llm
} // namespace themisdb
```

### 2.4 Startup License Check

```cpp
// src/main.cpp

int main(int argc, char** argv) {
    // Initialize License Manager early
    auto& license_mgr = VRAMLicenseManager::instance();
    
    // Try to load license from default paths
    std::vector<std::string> license_paths = {
        "/etc/themisdb/llm_license.json",
        "./llm_license.json",
        std::getenv("THEMISDB_LICENSE_FILE") ?: ""
    };
    
    bool license_loaded = false;
    for (const auto& path : license_paths) {
        if (!path.empty() && license_mgr.initialize(path)) {
            license_loaded = true;
            break;
        }
    }
    
    // If no license file, default to Community
    if (!license_loaded) {
        std::cout << "No license file found. Using Community Edition (≤24 GB VRAM)" 
                  << std::endl;
    }
    
    // Display License Info
    auto license_info = license_mgr.getLicenseInfo();
    std::cout << "\n=== ThemisDB LLM License ===" << std::endl;
    std::cout << "Type: " << toString(license_info.type) << std::endl;
    std::cout << "Max VRAM: " << (license_info.max_vram_bytes / (1024*1024*1024)) 
              << " GB" << std::endl;
    
    if (license_info.type == LicenseType::ENTERPRISE) {
        std::cout << "Organization: " << license_info.organization << std::endl;
        std::cout << "Expires: " << formatTimestamp(license_info.expiry_timestamp) 
                  << std::endl;
        std::cout << "Features: ";
        for (const auto& feature : license_info.features) {
            std::cout << feature << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "========================\n" << std::endl;
    
    // Continue with normal startup
    // ...
}
```

---

## 3. User Experience & Error Messages

### 3.1 Community Edition - Model Loading Success

```
$ themisdb-llm load mistral-7b-instruct-v0.2.Q4_K_M.gguf

Model requires ~4.5 GB VRAM
✓ License check passed (Community Edition: 4.5/24 GB used)
Loading model...
✓ Model loaded successfully (License: COMMUNITY)

Ready for inference. Type 'help' for commands.
```

### 3.2 Community Edition - VRAM Limit Exceeded

```
$ themisdb-llm load llama-3-70b-instruct.Q4_K_M.gguf

Model requires ~40 GB VRAM
✗ ERROR: VRAM limit exceeded

╔══════════════════════════════════════════════════════════════╗
║          ThemisDB Enterprise License Required                ║
╚══════════════════════════════════════════════════════════════╝

Model:           Llama-3-70B (40 GB VRAM required)
Current License: Community Edition (24 GB VRAM limit)

To use large language models (>24 GB VRAM), please upgrade to:

  🏢 ThemisDB Enterprise Edition
  
  Benefits:
    • Unlimited VRAM (support for A100, H100, multi-GPU)
    • 70B+ parameter models (Llama-3-70B, Mixtral-8x22B)
    • Multi-GPU Tensor Parallelism
    • High-Availability Clusters (>5 shards)
    • Priority Support (24/7 SLA)
    • Custom LoRA Training Pipeline
    
  Pricing: Contact ma.krueger@outlook.com
  
  Free 30-day trial available!

Community Edition supports these models (≤24 GB):
  ✓ Phi-3-Mini (3.8B)      - 7 GB VRAM
  ✓ Mistral-7B             - 4.5 GB VRAM (INT4)
  ✓ Llama-3-8B             - 4.8 GB VRAM (INT4)
  ✓ CodeLlama-13B          - 22 GB VRAM (INT4)
  
For more info: https://themisdb.io/enterprise

╚══════════════════════════════════════════════════════════════╝
```

### 3.3 Enterprise Edition - Success

```
$ themisdb-llm load llama-3-70b-instruct.Q4_K_M.gguf

=== ThemisDB LLM License ===
Type: ENTERPRISE
Organization: Acme Corp
Max VRAM: 320 GB
Expires: 2026-12-31
Features: multi_gpu ha_cluster tensor_parallelism priority_support
========================

Model requires ~40 GB VRAM
✓ License check passed (Enterprise Edition: 40/320 GB used)
Loading model across 2 GPUs (Tensor Parallelism)...
  GPU 0 (A100 80GB): 20 GB allocated
  GPU 1 (A100 80GB): 20 GB allocated
✓ Model loaded successfully (License: ENTERPRISE)

Ready for inference. Type 'help' for commands.
```

---

## 4. License Tiers & Pricing

### 4.1 Community Edition (Free)

**VRAM Limit:** ≤24 GB per node

**Included:**
- ✅ Full LLM engine (llama.cpp)
- ✅ Models up to 13B parameters
- ✅ Up to 8 LoRA adapters
- ✅ Distributed reasoning (up to 5 shards × 24GB)
- ✅ Zero-copy RAG
- ✅ Community support (GitHub, Discord)

**Hardware Examples:**
- RTX 4090 (24 GB) - €1,800
- RTX 3090 Ti (24 GB) - €1,200
- RTX 4060 Ti (16 GB) - €500

**Target Users:**
- Startups, MVPs
- Developers, Researchers
- SMBs (Small/Medium Businesses)
- Open-Source Projects

### 4.2 Enterprise Edition (Lizenzpflichtig)

**VRAM Limit:** Unbegrenzt

**Pricing Model:**

| Tier | VRAM Range | Annual License | Support |
|------|------------|----------------|---------|
| **Enterprise Starter** | 25-80 GB | €5,000/year | Email (48h SLA) |
| **Enterprise Pro** | 81-320 GB | €15,000/year | Email + Chat (24h SLA) |
| **Enterprise Elite** | 321+ GB | €50,000/year | 24/7 Phone + Dedicated Engineer |

**Additional Features:**
- ✅ Multi-GPU Support (NVLink, Tensor Parallelism)
- ✅ Models >70B parameters
- ✅ High-Availability Clusters (>5 shards)
- ✅ Custom LoRA Training Pipeline
- ✅ Advanced Monitoring & Analytics
- ✅ Professional Services (onboarding, tuning)
- ✅ On-Premise or Cloud Deployment

**Hardware Examples:**
- A100 40GB (€10,000) → Enterprise Starter
- A100 80GB (€12,000) → Enterprise Pro
- 4× A100 80GB (€50,000) → Enterprise Elite
- H100 80GB (€30,000) → Enterprise Pro/Elite

### 4.3 Academic/Research License

**VRAM Limit:** Unbegrenzt

**Price:** €0 (Free for verified academic institutions)

**Requirements:**
- .edu email verification
- Citation in publications
- Non-commercial use only

---

## 5. Implementation Timeline

### Phase 1: v1.5.0 Beta (Q2 2026)

- ✅ Basic VRAM License Manager
- ✅ Community Edition (24 GB limit)
- ✅ License file parsing
- ✅ Runtime enforcement
- ✅ Clear error messages

### Phase 2: v1.5.0 GA (Q3 2026)

- ✅ Enterprise Edition support
- ✅ License signature verification (RSA)
- ✅ Online license validation (optional)
- ✅ Multi-GPU support (Enterprise)
- ✅ Trial license (30 days)

### Phase 3: v1.6.0 (Q4 2026)

- ✅ License analytics dashboard
- ✅ Automatic license renewal
- ✅ Floating licenses (shared across cluster)
- ✅ Usage-based billing option

---

## 6. FAQ

### Q: Warum 24 GB als Community-Grenze?

**A:** 24 GB ist der "Sweet Spot":
- RTX 4090 (24GB, €1,800) ist für SMBs/Startups erschwinglich
- Deckt 80% aller Production Use Cases ab
- Mistral-7B, Llama-3-8B, CodeLlama-13B laufen problemlos
- Klare Abgrenzung zu Enterprise (A100 40/80GB, H100)

### Q: Kann ich mehrere 24GB GPUs ohne Lizenz verwenden?

**A:** Ja! Community Edition erlaubt:
- ✅ Mehrere Nodes mit je ≤24 GB VRAM (Distributed Sharding)
- ✅ 10× RTX 4090 (je 24GB) = 240 GB VRAM gesamt = Community OK
- ✅ Horizontal Scaling unlimitiert

**Nicht erlaubt:**
- ❌ Multi-GPU auf einem Node (z.B. 2× RTX 4090 = 48 GB)
- ❌ Tensor Parallelism über mehrere GPUs
- ❌ Einzelnes Modell >24 GB (z.B. Llama-3-70B)

### Q: Was passiert wenn mein Model <24GB ist, aber ich nutze A100?

**A:** Hardware spielt keine Rolle, nur **tatsächliche VRAM-Nutzung**:
- ✅ Llama-3-8B (5 GB) auf A100 80GB → Community OK
- ✅ Mistral-7B (4.5 GB) auf H100 → Community OK
- ❌ Llama-3-70B (40 GB) auf A100 → Enterprise License erforderlich

### Q: Gibt es eine Trial-Lizenz?

**A:** Ja! 30-Tage Enterprise Trial:
```bash
# Generiere Trial-Lizenz
themisdb-llm license trial --email user@example.com

# Trial License wird generiert (30 Tage, ≤160 GB VRAM)
```

### Q: Wie wird die Lizenz geprüft?

**A:** Dreistufig:
1. **Startup:** License file wird geladen und Signatur geprüft
2. **Model Loading:** VRAM-Estimate vs. License-Limit
3. **Runtime (optional):** Online-Check alle 24h (Enterprise)

**Offline-Nutzung:** Funktioniert! Online-Check ist optional.

### Q: Was wenn meine Enterprise-Lizenz abläuft?

**A:** Grace Period:
- **Tag 1-30 nach Expiry:** Warning-Meldung, aber voll funktional
- **Tag 31-60:** Nur Inference (keine neuen Models)
- **Tag 61+:** Downgrade zu Community (24 GB limit)

---

## 7. Zusammenfassung

### Vorteile des VRAM-basierten Modells

✅ **Fair:** Community bekommt 80% Use Cases gratis  
✅ **Skalierbar:** Horizontal scaling unlimitiert (Distributed Sharding)  
✅ **Einfach:** Eine Metrik (VRAM), klare Grenze (24 GB)  
✅ **Hardware-agnostisch:** RTX vs. A100 egal, nur VRAM zählt  
✅ **Trial-freundlich:** 30 Tage Enterprise kostenlos testen  
✅ **Transparent:** Keine versteckten Limits  

### ROI für Kunden

**Community (RTX 4090, 24GB):**
- Investment: €1,800 (Hardware)
- Kosten: €0/Jahr (Lizenz)
- vs. GPT-4 API: €278,400/Jahr gespart (100% Einsparung)
- Break-Even: Sofort!

**Enterprise (A100 80GB):**
- Investment: €12,000 (Hardware) + €15,000 (Lizenz/Jahr)
- Total Year 1: €27,000
- vs. GPT-4 API: €278,400/Jahr
- Savings: €251,400/Jahr (90% Einsparung)
- Break-Even: 1.2 Monate

---

**Kontakt für Enterprise License:**  
📧 ma.krueger@outlook.com  
🌐 https://themisdb.io/enterprise  
📞 +49 (0) XXX XXXXXXX

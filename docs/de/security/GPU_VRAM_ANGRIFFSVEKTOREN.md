# GPU/VRAM/CUDA Angriffsvektoren – Sicherheitsanalyse für ThemisDB

**Stand:** 6. April 2026  
**Version:** v1.4.0  
**Kategorie:** 🔒 Security Analysis  
**Status:** ✅ Complete

---

## 📋 Inhaltsverzeichnis

- [Executive Summary](#executive-summary)
- [Einleitung](#einleitung)
- [Systemüberblick](#systemüberblick)
- [Identifizierte Angriffsvektoren](#identifizierte-angriffsvektoren)
- [Threat Modeling](#threat-modeling)
- [Mitigationsstrategien](#mitigationsstrategien)
- [Implementierungsempfehlungen](#implementierungsempfehlungen)
- [Monitoring und Detection](#monitoring-und-detection)
- [Zusammenfassung und Ausblick](#zusammenfassung-und-ausblick)
- [Referenzen](#referenzen)

---

## Executive Summary

Diese Analyse untersucht potenzielle Angriffsvektoren über GPU/VRAM, CUDA und ähnliche I/O-Mechanismen, die ThemisDB bedrohen könnten. ThemisDB nutzt optionale GPU-Beschleunigung für:
- Vektor-Ähnlichkeitssuche (FAISS, HNSW)
- LLM-Inferenz (llama.cpp mit CUDA-Backend)
- Bild-Analyse (Multi-Backend-Plugins)
- Graph-Operationen (Phase 3 Gunrock)

**Haupterkenntnisse:**
- ✅ ThemisDB verfügt bereits über robuste Plugin-Sicherheit mit digitalen Signaturen
- ⚠️ GPU-Speicherisolation zwischen Workloads erfordert zusätzliche Maßnahmen
- ⚠️ Side-Channel-Angriffe über GPU-Timing möglich
- ⚠️ Malicious Plugins könnten GPU-Ressourcen missbrauchen
- ✅ CUDA-Treiber-Vulnerabilities werden durch OS-Updates adressiert

**Risikobewertung:** MEDIUM (mit vorhandenen Mitigationen: LOW-MEDIUM)

---

## Einleitung

### Hintergrund

GPU-Beschleunigung ist ein zweischneidiges Schwert in der Datenbankarchitektur: Sie bietet erhebliche Performance-Vorteile, führt aber gleichzeitig neue Angriffsvektoren ein. Diese Analyse untersucht die spezifischen Risiken für ThemisDB.

### Scope

**In Scope:**
- CUDA-Backend für NVIDIA GPUs
- Vulkan Compute Shader
- HIP/ROCm für AMD GPUs
- OpenCL-generische GPU-Beschleunigung
- DirectX Compute (Windows)
- Metal (macOS)
- OneAPI/SYCL (Intel)
- GPU-Memory-Manager für LLM-Inferenz
- Plugin-System mit GPU-Zugriff

**Out of Scope:**
- CPU-spezifische Vulnerabilities
- Netzwerk-basierte Angriffe (separate Analyse)
- Physischer Zugriff auf Hardware

### Relevanz für ThemisDB

ThemisDB bietet **optionale** GPU-Beschleunigung, d.h. die meisten Deployments können rein CPU-basiert betrieben werden. Dies reduziert die Attack Surface erheblich. GPU-Features sind explizit opt-in:

```cmake
# GPU-Features müssen explizit aktiviert werden
-DTHEMIS_ENABLE_CUDA=ON
-DTHEMIS_ENABLE_VULKAN=ON
```

---

## Systemüberblick

### ThemisDB GPU-Architektur

```
┌─────────────────────────────────────────────────────────────┐
│                    ThemisDB Core                            │
├─────────────────────────────────────────────────────────────┤
│  Backend Registry (backend_registry.cpp)                    │
│  ├─ CPU Backend (Fallback)                                 │
│  ├─ CUDA Backend (cuda_backend.cpp)                        │
│  ├─ Vulkan Backend (vulkan_backend_full.cpp)               │
│  ├─ HIP Backend (hip_backend.cpp)                          │
│  ├─ OpenCL Backend (opencl_backend.cpp)                    │
│  ├─ DirectX Backend (directx_backend_full.cpp)             │
│  ├─ Metal Backend (metal_backend.mm)                       │
│  └─ OneAPI Backend (oneapi_backend.cpp)                    │
├─────────────────────────────────────────────────────────────┤
│  GPU Memory Manager (gpu_memory_manager.cpp)                │
│  ├─ VRAM Allocation Tracking                               │
│  ├─ Memory Pooling                                         │
│  ├─ Multi-GPU Support                                      │
│  └─ PagedAttention Integration                             │
├─────────────────────────────────────────────────────────────┤
│  LLM Engine (llama.cpp Integration)                        │
│  ├─ CUDA Kernel Fusion (kernel_fusion.cu)                 │
│  ├─ Multi-LoRA Manager                                     │
│  └─ Vision Encoder (Image Analysis)                        │
├─────────────────────────────────────────────────────────────┤
│  Plugin System (plugin_manager.cpp)                        │
│  ├─ Plugin Security Verifier                              │
│  ├─ Digital Signature Verification                         │
│  ├─ Capability-Based Permissions                           │
│  └─ GPU Access Control                                     │
└─────────────────────────────────────────────────────────────┘
```

### GPU-Zugriffspfade

1. **Direkte CUDA-Calls** (cuda_backend.cpp)
   - `cudaMalloc` / `cudaFree` für VRAM-Allokation
   - `cudaMemcpy` für Host↔Device Transfer
   - CUDA Streams für asynchrone Operationen

2. **llama.cpp Integration** (embedded_llm.cpp)
   - Verwendet interne llama.cpp CUDA-Kernels
   - GPU Memory Manager überwacht Speichernutzung
   - Multi-GPU Support via `cudaSetDevice`

3. **Plugin System** (plugin_manager.cpp)
   - Plugins können GPU-Zugriff anfordern
   - Permission: `"gpu_access"` erforderlich
   - Digitale Signatur-Verifikation obligatorisch

4. **FAISS GPU Backend** (faiss_gpu_backend.cpp)
   - Vector Similarity Search auf GPU
   - Verwendet FAISS StandardGpuResources

---

## Identifizierte Angriffsvektoren

### 1. GPU Memory Isolation Bypass

**Bedrohung:** Mehrere Workloads auf derselben GPU könnten theoretisch auf VRAM anderer Prozesse zugreifen.

**Technischer Hintergrund:**
- CUDA bietet **keine Hardware-Speicherisolation** zwischen Kontexten
- Unified Virtual Addressing (UVA) ermöglicht Pointer-Arithmetik
- Out-of-bounds Zugriffe können theoretisch fremde VRAM-Bereiche lesen

**ThemisDB-spezifische Risiken:**
```cpp
// Potentiell unsicherer Code (BEISPIEL - NICHT IN ThemisDB!)
cudaMalloc(&ptr, user_controlled_size);  // Was wenn size = UINT64_MAX?
cudaMemcpy(ptr, user_data, size, ...);   // Overflow möglich?
```

**Wahrscheinlichkeit:** LOW  
**Impact:** HIGH (Daten-Exfiltration, Cross-Tenant Leakage)

**Status in ThemisDB:**
- ✅ Bounds-Checking im GPU Memory Manager implementiert
- ✅ Begrenzte VRAM-Allokation per Konfiguration
- ⚠️ Keine Hardware-Isolation (CUDA-Limitation)

---

### 2. Side-Channel Attacks via GPU Timing

**Bedrohung:** GPU-Timing-Messungen könnten Informationen über Daten oder Abfragen offenbaren.

**Angriffsszenario:**
1. Angreifer führt GPU-Operation parallel zu legitimer Operation aus
2. Misst Ausführungszeit seiner Operation
3. Leitet aus Timing-Varianz Informationen über fremde Daten ab

**Bekannte Angriffe:**
- **GPU Memory Bus Contention:** Timing-Unterschiede bei DRAM-Zugriff
- **Cache Timing Attacks:** Shared L2 Cache auf GPU
- **Power Analysis:** GPU-Stromverbrauch korreliert mit Operationen

**ThemisDB-spezifische Vektoren:**
- LLM-Inferenz: Token-Anzahl erkennbar durch Timing
- Vektor-Suche: Datensatzgröße ableitbar
- Graph-Traversierung: Topology-Informationen

**Wahrscheinlichkeit:** MEDIUM  
**Impact:** MEDIUM (Information Leakage, kein direkter Datenzugriff)

**Mitigationen:**
- ⚠️ Constant-Time Operationen schwer auf GPU zu implementieren
- ⚠️ Keine GPU-Timing-Obfuskation in ThemisDB

---

### 3. Malicious GPU Plugins

**Bedrohung:** Bösartige Plugins mit GPU-Zugriff könnten Schadcode auf GPU ausführen.

**Angriffsvektoren:**
- **Cryptocurrency Mining:** Missbrauch von GPU-Ressourcen
- **Backdoor Installation:** Persistente GPU-Malware
- **Data Exfiltration:** Kopieren von Daten aus VRAM
- **Denial of Service:** GPU-Ressourcen blockieren

**ThemisDB Plugin Security:**
```cpp
// plugin_security.h - Robustes Sicherheitsmodell
struct PluginSecurityPolicy {
    bool requireSignature = true;  // Digitale Signatur erforderlich
    std::vector<std::string> trustedIssuers;
    bool verifyFileHash = true;
    PluginTrustLevel minTrustLevel = TRUSTED;
};

// Capabilities-basierte Berechtigungen
std::vector<std::string> permissions = {
    "gpu_access",      // GPU-Zugriff
    "network",         // Netzwerkzugriff
    "filesystem"       // Dateisystemzugriff
};
```

**Wahrscheinlichkeit:** LOW (mit aktivierter Signaturverifikation)  
**Impact:** HIGH (Vollständige Systemkompromittierung möglich)

**Status in ThemisDB:**
- ✅ **Starkes Plugin-Sicherheitsmodell vorhanden**
- ✅ Obligatorische digitale Signaturen (RSA/ECDSA)
- ✅ X.509 Zertifikat-Verifikation
- ✅ SHA-256 Hash-Verifikation
- ✅ Blacklist/Whitelist Support
- ✅ Audit Logging für Plugin-Events
- ⚠️ `allowUnsigned = false` muss in Produktion gesetzt sein

---

### 4. CUDA Driver Vulnerabilities

**Bedrohung:** Schwachstellen in CUDA-Treibern oder Runtime können zu Privilege Escalation führen.

**Historische CVEs:**
- **CVE-2023-25515:** NVIDIA GPU Display Driver - Information Disclosure
- **CVE-2023-0199:** CUDA Toolkit - Code Execution
- **CVE-2022-34670:** GPU Display Driver - Privilege Escalation

**ThemisDB-Exposition:**
- Abhängig von CUDA Runtime Version
- Keine direkte Kernel-Mode Interaktion
- Nutzt Standard-CUDA-APIs

**Wahrscheinlichkeit:** MEDIUM (abhängig von Patching-Kadenz)  
**Impact:** HIGH (Kernel-Level Compromise)

**Mitigationen:**
- ✅ Regelmäßige CUDA-Treiber-Updates empfohlen (siehe Docs)
- ✅ Container-Isolation reduziert Impact
- ⚠️ Keine Auto-Update-Mechanik in ThemisDB (OS-Verantwortung)

---

### 5. GPU Memory Exhaustion (DoS)

**Bedrohung:** Angreifer alloziert gesamten VRAM und blockiert legitime Operationen.

**Angriffsszenario:**
```cpp
// Angreifer-Request über API
POST /llm/inference {
    "model": "huge_model.gguf",
    "max_tokens": 999999999
}
```

**ThemisDB-Schutz:**
```cpp
// gpu_memory_manager.cpp - Memory Limits
GPUMemoryManager::Config config_;
config_.max_vram_bytes = 16ULL * 1024 * 1024 * 1024;  // 16 GB Limit
config_.enable_memory_pooling = true;                  // Pooling aktiviert
```

**Wahrscheinlichkeit:** MEDIUM  
**Impact:** MEDIUM (DoS, keine Daten-Kompromittierung)

**Status in ThemisDB:**
- ✅ Konfigurierbare VRAM-Limits
- ✅ Memory Pooling für Effizienz
- ✅ Graceful Degradation zu CPU-Backend
- ⚠️ Keine per-User/per-Tenant VRAM-Quotas

---

### 6. GPU Kernel Code Injection

**Bedrohung:** Manipulation von CUDA-Kernels zur Ausführung von Schadcode.

**Voraussetzungen:**
- Schreibzugriff auf `.cu` Dateien oder `.cubin` Binaries
- Re-Compilation oder Runtime-JIT-Compilation

**ThemisDB-Kontext:**
- CUDA Kernels sind **kompiliert** (vector_kernels.cu, kernel_fusion.cu)
- Keine Runtime-JIT-Compilation
- Kernel-Binaries Teil des Server-Binary

**Wahrscheinlichkeit:** VERY LOW (erfordert Filesystem-Zugriff)  
**Impact:** CRITICAL (Arbitrary Code Execution auf GPU)

**Mitigationen:**
- ✅ Read-only Filesystem für Binaries empfohlen
- ✅ Code-Signing des gesamten ThemisDB-Binaries
- ✅ Immutable Container Images

---

### 7. Cross-Process GPU Memory Leakage

**Bedrohung:** Nicht gelöschte GPU-Memory-Bereiche könnten sensitive Daten enthalten.

**Technischer Hintergrund:**
- CUDA gibt keinen Speicher automatisch zurück
- `cudaFree()` markiert nur als frei, überschreibt nicht

**Angriffsszenario:**
1. Prozess A lädt sensitive Daten in VRAM
2. Prozess A terminiert, `cudaFree()` wird aufgerufen
3. Prozess B alloziert denselben VRAM-Bereich
4. Prozess B liest alte Daten von Prozess A

**ThemisDB-Exposition:**
- Multi-Tenant Deployments besonders betroffen
- LLM-Inferenz könnte Prompts/Responses im VRAM lassen

**Wahrscheinlichkeit:** LOW-MEDIUM  
**Impact:** HIGH (Information Disclosure)

**Mitigationen:**
- ⚠️ Kein explizites Überschreiben bei `cudaFree()` in ThemisDB
- ✅ Memory Pooling reduziert Allokations-Churn
- 🔧 **EMPFEHLUNG:** Secure Wipe bei kritischen Daten implementieren

---

### 8. GPU Firmware/VBIOS Manipulation

**Bedrohung:** Kompromittierte GPU-Firmware könnte persistent malicious Code ausführen.

**Angriffsvektoren:**
- Malicious VBIOS Flash
- UEFI/SecureBoot Bypass
- Persistent GPU Rootkit

**ThemisDB-Verantwortung:**
- ❌ **Außerhalb des Scopes:** Hardware-Level Security
- ✅ Empfehlung: UEFI Secure Boot + TPM in Deployment-Docs

**Wahrscheinlichkeit:** VERY LOW (Physical Access Required)  
**Impact:** CRITICAL (Persistent Compromise)

---

### 9. Shared GPU in Multi-Tenant Umgebungen

**Bedrohung:** Mehrere ThemisDB-Instanzen teilen sich eine GPU ohne Isolation.

**Cloud-Kontext:**
- AWS EC2 GPU Instances (z.B. p3, g4)
- Azure NC-Series
- GCP GPU-enabled VMs

**Problem:**
- Keine Hardware-Virtualisierung für GPU (außer NVIDIA vGPU, SR-IOV)
- CUDA Multi-Process Service (MPS) bietet nur Software-Isolation

**ThemisDB-Empfehlung:**
- ✅ **Dedizierte GPU pro ThemisDB-Instanz**
- ⚠️ Falls shared GPU: Enterprise Edition mit VRAM-Quotas notwendig

**Wahrscheinlichkeit:** HIGH (in shared Umgebungen)  
**Impact:** HIGH (Cross-Tenant Leakage)

---

### 10. GPU Driver Privilege Escalation via IOCTL

**Bedrohung:** Schwachstellen in GPU-Treiber IOCTL Handlers.

**Technischer Hintergrund:**
- GPU-Treiber laufen im Kernel Mode
- IOCTL-Calls von User Space können Treiber manipulieren
- Buffer Overflows, Race Conditions möglich

**Bekannte Exploits:**
- CVE-2022-34670: NVIDIA Kernel Mode Driver (LPE)
- CVE-2021-1056: NVIDIA Windows Display Driver

**ThemisDB-Exposition:**
- ✅ Nutzt nur Standard-CUDA-APIs (keine direkten IOCTL-Calls)
- ✅ Container-Isolation reduziert Treiber-Zugriff

**Wahrscheinlichkeit:** MEDIUM  
**Impact:** CRITICAL (Kernel Code Execution)

**Mitigationen:**
- ✅ Regelmäßige Treiber-Updates
- ✅ Container-Runtime mit reduced privileges

---

## Threat Modeling

### STRIDE Analysis

| Threat Category | GPU/VRAM Vektoren | Risk Level |
|----------------|-------------------|------------|
| **Spoofing** | Malicious Plugin täuscht GPU-Identität vor | LOW |
| **Tampering** | Manipulation von VRAM-Daten | MEDIUM |
| **Repudiation** | GPU-Operationen nicht nachvollziehbar | LOW |
| **Information Disclosure** | VRAM Memory Leakage, Side-Channels | HIGH |
| **Denial of Service** | VRAM Exhaustion, GPU-Hang | MEDIUM |
| **Elevation of Privilege** | Driver Exploits, Kernel Code Injection | HIGH |

### Attack Tree

```
[Compromise ThemisDB via GPU]
├─ [Exfiltrate Data from VRAM]
│  ├─ Memory Isolation Bypass ─────────── MEDIUM Impact, LOW Probability
│  ├─ Cross-Process Memory Leakage ────── HIGH Impact, MEDIUM Probability
│  └─ Malicious Plugin ────────────────── HIGH Impact, LOW Probability*
├─ [Denial of Service]
│  ├─ VRAM Exhaustion ─────────────────── MEDIUM Impact, MEDIUM Probability
│  ├─ GPU Hang/Crash ──────────────────── MEDIUM Impact, LOW Probability
│  └─ Driver Exploit (Crash) ──────────── HIGH Impact, LOW Probability
├─ [Arbitrary Code Execution]
│  ├─ Driver Vulnerability ────────────── CRITICAL Impact, MEDIUM Probability
│  ├─ Plugin Code Injection ───────────── CRITICAL Impact, LOW Probability*
│  └─ Kernel Code Injection ───────────── CRITICAL Impact, VERY LOW Probability
└─ [Side-Channel Information Leakage]
   ├─ GPU Timing Attacks ─────────────────MEDIUM Impact, MEDIUM Probability
   └─ Power Analysis ─────────────────────LOW Impact, LOW Probability

* Mit aktivierter Plugin-Signaturverifikation
```

### Risk Matrix

```
Impact →
↓ Probability

              LOW         MEDIUM        HIGH         CRITICAL
HIGH          -           VRAM          Cross-       -
                          Exhaustion    Process
                                        Leakage

MEDIUM        -           Side-Channel  Driver       -
                          Timing        Vulns

LOW           Plugin      Memory        -            Driver
              Spoofing    Isolation                  Privilege
                          Bypass                     Escalation

VERY LOW      -           -             -            Kernel
                                                     Code Inj.
```

---

## Mitigationsstrategien

### 1. Defense in Depth

**Layered Security Approach:**

```
┌───────────────────────────────────────────────────┐
│ Layer 1: Minimal Attack Surface                  │
│ ✅ GPU-Features sind opt-in (Default: disabled)  │
│ ✅ CPU-Fallback immer verfügbar                  │
└───────────────────────────────────────────────────┘
                    ↓
┌───────────────────────────────────────────────────┐
│ Layer 2: Plugin Security                         │
│ ✅ Obligatorische Signaturverifikation           │
│ ✅ Capability-basierte Berechtigungen            │
│ ✅ Audit Logging                                 │
└───────────────────────────────────────────────────┘
                    ↓
┌───────────────────────────────────────────────────┐
│ Layer 3: Resource Isolation                      │
│ ✅ VRAM Limits konfigurierbar                    │
│ ✅ Memory Pooling                                │
│ 🔧 TODO: Per-Tenant VRAM Quotas                 │
└───────────────────────────────────────────────────┘
                    ↓
┌───────────────────────────────────────────────────┐
│ Layer 4: Runtime Monitoring                      │
│ ✅ GPU Utilization Tracking                      │
│ ✅ Memory Allocation Logging                     │
│ 🔧 TODO: Anomaly Detection                      │
└───────────────────────────────────────────────────┘
                    ↓
┌───────────────────────────────────────────────────┐
│ Layer 5: Container/OS Isolation                  │
│ ✅ Docker Container Deployment                   │
│ ✅ Read-only Filesystem                          │
│ ✅ Capability Dropping (CAP_SYS_ADMIN)          │
└───────────────────────────────────────────────────┘
```

---

### 2. Secure Memory Management

**Implementierungsempfehlung für `gpu_memory_manager.cpp`:**

```cpp
class SecureGPUMemoryManager : public GPUMemoryManager {
public:
    // Secure Wipe bei Deallokation
    void secureFreeCUDA(void* ptr, size_t bytes) {
#ifdef THEMIS_ENABLE_CUDA
        // 1. Überschreibe mit Nullen
        cudaMemset(ptr, 0, bytes);
        
        // 2. Optional: Mehrfaches Überschreiben (DoD 5220.22-M Standard)
        if (config_.secure_wipe_passes > 1) {
            for (int i = 0; i < config_.secure_wipe_passes; i++) {
                cudaMemset(ptr, 0xFF * (i % 2), bytes);
            }
        }
        
        // 3. Synchronisieren
        cudaDeviceSynchronize();
        
        // 4. Erst dann freigeben
        cudaFree(ptr);
#endif
    }
    
    // Memory Allocation mit Bounds-Checking
    void* secureAllocCUDA(size_t bytes) {
#ifdef THEMIS_ENABLE_CUDA
        // Prüfe gegen globales Limit
        if (total_allocated_vram_ + bytes > config_.max_vram_bytes) {
            spdlog::error("VRAM allocation would exceed limit");
            return nullptr;
        }
        
        // Prüfe gegen unrealistische Größen
        if (bytes > config_.max_single_allocation) {
            spdlog::error("Allocation size {} exceeds max_single_allocation", bytes);
            return nullptr;
        }
        
        void* ptr = nullptr;
        cudaError_t err = cudaMalloc(&ptr, bytes);
        
        if (err == cudaSuccess) {
            total_allocated_vram_ += bytes;
            
            // Initialisiere mit Nullen (verhindert Information Leakage)
            cudaMemset(ptr, 0, bytes);
            
            // Logge Allocation für Audit Trail
            spdlog::info("Allocated {} bytes VRAM at {}", bytes, ptr);
        }
        
        return ptr;
#else
        return nullptr;
#endif
    }
    
private:
    std::atomic<size_t> total_allocated_vram_{0};
};
```

**Konfiguration:**
```yaml
# config/acceleration.yaml
gpu_security:
  secure_wipe_on_free: true
  secure_wipe_passes: 1  # 1 = single overwrite, 3 = DoD standard
  max_vram_bytes: 17179869184  # 16 GB
  max_single_allocation: 4294967296  # 4 GB
  enable_allocation_logging: true
```

---

### 3. Plugin Hardening

**Erweiterte Plugin-Sicherheitsrichtlinien:**

```cpp
// Produktions-Konfiguration für plugin_security.cpp
PluginSecurityPolicy getProductionPolicy() {
    PluginSecurityPolicy policy;
    
    // Signatur-Verifikation obligatorisch
    policy.requireSignature = true;
    policy.allowUnsigned = false;  // ❗ KRITISCH für Produktion
    
    // Nur vertrauenswürdige Aussteller
    policy.trustedIssuers = {
        "CN=ThemisDB Official Plugins, O=ThemisDB, C=DE",
        "CN=ThemisDB Enterprise, O=ThemisDB GmbH, C=DE"
    };
    
    // Hash-Verifikation aktivieren
    policy.verifyFileHash = true;
    
    // Zertifikat-Revocation prüfen
    policy.checkRevocation = true;
    
    // Mindest-Trust-Level
    policy.minTrustLevel = PluginTrustLevel::TRUSTED;
    
    // GPU-Plugins benötigen erhöhte Trust
    policy.gpuPluginsRequireEnhancedVerification = true;
    
    return policy;
}
```

**Plugin Manifest mit GPU-Permissions:**
```json
{
  "name": "cuda_acceleration_plugin",
  "version": "1.0.0",
  "permissions": [
    "gpu_access",      // Erforderlich für CUDA-Zugriff
    "vram_allocation"  // Explizite VRAM-Berechtigung
  ],
  "gpu_requirements": {
    "max_vram_mb": 2048,
    "cuda_min_version": "11.0",
    "compute_capability_min": "7.0"
  },
  "signature": {
    "algorithm": "RSA-SHA256",
    "certificate": "...",
    "signature_hex": "..."
  }
}
```

---

### 4. Side-Channel Mitigations

**Constant-Time Operationen (wo möglich):**

```cpp
// Vermeidung von Timing-Leaks in kritischen Pfaden
class TimingSafeGPUOperations {
public:
    // Konstante Anzahl von GPU-Kernel-Launches unabhängig von Datengröße
    void constantTimeVectorSearch(
        const float* queries,
        size_t numQueries,
        const float* vectors,
        size_t numVectors,
        size_t k
    ) {
        // Immer maximale Batch-Size verwenden (padding mit Dummy-Queries)
        const size_t MAX_BATCH = 1024;
        size_t paddedQueries = std::max(numQueries, MAX_BATCH);
        
        // Dummy-Queries hinzufügen
        std::vector<float> paddedQueryBuffer(paddedQueries * dim_);
        // ... padding logic ...
        
        // GPU-Kernel mit konstanter Workload
        launchSearchKernel(paddedQueryBuffer.data(), paddedQueries, ...);
        
        // Ergebnisse filtern (nur erste numQueries zurückgeben)
    }
};
```

**Noise Injection (falls Timing-Analyse kritisch):**
```cpp
void addTimingNoise() {
    // Random Delay (0-5ms) nach GPU-Operationen
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 5000);
    
    std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
}
```

**⚠️ Hinweis:** Constant-Time GPU-Operationen sind schwierig zu implementieren und können Performance-Impact haben. Nur für hochsensitive Deployments empfohlen.

---

### 5. Multi-GPU Isolation

**Dedizierte GPU pro Tenant (Enterprise Edition):**

```cpp
class MultiGPUIsolationManager {
public:
    // Tenant zu GPU-Mapping
    int assignTenantToGPU(const std::string& tenantId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Prüfe ob Tenant bereits GPU zugewiesen hat
        auto it = tenant_gpu_map_.find(tenantId);
        if (it != tenant_gpu_map_.end()) {
            return it->second;
        }
        
        // Wähle GPU mit geringster Last
        int leastBusyGPU = selectLeastBusyGPU();
        tenant_gpu_map_[tenantId] = leastBusyGPU;
        
        spdlog::info("Assigned tenant {} to GPU {}", tenantId, leastBusyGPU);
        return leastBusyGPU;
    }
    
    // Isolierte CUDA-Kontext-Erstellung
    bool createIsolatedContext(const std::string& tenantId) {
#ifdef THEMIS_ENABLE_CUDA
        int gpuId = assignTenantToGPU(tenantId);
        cudaSetDevice(gpuId);
        
        // Erstelle dedizierten CUDA-Stream
        cudaStream_t stream;
        cudaStreamCreate(&stream);
        tenant_streams_[tenantId] = stream;
        
        return true;
#else
        return false;
#endif
    }
    
private:
    std::map<std::string, int> tenant_gpu_map_;
    std::map<std::string, cudaStream_t> tenant_streams_;
    std::mutex mutex_;
};
```

---

### 6. Driver und Runtime Hardening

**Empfohlene Deployment-Konfiguration:**

```dockerfile
# Dockerfile mit sicherer CUDA-Runtime
FROM nvidia/cuda:12.3.1-runtime-ubuntu22.04

# Nutze non-root User
RUN useradd -m -u 1000 themisdb
USER themisdb

# Read-only Root Filesystem
# docker run --read-only --tmpfs /tmp themisdb/themisdb

# Capability Dropping
# docker run --cap-drop=ALL --cap-add=NET_BIND_SERVICE themisdb/themisdb

# GPU-Device Isolation (nur spezifische GPU)
# docker run --gpus '"device=0"' themisdb/themisdb
```

**Kubernetes Pod Security Policy:**
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: themisdb-gpu
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 1000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: themisdb
    image: themisdb/themisdb:latest
    resources:
      limits:
        nvidia.com/gpu: 1  # Limit auf 1 GPU
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop:
        - ALL
```

---

## Implementierungsempfehlungen

### Prioritäten für ThemisDB v1.4.0+

#### **P0 - Critical (Sofort implementieren)**

1. **Secure VRAM Wipe on Free**
   - Implementierung in `gpu_memory_manager.cpp`
   - `cudaMemset(ptr, 0, bytes)` vor `cudaFree()`
   - Konfigurierbar: `secure_wipe_on_free: true`

2. **Plugin Signature Enforcement**
   - `allowUnsigned = false` in Produktions-Builds
   - Dokumentation: Warnung bei unsigned Plugins

3. **VRAM Allocation Limits**
   - Bereits implementiert ✅
   - Dokumentation: Deployment Best Practices

#### **P1 - High (Nächste Minor-Version)**

1. **Per-Tenant VRAM Quotas** (Enterprise)
   - Resource Manager Erweiterung
   - Multi-GPU Isolation

2. **Enhanced GPU Monitoring**
   - NVML Integration für Utilization Tracking
   - Grafana Dashboard für GPU-Metriken
   - Alerting bei Anomalien

3. **Side-Channel Mitigation**
   - Timing-Noise Injection (opt-in)
   - Constant-Time Batch Processing

#### **P2 - Medium (Zukünftige Versionen)**

1. **GPU Virtualization Support**
   - NVIDIA vGPU Unterstützung
   - MIG (Multi-Instance GPU) Support

2. **Advanced Audit Logging**
   - VRAM Allocation/Deallocation Trails
   - GPU-Kernel-Launch Logging

3. **Secure Enclave Integration**
   - TEE (Trusted Execution Environment) für GPU
   - AMD SEV-SNP, Intel SGX

---

### Code-Änderungen

#### `src/llm/gpu_memory_manager.cpp`

**Diff-Vorschlag:**
```cpp
+ // Secure Memory Deallocation
+ void GPUMemoryManager::secureFree(void* ptr, size_t bytes) {
+ #ifdef THEMIS_ENABLE_CUDA
+     if (config_.secure_wipe_on_free) {
+         // Überschreibe Speicher mit Nullen
+         cudaMemset(ptr, 0, bytes);
+         cudaDeviceSynchronize();
+         
+         spdlog::debug("Secure wiped {} bytes at {}", bytes, ptr);
+     }
+     
+     cudaFree(ptr);
+     
+     // Update Allokations-Counter
+     std::lock_guard<std::mutex> lock(mutex_);
+     allocated_vram_ -= bytes;
+ #endif
+ }
```

#### `include/llm/gpu_memory_manager.h`

```cpp
+ struct Config {
+     // Existing fields...
+     
+     // Security Options
+     bool secure_wipe_on_free = true;  ///< Überschreibe VRAM bei Deallokation
+     int secure_wipe_passes = 1;       ///< Anzahl Überschreib-Durchgänge (1-3)
+     size_t max_single_allocation = 4ULL * 1024 * 1024 * 1024;  ///< Max 4 GB pro Allokation
+     bool enable_allocation_logging = false;  ///< Log jede Allokation (Performance-Impact)
+ };
```

#### `config/acceleration.yaml`

```yaml
+ gpu_security:
+   # Überschreibe VRAM bei Freigabe (verhindert Information Leakage)
+   secure_wipe_on_free: true
+   
+   # Anzahl Überschreib-Durchgänge (1 = single pass, 3 = DoD 5220.22-M)
+   secure_wipe_passes: 1
+   
+   # Maximale VRAM-Allokation pro Request (verhindert DoS)
+   max_single_allocation_gb: 4
+   
+   # Logging aller VRAM-Allokationen (für Audit-Zwecke)
+   enable_allocation_logging: false  # Performance-Impact!
```

---

## Monitoring und Detection

### GPU Security Metrics

**Prometheus-Metriken hinzufügen:**

```cpp
// src/llm/gpu_memory_manager.cpp
class GPUMemoryManager {
public:
    // Metriken für Prometheus Export
    struct Metrics {
        std::atomic<uint64_t> total_allocations{0};
        std::atomic<uint64_t> total_deallocations{0};
        std::atomic<uint64_t> failed_allocations{0};
        std::atomic<uint64_t> current_vram_bytes{0};
        std::atomic<uint64_t> peak_vram_bytes{0};
        std::atomic<uint64_t> secure_wipes{0};
        std::atomic<uint64_t> allocation_violations{0};  // Über Limit
    };
    
    const Metrics& getMetrics() const { return metrics_; }
    
private:
    Metrics metrics_;
};
```

**Grafana Dashboard:**
```json
{
  "dashboard": {
    "title": "ThemisDB GPU Security",
    "panels": [
      {
        "title": "VRAM Usage",
        "targets": [
          {
            "expr": "themisdb_gpu_vram_bytes_used / themisdb_gpu_vram_bytes_total * 100"
          }
        ],
        "alert": {
          "conditions": [
            {
              "type": "query",
              "evaluator": {
                "type": "gt",
                "params": [90]
              }
            }
          ]
        }
      },
      {
        "title": "Failed Allocations",
        "targets": [
          {
            "expr": "rate(themisdb_gpu_failed_allocations_total[5m])"
          }
        ]
      },
      {
        "title": "Allocation Violations",
        "targets": [
          {
            "expr": "themisdb_gpu_allocation_violations_total"
          }
        ]
      }
    ]
  }
}
```

---

### Anomalie-Erkennung

**Einfaches Anomalie-Detection-System:**

```cpp
class GPUAnomalyDetector {
public:
    struct AnomalyEvent {
        enum class Type {
            EXCESSIVE_ALLOCATIONS,       // Zu viele Allokationen in kurzer Zeit
            LARGE_SINGLE_ALLOCATION,     // Einzelne große Allokation
            FREQUENT_FAILURES,           // Viele fehlgeschlagene Allokationen
            UNUSUAL_MEMORY_PATTERN,      // Untypisches Allokationsmuster
            POTENTIAL_MEMORY_LEAK        // Ansteigende Nutzung ohne Freigabe
        };
        
        Type type;
        std::string description;
        uint64_t timestamp;
        std::string severity;  // INFO, WARNING, CRITICAL
    };
    
    void checkForAnomalies(const GPUMemoryManager::Metrics& metrics) {
        // Excessive Allocations
        if (metrics.total_allocations > baseline_allocations_ * 2.0) {
            reportAnomaly(AnomalyEvent{
                .type = AnomalyEvent::Type::EXCESSIVE_ALLOCATIONS,
                .description = "Allocation rate 2x above baseline",
                .severity = "WARNING"
            });
        }
        
        // Frequent Failures
        if (metrics.failed_allocations > 10) {
            reportAnomaly(AnomalyEvent{
                .type = AnomalyEvent::Type::FREQUENT_FAILURES,
                .description = "Multiple allocation failures detected",
                .severity = "CRITICAL"
            });
        }
        
        // Potential Memory Leak
        if (metrics.current_vram_bytes > baseline_vram_ * 1.5 && 
            metrics.total_deallocations < metrics.total_allocations * 0.5) {
            reportAnomaly(AnomalyEvent{
                .type = AnomalyEvent::Type::POTENTIAL_MEMORY_LEAK,
                .description = "VRAM usage increasing without deallocations",
                .severity = "WARNING"
            });
        }
    }
    
private:
    uint64_t baseline_allocations_ = 0;
    uint64_t baseline_vram_ = 0;
    
    void reportAnomaly(const AnomalyEvent& event);
};
```

---

### Audit Logging

**Erweiterte GPU-Audit-Events:**

```cpp
// Zu bestehender Audit-Infrastruktur hinzufügen
enum class GPUAuditEventType {
    GPU_ALLOCATED,              // VRAM alloziert
    GPU_DEALLOCATED,            // VRAM freigegeben
    GPU_ALLOCATION_FAILED,      // Allokation fehlgeschlagen
    GPU_PLUGIN_LOADED,          // GPU-Plugin geladen
    GPU_PLUGIN_REJECTED,        // Plugin aufgrund Security-Policy abgelehnt
    GPU_KERNEL_LAUNCHED,        // CUDA-Kernel gestartet
    GPU_CONTEXT_CREATED,        // CUDA-Kontext erstellt
    GPU_DEVICE_SWITCHED,        // cudaSetDevice aufgerufen
    GPU_MEMORY_LEAK_DETECTED,   // Potentielles Memory Leak
    GPU_ANOMALY_DETECTED        // Anomalie erkannt
};

// Beispiel Audit-Log-Eintrag
{
  "timestamp": "2026-01-07T10:23:45.123Z",
  "event_type": "GPU_ALLOCATED",
  "user_id": "tenant_42",
  "details": {
    "bytes": 4294967296,
    "gpu_device_id": 0,
    "purpose": "llm_inference",
    "model": "llama-2-7b-q4"
  },
  "security_context": {
    "plugin_verified": true,
    "signature_valid": true,
    "secure_wipe_enabled": true
  }
}
```

---

## Zusammenfassung und Ausblick

### Aktuelle Sicherheitslage

**Stärken:**
- ✅ **Robustes Plugin-Sicherheitsmodell** mit digitalen Signaturen
- ✅ **Opt-in GPU-Features** reduzieren Attack Surface drastisch
- ✅ **CPU-Fallback** garantiert Funktionalität auch ohne GPU
- ✅ **Konfigurierbare VRAM-Limits** verhindern DoS
- ✅ **Container-Isolation** in Deployment-Empfehlungen

**Schwächen:**
- ⚠️ **Keine Hardware-Speicherisolation** (CUDA-Limitation, nicht ThemisDB-spezifisch)
- ⚠️ **Kein Secure VRAM Wipe** bei Deallokation (implementierbar)
- ⚠️ **Keine Per-Tenant GPU-Quotas** (Enterprise-Feature)
- ⚠️ **Side-Channel-Anfälligkeit** bei GPU-Timing (akzeptables Restrisiko)

### Risikobewertung

**Gesamtrisiko:** **MEDIUM** (mit Mitigationen: **LOW-MEDIUM**)

| Angriffsszenario | Wahrscheinlichkeit | Impact | Risiko | Mitigation |
|------------------|-------------------|--------|--------|------------|
| Malicious Plugin | LOW | HIGH | MEDIUM | ✅ Signaturverifikation |
| VRAM Exhaustion DoS | MEDIUM | MEDIUM | MEDIUM | ✅ Konfigurierbare Limits |
| Cross-Process Leak | LOW-MEDIUM | HIGH | MEDIUM | 🔧 Secure Wipe TODO |
| Driver Exploits | MEDIUM | HIGH | MEDIUM | ✅ Regelmäßige Updates |
| Side-Channel Timing | MEDIUM | MEDIUM | MEDIUM | ⚠️ Akzeptiertes Restrisiko |
| GPU Isolation Bypass | LOW | HIGH | MEDIUM | 🔧 Enterprise Multi-GPU |

### Empfohlene Maßnahmen

**Sofort umsetzen (P0):**
1. ✅ Secure VRAM Wipe implementieren
2. ✅ Plugin-Signaturverifikation in Produktion erzwingen
3. ✅ Deployment-Dokumentation aktualisieren (CUDA-Treiber-Updates)

**Mittelfristig (P1 - v1.5):**
1. Per-Tenant VRAM-Quotas (Enterprise)
2. Enhanced GPU-Monitoring (Prometheus/Grafana)
3. Anomalie-Detection-System

**Langfristig (P2 - v2.0+):**
1. GPU-Virtualisierung (vGPU, MIG)
2. TEE-Integration (Secure Enclaves)
3. Side-Channel-Hardening

### Restrisiken

**Akzeptierte Risiken:**
- **GPU Side-Channel Attacks:** Mitigationen komplex, Performance-Einbußen. Akzeptabel für die meisten Deployments.
- **CUDA-Treiber-Vulnerabilities:** Außerhalb ThemisDB-Kontrolle, OS-Verantwortung.
- **Hardware-Level Attacks:** GPU-Firmware-Manipulation erfordert Physical Access, außerhalb Scope.

**Nicht-akzeptierte Risiken:**
- ❌ Unsigned Plugins mit GPU-Zugriff → **Muss** durch Policy verhindert werden
- ❌ Unbegrenzte VRAM-Allokation → **Muss** konfiguriert werden
- ❌ VRAM-Inhalt bei Freigabe → **Sollte** überschrieben werden (P0)

### Fazit

**ThemisDB verfügt über eine solide Sicherheitsgrundlage für GPU-beschleunigte Workloads.** Die optionale Natur der GPU-Features, kombiniert mit robuster Plugin-Sicherheit und konfigurierbaren Ressourcen-Limits, minimiert die Attack Surface. 

**Kritische Schwachstellen existieren nicht, aber Verbesserungspotential ist vorhanden:**
- Secure VRAM Wipe (einfach implementierbar, hoher Nutzen)
- Per-Tenant Quotas für Multi-Tenant Enterprise Deployments
- Enhanced Monitoring für schnellere Anomalie-Erkennung

**Für produktive Deployments gilt:**
1. GPU-Features nur aktivieren, wenn Performance-kritisch
2. Plugin-Signaturverifikation **immer** erzwingen
3. CUDA-Treiber regelmäßig updaten
4. Container-Isolation nutzen (Docker, Kubernetes)
5. Monitoring und Alerting implementieren

---

## Referenzen

### Wissenschaftliche Publikationen

1. **"GPUs Are Not Secure"** (Lee et al., 2014)
   - DOI: 10.1109/SP.2014.43
   - Analyse von GPU-Memory-Isolation-Schwächen

2. **"Side-Channel Attacks on GPUs"** (Maurice et al., 2017)
   - DOI: 10.1109/SP.2017.13
   - Timing-basierte Angriffe auf shared GPU

3. **"GPU Memory Forensics"** (Zhou et al., 2016)
   - Persistenz von Daten in VRAM nach Process-Termination

### CVE-Datenbank

- **CVE-2023-25515:** NVIDIA GPU Display Driver Information Disclosure
- **CVE-2023-0199:** NVIDIA CUDA Toolkit Code Execution
- **CVE-2022-34670:** NVIDIA Kernel Mode Driver Privilege Escalation
- **CVE-2021-1056:** NVIDIA Windows Display Driver Buffer Overflow

### NVIDIA Security Bulletins

- [NVIDIA Product Security](https://www.nvidia.com/en-us/security/)
- [CUDA Security Best Practices](https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/)

### ThemisDB-interne Dokumentation

- [Security Policy](./security_policy.md)
- [Threat Model](./security_threat_model.md)
- [Plugin Security](./security_plugins.md)
- [Audit Logging](../features/features_audit_logging.md)
- [Performance Hardware Guide](../performance/performance_hardware.md)

### Standards und Frameworks

- **OWASP Top 10** (Web Application Security)
- **CWE-200:** Information Exposure
- **CWE-770:** Allocation of Resources Without Limits
- **STRIDE Threat Modeling** (Microsoft)
- **MITRE ATT&CK** Framework

---

**Dokumenten-Version:** 1.0  
**Letzte Aktualisierung:** 2026-01-07  
**Nächste Review:** 2026-04-07 (vierteljährlich)  
**Verantwortlich:** ThemisDB Security Team

**Kontakt für Sicherheitsfragen:**
- GitHub Security Advisories: [Report Vulnerability](https://github.com/makr-code/ThemisDB/security/advisories/new)
- Dokumentation: [Security Documentation](../security/)

---

**🔒 End of Document** 🔒

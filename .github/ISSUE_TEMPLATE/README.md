# GitHub Issues Templates für ThemisDB Kompendium

Dieses Verzeichnis enthält Issue-Templates für standardisierte Aufgaben im Kompendium-Projekt und für die LoRA-Trainings-Implementation.

## Verfügbare Templates

### 📖 Kompendium Templates

#### kapitel-verbesserung.md
**Für:** Verbesserung bestehender Kapitel  
**Verwendung:** Neuen Issue erstellen → "Kapitel-Generierung / Kapitel-Verbesserung" wählen

**Enthält:**
- ⚠️ Warnung: Kapitel existieren bereits!
- 📋 Kapitel-Details & Ziele
- 🔍 Recherche-Material
- 🎯 Arbeitsschritte (4 Phasen)
- 📝 LLM-Prompt-Vorlage
- 🔗 Links zu allen Richtlinien
- ✅ Akzeptanz-Kriterien
- 📊 Checklisten

### 🧠 LoRA Training Implementation Templates

#### 00-meta-llm-lora.md (Meta Issue - Tracking)
**Status:** In Progress  
**Description:** Master tracking issue for entire LLM/LoRA system  
**Current:** 90%+ implementation complete, documentation updates pending

#### 01-llm-infrastructure.md (Phase 1 - ✅ COMPLETE)
**Status:** Completed - Real llama.cpp integration implemented  
**Description:** llama.cpp model loading, GPU offloading, lazy loading  
**Lines:** 2,115 lines in `src/llm/llama_wrapper.cpp` and `src/llm/model_loader.cpp`

#### 02-sampling-strategies.md (Phase 1 - ✅ COMPLETE)
**Status:** Completed - All sampling strategies implemented  
**Description:** Greedy, nucleus, mirostat sampling  
**Files:** `src/llm/sampling_strategy.cpp`

#### 03-security-validation.md (Phase 1 - ⚠️ 70% COMPLETE)
**Status:** Format validation complete, crypto verification pending  
**Description:** LoRA adapter security validation  
**Remaining:** RSA-SHA256 signatures, X.509 certificates, CRL checking

#### 04-lora-training.md (Phase 1 - ✅ COMPLETE)
**Status:** Completed - Real training implemented  
**Description:** LoRA training with forward/backward/optimizer  
**Lines:** 893 lines in `src/llm/lora_framework/lora_training_service.cpp`

#### 05-lora-gpu-acceleration.md (Phase 1 - ✅ COMPLETE)
**Status:** Completed - Multi-backend GPU support  
**Description:** CUDA, HIP, Vulkan, DirectX kernels  
**Lines:** 9,978 lines across GPU framework

#### 06-lora-adam-optimizer.md (Phase 1 - ✅ COMPLETE)
**Status:** Completed - SGD/Adam/AdamW implemented  
**Description:** Advanced optimizers with LR scheduling  
**Files:** `src/llm/lora_framework/lr_scheduler.cpp`

#### 07-lora-llamacpp-integration.md (Phase 1 - ✅ COMPLETE)
**Status:** Completed - Base model adapter working  
**Description:** GGUF parsing, layer identification  
**Files:** `src/llm/lora_framework/base_model_adapter.cpp`

#### 08-lora-production-features.md (Phase 2 - ✅ COMPLETE)
**Status:** Completed - Checkpointing, metrics, monitoring  
**Description:** Production training features  

#### 09-qlora-quantized-training.md (Phase 2 - ✅ COMPLETE)
**Status:** Completed - QLoRA fully implemented  
**Description:** 4-bit/8-bit quantization, NF4, double quantization  
**Files:** `src/llm/lora_framework/quantization.cpp` (360 lines)

#### 10-themisdb-model-loading.md (Phase 2 - ✅ COMPLETE)
**Status:** Completed - Lazy loading operational  
**Description:** Model caching, LRU eviction, VRAM tracking  

### 🆕 NEW: Remaining Work Templates (2026-01-16)

#### 25-security-cryptographic-verification.md (⚠️ HIGH PRIORITY)
**Status:** NEW - Created 2026-01-16  
**Description:** Complete cryptographic verification for LoRA adapters  
**Priority:** 🔴 Critical  
**Effort:** 2-3 weeks  

**Missing Implementation:**
- RSA-SHA256 signature verification
- X.509 certificate chain validation
- CRL checking
- Chain of Responsibility pattern
- Security tests

**Why Important:** Required before production deployment

#### 26-async-model-loading.md (🟡 MEDIUM PRIORITY)
**Status:** NEW - Created 2026-01-16  
**Description:** Implement asynchronous model loading  
**Priority:** 🟡 High  
**Effort:** 1-2 weeks  
**Target:** v1.3.0

**Missing Implementation:**
- Async loading API with `std::future`
- Thread pool management
- Progress callbacks
- Concurrent request handling
- Model preloading

**Why Important:** Performance optimization, better user experience

#### 27-grammar-constrained-sampling.md (🟢 LOW PRIORITY)
**Status:** NEW - Created 2026-01-16  
**Description:** Grammar-constrained sampling for structured outputs  
**Priority:** 🟢 Low  
**Effort:** 1-2 weeks (once llama.cpp API stable)  
**Blocked By:** Waiting for stable llama.cpp API

**Missing Implementation:**
- `llama_grammar_sample()` integration
- `llama_grammar_accept()` integration
- Grammar-aware sampling strategy
- JSON schema support

**Why Important:** Advanced feature for structured output generation

#### 28-documentation-status-update.md (🔴 HIGH PRIORITY)
**Status:** NEW - Created 2026-01-16  
**Description:** Update documentation to reflect 90%+ actual implementation  
**Priority:** 🔴 Critical  
**Effort:** 2-3 days  

**Required Updates:**
- Change "20-40% complete" to "90%+ complete"
- Remove "stub implementation" claims
- Update phase completion percentages
- Create verification guide
- Update README.md status

**Why Important:** Accurate documentation prevents confusion about readiness
- AdamW variant with decoupled weight decay
- Learning rate scheduling
- Faster convergence (target: 3-5x vs SGD)

#### 07-lora-llamacpp-integration.md (Phase 2 - Future)
**For:** Integration with llama.cpp base models  
**Priority:** P1 (High)  
**Effort:** 3-4 weeks  
**Labels:** `priority:P1`, `type:feature`, `area:llm`, `effort:large`, `phase:2`

**Key Tasks:**
- Load GGUF models via llama.cpp
- Inject LoRA adapters into attention/MLP layers
- Forward: base + LoRA, Backward: LoRA only
- Text data processing and tokenization
- Train on real datasets (Alpaca, ShareGPT)

#### 08-lora-production-features.md (Phase 3 - Future)
**For:** Production-ready training features  
**Priority:** P2 (Medium)  
**Effort:** 4-6 weeks  
**Labels:** `priority:P2`, `type:feature`, `area:llm`, `effort:large`, `phase:3`

**Key Tasks:**
- Mixed precision (FP16/BF16) - 2x speedup
- Gradient accumulation - large batch sizes
- Gradient clipping - training stability
- Checkpointing - crash recovery
- Distributed training (multi-GPU) - linear scaling
- Monitoring and logging

#### 09-qlora-quantized-training.md (Phase 2 - Future)
**For:** QLoRA (Quantized LoRA) implementation  
**Priority:** P1 (High)  
**Effort:** 4-6 weeks  
**Labels:** `priority:P1`, `type:feature`, `area:llm`, `area:performance`, `effort:large`, `phase:2`

**Key Tasks:**
- 4-bit NF4 quantization for base models
- 8-bit INT8 quantization
- Double quantization for constants
- Paged optimizers (CPU ↔ GPU)
- Memory reduction: 60-70% vs full LoRA
- Train Llama-65B on consumer GPUs

**Benefits:**
- Memory: ~5-6 GB for Llama-7B (vs ~14 GB)
- Enables: Llama-30B on 24GB, Llama-65B on 40GB
- Accuracy: Within 1-2% of full precision

---

### 🚨 Production-Readiness Issue Templates (NEW - January 2026)

#### 10-themisdb-model-loading.md (P0 - CRITICAL)
**For:** Model Loading aus ThemisDB Blob Store  
**Status:** 🔴 **PRODUKTIONSBLOCKER**  
**Priority:** P0 (Critical)  
**Effort:** 2-3 weeks  
**Labels:** `priority:P0`, `type:feature`, `area:llm`, `area:storage`, `effort:large`, `phase:production`

**Key Tasks:**
- Implement `loadModelFromThemisDB()` (currently returns false)
- Blob Store integration für GGUF models
- Streaming für large models (>5GB)
- Encryption/Decryption support
- Model metadata management
- Production key providers (NOT MockKeyProvider)

**Impact:** 
- ❌ **BLOCKS:** Core "native LLM integration" feature
- ❌ **BLOCKS:** Model serving from database
- ❌ **BLOCKS:** Multi-tenant model sharing

**Related Analysis:** `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md` §2.1, §4

---

#### 11-lora-storage-backend.md (P0 - CRITICAL)
**For:** Complete LoRa Storage Backend für ThemisDB und S3  
**Status:** 🔴 **PRODUKTIONSBLOCKER**  
**Priority:** P0 (Critical)  
**Effort:** 2-3 weeks  
**Labels:** `priority:P0`, `type:feature`, `area:llm`, `area:storage`, `effort:large`, `phase:production`

**Key Tasks:**
- Implement ThemisDB backend (save/load/delete/metadata) - 4x TODO
- Implement S3 backend
- Fix blob deletion (currently only deletes metadata)
- Replace MockKeyProvider with production keys
- Blob reference management
- Encryption integration

**Impact:**
- ❌ **BLOCKS:** Persistent LoRa adapter storage
- ❌ **BLOCKS:** Adapter lifecycle management
- ⚠️ **RISK:** Data loss (only filesystem works)

**Related Analysis:** `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md` §1.1.2, §4

---

#### 12-lora-training-control.md (P0 - CRITICAL)
**For:** Training Stop Logic und Production Features  
**Status:** 🔴 **PRODUKTIONSBLOCKER**  
**Priority:** P0 (Critical)  
**Effort:** 1 week  
**Labels:** `priority:P0`, `type:feature`, `area:llm`, `effort:medium`, `phase:production`

**Key Tasks:**
- Implement `stopTraining()` (currently non-functional)
- Checkpoint save/load functionality
- Resume training from checkpoint
- Graceful shutdown on SIGTERM/SIGINT
- Resource cleanup (prevent leaks)

**Impact:**
- ❌ **BLOCKS:** Graceful training control
- ⚠️ **RISK:** Resource leaks, no crash recovery
- ⚠️ **RISK:** Cannot stop long-running training jobs

**Related Analysis:** `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md` §1.1.1

---

#### 13-remove-simulation-code.md (P1 - HIGH)
**For:** Entfernung aller Simulation-Codes  
**Status:** ⚠️ **Performance-Problem**  
**Priority:** P1 (High)  
**Effort:** 2 weeks  
**Labels:** `priority:P1`, `type:refactoring`, `area:llm`, `effort:medium`, `phase:production`

**Key Tasks:**
- Remove 9 sleep() calls (artificial latency)
- Replace 12 stub implementations
- Replace dummy embeddings with real implementations
- Implement 30+ production validator tests (currently all TODO)
- Fix performance metrics to reflect reality

**Impact:**
- ⚠️ **ISSUE:** Performance metrics not accurate
- ⚠️ **ISSUE:** Cannot validate production readiness
- ⚠️ **ISSUE:** Artificial latency distorts benchmarks

**Related Analysis:** `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md` §2.3, §5

---

### 📊 Production-Readiness Status

| Template | Status | Priority | Blocks Production? |
|----------|--------|----------|-------------------|
| 10-themisdb-model-loading | 🔴 Critical Gap | P0 | ✅ YES - Core feature |
| 11-lora-storage-backend | 🔴 Critical Gap | P0 | ✅ YES - Data loss risk |
| 12-lora-training-control | 🔴 Critical Gap | P0 | ✅ YES - Resource leaks |
| 13-remove-simulation-code | ⚠️ Performance Issue | P1 | ⚠️ Metrics unreliable |

**Overall Production Readiness:** **47%** ❌ (See `EXECUTIVE_SUMMARY_GAPS_ANALYSIS.md`)

**Estimated Time to Production:** **6-8 weeks** with focused P0/P1 work

## Verwendungsbeispiele

### Beispiel 1: Kapitel 6 verbessern
```
Title: "Kapitel-Verbesserung: chapter_06_graph.md - Graph-Datenmodell"
Template: kapitel-verbesserung.md

Ziele:
- Wissenschaftlichere Sprache
- RocksDB Speicher-Details integrieren
- Code-Beispiele: 2 → 5
- Performance-Benchmarks vs. Neo4j
```

### Beispiel 2: Kapitel 15 mit neuen Quellen
```
Title: "Kapitel-Verbesserung: chapter_15_analytics.md - Analytics"
Template: kapitel-verbesserung.md

Ziele:
- Boost C++ Analytics-Bibliotheken integrieren
- Benchmark-Daten hinzufügen
- Design-Standards beachten
```

## Wichtige Richtlinien

Vor der Verwendung dieser Templates **UNBEDINGT** lesen:

1. **[KAPITEL_MINDSET.md](../KAPITEL_MINDSET.md)** ⭐
   - Mentalität: Kapitel verbessern, nicht neu schreiben
   - 47 Kapitel existieren bereits
   - <1% neue Kapitel brauchen

2. **[CHAPTER_GENERATION_GUIDE.md](../CHAPTER_GENERATION_GUIDE.md)**
   - Vollständiger Guide mit Prompt-Template
   - Best Practices & Qualitätskriterien

3. **[SOURCES_INVENTORY.md](../SOURCES_INVENTORY.md)**
   - Alle 92 verfügbaren Quellen
   - Externe Libraries, Richtlinien, Doku

## Workflow

```
1. Issue erstellen
   → "Kapitel-Verbesserung / Kapitel-Generierung" Template
   
2. Details ausfüllen
   → Kapitel-Nummer, Ziele, Recherche
   
3. LLM-Prompt erstellen
   → Bestehendes Kapitel + Anforderungen
   
4. Kapitel verbessern
   → Quellen integrieren, Code-Beispiele, Performance
   
5. Validierung
   → Code testen, Links überprüfen, Design beachten
   
6. Pull Request
   → chapter_XX.md aktualisieren (nicht erstellen)
```

## Design- & Richtlinien-Dokumente

Diese sind in den Issue-Templates referenziert:

- **IMPLEMENTATION_COMPLETE.md** - Layout-Standards
- **THEMISDB_CUSTOM_THEME.md** - Design-Richtlinien
- **STRATEGY_WITH_EXAMPLES.md** - Struktur-Vorbilder
- **styles_modern_book.scss** - CSS-Design-Philosophie

## Häufige Fragen

**Q: Darf ich ein neues Kapitel erstellen?**  
A: Nur wenn das Thema in KEINEM der 47 bestehenden Kapitel behandelt wird. Mit Team abstimmen!

**Q: Soll ich chapter_06_graph_v2.md erstellen?**  
A: NEIN! Immer chapter_06_graph.md ÜBERSCHREIBEN, nicht duplizieren.

**Q: Welche LLM verwenden?**  
A: Claude 3.5 Sonnet (empfohlen) oder GPT-4o - Siehe CHAPTER_GENERATION_GUIDE.md

**Q: Wie lange dauert ein Kapitel?**  
A: ~5-8 Stunden (Recherche, Prompt, Generierung, Validierung, Testing)

---

**Erstellt:** 13. Januar 2026  
**Status:** Aktiv & verfügbar  
**Letzte Aktualisierung:** 13. Januar 2026

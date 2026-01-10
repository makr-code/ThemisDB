# llama.cpp API Feature-Analyse für ThemisDB

**Datum:** 5. Januar 2026  
**Status:** Research & Analysis  
**Zweck:** Identifizierung zusätzlicher llama.cpp Features für ThemisDB

---

## 🎯 Ziel

Diese Analyse untersucht die llama.cpp API systematisch, um Features zu identifizieren, die ThemisDB noch nicht nutzt, aber von Nutzen sein könnten.

---

## 📊 Übersicht: Aktuelle ThemisDB Integration

### Bereits genutzte llama.cpp Features

ThemisDB v1.3.0 verwendet aktuell folgende llama.cpp Features:

#### 1. **Basis-Inferenz**
- ✅ `llama_backend_init()` / `llama_backend_free()` - Backend-Initialisierung
- ✅ `llama_load_model_from_file()` - GGUF-Model Loading
- ✅ `llama_new_context_with_model()` - Context-Erstellung
- ✅ `llama_tokenize()` - Text → Token Konvertierung
- ✅ `llama_eval()` / `llama_decode()` - Inferenz-Ausführung
- ✅ `llama_sample_token()` - Token-Sampling
- ✅ `llama_token_to_str()` - Token → Text Konvertierung

#### 2. **Model Management**
- ✅ `llama_model_params` - Model-Konfiguration (n_gpu_layers, use_mmap)
- ✅ `llama_context_params` - Context-Konfiguration (n_ctx, n_batch)
- ✅ Lazy Model Loading (ThemisDB-eigene Implementierung)

#### 3. **LoRA Support**
- ✅ `llama_lora_adapter_load()` - LoRA-Adapter laden
- ✅ `llama_lora_adapter_set()` - LoRA auf Context anwenden
- ✅ `llama_lora_adapter_remove()` - LoRA entfernen
- ✅ Multi-LoRA Management (ThemisDB-eigene Implementierung)

#### 4. **GPU-Beschleunigung**
- ✅ CUDA Support (NVIDIA)
- ✅ Metal Support (Apple Silicon)
- ✅ Vulkan Support (Cross-Platform)

---

## 🔍 Nicht genutzte llama.cpp Features (Potenzial für ThemisDB)

### Kategorie A: Hochprioritäre Features

#### 1. **Speculative Decoding (Draft Model)**
**Was ist das?**
- Verwendet ein kleineres "Draft-Model" zur Vorschlag-Generierung
- Großes Model validiert nur die Vorschläge (schneller)
- 2-3x Speedup bei gleicher Qualität

**llama.cpp API:**
```cpp
// Draft model für Speculative Decoding
llama_model* draft_model = llama_load_model_from_file(
    "/models/llama-160m-draft.gguf",
    draft_params
);

// Speculative decoding aktivieren
llama_sampling_params sampling;
sampling.n_draft = 8;  // 8 Draft-Tokens pro Schritt
```

**Nutzen für ThemisDB:**
- 🚀 **Performance**: 2-3x schnellere Inferenz ohne Qualitätsverlust
- 💰 **Effizienz**: Weniger GPU-Rechenzeit pro Response
- 🎯 **Use Case**: Ideal für interaktive Chat-Anwendungen

**Implementierungs-Aufwand:** Mittel (1-2 Wochen)

---

#### 2. **KV-Cache Reuse (Prefix Caching)**
**Was ist das?**
- Wiederverwendung von KV-Cache für identische Prompt-Präfixe
- System-Prompts müssen nicht neu berechnet werden
- Reduziert First-Token-Latency dramatisch

**llama.cpp API:**
```cpp
// KV-Cache speichern
std::vector<uint8_t> cache_state;
size_t cache_size = llama_state_get_size(ctx);
cache_state.resize(cache_size);
llama_state_get_data(ctx, cache_state.data());

// KV-Cache wiederherstellen
llama_state_set_data(ctx, cache_state.data());
```

**Nutzen für ThemisDB:**
- ⚡ **Latenz**: 10-20x schnellerer Start für wiederkehrende Prompts
- 💾 **Speicher**: Bereits in ThemisDB-Design vorgesehen (`llm_prefix_cache.h`)
- 🎯 **Use Case**: RAG mit gleichbleibenden System-Instructions

**Implementierungs-Aufwand:** Niedrig (3-5 Tage) - Skeleton bereits vorhanden

---

#### 3. **Continuous Batching**
**Was ist das?**
- Mehrere Inferenz-Requests gleichzeitig verarbeiten
- Dynamisches Hinzufügen/Entfernen von Sequences im Batch
- Ähnlich wie vLLM Continuous Batching

**llama.cpp API:**
```cpp
// Batch-Struktur
llama_batch batch = llama_batch_init(512, 0, 1);

// Mehrere Sequences hinzufügen
for (int i = 0; i < num_sequences; ++i) {
    llama_batch_add(batch, tokens[i], pos[i], {seq_ids[i]}, false);
}

// Batch ausführen
llama_decode(ctx, batch);
```

**Nutzen für ThemisDB:**
- 📈 **Throughput**: 5-10x höherer Durchsatz bei gleichem VRAM
- 🌐 **Skalierung**: Bessere Multi-User-Performance
- 🎯 **Use Case**: Multi-Tenant-Szenarien, API-Server

**Implementierungs-Aufwand:** Mittel-Hoch (2-3 Wochen)

---

#### 4. **Flash Attention / PagedAttention**
**Was ist das?**
- Optimierte Attention-Berechnung (weniger Memory, schneller)
- Paged KV-Cache für effizientere Speichernutzung
- Standard in modernen LLM-Serving-Systemen

**llama.cpp API:**
```cpp
// Flash Attention aktivieren (automatisch, wenn verfügbar)
llama_model_params params = llama_model_default_params();
params.use_flash_attn = true;  // Seit llama.cpp b2000+

// PagedAttention via n_ctx_per_seq
llama_context_params ctx_params = llama_context_default_params();
ctx_params.n_ctx = 32768;
ctx_params.n_ctx_per_seq = 4096;  // Pro Sequence nur 4K aktiv
```

**Nutzen für ThemisDB:**
- 💾 **Speicher**: 30-50% weniger KV-Cache Memory
- ⚡ **Performance**: 15-25% schnellere Attention
- 🎯 **Use Case**: Lange Kontexte (32K+ Tokens)

**Implementierungs-Aufwand:** Niedrig (schon in llama.cpp integriert)

---

#### 5. **Embeddings Extraction**
**Was ist das?**
- Direkte Extraktion von Token/Sequence-Embeddings
- Nutzung des LLM als Embedding-Model
- Alternative zu separaten Embedding-Modellen

**llama.cpp API:**
```cpp
// Embeddings extrahieren
llama_context_params ctx_params = llama_context_default_params();
ctx_params.embeddings = true;  // Embedding-Modus aktivieren

// Nach eval: Embeddings abrufen
float* embeddings = llama_get_embeddings(ctx);
int n_embd = llama_n_embd(model);

// Vektor kopieren
std::vector<float> embedding_vec(embeddings, embeddings + n_embd);
```

**Nutzen für ThemisDB:**
- 🔗 **Integration**: LLM + Embedding in einem Model
- 💾 **Effizienz**: Kein separates Embedding-Model nötig
- 🎯 **Use Case**: RAG Embedding-Generierung, Semantic Search

**Implementierungs-Aufwand:** Niedrig (2-3 Tage)

---

#### 6. **Grammar-Constrained Generation**
**Was ist das?**
- Erzwingt strukturierte Ausgabe (JSON, XML, etc.)
- Garantiert syntaktisch valide Responses
- Basiert auf GBNF (GGML BNF) Grammar-Definition

**llama.cpp API:**
```cpp
// Grammar definieren (JSON-Schema)
const char* json_grammar = R"(
root ::= object
object ::= "{" pair ("," pair)* "}"
pair ::= string ":" value
value ::= string | number | object | array
string ::= "\"" [^"]* "\""
number ::= [0-9]+
)";

// Grammar-Parser erstellen
llama_grammar* grammar = llama_grammar_init(
    llama_grammar_parse(json_grammar)
);

// Sampling mit Grammar
llama_sample_grammar(ctx, &candidates, grammar);
```

**Nutzen für ThemisDB:**
- ✅ **Zuverlässigkeit**: 100% valide JSON/XML-Responses
- 🤖 **Automation**: Keine Post-Processing-Fehler mehr
- 🎯 **Use Case**: API-Integration, Structured Extraction

**Implementierungs-Aufwand:** Mittel (1-2 Wochen)

---

#### 7. **Multi-Modal Support (Vision)**
**Was ist das?**
- Verarbeitung von Bildern + Text (LLaVA, LLaMA-3.2-Vision)
- Image-zu-Text-Embeddings im LLM-Context
- Native Vision-Model-Unterstützung

**llama.cpp API:**
```cpp
// Vision-Model laden (z.B. LLaVA)
llama_model* vision_model = llama_load_model_from_file(
    "/models/llava-v1.6-34b-q4.gguf",
    params
);

// Bild einbetten
clip_image_u8 image = clip_image_load("/path/to/image.jpg");
clip_image_f32 preprocessed = clip_image_preprocess(clip_ctx, image);

// In Context einbetten
llama_eval_image(ctx, preprocessed.data, n_image_tokens, 0);

// Jetzt Text-Prompt ausführen
llama_eval(ctx, text_tokens, n_text_tokens, n_image_tokens);
```

**Nutzen für ThemisDB:**
- 🖼️ **Multi-Modal**: Bild + Text in einem Workflow
- 🔗 **Integration**: ThemisDB hat bereits Image Analysis Plugin
- 🎯 **Use Case**: Document Processing, Visual Q&A

**Implementierungs-Aufwand:** Hoch (3-4 Wochen) - Separate CLIP-Integration nötig

---

### Kategorie B: Mittlere Priorität

#### 8. **Server Mode (HTTP API)**
**Was ist das?**
- llama.cpp hat eingebauten HTTP-Server
- OpenAI-kompatible API
- SSE-Streaming Support

**llama.cpp API:**
```bash
# Server starten
./llama-server -m model.gguf --port 8080 --host 0.0.0.0

# OpenAI-kompatible API
curl http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "gpt-3.5-turbo",
    "messages": [{"role": "user", "content": "Hello"}]
  }'
```

**Nutzen für ThemisDB:**
- 🌐 **Standards**: OpenAI-kompatible API
- 🔌 **Integration**: Einfachere Client-Integration
- 🎯 **Use Case**: Alternative zu direkter Library-Integration

**Implementierungs-Aufwand:** Niedrig (als separater Prozess) / Hoch (embedded)

---

#### 9. **Quantization (On-the-fly)**
**Was ist das?**
- Konvertierung von FP16/FP32 zu Q4/Q8 während des Ladens
- Dynamische Quantisierung basierend auf VRAM
- Keine vorquantisierten Models nötig

**llama.cpp API:**
```cpp
// Quantisierung während Model-Load
llama_model_params params = llama_model_default_params();
params.quantize_output_tensor = true;
params.quantize_mode = LLAMA_QUANT_Q4_K_M;

llama_model* model = llama_load_model_from_file(
    "/models/llama-3-8b-fp16.gguf",  // FP16 Input
    params  // → Q4_K_M Output
);
```

**Nutzen für ThemisDB:**
- 💾 **Speicher**: Flexibles VRAM-Management
- 🚀 **Geschwindigkeit**: Weniger Disk-IO
- 🎯 **Use Case**: Dynamic Model Loading basierend auf verfügbarem VRAM

**Implementierungs-Aufwand:** Niedrig-Mittel (API schon vorhanden)

---

#### 10. **RoPE Scaling (Extended Context)**
**Was ist das?**
- Erweitert Context-Length über Training-Length hinaus
- RoPE (Rotary Position Embedding) Scaling
- 4K Model → 32K Context via Scaling

**llama.cpp API:**
```cpp
// RoPE-Scaling aktivieren
llama_context_params ctx_params = llama_context_default_params();
ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_LINEAR;
ctx_params.rope_freq_base = 10000.0f;
ctx_params.rope_freq_scale = 1.0f;

// Beispiel: 4K Model auf 32K skalieren
ctx_params.n_ctx = 32768;
ctx_params.rope_freq_scale = 0.125f;  // 1/8 für 8x Scaling
```

**Nutzen für ThemisDB:**
- 📚 **Längere Contexts**: 4x-8x längere Prompts möglich
- 🎯 **Use Case**: Lange Dokumente, Code-Analyse
- ⚠️ **Trade-off**: Leichter Qualitätsverlust bei sehr langen Contexts

**Implementierungs-Aufwand:** Niedrig (nur Parameter-Tuning)

---

#### 11. **Custom Sampling Strategies**
**Was ist das?**
- Erweiterte Sampling-Parameter (Mirostat, Locally Typical, etc.)
- Custom Token-Penalties
- Repetition/Frequency/Presence Penalties

**llama.cpp API:**
```cpp
// Erweiterte Sampling-Parameter
llama_sampling_params sampling;
sampling.temp = 0.8f;
sampling.top_k = 40;
sampling.top_p = 0.95f;
sampling.min_p = 0.05f;
sampling.tfs_z = 1.0f;
sampling.typical_p = 1.0f;
sampling.mirostat = 2;  // Mirostat v2
sampling.mirostat_tau = 5.0f;
sampling.mirostat_eta = 0.1f;

// Penalties
sampling.repeat_penalty = 1.1f;
sampling.frequency_penalty = 0.0f;
sampling.presence_penalty = 0.0f;
sampling.penalty_repeat_last_n = 64;
```

**Nutzen für ThemisDB:**
- 🎯 **Qualität**: Bessere Output-Kontrolle
- 🤖 **Vielfalt**: Verschiedene Generierungs-Modi
- 🎯 **Use Case**: Fine-Tuning von Response-Charakteristik

**Implementierungs-Aufwand:** Niedrig (Parameter-Durchreichung)

---

### Kategorie C: Niedrige Priorität (Nische)

#### 12. **GGUF Metadata Inspection**
**Was ist das?**
- Auslesen von Model-Metadaten ohne vollständiges Laden
- Schnelle Model-Info-Abfrage
- Useful für Model-Registry/Discovery

**llama.cpp API:**
```cpp
// Metadaten auslesen
llama_model_params params = llama_model_default_params();
params.vocab_only = true;  // Nur Metadaten/Vocab laden

llama_model* model = llama_load_model_from_file(path, params);

// Metadaten abrufen
int n_vocab = llama_n_vocab(model);
int n_ctx_train = llama_n_ctx_train(model);
int n_embd = llama_n_embd(model);
int n_layer = llama_n_layer(model);

llama_free_model(model);  // Schnell, da nur Metadaten geladen
```

**Nutzen für ThemisDB:**
- 📊 **Discovery**: Model-Katalog ohne Full-Load
- ⚡ **Performance**: Schnelle Model-Info-Abfragen
- 🎯 **Use Case**: Model-Registry, Admin-UI

**Implementierungs-Aufwand:** Sehr niedrig (1-2 Tage)

---

#### 13. **Split Models (Tensor Parallelism)**
**Was ist das?**
- Model auf mehrere GPUs aufteilen
- Tensor Parallelism für große Models
- Alternative zu Pipeline Parallelism

**llama.cpp API:**
```cpp
// Split auf mehrere GPUs
llama_model_params params = llama_model_default_params();
params.tensor_split = {0.6f, 0.4f};  // 60% GPU0, 40% GPU1
params.n_gpu_layers = 40;

llama_model* model = llama_load_model_from_file(path, params);
```

**Nutzen für ThemisDB:**
- 🖥️ **Skalierung**: Große Models auf Multi-GPU
- 🎯 **Use Case**: 70B+ Models, Enterprise-Setups
- ⚠️ **Komplex**: Benötigt spezielle Hardware

**Implementierungs-Aufwand:** Mittel (Hardware-abhängig)

---

#### 14. **NUMA Support**
**Was ist das?**
- Non-Uniform Memory Access Optimierung
- CPU-Performance auf Multi-Socket-Systemen
- Wichtig für große CPU-Only Deployments

**llama.cpp API:**
```cpp
// NUMA-Optimierung
llama_model_params params = llama_model_default_params();
params.numa = LLAMA_NUMA_STRATEGY_DISTRIBUTE;

// Threads auf NUMA-Nodes verteilen
llama_context_params ctx_params = llama_context_default_params();
ctx_params.n_threads = 64;
ctx_params.n_threads_batch = 64;
```

**Nutzen für ThemisDB:**
- 🖥️ **CPU-Performance**: Bessere CPU-Skalierung
- 🎯 **Use Case**: Große Server ohne GPU
- 🔧 **Nische**: Nur relevant für spezielle Hardware

**Implementierungs-Aufwand:** Niedrig (nur Flags setzen)

---

## 📋 Prioritäts-Matrix

| Feature | Priorität | Aufwand | Impact | Empfehlung |
|---------|-----------|---------|--------|------------|
| **Speculative Decoding** | 🔴 Hoch | Mittel | Sehr Hoch | ✅ **Implementieren** (v1.4) |
| **KV-Cache Reuse** | 🔴 Hoch | Niedrig | Sehr Hoch | ✅ **Implementieren** (v1.3.1) |
| **Continuous Batching** | 🔴 Hoch | Hoch | Sehr Hoch | ✅ **Planen** (v1.4) |
| **Flash Attention** | 🔴 Hoch | Sehr Niedrig | Hoch | ✅ **Aktivieren** (sofort) |
| **Embeddings** | 🔴 Hoch | Niedrig | Hoch | ✅ **Implementieren** (v1.3.1) |
| **Grammar Generation** | 🟡 Mittel | Mittel | Mittel | 📅 Planen (v1.4) |
| **Vision Support** | 🟡 Mittel | Hoch | Hoch | 📅 Planen (v1.5+) |
| **Server Mode** | 🟡 Mittel | Niedrig/Hoch | Mittel | 📋 Optional |
| **On-the-fly Quant** | 🟡 Mittel | Mittel | Mittel | 📅 Planen (v1.4) |
| **RoPE Scaling** | 🟡 Mittel | Sehr Niedrig | Mittel | 📅 Testen (v1.4) |
| **Custom Sampling** | 🟢 Niedrig | Sehr Niedrig | Niedrig | 📋 Optional |
| **Metadata Inspection** | 🟢 Niedrig | Sehr Niedrig | Niedrig | 📋 Optional |
| **Tensor Parallelism** | 🟢 Niedrig | Mittel | Mittel | 🔧 Niche |
| **NUMA Support** | 🟢 Niedrig | Sehr Niedrig | Niedrig | 🔧 Niche |

---

## 🎯 Empfohlene Implementierungs-Roadmap

### Phase 1: Quick Wins (v1.3.1 - 1 Woche)
1. ✅ **Flash Attention aktivieren** (sofort, nur Config)
2. ✅ **KV-Cache Reuse implementieren** (Skeleton vorhanden)
3. ✅ **Embeddings-Extraktion** (einfache API-Erweiterung)
4. 📊 **Metadata Inspection** (Model Registry Basis)

**Aufwand:** ~5 Arbeitstage  
**Impact:** Sofortige Performance-Verbesserung

---

### Phase 2: Major Features (v1.4 - 6-8 Wochen)
1. 🚀 **Speculative Decoding** (Draft-Model-Support)
2. 📈 **Continuous Batching** (Multi-Request-Handling)
3. 🎯 **Grammar-Constrained Generation** (Structured Output)
4. 🔧 **On-the-fly Quantization** (Dynamic VRAM)

**Aufwand:** ~6-8 Wochen (gestaffelt)  
**Impact:** Massiver Performance-/Qualitäts-Boost

---

### Phase 3: Advanced Features (v1.5+ - 3-6 Monate)
1. 🖼️ **Vision Support** (Multi-Modal)
2. 🔧 **RoPE Scaling** (Extended Context)
3. 🌐 **Server Mode** (Optional als Addon)
4. 🎛️ **Custom Sampling Strategies** (Advanced Tuning)

**Aufwand:** ~3-6 Monate (parallel)  
**Impact:** Feature-Parity mit Top-Tier LLM-Servern

---

## 📊 Vergleich mit Wettbewerbern

| Feature | ThemisDB (aktuell) | vLLM | Ollama | llama.cpp Server |
|---------|-------------------|------|--------|------------------|
| Basic Inference | ✅ | ✅ | ✅ | ✅ |
| Multi-LoRA | ✅ | ✅ | ❌ | ⚠️ Limited |
| Lazy Loading | ✅ | ❌ | ✅ | ❌ |
| Continuous Batching | ❌ | ✅ | ❌ | ⚠️ Experimental |
| Speculative Decoding | ❌ | ✅ | ❌ | ✅ |
| KV-Cache Reuse | ⚠️ Skeleton | ✅ | ✅ | ✅ |
| Flash Attention | ❌ | ✅ | ✅ (auto) | ✅ (auto) |
| Vision Support | ⚠️ Separate Plugin | ✅ | ✅ | ✅ |
| Grammar Generation | ❌ | ✅ | ❌ | ✅ |
| Embeddings | ❌ | ✅ | ✅ | ✅ |

**Fazit:** ThemisDB ist in Lazy Loading + Multi-LoRA führend, aber hinkt bei Performance-Features (Batching, Spec Decoding) hinterher.

---

## 🔧 Technische Implementierungs-Notizen

### 1. Flash Attention aktivieren (Sofort)

```cpp
// In LlamaWrapper::Config
struct Config {
    bool use_flash_attn = true;  // NEU: Flash Attention aktivieren
    // ...
};

// In LlamaWrapper::loadModel()
llama_model_params model_params = llama_model_default_params();
model_params.use_flash_attn = config_.use_flash_attn;
```

**Commit-Aufwand:** 10 Minuten

---

### 2. KV-Cache Reuse (Phase 1)

```cpp
// Neue Klasse: KVCacheManager
class KVCacheManager {
public:
    // Cache speichern
    std::string saveCache(llama_context* ctx, const std::string& prefix_hash);
    
    // Cache laden
    bool loadCache(llama_context* ctx, const std::string& prefix_hash);
    
private:
    std::unordered_map<std::string, std::vector<uint8_t>> cache_store_;
    size_t max_cache_size_mb_;
};

// Integration in LlamaWrapper
class LlamaWrapper {
private:
    std::unique_ptr<KVCacheManager> kv_cache_manager_;
public:
    InferenceResponse generate(const InferenceRequest& request) override {
        // Prefix-Hash berechnen
        std::string prefix_hash = computeHash(request.system_prompt);
        
        // Cache versuchen zu laden
        if (kv_cache_manager_->loadCache(context_, prefix_hash)) {
            // Cache-Hit: Nur User-Prompt evaluieren
            // ...
        } else {
            // Cache-Miss: Vollständige Evaluation
            // ...
            kv_cache_manager_->saveCache(context_, prefix_hash);
        }
    }
};
```

**Commit-Aufwand:** 2-3 Tage

---

### 3. Speculative Decoding (Phase 2)

```cpp
// Neue Klasse: SpeculativeDecoder
class SpeculativeDecoder {
public:
    struct Config {
        std::string draft_model_path;
        int n_draft = 8;  // Draft-Tokens pro Schritt
    };
    
    SpeculativeDecoder(const Config& config);
    
    // Speculative Decoding durchführen
    std::vector<llama_token> decode(
        llama_context* target_ctx,
        llama_context* draft_ctx,
        const std::vector<llama_token>& prompt,
        int max_tokens
    );
};

// Integration in LlamaWrapper
class LlamaWrapper {
private:
    std::unique_ptr<SpeculativeDecoder> spec_decoder_;
public:
    InferenceResponse generate(const InferenceRequest& request) override {
        if (request.use_speculative_decoding && spec_decoder_) {
            // Speculative Decoding verwenden
            auto tokens = spec_decoder_->decode(
                context_, draft_context_, prompt_tokens, request.max_tokens
            );
            // ...
        } else {
            // Standard-Decoding
            // ...
        }
    }
};
```

**Commit-Aufwand:** 1-2 Wochen

---

## 📚 Referenzen

### llama.cpp Dokumentation
- **GitHub:** https://github.com/ggerganov/llama.cpp
- **Examples:** https://github.com/ggerganov/llama.cpp/tree/master/examples
- **Server:** https://github.com/ggerganov/llama.cpp/tree/master/examples/server

### Research Papers
- **Speculative Decoding:** Chen et al., 2023 - "Accelerating LLM Inference with Speculative Sampling"
- **Flash Attention:** Dao et al., 2022 - "FlashAttention: Fast and Memory-Efficient Exact Attention"
- **PagedAttention:** Kwon et al., 2023 - "Efficient Memory Management for LLM Serving with PagedAttention"

### Wettbewerber-Analysen
- **vLLM:** https://docs.vllm.ai/
- **Ollama:** https://github.com/ollama/ollama
- **TensorRT-LLM:** https://github.com/NVIDIA/TensorRT-LLM

---

## ✅ Nächste Schritte

1. ✅ **Dokumentation finalisieren** (dieses Dokument)
2. 📋 **Phase 1 Features priorisieren** (KV-Cache, Flash Attention)
3. 🛠️ **POC implementieren** (Flash Attention aktivieren)
4. 📊 **Benchmarks durchführen** (Vorher/Nachher-Vergleich)
5. 📝 **GitHub Issues erstellen** für Phase 2/3 Features

---

**Status:** ✅ Research Complete  
**Nächster Review:** Januar 2026  
**Maintainer:** ThemisDB Core Team

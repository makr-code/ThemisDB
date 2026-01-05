# llama.cpp Feature Research - Quick Reference

**Datum:** 5. Januar 2026  
**Status:** Completed  
**Ziel:** Schnellübersicht über nutzbare llama.cpp Features für ThemisDB

---

## 🎯 Top 5 Prioritäten (Empfohlen für v1.3.1 - v1.4)

### 1. ⚡ Flash Attention (Sofort aktivierbar)
- **Aufwand:** 10 Minuten (nur Config)
- **Impact:** 15-25% schneller, 30% weniger Memory
- **Code:** Nur `params.use_flash_attn = true;` setzen

### 2. 💾 KV-Cache Reuse (Phase 1)
- **Aufwand:** 3-5 Tage
- **Impact:** 10-20x schnellerer First-Token bei wiederholten Prompts
- **Status:** Skeleton bereits vorhanden (`llm_prefix_cache.h`)

### 3. 🔗 Embeddings Extraction (Phase 1)
- **Aufwand:** 2-3 Tage
- **Impact:** LLM als Embedding-Model nutzen
- **Use Case:** RAG ohne separates Embedding-Model

### 4. 🚀 Speculative Decoding (Phase 2)
- **Aufwand:** 1-2 Wochen
- **Impact:** 2-3x schnellere Inferenz
- **Technik:** Draft-Model generiert, Target-Model validiert

### 5. 📈 Continuous Batching (Phase 2)
- **Aufwand:** 2-3 Wochen
- **Impact:** 5-10x höherer Throughput
- **Use Case:** Multi-User-Szenarien

---

## 📊 Feature-Matrix

| Feature | Priorität | Aufwand | ThemisDB Status |
|---------|-----------|---------|-----------------|
| **Flash Attention** | 🔴 | Sehr Niedrig | ❌ Nicht aktiv |
| **KV-Cache Reuse** | 🔴 | Niedrig | ⚠️ Skeleton vorhanden |
| **Embeddings** | 🔴 | Niedrig | ❌ Nicht implementiert |
| **Speculative Decoding** | 🔴 | Mittel | ❌ Nicht implementiert |
| **Continuous Batching** | 🔴 | Hoch | ❌ Nicht implementiert |
| **Grammar Generation** | 🟡 | Mittel | ❌ Nicht implementiert |
| **Vision Support** | 🟡 | Hoch | ⚠️ Separates Plugin |
| **On-the-fly Quant** | 🟡 | Mittel | ❌ Nicht implementiert |
| **RoPE Scaling** | 🟡 | Sehr Niedrig | ❌ Nicht implementiert |
| **Server Mode** | 🟡 | Niedrig/Hoch | ❌ Nicht implementiert |

---

## 🚀 Schnellstart: Flash Attention aktivieren

**1. Config erweitern** (`include/llm/llama_wrapper.h`):
```cpp
struct Config {
    bool use_flash_attn = true;  // NEU
    // ... existing fields ...
};
```

**2. Params setzen** (`src/llm/llama_wrapper.cpp`):
```cpp
llama_model_params model_params = llama_model_default_params();
#ifdef LLAMA_FLASH_ATTN
model_params.flash_attn = config_.use_flash_attn;
#endif
```

**3. Testen:**
```bash
cmake -B build -S . -DTHEMIS_ENABLE_LLM=ON
cmake --build build
./build/test_embedded_llm
```

---

## 📚 Vollständige Dokumentation

- 🇩🇪 **Deutsch (Detailliert):** [LLAMA_CPP_API_FEATURE_RESEARCH.md](./LLAMA_CPP_API_FEATURE_RESEARCH.md)
- 🇬🇧 **English (Summary):** [../en/llm/LLAMA_CPP_API_FEATURE_RESEARCH.md](../en/llm/LLAMA_CPP_API_FEATURE_RESEARCH.md)
- 🔧 **Implementation Guide:** [LLAMA_CPP_FEATURE_IMPLEMENTATION_GUIDE.md](./LLAMA_CPP_FEATURE_IMPLEMENTATION_GUIDE.md)

---

## 💡 Empfohlene Implementierungs-Reihenfolge

### Woche 1: Quick Wins
1. ✅ Flash Attention aktivieren
2. ✅ KV-Cache Reuse implementieren
3. ✅ Embeddings-Extraktion hinzufügen

**Gesamt-Aufwand:** ~5 Arbeitstage  
**Gesamt-Impact:** Sofortige 2-3x Performance-Verbesserung

### Wochen 2-8: Major Features
1. 🚀 Speculative Decoding
2. 📈 Continuous Batching
3. 🎯 Grammar Generation

**Gesamt-Aufwand:** ~6-8 Wochen  
**Gesamt-Impact:** 5-10x Performance-Boost

### Monate 3-6: Advanced Features
1. 🖼️ Vision Support
2. 🔧 RoPE Scaling
3. 🌐 Server Mode

**Gesamt-Aufwand:** ~3-6 Monate  
**Gesamt-Impact:** Feature-Parity mit Top-Tier LLM-Servern

---

## 🔍 Vergleich mit Wettbewerbern

ThemisDB ist aktuell in **Lazy Loading** und **Multi-LoRA** führend, aber hinkt bei **Performance-Features** hinterher:

| Stärken | Schwächen |
|---------|-----------|
| ✅ Lazy Model Loading (Ollama-Style) | ❌ Continuous Batching (wie vLLM) |
| ✅ Multi-LoRA Management (vLLM-Style) | ❌ Speculative Decoding |
| ✅ Native DB-Integration | ❌ Flash Attention (nicht aktiv) |
| ✅ RAG-Optimiert | ❌ KV-Cache Reuse (nur Skeleton) |

**Empfehlung:** Fokus auf Performance-Features in v1.3.1-v1.4

---

## 📞 Kontakt

- **Issues:** https://github.com/makr-code/ThemisDB/issues
- **Diskussionen:** https://github.com/makr-code/ThemisDB/discussions
- **Dokumentation:** https://makr-code.github.io/ThemisDB/

---

**Maintainer:** ThemisDB Core Team  
**Letzte Aktualisierung:** 5. Januar 2026

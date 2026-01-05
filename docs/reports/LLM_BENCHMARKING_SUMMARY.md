# LLM Benchmarking Setup - Zusammenfassung

## ✅ Erstellte Dateien

### 1. PowerShell Scripts

#### `scripts/download-ollama-models.ps1`
- **Funktion:** Downloads Modelle von Ollama, konvertiert zu GGUF, speichert in `.\models`
- **Features:**
  - ⚡ **Offline-First:** Prüft lokalen Ollama-Cache vor Download
  - Automatische Ollama-Erkennung und Model-Pull nur wenn nötig
  - GGUF-Export aus Ollama's interner Blob-Struktur
  - Intelligente Blob-Erkennung auch ohne Manifest
  - Metadaten-Generierung für jedes Modell
  - Benchmark-Konfigurations-Datei
- **Nutzung:** `.\scripts\download-ollama-models.ps1 -ModelNames @("llama3.2:1b")`

#### `scripts/run-llm-benchmarks.ps1`
- **Funktion:** Führt LLM-Inferencing-Benchmarks mit heruntergeladenen Modellen aus
- **Features:**
  - Auto-Detection von Modellen aus `.\models`
  - Integration mit Google Benchmark Framework
  - JSON + HTML Report-Generierung
  - Performance-Metriken für RAG, Embedding, Text-Generation
- **Nutzung:** `.\scripts\run-llm-benchmarks.ps1 -ModelPath ".\models\phi3_mini.gguf"`

#### `scripts/setup-llm-benchmarks.ps1`
- **Funktion:** Kompletter End-to-End Workflow
- **Features:**
  - Prerequisites-Check (Ollama, Build)
  - Automatisches Starten von Ollama Service
  - ThemisDB Build mit LLM-Support
  - Model-Download + Benchmark-Execution
  - HTML-Report automatisch öffnen
- **Nutzung:** `.\scripts\setup-llm-benchmarks.ps1` (ein Befehl für alles!)

### 2. Dokumentation

#### `models/README.md`
- Schnellstart-Anleitung
- Modell-Empfehlungen (tinyllama → mistral)
- Integration-Beispiele (C++, CLI, Environment)
- Troubleshooting-Guide

#### `docs/LLM_BENCHMARKING_GUIDE.md`
- Umfassende Dokumentation
- Script-Referenz mit allen Parametern
- Performance-Optimierungen
- Benchmark-Szenarien und Ziele

### 3. Benchmark-Code

#### `benchmarks/bench_llm_real_models.cpp`
- **Funktion:** Benchmark-Suite für reale LLM-Modelle
- **Features:**
  - `RealModel_TextEmbedding_Generation` - Embedding-Performance
  - `RealModel_TextGeneration_50Tokens` - Text-Generierung
  - `RealModel_RAG_Pipeline_EndToEnd` - Kompletter RAG-Workflow
  - `RealModel_BatchEmbedding_100Docs` - Batch-Processing
  - `RealModel_LoadingTime` - Model-Loading Performance
  - `RealModel_ContextScaling` - Context-Size Scaling
- **Integration:** Verwendet THEMIS_LLM_MODEL_PATH Environment Variable

## 🚀 Verwendung

### Einfachster Weg (empfohlen)

```powershell
# Alles in einem Befehl
cd C:\VCC\themis
.\scripts\setup-llm-benchmarks.ps1
```

Das erledigt automatisch:
1. ✅ Prüft Ollama-Installation
2. ✅ Startet Ollama Service falls nötig
3. ✅ Baut ThemisDB mit LLM-Support
4. ✅ Lädt `tinyllama:1.1b` herunter (~637 MB)
5. ✅ Führt Benchmarks aus
6. ✅ Generiert HTML-Report
7. ✅ Öffnet Report im Browser

### Individueller Workflow

```powershell
# 1. Modelle herunterladen
.\scripts\download-ollama-models.ps1 -ModelNames @("llama3.2:1b", "phi3:mini", "mistral:7b")

# 2. Benchmarks ausführen
.\scripts\run-llm-benchmarks.ps1 -ModelPath ".\models\llama3.2_1b.gguf" -Iterations 100

# 3. Spezifische Benchmarks
.\scripts\run-llm-benchmarks.ps1 -BenchmarkFilter "LLMInferencing.*RAG"
```

## 📊 Benchmark-Szenarien

| Benchmark | Beschreibung | Performance-Ziel |
|-----------|--------------|------------------|
| **EmbeddingGeneration_Store** | Text → 1536D Vektor + DB Storage | < 50ms |
| **RAG_Search_Retrieve_Top50** | Semantische Suche über 50k Docs | < 100ms |
| **MultiQueryExpansion_5Queries** | 5 Query-Varianten parallel | < 250ms |

## 📦 Empfohlene Modelle

### Development & Quick Tests
```powershell
.\scripts\download-ollama-models.ps1 -ModelNames @("tinyllama:1.1b")
```
- Größe: ~637 MB
- Download: ~30 Sekunden
- Benchmark-Zeit: ~2 Minuten

### Production Benchmarks
```powershell
.\scripts\download-ollama-models.ps1 -ModelNames @("phi3:mini", "llama3.2:3b")
```
- Phi3-mini: ~2.3 GB (Microsoft's effizientes 3.8B Modell)
- Llama3.2:3b: ~2 GB (Meta's performantes 3B Modell)

### Enterprise Tests
```powershell
.\scripts\download-ollama-models.ps1 -ModelNames @("mistral:7b", "llama3.1:8b")
```
- Mistral:7b: ~4.1 GB (Production-Grade)
- Llama3.1:8b: ~4.7 GB (Latest Meta)

## 🔧 Integration in ThemisDB

### Build mit LLM-Support

```powershell
# Via Script (empfohlen)
.\scripts\build-themis-server-llm.ps1

# Oder manuell
cmake -S . -B build-msvc -DTHEMIS_ENABLE_LLM=ON
cmake --build build-msvc --config Release
```

### Environment Configuration

```powershell
# Modell-Pfad setzen
$env:THEMIS_LLM_MODEL_PATH = "C:\VCC\themis\models\llama3.2_1b.gguf"

# ThemisDB starten
.\build-msvc\Release\themis_server.exe
```

### C++ API

```cpp
#include "llm/llm_plugin_manager.h"

auto& manager = themis::llm::LLMPluginManager::getInstance();

manager.createLlamaWrapper(
    "my_model",
    "C:/VCC/themis/models/phi3_mini.gguf",
    config
);

auto embedding = manager.generateEmbedding("your text");
```

## 📈 Output-Dateien

Nach erfolgreicher Ausführung:

```
C:\VCC\themis\
├── models\
│   ├── tinyllama_1.1b.gguf           # GGUF Modell
│   ├── tinyllama_1.1b.gguf.json      # Metadaten
│   ├── benchmark_config.json          # Konfiguration
│   └── README.md                      # Dokumentation
│
├── benchmark_llm_report_TIMESTAMP.json   # JSON-Ergebnisse
└── benchmark_llm_report_TIMESTAMP.html   # HTML-Report (öffnet automatisch)
```

## 🛠️ Troubleshooting

### "Ollama is not running"
```powershell
# Ollama Service starten
Start-Service Ollama
# Oder: ollama serve
```

### "Model not found"
```powershell
# Modell neu laden
.\scripts\download-ollama-models.ps1 -ModelNames @("llama3.2:1b")
```

### "Benchmark executable not found"
```powershell
# Build mit LLM-Support
.\scripts\build-themis-server-llm.ps1
```

## 📚 Dokumentation

- **Schnellstart:** [`models/README.md`](../models/README.md)
- **Umfassend:** [`docs/LLM_BENCHMARKING_GUIDE.md`](../docs/LLM_BENCHMARKING_GUIDE.md)
- **Benchmarks:** [`benchmarks/bench_llm_real_models.cpp`](../benchmarks/bench_llm_real_models.cpp)

## 🎯 Nächste Schritte

1. **Testen Sie den Workflow:**
   ```powershell
   .\scripts\setup-llm-benchmarks.ps1
   ```

2. **Experimentieren Sie mit verschiedenen Modellen:**
   ```powershell
   .\scripts\download-ollama-models.ps1 -ModelNames @("phi3:mini", "mistral:7b")
   ```

3. **Optimieren Sie die Performance:**
   - CPU-Threads anpassen: `$env:OMP_NUM_THREADS = 8`
   - GPU-Unterstützung aktivieren (CUDA/ROCm)
   - Context-Größe tunen

4. **Integration in CI/CD:**
   - Automatische Benchmarks bei Releases
   - Performance-Regression Detection
   - Model-Performance Tracking

---

**Status:** ✅ Komplett implementiert und einsatzbereit  
**Version:** 1.3.0+  
**Datum:** 24. Dezember 2024

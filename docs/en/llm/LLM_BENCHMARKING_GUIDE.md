# ThemisDB LLM Benchmarking mit Ollama-Modellen

## 📖 Übersicht

Diese Dokumentation beschreibt die Integration von Ollama-Modellen in ThemisDB für realistische LLM-Inferencing-Benchmarks.

## 🎯 Ziel

Reale GGUF-Modelle von Ollama beziehen und in ThemisDB als Inferencing-Engine für Performance-Benchmarks nutzen.

## 🚀 Schnellstart

### Komplett-Workflow (Empfohlen)

```powershell
# Alles in einem Schritt: Download + Build + Benchmark
.\scripts\setup-llm-benchmarks.ps1
```

Das Script führt automatisch aus:
1. ✓ Prüft Ollama-Installation
2. ✓ Baut ThemisDB mit LLM-Support
3. ✓ Lädt Modelle herunter (`tinyllama:1.1b` als Default)
4. ✓ Führt Benchmarks aus
5. ✓ Generiert HTML-Report

### Individueller Workflow

#### 1. Modelle herunterladen

```powershell
# Standard-Modelle für Tests
.\scripts\download-ollama-models.ps1

# Spezifische Modelle
.\scripts\download-ollama-models.ps1 -ModelNames @("llama3.2:1b", "phi3:mini")

# Production-Grade Modelle
.\scripts\download-ollama-models.ps1 -ModelNames @("mistral:7b", "llama3.1:8b")
```

**Intelligente Offline-Erkennung:**
- Das Script prüft automatisch, ob Modelle bereits von Ollama heruntergeladen wurden
- Falls vorhanden: Direkte Kopie aus lokalem Cache (⚡ schnell, keine Bandbreite)
- Falls nicht vorhanden: Download via `ollama pull` und anschließende Konvertierung

**Output:**
- `.\models\<model_name>.gguf` - GGUF Modell-Datei
- `.\models\<model_name>.gguf.json` - Metadaten
- `.\models\benchmark_config.json` - Benchmark-Konfiguration

#### 2. Benchmarks ausführen

```powershell
# Mit automatischer Modellerkennung
.\scripts\run-llm-benchmarks.ps1

# Spezifisches Modell
.\scripts\run-llm-benchmarks.ps1 -ModelPath ".\models\llama3.2_1b.gguf"

# Nur bestimmte Benchmarks
.\scripts\run-llm-benchmarks.ps1 -BenchmarkFilter "LLMInferencing.*RAG"

# Mehr Iterationen für genauere Ergebnisse
.\scripts\run-llm-benchmarks.ps1 -Iterations 200
```

## 📊 Verfügbare Benchmarks

### 1. Embedding Generation & Storage
**Test:** `LLMInferencingBench::EmbeddingGeneration_Store`

Misst die Performance von:
- Text → Embedding-Vektor (1536D) Konvertierung
- Speicherung in ThemisDB mit Metadaten
- Typischer RAG-Ingest-Workflow

**Ziel:** < 50ms pro Dokument

### 2. RAG Retrieval (Top-50)
**Test:** `LLMInferencingBench::RAG_Search_Retrieve_Top50`

Misst die Performance von:
- Semantische Suche über 50.000 Embeddings
- Top-K (K=50) Nearest Neighbor Retrieval
- Context-Assembly für LLM-Prompts

**Ziel:** < 100ms pro Query

### 3. Multi-Query Expansion
**Test:** `LLMInferencingBench::MultiQueryExpansion_5Queries`

Misst die Performance von:
- 5 Query-Variationen aus Original-Query
- Parallele Vektor-Suchen
- Result Fusion & Ranking

**Ziel:** < 250ms für 5 Queries

## 🔧 Script-Referenz

### download-ollama-models.ps1

Downloads Modelle von Ollama und konvertiert zu GGUF. Prüft automatisch auf bereits vorhandene Modelle im lokalen Ollama-Cache für schnelleres Kopieren ohne erneuten Download.

**Parameter:**
- `-ModelNames` - Array von Modell-Namen (z.B. `@("llama3.2:1b")`)
- `-OutputDir` - Zielverzeichnis (default: `.\models`)
- `-OllamaUrl` - Ollama API URL (default: `http://localhost:11434`)

**Smart Features:**
- ✅ **Offline-First:** Prüft lokalen Ollama-Cache vor Download
- ✅ **Bandwidth-Saving:** Kopiert vorhandene Modelle direkt (keine Bandbreite)
- ✅ **Auto-Detection:** Findet Modell-Blobs auch ohne Manifest
- ✅ **Metadaten:** Generiert JSON-Metadaten für ThemisDB

**Beispiele:**
```powershell
# Einzelnes Modell
.\scripts\download-ollama-models.ps1 -ModelNames @("phi3:mini")

# Mehrere Modelle
.\scripts\download-ollama-models.ps1 -ModelNames @("llama3.2:1b", "mistral:7b", "phi3:mini")

# Mit Custom Output
.\scripts\download-ollama-models.ps1 -ModelNames @("tinyllama:1.1b") -OutputDir "D:\LLM-Models"
```

### run-llm-benchmarks.ps1

Führt LLM-Inferencing-Benchmarks mit heruntergeladenen Modellen aus.

**Parameter:**
- `-ModelPath` - Pfad zum GGUF-Modell (auto-detect wenn leer)
- `-BenchmarkFilter` - Google Benchmark Filter (default: `"LLMInferencing"`)
- `-BuildDir` - Build-Verzeichnis (default: `.\build-msvc`)
- `-Iterations` - Anzahl Iterationen (default: `100`)
- `-OutputReport` - Report-Pfad (default: `.\benchmark_llm_report.json`)

**Beispiele:**
```powershell
# Standard
.\scripts\run-llm-benchmarks.ps1

# Mit spezifischem Modell und mehr Iterationen
.\scripts\run-llm-benchmarks.ps1 `
    -ModelPath ".\models\phi3_mini.gguf" `
    -Iterations 200

# Nur RAG-Benchmarks
.\scripts\run-llm-benchmarks.ps1 -BenchmarkFilter ".*RAG.*"
```

### setup-llm-benchmarks.ps1

Kompletter Workflow: Prerequisites → Download → Build → Benchmark.

**Parameter:**
- `-Models` - Zu ladende Modelle (default: `@("tinyllama:1.1b")`)
- `-SkipDownload` - Download überspringen
- `-SkipBuild` - Build-Check überspringen
- `-BenchmarkIterations` - Iterationen (default: `50`)

**Beispiele:**
```powershell
# Schneller Test
.\scripts\setup-llm-benchmarks.ps1

# Production Benchmarks
.\scripts\setup-llm-benchmarks.ps1 `
    -Models @("mistral:7b", "llama3.1:8b") `
    -BenchmarkIterations 200

# Nur Benchmarks (Modelle bereits vorhanden)
.\scripts\setup-llm-benchmarks.ps1 -SkipDownload
```

## 📦 Modell-Empfehlungen

| Anwendungsfall | Modell | Größe | Download-Zeit | Benchmark-Zeit |
|----------------|--------|-------|---------------|----------------|
| **Quick Tests** | `tinyllama:1.1b` | ~637 MB | ~30 sec | ~2 min |
| **Development** | `llama3.2:1b` | ~1.3 GB | ~1 min | ~3 min |
| **Benchmarks** | `phi3:mini` | ~2.3 GB | ~2 min | ~5 min |
| **Production** | `mistral:7b` | ~4.1 GB | ~5 min | ~10 min |
| **Enterprise** | `llama3.1:8b` | ~4.7 GB | ~6 min | ~12 min |

## 🔍 Output-Dateien

Nach erfolgreicher Ausführung:

```
.\models\
├── tinyllama_1.1b.gguf              # GGUF Modell
├── tinyllama_1.1b.gguf.json         # Metadaten
├── benchmark_config.json             # Benchmark Config
└── README.md                         # Dokumentation

.\
├── benchmark_llm_report_TIMESTAMP.json   # Detaillierte Ergebnisse
└── benchmark_llm_report_TIMESTAMP.html   # Visueller Report
```

## 🛠️ Troubleshooting

### Problem: "Ollama is not running"

**Lösung:**
```powershell
# Ollama Service starten
Start-Service Ollama

# Oder manuell
ollama serve
```

### Problem: "Model file not found"

**Lösung:**
```powershell
# Verfügbare Modelle prüfen
ollama list

# Modell neu laden
.\scripts\download-ollama-models.ps1 -ModelNames @("llama3.2:1b")
```

### Problem: "Benchmark executable not found"

**Lösung:**
```powershell
# ThemisDB mit LLM-Support bauen
.\scripts\build-themis-server-llm.ps1

# Oder manuell
cmake -S . -B build-msvc -DTHEMIS_ENABLE_LLM=ON
cmake --build build-msvc --config Release --target bench_comprehensive
```

### Problem: "GGUF export failed"

**Hintergrund:** Ollama speichert Modelle in einem eigenen Format. Das Script versucht, das größte Blob zu exportieren.

**Lösung:**
```powershell
# Manueller Export
$modelName = "llama3.2_1b"
$ollamaDir = "$env:USERPROFILE\.ollama\models\blobs"

# Größtes Blob finden
$blob = Get-ChildItem $ollamaDir | Sort-Object Length -Descending | Select-Object -First 1

# Kopieren
Copy-Item $blob.FullName ".\models\$modelName.gguf"
```

## 📈 Performance-Optimierung

### CPU-Only

```powershell
# Threads optimieren
$env:OMP_NUM_THREADS = 8
.\scripts\run-llm-benchmarks.ps1
```

### Mit GPU (falls verfügbar)

ThemisDB's llama.cpp Integration unterstützt:
- CUDA (NVIDIA)
- ROCm (AMD)
- Metal (macOS)
- Vulkan (cross-platform)

```powershell
# Build mit GPU-Support
cmake -S . -B build-msvc `
    -DTHEMIS_ENABLE_LLM=ON `
    -DLLAMA_CUDA=ON

cmake --build build-msvc --config Release
```

## 🔗 Integration in Anwendungen

### C++ API

```cpp
#include "llm/llm_plugin_manager.h"

// Plugin Manager initialisieren
auto& manager = themis::llm::LLMPluginManager::getInstance();

// Modell laden
manager.createLlamaWrapper(
    "production_model",
    "C:/VCC/themis/models/mistral_7b.gguf",
    config
);

// Inferencing
auto results = manager.generate("your prompt", params);
```

### Command-Line

```bash
# ThemisDB Server mit LLM
themis_server \
    --llm-model=./models/phi3_mini.gguf \
    --llm-threads=4 \
    --llm-context-size=2048
```

### Environment Variables

```powershell
$env:THEMIS_LLM_MODEL_PATH = "C:\VCC\themis\models\llama3.2_1b.gguf"
$env:THEMIS_LLM_THREADS = "8"
$env:THEMIS_LLM_GPU_LAYERS = "32"  # Falls GPU verfügbar
```

## 📚 Weitere Ressourcen

- [Ollama Model Library](https://ollama.ai/library)
- [ThemisDB LLM Documentation](../docs/llm_integration.md)
- [llama.cpp GitHub](https://github.com/ggerganov/llama.cpp)
- [Benchmark Framework](../benchmarks/README.md)

## 🤝 Support & Contribution

Bei Fragen oder Problemen:
1. Prüfen Sie die [Troubleshooting](#troubleshooting) Sektion
2. Schauen Sie in die [Issues](https://github.com/your-repo/themis/issues)
3. Erstellen Sie einen neuen Issue mit Details

---

**Version:** 1.3.0+  
**Datum:** 24. Dezember 2024  
**Maintainer:** ThemisDB Team

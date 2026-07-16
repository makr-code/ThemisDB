# LLM-Engine-Ausarbeitung fuer ThemisDB

Status: Entwurf
Datum: 2026-04-18
Scope: In-Process-Engines (keine API-basierten Engines)

## 1. Zielbild

Diese Ausarbeitung bewertet LLM-Inferenz-Engines fuer ThemisDB unter den folgenden Leitplanken:

- Keine API-zentrierten Engine-Modelle als Primaerstrategie
- Tiefe Integration in den bestehenden C++-Kern
- Hohe Cross-Platform-Tauglichkeit (Windows/Linux, optional macOS)
- MIT-kompatible Lizenzstrategie
- Eignung fuer lokale/offline und ggf. air-gapped Deployments

Die Bewertung baut auf dem aktuellen ThemisDB-Kontext auf:

- Bestehende Plugin-Abstraktion via ILLMPlugin
- Bereits integrierter llama_cpp-Backend-Pfad
- Vorhandene ONNX-Backends (CPU/CUDA/DirectML/TensorRT) im ONNX-CLIP-Bereich
- CMake-/Submodule-getriebener Build, stark C++-zentriert

## 2. Relevante Rahmenbedingungen in ThemisDB

### 2.1 Architekturelle Ausgangslage

ThemisDB besitzt bereits eine klare LLM- und Plugin-Struktur:

- src/llama_cpp/ als dynamisch ladbarer LLM-Backend-Pluginpfad
- src/llm/ mit Async-/Enhanced-Inference und EmbeddedLLM
- include/llm/llm_plugin_interface.h als gemeinsamer Vertragslayer fuer Backends

Das reduziert die Integrationshuerde fuer weitere In-Process-Backends deutlich, sofern sie sauber an den bestehenden Interface-Vertrag gebunden werden.

### 2.2 Lizenz-/Compliance-Leitplanke

Falls MIT-only priorisiert wird, sind folgende Kandidaten relevant:

- llama.cpp (MIT)
- ONNX Runtime GenAI (MIT)
- PowerInfer (MIT)
- mistral.rs (MIT)

Dual-Lizenz (MIT oder Apache-2.0):

- Candle

Nicht MIT-only:

- vLLM, SGLang, LMDeploy (Apache-2.0)
- TensorRT-LLM (nicht MIT)

Hinweis: Engine-Lizenz und Modell-Lizenz sind getrennt zu pruefen.

## 3. Kandidaten (fokussiert auf weniger bekannte Optionen)

## 3.1 PowerInfer (Tiiny-AI/PowerInfer)

Kurzprofil:

- Typ: Lokale CPU/GPU-Hybrid-Inferenzengine (C/C++-nah)
- Lizenz: MIT
- Schwerpunkt: Consumer-GPU-Beschleunigung, teils llama.cpp-kompatible Bedienmuster

Staerken:

- Gute Passung zu C++-zentriertem In-Process-Design
- MIT-Lizenz
- Lokaler Betrieb ohne API-Zwang
- Windows- und Linux-Pfade vorhanden

Risiken:

- Starke Optimierung auf bestimmte Sparse/ReLU-Modellfamilien
- Potenziell schmalere Modellbreite gegenueber llama.cpp
- Projektfokus liegt nicht primaer auf maximaler Allgemeinheit

Eignung fuer ThemisDB:

- Interessanter Spezial-Backend-Kandidat fuer lokal/perf-sensitive Setups
- Nicht als alleinige Universal-Engine empfehlenswert

## 3.2 Candle (huggingface/candle)

Kurzprofil:

- Typ: Rust-ML-Runtime mit lokalen Inferenzpfaden
- Lizenz: MIT oder Apache-2.0 (Dual)
- Schwerpunkt: Lightweight, Python-freie Runtime, mehrere Backends inkl. CPU/CUDA/Metal/WASM

Staerken:

- Sehr moderne Runtime-Architektur
- Cross-Platform stark
- Gute Perspektive fuer kompaktes Deployment

Risiken:

- Rust-first: tiefe C++-Integration erfordert FFI/Bridge
- Erhoehte Komplexitaet im Build/Debug/Profiling gegenueber reinem C++-Pfad

Eignung fuer ThemisDB:

- Strategisch interessant, falls Rust-Bruecke akzeptiert wird
- Kurzfristig fuer tiefen C++-In-Process-Pfad aufwendiger als llama.cpp/ONNX

## 3.3 CPM.cu (OpenBMB/CPM.cu)

Kurzprofil:

- Typ: CUDA-fokussierte LLM-Inferenz mit starkem Optimierungsfokus
- Lizenz: Apache-2.0
- Schwerpunkt: Endgeraete/GPU, Speculative Decoding, Quantisierung

Staerken:

- Technisch spannend fuer CUDA-zentrierte Beschleunigung

Risiken:

- Nicht MIT-only
- Staerkere Python-/Service-Umgebung in typischen Nutzungswegen
- Geringere Passung zur geforderten tiefen C++-Kernintegration

Eignung fuer ThemisDB:

- Unter MIT-only und In-Process-Prioritaet derzeit nachrangig

## 3.4 LMDeploy (InternLM/lmdeploy)

Kurzprofil:

- Typ: Toolkit fuer Kompression, Deployment und Serving
- Lizenz: Apache-2.0
- Schwerpunkt: High-Performance-Serving (TurboMind/PyTorch)

Staerken:

- Starke Performance- und Quantisierungs-Story

Risiken:

- Nicht MIT-only
- Tooling/Serving-lastig statt C++-Kernintegration-first

Eignung fuer ThemisDB:

- Bei API-/Service-Verzicht nur begrenzt passend

## 4. Abgrenzung: Engine vs. Quantisierung

AWQ (mit-han-lab/llm-awq) ist keine vollwertige Engine-Alternative, sondern eine Quantisierungs-/Optimierungsschicht.

Konsequenz fuer ThemisDB:

- AWQ ist als zusaetzlicher Baustein sinnvoll
- AWQ ersetzt keine Basis-Engine

## 5. Entscheidungs-Matrix (ThemisDB-spezifisch)

Skala: 1 (schwach) bis 5 (sehr gut)

| Kriterium | llama.cpp | ONNX Runtime GenAI | PowerInfer | Candle |
|---|---:|---:|---:|---:|
| Tiefe C++-Integration | 5 | 5 | 4 | 2 |
| Cross-Platform (Win/Linux) | 5 | 5 | 4 | 4 |
| MIT-Kompatibilitaet | 5 | 5 | 5 | 5 (dual) |
| Integrationsaufwand in ThemisDB | 5 | 4 | 3 | 2 |
| Modell-/Backend-Breite | 5 | 4 | 2-3 | 3 |
| Offline/Air-Gap-Eignung | 5 | 5 | 4 | 4 |
| Betriebsrisiko (kurzfristig) | 5 | 4 | 3 | 2 |

Interpretation:

- Primaer: llama.cpp (bestehender Goldpfad)
- Sekundaer: ONNX Runtime GenAI (starker, plattformweiter Gegenpfad)
- Spezialpfad: PowerInfer fuer ausgewahlte lokale Performance-Szenarien
- Explorativ: Candle bei strategischer Rust-Bereitschaft

## 6. Empfehlung

### 6.1 Kurzfristig (0-1 Quartal)

- llama.cpp als Primaer-Engine beibehalten
- ONNX Runtime GenAI als zweiten lokalen Engine-Pfad formalisieren
- Keine API-zentrierten Engines als Kernstrategie

### 6.2 Mittelfristig (1-2 Quartale)

- PowerInfer als optionales Spezial-Plugin evaluieren (nur Go bei klarer Mehrleistung auf euren Zielmodellen)
- AWQ als Optimierungsschicht pruefen (unabhaengig von Basis-Engine)

### 6.3 Langfristig

- Candle nur dann vertiefen, wenn Rust-FFI in Architektur und Betrieb explizit gewollt ist

## 7. Konkreter Umsetzungsplan (ThemisDB)

### Phase 1: Interface-Haertung

- ILLMPlugin-Vertrag final gegen alle Kern-Use-Cases pruefen:
  - Streaming
  - Embeddings
  - LoRA
  - Structured Output / Tool-Calls
  - Tracing/Telemetry-Felder
- Einheitliche Capability-Matrix in Plugin-Registry erzwingen

### Phase 2: ONNX-Backend als 1st-class Engine

- ONNX Runtime GenAI Adapter als vollwertigen LLM-Pluginpfad implementieren
- Gemeinsame Test-Suite (Contract-Tests) mit llama.cpp
- Build-Optionen edition-spezifisch schaltbar halten

### Phase 3: PowerInfer Spike

- Kleiner Adapter-Prototyp hinter ILLMPlugin
- Fokus-Benchmarks auf identischen Prompts/Modellen:
  - Token/s
  - P95-Latenz
  - Speicherverbrauch
  - Stabilitaet unter Last
- Go/No-Go anhand harter Schwellwerte

## 8. Go/No-Go Kriterien fuer neue Engine

Eine neue Engine wird nur dauerhaft aufgenommen, wenn alle Punkte erfuellt sind:

- Erfuellt Plugin-Contract ohne Sonderpfade
- Build und Tests laufen stabil auf Windows und Linux
- Liefert reproduzierbaren Mehrwert (mindestens ein KPI deutlich besser)
- Erhoeht den operativen Aufwand nicht unverhaeltnismaessig
- Lizenz- und Modell-Compliance sauber dokumentiert

## 9. Zusammenfassung

Fuer ThemisDB unter den Vorgaben "cross-platform + tiefe Integration + keine API-Engine als Kern + MIT-orientiert" ist das Zielbild:

- Basis: llama.cpp
- Zweiter strategischer Pfad: ONNX Runtime GenAI
- Optionale Spezialisierung: PowerInfer
- Explorative Schiene: Candle (nur bei expliziter Rust-Strategie)

Damit bleibt die Architektur robust, lokal betreibbar und anschlussfaehig fuer kuenftige Performance-Optimierungen ohne Lock-in auf servicezentrierte Laufzeiten.

> **Status:** 2026-09-22 – mit aktuellem ONNX CLIP-Code (`onnx_clip_plugin.cpp`, `onnx_clip_plugin.h`) abgeglichen.

# ThemisDB ONNX CLIP Plugin - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des ONNX CLIP-Plugins.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für die CLIP-basierte Bild- und Texteinbettungsgenerierung.

## Dokumentabgrenzung (Canonical Split)

- **`src/onnx_clip/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/onnx_clip/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/onnx_clip/ARCHITECTURE.md`:** Komponentendesign, Schnittstellen, Datenflussbeschreibung.
- **`src/onnx_clip/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/onnx_clip/SECURITY.md`:** Threat-Modelle, Sicherheitskontrollen, bekannte Limitierungen.
- **`src/onnx_clip/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

### Initialisierung und Konfiguration

- **MUST:** Plugin wird über `initialize(config, backend)` mit vollständiger `PluginConfig` initialisiert; keine Defaults ohne explizite Übergabe.
- **MUST:** Konfigurationswerte werden beim Start validiert; ungültige Konfigurationen führen zu `initialize()`-Fehler.
- **MUST:** `model.name`, `model.embedding_dim`, und `max_batch_size` müssen explizit gesetzt sein.
- **MUST NOT:** Standard-Defaults (z.B. `clip-vit-base-patch32` mit 512 Dimensionen) in Produktionsdeployments ohne explizite Validierung verwenden.

### Backend-Selektion und -Verfügbarkeit

- **MUST:** Backend-Selektion wird durch den angeforderten `BackendType` gesteuert; keine implizite Fallback-Änderung ohne Logging.
- **MUST:** Im `AUTO`-Modus wird die Backend-Probe-Reihenfolge (CUDA → TensorRT → DirectML → CPU) streng eingehalten.
- **MUST:** Wenn der angeforderte Backend nicht verfügbar ist, kehrt `initialize()` mit `false` zurück; kein Silent-Fallback.
- **MUST:** GPU-Backend (CUDA/TensorRT/DirectML) erfordert explizite Installation und Konfiguration; fehlende GPU-Bibliotheken führen zu `initialize()`-Fehler.

### Modell-Integrität und -Verifikation

- **MUST:** Wenn `model.expected_sha256` in der Konfiguration gesetzt ist, wird die SHA-256-Integrität beim Laden verifiziert.
- **MUST:** Hash-Mismatch führt zu `initialize()`-Fehler; das Plugin wird nicht mit einem veränderten Modell gestartet.
- **MUST:** Die SHA-256-Verifizierung läuft über OpenSSL EVP, wenn `THEMIS_HAS_OPENSSL` definiert ist; sonst kann ein injizierter `ModelHashFn` bereitgestellt werden.
- **MUST NOT:** SHA-256-Verifizierung in Produktionspfaden deaktivieren oder überspringen.

### Anfrage-Verarbeitung und Thread-Sicherheit

- **MUST:** Alle Anfragen (`generateEmbedding()`, `generateEmbeddingBatch()`, `generateTextEmbedding()`) werden durch einen `std::mutex` serialisiert.
- **MUST:** Nebenläufige Anfragen von mehreren Threads werden sequenziell verarbeitet; kein Silent-Merge oder Parallelisierung ohne explizite Codeanpassung.
- **MUST:** Eine Anfrage wird vollständig verarbeitet oder mit `EmbeddingResult{ok=false}` zurückgewiesen; keine partiellen Ergebnisse.
- **MUST:** Fehler während der Inferenz werden propagiert; kein Silent-Fallback auf Default-Embeddings.

### Batch-Verarbeitung

- **MUST:** Batches werden automatisch in Sub-Batches aufgeteilt, wenn die Eingabegröße `max_batch_size` überschreitet.
- **MUST:** `max_batch_size` wird aus der Konfiguration gelesen; Standard ist 16 (CPU) oder 64 (GPU). <!-- source: src/onnx_clip/onnx_clip_plugin.cpp Impl struct -->
- **MUST:** Leere Batches oder Batches mit `null`-Bildern führen zu `EmbeddingResult{ok=false}` für betroffene Einträge.

### Modell-Neuladen (Hot-Swap) – Phase 3B

- **MUST:** `reloadModel(new_config)` lädt das Modell neu, ohne den Server zu stoppen oder bestehende Anfragen zu unterbrechen.
- **MUST:** In-Flight-Anfragen werden auf Completion gewartet (30-Sekunden-Timeout); keine Anfrage wird abgebrochen.
- **MUST:** Das neue Modell wird validiert (Konfiguration, Integrität, Backend-Verifikation) vor dem Swap.
- **MUST:** Auf Fehler (Validierung, Integrität, Konfiguration) oder Drain-Timeout wird Rollback ausgeführt; das alte Modell bleibt aktiv, und `reloadModel()` gibt `false` zurück.
- **MUST NOT:** `reloadModel()` aufrufen während das Plugin nicht initialisiert ist; `isReady()` muss `true` sein.

### Speicher-Gemapte Modellladung – Phase 4B

- **MUST:** Wenn `enable_mmap_loading` in der Konfiguration `true` ist, wird das Modell speicher-gemappt (Linux: `mmap()`, Windows: `MapViewOfFile()`).
- **MUST:** Memory-Mapping reduziert Peak-Memory, aber erhöht Latenz bei der ersten Inferenz; nur für Workloads mit Memory-Constraints empfohlen.
- **MUST:** Memory-Mapping ist optional; Standard ist `false` (traditionelles File-Loading).

## Verbindliche Sicherheitsanforderungen

### Bildverarbeitung und Input-Validierung

- **MUST:** Bilder werden dekodiert und auf [1, 3, 224, 224] (RGB, 224×224 Pixel) normalisiert.
- **MUST:** Eingabebilder, die diese Größe überschreiten, werden resized, nicht expanded oder buffered.
- **MUST:** Ungültige Bildformate oder korrupte Bilder führen zu `EmbeddingResult{ok=false}`.
- **MUST NOT:** Originale Bilddaten mit ungültiger Größe an die ONNX-Session weitergeben.

### Text-Verarbeitung und Tokenisierung

- **MUST:** Texteingaben werden durch einen BPE-ähnlichen Tokenizer verarbeitet.
- **MUST:** Leere oder `null`-Texte führen zu `EmbeddingResult{ok=false}`.
- **MUST:** Maximal zulässige Text-Länge wird durch das Modell definiert (typisch: 77 Tokens für CLIP); längere Texte werden abgeschnitten.

### GPU-Ressourcen und Speicherschutz

- **MUST:** GPU-Memory wird durch die feste Input-Größe und `max_batch_size`-Limit begrenzt.
- **MUST:** Keine Anfrage kann mehr als ~600 MB GPU-Memory verbrauchen (typisch 400-600 MB für ViT-base Modell mit Batch-Größe bis 64).
- **MUST:** Out-of-Memory-Fehler führen zu `EmbeddingResult{ok=false}`, nicht zu Prozess-Crash.

### Datenblöcke und Biometric-Sensitivität

- **MUST:** CLIP-Embeddings sind semantisch, nicht biometrisch; jedoch können Gesichts-Embeddings für Re-Identifikation genutzt werden.
- **MUST NOT:** Biometrische Bilder (Gesichter) ohne DSGVO-Artikel-9-Bewertung und Zugriffskontrolle im ThemisDB-Index speichern.

## Betriebsgrenzen

### Konfiguration und Umgebung

- Deployment-spezifische Konfigurationswerte müssen explizit gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Model Path muss auf eine lesbar/unveränderbar gehaltene Datei zeigen; keine schreibbaren Modell-Dateien in Produktionsdeployments.
- GPU-Backends erfordern installierte NVIDIA CUDA / TensorRT oder entsprechende Bibliotheken.

### Performance und Latenz

- Typische Inference-Latenz: 10-50ms pro Bild (CPU), 5-15ms (GPU) bei 224×224-Eingabe.
- Batch-Latenz: 20-100ms für 64 Bilder (CPU), 15-40ms (GPU).
- Maximale akzeptable Latenz pro Anfrage: 5 Sekunden (sonst `EmbeddingResult{ok=false}`).

### Ressource-Limits

- RAM: ~500-800 MB für Modellgewichte (ViT-base), ~200-300 MB für Aktivierungen pro Batch.
- GPU-Memory: ~400-600 MB (ViT-base), skaliert mit Batch-Größe bis zu ~600 MB bei max_batch_size.
- Thread-Serialisierung: Maximal 1 gleichzeitige Inference pro Plugin-Instanz.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Modul-Konfiguration vollständig und beim `initialize()`-Aufruf validiert
- [ ] Backend-Selektion korrekt (keine impliziten Fallbacks)
- [ ] Model-Integritätsprüfung aktiv (SHA-256 oder injizierter `ModelHashFn`)
- [ ] Bildeingaben validiert und auf 224×224 normalisiert
- [ ] Texteingaben validiert und tokenisiert
- [ ] Thread-Sicherheit via `std::mutex` durchgesetzt
- [ ] Batch-Splitting mit `max_batch_size` aktiv
- [ ] Hot-Swap Model Reloading funktioniert ohne Anfrage-Verlust (wenn verwendet)
- [ ] Fehlerbehandlung: Alle Fehler zurückgeliefert als `EmbeddingResult{ok=false}`, nicht als Exceptions
- [ ] Statistiken verfügbar über `getStatistics()` (calls, latency, backend, model_variant, batch_size)
- [ ] Health-Check über `healthCheck()` funktioniert korrekt
- [ ] Warmup-Pfad via `warmup()` für GPU-Kernel-Kompilation durchgeführt
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/onnx_clip/PRODUCTION_REQUIREMENTS.md` (dieses Dokument)
- `src/onnx_clip/onnx_clip_plugin.h` (Public-API-Schnittstelle)
- `src/onnx_clip/onnx_clip_plugin.cpp` (Implementierung: Initialisierung, Inferenz, Hot-Swap, Memory-Mapping)
- `src/onnx_clip/CMakeLists.txt` (Build-Gating, Abhängigkeiten)

### Validierung

- Unit-Tests: `tests/onnx_clip/test_onnx_clip_plugin.cpp`, `test_onnx_clip_contract_hardening_focused.cpp`
- Integration-Tests: `test_onnx_clip_golden_embeddings_focused.cpp`, `test_onnx_clip_hot_swap_focused.cpp`, `test_onnx_clip_mmap_focused.cpp`
- Benchmarks: `benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp`, `bench_onnx_clip_vit_backend.cpp`
- Compliance: Doxygen-Coverage, SHA-256-Integritätsprüfung aktiv

---

**Version:** 1.0  
**Gültig ab:** 2026-09-22  
**Nächste Überprüfung:** 2026-12-22 oder bei Versionsupgrade

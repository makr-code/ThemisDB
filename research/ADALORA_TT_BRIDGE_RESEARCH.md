# AdaLoRA ↔ Tensor-Train Bridge: Research Review (ThemisDB)

**Status:** Review-ready research note (codebase-aligned)
**Stand:** 2026-05-13
**Korrespondierende Artefakte:** `include/training/adalora_tt_bridge.h`, `src/training/adalora_tt_bridge.cpp`, `tests/test_adalora_tt_bridge.cpp`

---

## Abstract / Zusammenfassung

Dieses Dokument prüft die AdaLoRA↔Tensor-Train-(TT)-Bridge fachlich gegen den aktuellen ThemisDB-Stand und trennt klar zwischen **bereits implementiertem Verhalten** und **Roadmap-Zielen**. Mathematisch bleibt die 2D-Abbildung zwischen low-rank Faktorisierung und rank-2 TT-Darstellung konsistent. In der aktuellen Implementierung sind `exportToTT()`, `importFromTT()`, `roundAndReallocate()`, `findSimilarAdapters()` sowie injizierbare Bridge-Hooks (`mapAdapter`, `TrainingStepFn`) vorhanden.

Die zentrale Korrektur gegenüber der vorherigen Fassung: Aussagen zu "vollständig zero-copy", harter Hot-Load-Latenz im einstelligen Millisekundenbereich und produktiver FLARE-Live-Umschaltung sind derzeit **nicht als gemessene Produktionswerte belegt**, sondern als Zielpfade dokumentiert. Dieses Review liefert dafür eine evidenzbasierte Argumentationskette mit Code- und Benchmark-Bezug.

---

## Introduction / Einleitung

### Problemstellung

ThemisDB benötigt einen robusten Pfad, um AdaLoRA-Adapter effizient zu serialisieren, wiederzuverwenden und perspektivisch für schnelles Adapter-Switching im Multi-Model-Kontext bereitzustellen. Frühere Entwürfe des Bridge-Dokuments mischten jedoch theoretische Vorteile, Zielwerte und aktuellen Implementierungsstand.

### Ziel dieser Review-Version

1. Terminologie und Komponenten-Namen mit dem Quellcode vereinheitlichen.
2. Nicht belegte Leistungsbehauptungen entfernen oder als Zielwerte markieren.
3. Eine nachvollziehbare Struktur herstellen: **Problem → Ansatz → Evaluation → Grenzen → Fazit**.

### Einheitliche Terminologie (ThemisDB)

- **AdaLoRA-Bridge-Komponente:** `AdaLoraTTBridge`
- **TT-Speicherkomponente (API-Ziel):** `TensorNetworkStorageEngine`
- **Ähnlichkeit/Dedup-Komponente:** `TensorFingerprintGraph`
- **GGML-Adapter-Mapping:** `GgmlTensorBridge::mapAdapter()`
- **Rangreduktion im TT-Raum:** `TensorTrainDecomposer::round()`

---

## Methodik / Ansatz

### M1 — Codebasierter Faktencheck

Die inhaltlichen Claims wurden gegen folgende Artefakte geprüft:

- API-Vertrag: `include/training/adalora_tt_bridge.h`
- Laufzeitverhalten: `src/training/adalora_tt_bridge.cpp`
- Bridge-/Stub-Grenzen im Serving-Pfad: `include/storage/ggml_tensor_bridge.h`, `src/storage/ggml_tensor_bridge.cpp`
- Testabdeckung für Phase-3/4-Bridge-Hooks: `tests/test_adalora_tt_bridge.cpp`
- Zielwerte statt Messwerte: `src/training/PERFORMANCE_EXPECTATIONS.md`, `src/training/ROADMAP.md`

### M2 — Claim-Klassifikation

Jeder zentrale Claim wurde in eine der Kategorien einsortiert:

- **Implementiert (Code-verifiziert)**
- **Geplant (Roadmap/Zielwert)**
- **Unbelegt (entfernt oder umformuliert)**

### M3 — Mathematische Konsistenzprüfung

Die 2D-Abbildung bleibt als Arbeitsmodell gültig:

- AdaLoRA-Form: \(\Delta W = P \Lambda Q^T\)
- TT-Form (2 Kerne): \(G_0 \in \mathbb{R}^{1 \times d \times r},\; G_1 \in \mathbb{R}^{r \times k \times 1}\)

Im aktuellen Code erfolgt die praktische Konstruktion über normbasierte Approximationen (`approximateSingularValues`, `buildG0`, `buildG1`) und nicht über eine explizite vollständige SVD-Rekonstruktion jeder Schicht.

### M4 — Bewertungsregeln für Performance-Claims

- Nur als "gemessen" bezeichnen, wenn im Repository ein reproduzierbarer Benchmarkpfad + Ergebnisartefakt vorliegt.
- Ohne Messartefakt nur als **Zielwert** (SLO/Target) ausweisen.

---

## Evaluation / Experimente

### E1 — Implementierungsstand (Ist)

| Bereich | Status | Evidenz |
|---|---|---|
| Export/Import AdaLoRA↔TT (`exportToTT`, `importFromTT`) | Implementiert | `src/training/adalora_tt_bridge.cpp` |
| Layer-basierte TT-Rundung (`roundAndReallocate`) | Implementiert (Built-in + injizierbarer Hook) | `src/training/adalora_tt_bridge.cpp`, `tests/test_adalora_tt_bridge.cpp` |
| Adapter-Ähnlichkeit (`findSimilarAdapters`) | Implementiert via `TensorFingerprintGraph` | `src/training/adalora_tt_bridge.cpp` |
| `mapAdapter`-Integration an GGML-Pfad | Als injizierbare Bridge vorhanden | `include/training/adalora_tt_bridge.h`, `tests/test_adalora_tt_bridge.cpp` |
| GGML-Bridge als vollproduktiver Zero-Copy-Backendpfad | Noch nicht vollständig (Stub-/Simulation-Hinweise dokumentiert) | `src/storage/ggml_tensor_bridge.cpp` |

### E2 — Performance-Aussagen: Evidenz vs. Ziel

| Aussage | Einstufung | Begründung |
|---|---|---|
| "Adapter-Hot-Load 2–11 ms" | Nicht als Messwert belegbar | Im geprüften Stand liegt hierfür kein direktes Benchmark-Ergebnisartefakt in dieser Research-Datei vor; Zielwerte sind separat dokumentiert. |
| LoRA Hot-Load hat definierte Ziel-ID | Belegt als Zielpfad | `L-3` in `src/training/PERFORMANCE_EXPECTATIONS.md` verweist auf `BM_Storage_LoadMetadata`. |
| Numerische Trainingsmodul-Grenze für Adapter-Load | Belegt als Zielwert | `TRNG-3 <= 85 ms` in `src/training/PERFORMANCE_EXPECTATIONS.md`. |
| Zero-copy / FLARE-Live-Switch als Roadmap-Ziel | Geplant, nicht als heutiger Produktivstatus | Phase-2/3/4-Plan in `src/training/ROADMAP.md`; GGML-Bridge enthält explizite Stub-/Delta-Hinweise. |

### E3 — Testlage

Vorhanden sind fokussierte Tests für Bridge-Injections (Phase 3/4):

- `ALTB-P3-01..03`: Verhalten von `mapAdapter()` mit/ohne injiziertes Callback
- `ALTB-P4-01..03`: Verhalten von `roundAndReallocate()` mit/ohne `TrainingStepFn`

Damit ist die API-Mechanik belegt; ein Ende-zu-Ende-Benchmark für produktive GGML-Adapter-Injektion ist im hier geprüften Stand nicht Teil dieser Datei.

---

## Limitations / Known Issues

1. **Statusdifferenz zwischen Forschungstext und Produktivpfad**
   Einige historisch formulierte Claims (z. B. harte Latenzzahlen im einstelligen ms-Bereich) sind aktuell eher Zielwerte als reproduziert dokumentierte Messwerte.

2. **Storage-Pfad in Bridge-Komponente**
   Der aktuelle `AdaLoraTTBridge::store()`-Pfad nutzt in der Implementierung primär Cache/Fingerprint-Graph-Logik; die dauerhafte End-to-End-Persistenz über den finalen Produktionspfad bleibt ein Integrationspunkt gemäß Roadmap.

3. **GGML-Backend-Integration**
   `GgmlTensorBridge` dokumentiert weiterhin Stub-/Simulationsteile (insbesondere rund um echte GGML-Typregistrierung und produktive Tensor-Injektion).

4. **Benchmark-Evidenz für dedizierte Bridge-Kennzahlen**
   Für Claims wie "Adapter-Switch-Latenz" oder "Dedup-Speichergewinn" fehlen in diesem Dokument reproduzierbare Messprotokolle; diese sollten als Experimente unter `research/experiments/` nachgeführt werden.

---

## Fazit

Die AdaLoRA↔TT-Bridge ist in ThemisDB **konzeptionell tragfähig** und in zentralen API-Bausteinen bereits implementiert. Für ein belastbares Architektur-Review muss jedoch klar zwischen Ist-Stand und Roadmap-Ziel unterschieden werden. Nach dieser Überarbeitung ist das Dokument konsistent mit dem aktuellen Code: mathematische Argumentation bleibt erhalten, unbelegte Performance-Claims wurden entschärft, und die Grenzen sind explizit benannt.

---

## References / Quellen

### Externe Fachquellen (aufloesbare DOI/URL)

1. Zhang, Q. et al. (2023). *AdaLoRA: Adaptive Budget Allocation for Parameter-Efficient Fine-Tuning*. ICLR 2023.
   URL: https://arxiv.org/abs/2303.10512
2. Oseledets, I. V. (2011). *Tensor-Train Decomposition*. SIAM Journal on Scientific Computing, 33(5).
   DOI: https://doi.org/10.1137/090752142
3. Holtz, S., Rohwedder, T., Schneider, R. (2012). *On Manifolds of Tensors of Fixed TT-Rank*. SIAM Journal on Matrix Analysis and Applications, 33(4).
   DOI: https://doi.org/10.1137/110835336
4. Hu, E. J. et al. (2022). *LoRA: Low-Rank Adaptation of Large Language Models*. ICLR 2022.
   URL: https://arxiv.org/abs/2106.09685
5. Dettmers, T. et al. (2023). *QLoRA: Efficient Finetuning of Quantized LLMs*. NeurIPS 2023.
   URL: https://arxiv.org/abs/2305.14314
6. Yadav, P. et al. (2023). *TIES-Merging: Resolving Interference When Merging Models*. NeurIPS 2023.
   URL: https://arxiv.org/abs/2306.01708

### Interne Artefakte (Code-/Benchmark-Bezug)

- `include/training/adalora_tt_bridge.h`
- `src/training/adalora_tt_bridge.cpp`
- `tests/test_adalora_tt_bridge.cpp`
- `include/storage/ggml_tensor_bridge.h`
- `src/storage/ggml_tensor_bridge.cpp`
- `src/training/PERFORMANCE_EXPECTATIONS.md`
- `src/training/ROADMAP.md`

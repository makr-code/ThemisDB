[docs](../../README.md) > [en](../INDEX.md) > [research](./research.md) > [feature](./feature.md)

---
Datum: 2026-04-16
Status: draft
Primary (Quelle der Wahrheit): include/training/incremental_lora_trainer.h, include/training/auto_labeler.h, include/llm/lora_framework/lora_metrics.h, include/aql/llm_metrics_collector.h, include/observability/metrics_collector.h, include/performance/workload_adaptive_optimizer.h, include/rag/continuous_learning_orchestrator.h
Bezug / Reference: THEMISDB_LORA_RESEARCH_PAPER.md · Hu et al. (2022) LoRA arXiv:2106.09685 · Prometheus OpenMetrics spec
---

# ThemisDB-LoRA — Projektüberblick & Metrikkatalog

*Was wir bauen · Womit wir messen · Was wir daraus schließen*

---

## Inhaltsverzeichnis

1. [Was wir bauen — Projektbeschreibung](#1-was-wir-bauen--projektbeschreibung)
   - 1.1 [Das Problem](#11-das-problem)
   - 1.2 [Die Lösung in einem Satz](#12-die-lösung-in-einem-satz)
   - 1.3 [Die zwei Schichten](#13-die-zwei-schichten)
   - 1.4 [Was wir konkret implementieren](#14-was-wir-konkret-implementieren)
   - 1.5 [Was wir explizit NICHT bauen](#15-was-wir-explizit-nicht-bauen)
2. [Architekturskizze](#2-architekturskizze)
3. [Metrikkatalog](#3-metrikkatalog)
   - 3.1 [Layer 1 — Training (LoRA-Adapter)](#31-layer-1--training-lora-adapter)
   - 3.2 [Layer 2 — Inferenz (LLM-Advisor)](#32-layer-2--inferenz-llm-advisor)
   - 3.3 [Layer 3 — DB-Advisor Qualität](#33-layer-3--db-advisor-qualität)
   - 3.4 [Layer 4 — RAG-Kontext](#34-layer-4--rag-kontext)
   - 3.5 [Layer 5 — A/B-Test & Continuous Learning](#35-layer-5--ab-test--continuous-learning)
   - 3.6 [Layer 6 — Systemressourcen](#36-layer-6--systemressourcen)
4. [Messmittel-Übersicht](#4-messmittel-übersicht)
5. [Erwartungswert-Referenztabelle](#5-erwartungswert-referenztabelle)
6. [Schlussfolgerungsregeln (Decision Logic)](#6-schlussfolgerungsregeln-decision-logic)
7. [Offene Fragen zum Messprogramm](#7-offene-fragen-zum-messprogramm)

---

## 1. Was wir bauen — Projektbeschreibung

### 1.1 Das Problem

ThemisDB ist eine hybride Datenbank: Dokumente, Graphen, Vektoren und Zeitreihen
in einem System. Wer dieses System betreibt, muss heute selbst entscheiden:

- Welche HNSW-Parameter (`efSearch`, `M`, `efConstruction`) passen zu meiner
  aktuellen Anfragelast?
- Wann lohnt sich ein neues Index? Wann soll ich einen Index droppen?
- Welche RocksDB-Compaction-Strategie ist richtig für meinen Workload?
- Warum ist meine p99-Latenz gestiegen? Was soll ich tun?

Die heute eingesetzten regelbasierten Systeme (`HnswParameterTuner`,
`WorkloadAdaptiveOptimizer`, `BaoOptimizer`) geben korrekte Empfehlungen — aber
sie können nicht erklären, nicht dialogisch nachfragen, nicht mehrschrittige
Optimierungspläne ausformulieren.

Datenbankadministratoren verlieren Zeit, weil sie zwischen Prometheus-Dashboards,
Docs und dem Admin-UI hin- und herspringen müssen.

### 1.2 Die Lösung in einem Satz

> **Ein ThemisDB-spezialisiertes LLM, das Optimierungsempfehlungen in
> verständlicher Sprache erklärt, seine Rationale offenlegt und im Dialog
> verfeinert werden kann.**

Das LLM kennt ThemisDB von innen (durch LoRA-Training auf echten
Optimizer-Entscheidungen), weiß aber auch, was die aktuelle Instanz gerade macht
(durch RAG auf Live-Metriken).

### 1.3 Die zwei Schichten

```
┌─────────────────────────────────────────────────────────┐
│  SCHICHT A — STATISCHES WISSEN (LoRA-Adapter)           │
│                                                         │
│  "Was bedeutet ein hoher efSearch?"                     │
│  "Was ist eine gute M-Wahl für 768-dim Vektoren?"       │
│  "Welche Compaction-Strategie passt zu OLAP?"           │
│                                                         │
│  Quelle: historische EXPLAIN-Paare, BAO-Entscheidungen, │
│          HnswParameterTuner-Logs — gelabelt mit Δp99    │
│                                                         │
│  Update: monatlich / pro Release (kleiner Adapter-File) │
└─────────────────────────────────────────────────────────┘
                    +
┌─────────────────────────────────────────────────────────┐
│  SCHICHT B — DYNAMISCHES WISSEN (RAG bei Inferenz)      │
│                                                         │
│  "Die aktuelle p99 meiner Instanz ist 380 ms"           │
│  "Cache-Hit-Rate ist 61 % — unter dem Ziel von 80 %"   │
│  "Die letzten 20 BAO-Entscheidungen bevorzugten Plan 2" │
│                                                         │
│  Quelle: WorkloadAdaptiveOptimizer, HnswParameterTuner, │
│          BaoOptimizer, Prometheus-Scrape (live)         │
│                                                         │
│  Update: pro Anfrage (< 100 ms Kontext-Assemblierung)   │
└─────────────────────────────────────────────────────────┘
```

### 1.4 Was wir konkret implementieren

| Schritt | Komponente | Status |
|---|---|---|
| **1** | `DomainType::DATABASE_OPTIMIZER` in `LegalAutoLabeler` | 🔴 Offen |
| **2** | Confidence-Funktion: `Δp99 → [0.0, 1.0]` | 🔴 Offen |
| **3** | Domain-Keywords für `LoRADataSelectionConfig` | 🔴 Offen |
| **4** | Optimizer-Log-Export CLI (EXPLAIN-Paare, JSONL) | 🔴 Offen |
| **5** | Erstes Adapter-Training `themisdb-expert-v1` (Llama-3.1-8B + NF4 QLoRA) | 🔴 Offen |
| **6** | RAG-Kontext-Serialisierer (WorkloadProfile → JSON ≤ 2 000 Token) | 🔴 Offen |
| **7** | A/B-Deployment via `deployVersionEx(traffic_split=0.1)` | 🔴 Offen |
| **8** | DBA-Feedback-UI (👍/👎 → Trainingssignal) | 🔴 Offen |
| **9** | `ContinuousLearningOrchestrator` auf wöchentlichen Retrain-Zyklus konfigurieren | 🔴 Offen |

Alles, was in der Tabelle oben steht, baut auf **bereits produktionsreifem Code**
auf. Die Infrastruktur (`IncrementalLoRATrainer`, `AdaLoRAAdapter`,
`LoRADataSelectionPipeline`, `RAGIngestionBridge`, `ContinuousLearningOrchestrator`)
ist vorhanden. Wir schließen nur die Lücke im Domänen-Labeling und der
Kontext-Assemblierung.

### 1.5 Was wir explizit NICHT bauen

- Kein eigenes LLM von Grund auf — wir nutzen Llama-3.1-8B als Basismodell
- Keinen eigenen Prometheus-Stack — `PrometheusMetricsAdapter` ist bereits vorhanden
- Keine neue Trainingspipeline — `IncrementalLoRATrainer` ist production-ready
- Keine eigene Vektorsuche für RAG — `RAGIngestionBridge` ist bereits vorhanden

---

## 2. Architekturskizze

```
ADMIN-FRAGE:
  "Meine Vektorsuchlatenz ist seit gestern 40 % höher. Was ist los?"
         │
         ▼
  ┌─────────────────────────────────┐
  │  RAG CONTEXT ASSEMBLY (≤ 100ms) │
  │                                 │
  │  HnswParameterTuner             │
  │    → efSearch=128, M=16         │
  │    → recall_estimate=0.94       │
  │    → dataset_size=2.1M vectors  │
  │                                 │
  │  WorkloadAdaptiveOptimizer      │
  │    → WorkloadType=VECTOR        │
  │    → read_write_ratio=0.98      │
  │    → avg_complexity=3.2         │
  │                                 │
  │  BaoOptimizer (letzte 20)       │
  │    → plan_2 gewählt: 17/20 mal  │
  │    → Δlatency: +38ms median     │
  │                                 │
  │  Prometheus-Scrape              │
  │    → p99=380ms (war 270ms)      │
  │    → cache_hit_rate=61%         │
  │    → cpu_util=82%               │
  │                                 │
  │  → JSON-Block (≈ 1 800 Token)   │
  └────────────────┬────────────────┘
                   │
                   ▼
  ┌─────────────────────────────────┐
  │  LLM INFERENCE                  │
  │                                 │
  │  Llama-3.1-8B                   │
  │  + LoRA Adapter "themisdb-v2"   │
  │    (kennt: HNSW-Algorithmus,    │
  │     RocksDB-Compaction,         │
  │     AQL-Planoptimierung,        │
  │     ThemisDB-Anti-Patterns)     │
  └────────────────┬────────────────┘
                   │
                   ▼
  ANTWORT:
  "Der Anstieg erklärt sich durch den CPU-Spike auf 82 %: Bei
   efSearch=128 und 2.1M Vektoren werden pro Query ≈ 26 000
   Distanzberechnungen durchgeführt. Empfehlung: efSearch temporär
   auf 64 reduzieren (–10% Recall, –35% CPU) oder CPU-Limit erhöhen.
   Außerdem ist cache_hit_rate=61% unter dem Ziel von 80% — prüfe,
   ob der L2-Cache kürzlich verkleinert wurde."
```

---

## 3. Metrikkatalog

### 3.1 Layer 1 — Training (LoRA-Adapter)

Diese Metriken entstehen während des Trainings in `IncrementalLoRATrainer`
und werden über `LoRAMetricsCollector` nach Prometheus exportiert.

---

#### 3.1.1 Training Loss (pro Schritt / Epoche)

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_lora_training_loss` |
| **Typ** | Gauge |
| **Messmittel** | `LoRAMetricsCollector::updateTrainingLoss()` → Prometheus |
| **Quelle im Code** | `EpochMetrics::train_loss`, `TrainingMetrics::step_losses` |
| **Einheit** | dimensionslos (Cross-Entropy-Loss) |
| **Erwartungswert** | Startwert 2.0–3.0; nach 3 Epochen < 0.5 für einen sauberen Datensatz |
| **Warnschwelle** | Loss sinkt nach Epoche 1 nicht unter 1.5 → Datensatzqualität prüfen |
| **Fehlerschwelle** | Loss > 2.0 nach Epoche 3 → Training fehlgeschlagen; kein Adapter deployen |

**Schlussfolgerung:**  
Ein monoton sinkender Loss über alle Epochen bestätigt, dass das Modell
tatsächlich lernt. Ein Plateau deutet auf zu geringe Learning Rate oder
zu wenig Diversität im Datensatz hin. Loss-Anstieg nach Validierungs-Split
= Overfitting → `dropout` erhöhen oder Datensatz vergrößern.

---

#### 3.1.2 Validation Loss & Validation Accuracy

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_lora_validation_accuracy` |
| **Typ** | Gauge |
| **Messmittel** | `LoRAMetricsCollector::updateValidationAccuracy()` |
| **Quelle im Code** | `EpochMetrics::val_loss`, `EpochMetrics::accuracy` |
| **Einheit** | % (0.0–1.0) |
| **Erwartungswert** | ≥ 0.75 nach 3 Epochen (Ziel aus Evaluation Framework) |
| **Warnschwelle** | < 0.65 nach Epoche 2 |
| **Fehlerschwelle** | < 0.50 = kein besseres Ergebnis als Zufalls-Baseline |

**Schlussfolgerung:**  
Validation Accuracy ist die wichtigste Go/No-Go-Metrik für den Adapter-Deploy.
Ein Adapter mit Accuracy < 0.75 wird nicht in den A/B-Test übernommen.

---

#### 3.1.3 Training Samples Count

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_lora_training_samples_total` |
| **Typ** | Counter |
| **Messmittel** | `LoRAMetricsCollector::recordTrainingSamples()` |
| **Quelle im Code** | `TrainingResult::samples_trained` |
| **Einheit** | Anzahl |
| **Erwartungswert** | Initiales Training: ≥ 1 000 Samples; inkrementelles Update: ≥ 200 |
| **Warnschwelle** | < 500 Samples → Accuracy-Ziele möglicherweise nicht erreichbar |

**Schlussfolgerung:**  
Zu wenige Samples führen zuverlässig zu Overfitting (erkennbar an der
Schere zwischen Train Loss und Val Loss). Als Faustregel gilt:
min_confidence=0.7 produziert aus 3 000 Rohdaten ca. 900–1 200 saubere Samples.

---

#### 3.1.4 Adapter Load-Zeit

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_lora_adapter_load_duration_ms` |
| **Typ** | Histogram |
| **Messmittel** | `LoRAMetricsCollector::recordAdapterLoad()` |
| **Einheit** | Millisekunden |
| **Erwartungswert** | < 500 ms für einen NF4-quantisierten Adapter < 100 MB |
| **Warnschwelle** | > 2 000 ms → I/O-Problem oder Adapter zu groß |

**Schlussfolgerung:**  
Hohe Load-Zeiten verzögern Adapter-Switches im A/B-Test und erhöhen den
p99 beim ersten Request nach einem Adapter-Wechsel. Ziel: Hot-Swap < 200 ms
via Adapter-Cache (`recordCacheHit` / `recordCacheMiss`).

---

#### 3.1.5 AdaLoRA Layer Importance

| Attribut | Wert |
|---|---|
| **Metrikname** | (intern) `AdaLoRALayerStats::importance` |
| **Typ** | Gauge (pro Layer) |
| **Messmittel** | `AdaLoRAAdapter::reallocateRanks()` → Logs / JSON-Export |
| **Einheit** | ‖ΔW‖²_F (Frobenius-Norm des Gewichts-Deltas) |
| **Erwartungswert** | Attention-Layer: Importance > 0.6; FFN-Layer: < 0.3 (für DB-Optimizer-Domäne) |

**Schlussfolgerung:**  
Wenn FFN-Layer eine unerwartet hohe Importance erhalten, werden Syntax-Muster
überproportional gelernt statt Reasoning-Muster. → Rank-Budget zu FFN umlenken
oder Datensatz auf komplexere Optimierungsfälle erweitern.

---

### 3.2 Layer 2 — Inferenz (LLM-Advisor)

Gemessen durch `LLMMetricsCollector` (inkl. RAG- und Embedding-Operationen).

---

#### 3.2.1 Inferenz-Latenz (end-to-end)

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_llm_inference_duration_ms` |
| **Typ** | Histogram |
| **Messmittel** | `LLMMetricsCollector::recordInference()` |
| **Einheit** | Millisekunden |
| **Erwartungswert** | p50 < 800 ms · p99 < 3 000 ms (7 B Modell, NF4, 1× RTX 3090) |
| **Warnschwelle** | p99 > 5 000 ms → interaktiv nicht mehr akzeptabel |

**Schlussfolgerung:**  
Latenz > 3 s p99 deutet meist auf Token-Budget-Überschreitung oder I/O-Bottleneck
beim Modell-Load hin. Gegenmaßnahme: RAG-Kontext auf 2 000 Token begrenzen
(bereits als Ziel spezifiziert), Output-Länge auf 512 Token limitieren.

---

#### 3.2.2 Token-Durchsatz

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_llm_output_tokens_total` |
| **Typ** | Counter |
| **Messmittel** | `LLMMetricsCollector::recordInference(output_tokens=...)` |
| **Einheit** | Tokens/s (abgeleitet aus Latenz + Output-Tokens) |
| **Erwartungswert** | ≥ 25 Token/s auf RTX-Klasse GPU (NF4 Quantisierung) |
| **Warnschwelle** | < 10 Token/s → GPU-Auslastung oder VRAM-Overflow prüfen |

---

#### 3.2.3 Fehlerrate (Inferenz)

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_llm_inference_errors_total` |
| **Typ** | Counter (Label: `error_code`) |
| **Messmittel** | `LLMMetricsCollector::recordInference(success=false, error_code=...)` |
| **Erwartungswert** | < 0.5 % der Anfragen |
| **Fehlerschwelle** | > 2 % → Adapter-Rollback auslösen |

---

#### 3.2.4 AQL-Gültigkeitsrate

| Attribut | Wert |
|---|---|
| **Metrikname** | (abgeleitet) AQL-Fehler / gesamt generierte AQL-Anfragen |
| **Typ** | Gauge |
| **Messmittel** | AQL-Parser-Fehlerlog → `MetricsCollector::incrementCounter()` |
| **Erwartungswert** | ≥ 95 % gültige AQL-Anfragen |
| **Fehlerschwelle** | < 90 % → Adapter hat Syntax-Halluzinationen; kein Deploy |

**Schlussfolgerung:**  
AQL-Ungültigkeit ist ein direktes Signal für Halluzination im syntaktischen
Bereich. Unter 90 % ist der Adapter für produktiven Einsatz nicht geeignet,
selbst wenn Advisor Accuracy in Ordnung ist.

---

### 3.3 Layer 3 — DB-Advisor Qualität

Diese Metriken messen, ob die Empfehlungen des LLM-Advisors tatsächlich
die Datenbankperformance verbessern. Sie entstehen durch Messung vor und
nach der Ausführung einer Empfehlung (Sandbox-Replay oder Produktionsmessung).

---

#### 3.3.1 Advisor Accuracy (primär)

| Attribut | Wert |
|---|---|
| **Definition** | Anteil der Empfehlungen, die p99 um > 10 % verbessern (Sandbox-Replay) |
| **Messmittel** | Optimizer-Log + EXPLAIN-Vergleich → `MetricsCollector::setGauge()` |
| **Erwartungswert** | ≥ 75 % |
| **Warnschwelle** | 65–75 % → inkrementelles Retrain mit neueren Daten |
| **Fehlerschwelle** | < 65 % → Adapter nicht deployen / zurückrollen |

**Schlussfolgerung:**  
Advisor Accuracy < 75 % nach initialem Training deutet meist auf einen
der folgenden Fälle hin:
1. Zu wenig Trainingsdaten (< 800 saubere Samples)
2. Confidence-Schwelle zu niedrig gesetzt (noise-Samples im Training)
3. Domäne zu heterogen → getrennte Adapter pro Workload-Typ

---

#### 3.3.2 Δp99 Latenz (pro Empfehlung)

| Attribut | Wert |
|---|---|
| **Definition** | Prozentualer Rückgang der p99-Anfrage-Latenz nach Umsetzung der Empfehlung |
| **Messmittel** | `HnswParameterTuner` + Prometheus p99-Messung (Vorher/Nachher) |
| **Einheit** | % |
| **Erwartungswert** | Median-Δp99 ≥ +15 % bei VECTOR-Workload; ≥ +10 % bei OLTP |
| **Warnschwelle** | Δp99 < +5 % bei > 30 % der Empfehlungen → Confidence-Funktion anpassen |
| **Fehlerschwelle** | Δp99 < 0 (Verschlechterung) bei > 5 % der Empfehlungen → Rollback |

**Schlussfolgerung:**  
Δp99 ist die Leitmetrik für den Produktionswert des Advisors. Sie ist auch
die Grundlage für das Confidence-Labeling neuer Trainingssamples:

```
Δp99 > 30 %  →  confidence = 0.9
Δp99 5–30 %  →  confidence = 0.7
Δp99 < 5 %   →  confidence = 0.5 (human review flag)
Δp99 < 0 %   →  rejected
```

---

#### 3.3.3 Halluzinationsrate

| Attribut | Wert |
|---|---|
| **Definition** | Anteil der Antworten, die nicht-existente Index-Typen, Parameter oder Funktionen nennen |
| **Messmittel** | Manuelle Stichprobe (initial) + automatisierter Schema-Checker (mittelfristig) |
| **Erwartungswert** | < 3 % |
| **Fehlerschwelle** | > 5 % → Adapter ist nicht production-fähig |

**Schlussfolgerung:**  
Halluzinationen im DB-Advisor-Kontext sind gefährlicher als in allgemeinen
LLMs, weil sie zu falschen DDL-Ausführungen führen können (z. B. DROP INDEX
auf falschem Zielobjekt). Gegenmaßnahme: Tool-Call-Validierungsschicht
(DDL ohne explizite Human-Confirmation blockieren).

---

#### 3.3.4 DBA-Akzeptanzrate (Human Feedback)

| Attribut | Wert |
|---|---|
| **Definition** | Anteil der Empfehlungen, die ein DBA mit 👍 bewertet (vs. 👎 oder ignoriert) |
| **Messmittel** | Feedback-UI → `ContinuousLearningOrchestrator` Feedback-Schleife |
| **Erwartungswert** | ≥ 60 % nach initialem Deployment; ≥ 75 % nach 2. Retrain-Zyklus |
| **Warnschwelle** | < 40 % → schwerwiegendes Qualitätsproblem im Adapter |

**Schlussfolgerung:**  
DBA-Feedback ist der wichtigste Langzeit-Qualitätsindikator und gleichzeitig
Trainingsignal für den nächsten Retrain-Zyklus (RLHF-ähnlich, ohne
explizites Reward-Modell). Ein niedriger Akzeptanzwert bei gleichzeitig hoher
Advisor Accuracy zeigt eine Lücke zwischen technisch-korrekter und
praktisch-umsetzbarer Empfehlung.

---

### 3.4 Layer 4 — RAG-Kontext

Gemessen durch `LLMMetricsCollector::recordRAG()` und `RAGIngestionBridge`.

---

#### 3.4.1 RAG-Latenz (Kontext-Assemblierung)

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_llm_rag_duration_ms` |
| **Typ** | Histogram |
| **Messmittel** | `LLMMetricsCollector::recordRAG()` |
| **Erwartungswert** | p99 < 100 ms (Ziel: kein wahrnehmbarer Overhead für den User) |
| **Warnschwelle** | p99 > 250 ms → Kontext-Assemblierung parallelisieren oder cachen |

**Schlussfolgerung:**  
Wenn RAG-Latenz > 100 ms p99 ist, ist das Bottleneck meist der
Prometheus-Scrape oder der Workload-Profiler. Lösung: Live-Metriken
in ein kleines In-Memory-Cache schreiben, das alle 5 s aktualisiert wird.

---

#### 3.4.2 Cache-Hit-Rate (Prefix / Response Cache)

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_llm_cache_hits_total` / `themis_llm_cache_misses_total` |
| **Typ** | Counter |
| **Messmittel** | `LLMMetricsCollector::recordCacheAccess()` |
| **Erwartungswert** | ≥ 30 % bei wiederholten ähnlichen Fragen (Prefix-Cache) |

**Schlussfolgerung:**  
Ein Prefix-Cache auf RAG-Kontext-Blöcken reduziert die Inferenz-Latenz
drastisch für gleichartige Fragen (z. B. "p99 hoch — was tun?" kommt
mehrfach täglich). Cache-Hit-Rate < 10 % bedeutet, dass Anfragen zu
heterogen sind oder der Cache-Key nicht granular genug ist.

---

#### 3.4.3 Retrieved Documents Count

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_llm_rag_retrieved_docs` |
| **Typ** | Histogram |
| **Messmittel** | `LLMMetricsCollector::recordRAG(retrieved_docs=...)` |
| **Erwartungswert** | 3–8 Dokumente pro Anfrage (Optimizer-Log-Snapshots) |
| **Warnschwelle** | > 15 Dokumente → Token-Budget-Überschreitung wahrscheinlich |

---

### 3.5 Layer 5 — A/B-Test & Continuous Learning

Gemessen durch `ContinuousLearningOrchestrator` und den internen A/B-Testing-Framework.

---

#### 3.5.1 Accuracy Drop (Retrain-Trigger)

| Attribut | Wert |
|---|---|
| **Definition** | Relativer Rückgang der Advisor Accuracy gegenüber Baseline-Messung |
| **Messmittel** | `ContinuousLearningOrchestrator` intern, Schwellenwert: `min_accuracy_drop` |
| **Konfigurierter Wert** | 5 % Drop → Retrain auslösen (`min_accuracy_drop = 0.05`) |
| **Erwartungswert** | Ohne Workload-Shift: < 2 % Drift pro Woche |
| **Warnschwelle** | > 10 % Drop innerhalb 48 h → ungeplanter Workload-Shift; sofortiger Retrain |

**Schlussfolgerung:**  
Schneller Accuracy-Drop (> 10 % in 48 h) ist ein Indikator für einen
strukturellen Workload-Shift (z. B. neue Applikation auf der DB, Daten-
Migration) und nicht nur für normales Konzept-Drift. In diesem Fall ist
ein inkrementelles Retrain nicht ausreichend — ein vollständiger Adapter-
Rebuild mit neuen Daten ist nötig.

---

#### 3.5.2 A/B-Traffic-Split & Promotion-Kriterien

| Attribut | Wert |
|---|---|
| **Konfiguration** | `ab_test_traffic_split = 0.1` (10 % Treatment, 90 % Control) |
| **Min. Samples** | `min_ab_samples = 500` Queries vor Promotionsentscheidung |
| **Promotion-Schwelle** | `min_improvement_threshold = 0.02` (2 % Advisor-Accuracy-Verbesserung) |
| **Rollback** | Automatisch bei > 5 % Advisor-Accuracy-Regression (`enable_auto_rollback = true`) |

**Schlussfolgerung:**  
Der Promotionsprozess ist statistisch gesichert durch Minimum Sample Size.
Bei zu kleinem Traffic-Split dauert die Promotion-Entscheidung länger, aber
das Risiko einer schlechten Empfehlung für viele User sinkt. 10 % ist ein
guter Kompromiss bei erwartetem DBA-Traffic von 50–200 Anfragen/Tag.

---

#### 3.5.3 Adapter-Version-Verteilung

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_lora_active_adapters` |
| **Typ** | Gauge |
| **Messmittel** | `LoRAMetricsCollector::updateActiveAdapters()` |
| **Erwartungswert** | Im A/B-Test: genau 2 aktive Adapter (Control + Treatment) |
| **Fehlerschwelle** | > 3 aktive Adapter → Lifecycle-Problem; Aufräumen nötig |

---

### 3.6 Layer 6 — Systemressourcen

---

#### 3.6.1 GPU VRAM-Verbrauch

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_lora_vram_usage_bytes` |
| **Typ** | Gauge |
| **Messmittel** | `llm::lora::GpuUtilizationMonitor` → Prometheus |
| **Einheit** | Bytes |
| **Erwartungswert** | Llama-3.1-8B + NF4: ≈ 6.5 GB VRAM; mit Adapter: + 80 MB |
| **Warnschwelle** | > 22 GB auf RTX 3090 (24 GB) → OOM-Risiko bei gleichzeitigem Training |

**Schlussfolgerung:**  
Training und Inferenz auf derselben GPU gleichzeitig ist bei 8 B NF4 auf 24 GB
möglich, aber eng. In Produktion Training auf separater GPU oder zu Off-Peak-Zeiten
planen.

---

#### 3.6.2 CPU-Auslastung (HNSW-Distanzberechnung)

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_hnsw_cpu_utilization` (via WorkloadAdaptiveOptimizer) |
| **Typ** | Gauge |
| **Messmittel** | `WorkloadAdaptiveOptimizer::record_query()` → Profiler |
| **Erwartungswert** | < 70 % CPU auf VECTOR-Workload bei `efSearch=64` |
| **Warnschwelle** | > 80 % → `efSearch` temporär reduzieren; LLM-Advisor informieren |

---

#### 3.6.3 Cache-Hit-Rate (DB-Level)

| Attribut | Wert |
|---|---|
| **Metrikname** | `themis_cache_hit_rate` (L1+L2+L3 aus `CacheMetrics`) |
| **Typ** | Gauge |
| **Messmittel** | `CacheMetrics::l1_hits / (l1_hits + l2_hits + l3_hits + misses)` |
| **Erwartungswert** | ≥ 80 % für OLTP-Workload |
| **Warnschwelle** | < 60 % → Cache-Sizing oder Eviction-Strategie prüfen |

**Schlussfolgerung:**  
Die DB-Level-Cache-Hit-Rate ist eine der stärksten Einzelprädiktoren für
p99-Latenz. Sie ist gleichzeitig das wichtigste Feature im RAG-Kontext-Block
für den LLM-Advisor, weil viele Optimierungsfragen indirekt damit zusammenhängen.

---

## 4. Messmittel-Übersicht

| Messmittel | Wo im Code | Was wird damit gemessen |
|---|---|---|
| **LoRAMetricsCollector** | `include/llm/lora_framework/lora_metrics.h` | Training Loss, Adapter Load/Switch, Cache, VRAM |
| **LLMMetricsCollector** | `include/aql/llm_metrics_collector.h` | Inferenz-Latenz, RAG-Latenz, Token-Throughput, Fehlerrate |
| **MetricsCollector** | `include/observability/metrics_collector.h` | Alle Subsystem-Metriken, Prometheus-Export via `/metrics` |
| **AdvancedMetrics** | `include/observability/advanced_metrics.h` | Quantile (p50/p95/p99), Exponential-Histogramm, Rate |
| **WorkloadAdaptiveOptimizer** | `include/performance/workload_adaptive_optimizer.h` | WorkloadType, read_write_ratio, query_complexity |
| **HnswParameterTuner** | `include/index/hnsw_parameter_tuner.h` | efSearch, recall_estimate, dataset_size |
| **BaoOptimizer** | `include/performance/phase3/bao.h` | plan_chosen, predicted_latency, actual_latency |
| **ContinuousLearningOrchestrator** | `include/rag/continuous_learning_orchestrator.h` | Accuracy-Drop, Retrain-Trigger, A/B-Ergebnis |
| **PrometheusMetricsAdapter** | `include/core/concerns/prometheus_metrics_adapter.h` | Brücke von IMetrics → Prometheus (scrapebare Endpunkte) |
| **CacheMetrics** | `include/cache/cache_metrics.h` | L1/L2/L3 Hit Rate, Evictions, Kompressionsrate |
| **GrafanaMetrics** | `include/llm/grafana_metrics.h` | Dashboard-Integration (Grafana-Panels) |

---

## 5. Erwartungswert-Referenztabelle

| Metrik | Ziel (Gut) | Warnung | Fehler / Rollback |
|---|---|---|---|
| Training Loss (nach 3 Epochen) | < 0.5 | 0.5–1.5 | > 2.0 |
| Validation Accuracy | ≥ 0.75 | 0.65–0.75 | < 0.65 |
| AQL-Gültigkeitsrate | ≥ 95 % | 90–95 % | < 90 % |
| Halluzinationsrate | < 3 % | 3–5 % | > 5 % |
| Advisor Accuracy (Sandbox) | ≥ 75 % | 65–75 % | < 65 % |
| Δp99 Latenz (Median) | ≥ +15 % | +5–15 % | < 0 % (Regression) |
| DBA-Akzeptanzrate | ≥ 60 % | 40–60 % | < 40 % |
| Inferenz-Latenz p99 | < 3 000 ms | 3–5 000 ms | > 5 000 ms |
| RAG-Latenz p99 | < 100 ms | 100–250 ms | > 250 ms |
| Token-Throughput | ≥ 25 Token/s | 10–25 Token/s | < 10 Token/s |
| VRAM-Verbrauch (8B NF4) | < 8 GB | 8–20 GB | > 22 GB |
| CPU-Auslastung (HNSW) | < 70 % | 70–80 % | > 80 % |
| DB Cache-Hit-Rate | ≥ 80 % | 60–80 % | < 60 % |
| Accuracy-Drop (Woche) | < 2 % | 2–5 % | > 10 % (48 h) |
| Fehlerrate (Inferenz) | < 0.5 % | 0.5–2 % | > 2 % |

---

## 6. Schlussfolgerungsregeln (Decision Logic)

Diese Regeln werden von `ContinuousLearningOrchestrator` ausgewertet und
können als Grundlage für automatisierte Alerts (Prometheus Alertmanager)
dienen.

```
REGEL 1 — Adapter-Deploy-Gate
  IF (validation_accuracy < 0.75)
  OR (aql_validity_rate < 0.90)
  OR (hallucination_rate > 0.05)
  THEN  kein Deploy → manuellen Review auslösen

REGEL 2 — Retrain-Trigger
  IF (advisor_accuracy_drop > 0.05)   // gegenüber letzter Baseline
  OR (weekly_accuracy_drop > 0.10)
  THEN  incremental retrain starten
        IF (weekly_accuracy_drop > 0.10 IN 48h)
        THEN  full adapter rebuild (nicht inkrementell)

REGEL 3 — A/B-Promotion
  IF (ab_samples >= 500)
  AND (treatment_accuracy > control_accuracy + 0.02)
  AND (treatment_regression_rate < 0.05)
  THEN  Traffic-Split auf 100 % Treatment erhöhen
        alten Adapter archivieren

REGEL 4 — Rollback
  IF (treatment_advisor_accuracy < control_accuracy - 0.05)
  OR (treatment_p99_regression > 0.05)
  THEN  deployVersionEx(control_version, traffic_split=1.0)
        alert("Adapter rollback triggered")

REGEL 5 — HNSW Notfall-Derating
  IF (cpu_utilization > 0.80) AND (workload_type == VECTOR)
  THEN  HnswParameterTuner: efSearch auf 50 % des aktuellen Werts reduzieren
        LLM-Advisor: Kontext-Block "cpu_emergency=true" hinzufügen
        alert("efSearch emergency reduction")

REGEL 6 — Confidence-Labeling
  IF (delta_p99 > 0.30)  THEN  confidence = 0.9
  IF (delta_p99 IN [0.05, 0.30])  THEN  confidence = 0.7
  IF (delta_p99 < 0.05)  THEN  confidence = 0.5, flag_human_review = true
  IF (delta_p99 < 0.00)  THEN  reject_sample = true
```

---

## 7. Offene Fragen zum Messprogramm

1. **Halluzinations-Automatisierung**: Aktuell ist die Halluzinationsrate nur
   durch manuelle Stichproben messbar. Wir brauchen einen automatisierten
   Schema-Checker, der generierte Antworten gegen den ThemisDB-API-Contract
   validiert. → Offenes Implementierungsthema.

2. **Baseline-Definition**: Advisor Accuracy wird gegen den regelbasierten
   `HnswParameterTuner` gemessen. Dieser ist selbst nicht immer optimal —
   ein schlechter Baseline-Advisor kann eine übertrieben hohe Adapter-Accuracy
   vortäuschen. → Benchmark-Datensatz aus menschlich-validierten Entscheidungen
   erstellen.

3. **Sandbox vs. Produktion**: Der Sandbox-Replay für Δp99 kann von der
   Produktionsrealität abweichen (andere Last, anderes Hardware-Setup).
   Wie groß ist die Abweichung? → A/B-Test-Ergebnisse als Kalibrierung
   des Sandbox-Fehlers nutzen.

4. **Langzeitdrift**: Wie schnell verliert ein Adapter seine Gültigkeit?
   Die erste Messung des Accuracy-Drop über Zeit liefert den Retrain-Rhythmus.
   Hypothese: HNSW-Algorithmus-Wissen ist stabil (6–12 Monate), Query-Plan-
   Wissen verfällt schneller (4–8 Wochen bei aktivem Schema-Wachstum).

5. **Multi-Workload-Disaggregation**: Die Advisor Accuracy als Gesamtzahl
   verbirgt möglicherweise große Unterschiede zwischen Workload-Typen
   (OLTP / OLAP / VECTOR / GRAPH). Ziel: Accuracy-Metrik pro `WorkloadType`
   aufsplitten.

---

*ThemisDB Engineering Team · 2026-04-16 · Apache-2.0*

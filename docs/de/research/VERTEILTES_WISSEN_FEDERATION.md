[docs](../../README.md) > [de](../INDEX.md) > [research](./README.md)

---
Datum: 2026-04-17
Status: draft
Primary (Quelle der Wahrheit): include/distributed_knowledge/lora_federation_coordinator.h, include/distributed_knowledge/federated_rag_merger.h, include/distributed_knowledge/cross_shard_feedback_sync.h, include/distributed_knowledge/adapter_capability_announcement.h, include/sharding/gossip_protocol.h, include/importers/federated_learning.h, include/training/incremental_lora_trainer.h, include/rag/rlaif_trainer.h
Bezug / Reference: McMahan et al. (2017) FedAvg AISTATS · Dwork & Roth (2014) DP Foundations · Cormack et al. (2009) RRF SIGIR · Bai et al. (2022) Constitutional AI arXiv:2212.08073 · Lee et al. (2023) RLAIF arXiv:2309.00267 · Li et al. (2020) FedProx MLSys
---

# Verteiltes Wissen — RAID-Sharding der ThemisDB-Intelligenz

**Forschungsdokument — ThemisDB-Projekt**  
*Version 1.0 · 2026-04-17 · Apache-2.0*

---

## Inhaltsverzeichnis

- [0. Ausgangslage & Problemstellung](#0-ausgangslage--problemstellung)
- [1. Die RAID-Analogie auf Wissensebene](#1-die-raid-analogie-auf-wissensebene)
- [2. Vorhandene Bausteine & Verbindungslücken](#2-vorhandene-bausteine--verbindungslücken)
- [3. Ebene A — Gossip-basierte Adapter-Discovery](#3-ebene-a--gossip-basierte-adapter-discovery)
- [4. Ebene B — Federated LoRA Gradient Aggregation (RAID-5-Kern)](#4-ebene-b--federated-lora-gradient-aggregation-raid-5-kern)
- [5. Ebene C — Cross-Shard RAG Federation](#5-ebene-c--cross-shard-rag-federation)
- [6. Ebene D — Federated RLAIF (verteiltes DBA-Feedback)](#6-ebene-d--federated-rlaif-verteiltes-dba-feedback)
- [7. Sicherheit & Privacy-Garantien](#7-sicherheit--privacy-garantien)
- [8. Verbindung zu den Optimierungsebenen 5–10](#8-verbindung-zu-den-optimierungsebenen-510)
- [9. Gesamtarchitektur-Diagramm](#9-gesamtarchitektur-diagramm)
- [10. Implementierungsreihenfolge nach ROI](#10-implementierungsreihenfolge-nach-roi)
- [11. Offene Forschungsfragen](#11-offene-forschungsfragen)
- [12. Laufzeit-Einflussmechanismen: Schalter · Hebel · Optimierer](#12-laufzeit-einflussmechanismen-schalter--hebel--optimierer)
- [13. Referenzen](#13-referenzen)

---

## 0. Ausgangslage & Problemstellung

ThemisDB hat RAID-Sharding für **Daten** vollständig implementiert:
`ConsistentHash`, `RaftShardManager`, `QueryFederation`, `CrossShardTransaction`.
Jeder Shard verwaltet seinen Datenbestand redundant, verteilt und fehlertolerant.

Die Lernschichten — `LoRA-Adapter`, `RLAIF`, `ContinuousLearningOrchestrator` — sind
jedoch noch **shard-lokal**: Jeder Shard trainiert allein, kennt die Erkenntnisse der
anderen nicht.

**Das Problem:** Shard 7 erkennt ein neues Angriffsmuster. Shard 12 entwickelt eine
bessere Schema-Komprimierungsstrategie. Shard 3 erhält wertvolles DBA-Feedback zu
Denormalisierungs-Fehlern. Keiner der anderen Shards profitiert davon.

**Die Lösung:** RAID-Sharding der Intelligenz — ein föderiertes Lernsystem, das
Optimierungseinsichten zwischen Shards propagiert, **ohne Rohdaten die
Shardgrenzen überschreiten zu lassen**.

---

## 1. Die RAID-Analogie auf Wissensebene

Klassisches RAID schützt Daten durch Redundanz und Paritätsinformationen.
Dieselbe Logik lässt sich auf *Lernzustände* anwenden:

| RAID-Level | Datensharding (implementiert) | Wissenssharding (neu) |
|---|---|---|
| **RAID-0** | Striping ohne Redundanz | Jeder Shard trainiert isoliert — kein Transfer |
| **RAID-1** | Vollreplizierung | Volle LoRA-Adapter-Synchronisation aller Shards → prohibitiv teuer |
| **RAID-5** | Striping + verteilte Parität | **FedAvg** — Gradienten-Aggregation mit Differential Privacy |
| **RAID-6** | Doppelte Parität | Hierarchische Aggregation: Shard → Region → Global |
| **RAID-10** | Mirror + Stripe | Spezialisierte + geteilte Adapter kombiniert |

**Zielstrategie: RAID-5 für Wissen** — FedAvg-basierte LoRA-Adapter-Federation,
bei der kein Shard Rohdaten der anderen sieht. Die "Parität" ist der
DP-geschützte globale Gradientenvektor.

### Informationstheoretische Begründung

Das Funktionieren von RAID-5 beruht auf der Eigenschaft, dass Paritätsinformation
*abgeleitet* werden kann, ohne die Originaldaten aller Teilnehmer zu kennen.
Analog gilt für FedAvg (McMahan et al. 2017):

```
θ_global = Σ_k (n_k / N) · θ_k
```

wobei `n_k` die Stichprobengröße und `θ_k` die lokalen Modellgewichte von Shard `k`
sind. Die globale Summe ist eine hinreichende Statistik für den Gradientenraum —
ähnlich dem XOR-Paritätsblock in RAID-5.

Mit Differential Privacy (Dwork & Roth 2014, Gaussian-Mechanismus):

```
σ = Δf · √(2 · ln(1.25/δ)) / ε
```

garantiert der Aggregationsschritt (ε, δ)-DP: Kein Beobachter kann aus dem
globalen Delta den Trainingsinhalt eines einzelnen Shards rekonstruieren.

---

## 2. Vorhandene Bausteine & Verbindungslücken

### 2.1 Datenschicht (fertig)

| Komponente | Datei | Zweck |
|---|---|---|
| `GossipProtocol` | `include/sharding/gossip_protocol.h` | Cluster-Membership, Heartbeat-Propagation |
| `AdaptiveShardRouter` | `include/sharding/adaptive_shard_router.h` | Capability-based Query-Routing |
| `QueryFederation` | `include/query/query_federation.h` | Fan-out + Merge für AQL-Queries |
| `CrossShardTransaction` | `include/sharding/cross_shard_transaction.h` | 2PC über Shard-Grenzen |

### 2.2 Lernschicht (fertig, aber shard-lokal)

| Komponente | Datei | Zweck |
|---|---|---|
| `ContinuousLearningOrchestrator` | `include/rag/continuous_learning_orchestrator.h` | Loop-4-Koordination |
| `IncrementalLoRATrainer` | `include/training/incremental_lora_trainer.h` | LoRA-Training + Versioning |
| `FederatedAggregator` | `include/importers/federated_learning.h` | FedAvg/FedProx/Median **← KEY** |
| `DifferentialPrivacyManager` | `include/importers/federated_learning.h` | DP-Rauschen mit ε/δ-Budget |
| `RLAIFTrainer` | `include/rag/rlaif_trainer.h` | Constitutional AI + RLAIF |
| `FeedbackCollector` | `include/prompt_engineering/feedback_collector.h` | DBA-Feedback-Erfassung |
| `RAGIngestionBridge` | `include/rag/rag_ingestion_bridge.h` | Dokumentenindexierung + Entity-Extraktion |
| `AdapterRegistry` | `include/llm/adapter_registry.h` | Adapter-Versioning + A/B-Routing |

### 2.3 Verbindungslücken (neu zu schließen)

| Neu benötigt | Basis | Neue Datei |
|---|---|---|
| `AdapterCapabilityAnnouncement` | `GossipProtocol` | `include/distributed_knowledge/adapter_capability_announcement.h` |
| `GossipAdapterPublisher` | `GossipProtocol` | (same) |
| `ILoRAFederationCoordinator` | `FederatedAggregator` + `IncrementalLoRATrainer` | `include/distributed_knowledge/lora_federation_coordinator.h` |
| `LoRAFederationCoordinator` | (same) | `src/distributed_knowledge/lora_federation_coordinator.cpp` |
| `FederatedRAGMerger` | `QueryFederation` + `RAGIngestionBridge` | `include/distributed_knowledge/federated_rag_merger.h` |
| `CrossShardFeedbackSync` | `FeedbackCollector` + `GossipProtocol` | `include/distributed_knowledge/cross_shard_feedback_sync.h` |

---

## 3. Ebene A — Gossip-basierte Adapter-Discovery

### 3.1 Motivation

Ohne Discovery weiß `AdaptiveShardRouter` nicht, welcher Shard den
domänenspezialisiertesten LoRA-Adapter hat. Queries werden naiv verteilt.

Mit Discovery: Shard 7 hat `domain_type = SECURITY_MONITOR` und
`performance_delta_p99_ms = -8.4 ms`. Security-relevante Queries werden
bevorzugt dorthin geroutet — kein Training nötig, sofortiger Gewinn.

### 3.2 Datenstruktur: AdapterCapabilityAnnouncement

```cpp
struct AdapterCapabilityAnnouncement {
    std::string       shard_id;
    std::string       adapter_version;      // "v1.3.0"
    AdapterDomainType domain_type;          // SECURITY_MONITOR, SCHEMA_ADVISOR, ...
    double            performance_delta_p99_ms; // -8.4 = 8.4ms schneller
    double            accuracy_delta;       // +0.03 = 3% besser
    size_t            training_samples;     // 14200
    uint64_t          federation_round;     // letzte föderierte Runde
};
```

### 3.3 Transport

Die Ankündigung wird als JSON in `GossipMessage::payload` eingebettet
(`message_type = "adapter_capability"`). Kein Protokoll-Umbau nötig.

```
GossipProtocol::sendHeartbeat()
   ↓ payload += AdapterCapabilityAnnouncement.toJson()
AdaptiveShardRouter::onPeerDiscovered()
   ↓ updateCapabilityScore(shard_id, announcement)
Query mit domain_hint="SECURITY_MONITOR"
   ↓ AdaptiveShardRouter wählt Shard mit höchstem domain_score
```

### 3.4 Sicherheit

`GossipProtocol::verifyMessage()` prüft HMAC-Signatur vor der Verarbeitung.
`ZeroTrustPolicyEnforcer::evaluateRequest()` verifiziert die Shard-Identität
via mTLS-Zertifikat.

---

## 4. Ebene B — Federated LoRA Gradient Aggregation (RAID-5-Kern)

### 4.1 Mathematisches Fundament

**FedAvg** (McMahan et al. 2017):

Jeder Shard `k` führt `E` lokale Trainings-Epochen durch und exportiert
seinen Gewichtsvektor `w_k`:

```
Globaler Update:
  w_t+1 = Σ_k (n_k / N) · w_k^t

Differential Privacy (Gaussian-Mechanismus):
  w̃_t+1 = w_t+1 + N(0, σ²I)
  σ = Δ_f · √(2·ln(1.25/δ)) / ε
```

**FedProx** (Li et al. 2020) für heterogene Workloads — minimiert:

```
min_w  F_k(w) + (μ/2)·‖w - w_t‖²
```

Der Proximal-Term `μ/2·‖w - w_t‖²` begrenzt die Drift lokaler Modelle bei
heterogenen Shard-Verteilungen — relevant wenn Shard 1 primär
Transaktions-Workloads hat und Shard 7 Security-Workloads.

### 4.2 Ablauf pro Federation-Runde

```
┌─────────────────────────────────────────────────────────────────┐
│  Federated LoRA Round (alle 24h oder manuell getriggert)        │
│                                                                 │
│  Shard 1  ─┐                                                    │
│  Shard 2  ─┤─ EncryptedGradient(shard_k, round, data) ──────►  │
│  Shard N  ─┘                                                    │
│                    LoRAFederationCoordinator                     │
│                    .submitGradient(gradient_k)                  │
│                    ↓  (nach min_participants Submissions)        │
│                    FedAvg/FedProx Aggregation                   │
│                    ↓                                            │
│                    DifferentialPrivacyManager(ε=0.1, δ=1e-5)    │
│                    ↓                                            │
│                    GlobalAdapterDelta("global-v42")             │
│                    ↓  (broadcast via GossipProtocol)            │
│  Shard 1  ◄─── applyGlobalDelta(delta)                         │
│  Shard N  ◄─── applyGlobalDelta(delta)                         │
└─────────────────────────────────────────────────────────────────┘
```

### 4.3 Privacy-Budget-Management

Das DP-Budget wird akkumuliert: nach `T` Runden ist `ε_total = T · ε_round`.
Die `DifferentialPrivacyManager::verifyPrivacyBudget()` Funktion überwacht
das Budget. Bei Überschreitung wird eine Runde übersprungen.

**Empfohlene Konfiguration:**
- `ε_round = 0.1`, `δ = 1e-5` pro Runde
- `T_max = 50` Runden vor Budget-Reset → `ε_total = 5.0` (akzeptabel für
  viele praktische Anwendungen gemäß Dwork & Roth §3.5)

### 4.4 Konfiguration

```json
{
  "federation": {
    "min_participants": 2,
    "aggregation_algorithm": "FedAvg",
    "weight_by_sample_count": true,
    "dp_epsilon": 0.1,
    "dp_delta": 1e-5,
    "dp_sensitivity": 1.0,
    "federation_interval_hours": 24,
    "round_timeout_minutes": 60
  }
}
```

---

## 5. Ebene C — Cross-Shard RAG Federation

### 5.1 Motivation

Aktuell wertet `RAGIngestionBridge` nur den lokalen Shard aus. Eine Frage wie
*"Was sind die häufigsten Transaktionsfehler?"* erhält nur die Sicht von
Shard 3 — obwohl Shard 7 dazu die wertvollsten Dokumente hat.

Mit Federation: `QueryFederation::fanOut()` ruft `RAGIngestionBridge` auf
**allen** Shards parallel auf; `FederatedRAGMerger` kombiniert die Ergebnisse
zu einem globalen Kontext.

### 5.2 Reciprocal Rank Fusion

RRF (Cormack et al. 2009) ist die Standardmethode für Multi-Source Re-Ranking:

```
score(d) = Σ_{q ∈ Shards}  1 / (k + rank(d, q))
```

Vorteile:
- Keine Kalibrierung der Shard-Scores nötig (unabhängige Retrieval-Systeme)
- Robust gegenüber Ausreißern (logarithmische Dämpfung hoher Ränge)
- Deterministische Tie-Breaking durch Shard-ID

**Spezialisierungs-Boost:** Wenn `adapter_accuracy_delta > 0` (Shard hat einen
domänenspezialisierten Adapter), wird der RRF-Score mit `specialisation_boost`
(Default: 1.2) multipliziert — Dokumente vom besten Adapter dominieren die
Zusammenführung.

### 5.3 Ablauf

```
AQL-Query: "Häufigste Transaktionsfehler"
    ↓
QueryFederation::fanOut(queryPlan)  [parallel]
    ↓
    Shard 1: RAGIngestionBridge::enrichRetrievedDocuments(docs_1)
    Shard 2: RAGIngestionBridge::enrichRetrievedDocuments(docs_2)
    Shard N: RAGIngestionBridge::enrichRetrievedDocuments(docs_N)
    ↓
FederatedRAGMerger::merge(shard_results)
    [MergeStrategy::RECIPROCAL_RANK_FUSION, top_k=20, deduplicate=true]
    ↓
MergedRAGContext::buildPromptContext(max_docs=10, max_chars=4000)
    ↓
LLM-Prompt: "[Shard: shard-7] ... [Shard: shard-3] ..."
```

### 5.4 Merge-Strategien im Vergleich

| Strategie | Stärke | Schwäche | Empfehlung |
|---|---|---|---|
| `RECIPROCAL_RANK_FUSION` | Robust, keine Score-Kalibrierung | Ignoriert absolute Scores | **Default** |
| `SCORE_WEIGHTED` | Nutzt adapter_accuracy_delta | Setzt kalibrierte Scores voraus | Bei homogenen Shards |
| `ROUND_ROBIN` | Maximale Diversität | Keine Relevanz-Sortierung | Explorative Queries |

---

## 6. Ebene D — Federated RLAIF (verteiltes DBA-Feedback)

### 6.1 Motivation

Ein DBA auf Shard 3 lehnt eine Denormalisierungsempfehlung ab. Diese
Erfahrung bleibt bisher auf Shard 3 beschränkt. Alle anderen Shards werden
dieselbe fehlerhafte Empfehlung weiter geben.

Mit Federated RLAIF: Das Feedback wird anonymisiert propagiert; alle Shards
lernen aus dem lokalen DBA-Wissen — **ohne Datenschutz-Kompromiss**.

### 6.2 Datenschutz durch Embedding-Propagation

```
DBA gibt Feedback zu Query "ALTER TABLE orders ADD COLUMN..."
    ↓
FeedbackCollector::recordFeedback(entry)
    ↓
Embedding-Modell: reason_embedding = embed(entry.query)
    [nur der Embedding-Vektor, kein Klartext]
    ↓
CrossShardFeedbackSync::publishFeedback({
    feedback_type_label: "USER_NEGATIVE",
    reason_embedding: [0.12, -0.34, ...],  // 384-dim, kein Klartext
    shard_origin: "ANON"
})
    ↓
GossipProtocol propagiert FeedbackSummary
    ↓
Empfangende Shards: RLAIFTrainer::addPreferencePair(
    buildPairFromSummary(summary)
)
    ↓
Nächste IncrementalLoRATrainer-Runde enthält globales DBA-Wissen
```

### 6.3 Deduplication & Idempotenz

Gossip-Echos können dieselbe `FeedbackSummary` mehrfach liefern.
`CrossShardFeedbackSync` hält einen LRU-Cache der zuletzt gesehenen
`summary_id`-Hashes (Default: 10.000 Einträge). Duplikate werden
ohne Callback-Aufruf verworfen.

### 6.4 RLAIF-Präferenzpaar-Konstruktion aus Embedding

Da der Klartext nicht verfügbar ist, wird das Präferenzpaar synthetisch
aus dem Embedding konstruiert:

```python
# Pseudo-Code — tatsächliche Implementierung liegt beim Caller
chosen   = nearest_neighbors(summary.reason_embedding, polarity=+1)
rejected = nearest_neighbors(summary.reason_embedding, polarity=-1)
pair = PreferencePair(prompt=context, chosen=chosen, rejected=rejected)
rlaif_trainer.addPreferencePair(pair)
```

*Hinweis: Die Nearest-Neighbor-Konstruktion ist eine Näherung. Für
produktiven Einsatz sollte ein dediziertes Feedback-Embedding-Modell
trainiert werden (Forschungsfrage RQ-DK-3).*

---

## 7. Sicherheit & Privacy-Garantien

| Sicherheitsanforderung | Implementierung |
|---|---|
| **Zero-Knowledge-Constraint** | Keine Rohdaten kreuzen Shard-Grenzen — nur DP-Gradienten, Embeddings, anonyme Metriken |
| **mTLS für alle Shard-Kommunikation** | `MTLSClient` bereits in `GossipProtocol`; `ZeroTrustPolicyEnforcer` verifiziert |
| **Differential Privacy** | `LoRAFederationCoordinator` wendet Gaussian-Mechanismus an: σ = Δ·√(2·ln(1.25/δ))/ε |
| **Post-Quantum-Audit** | `SphincsPlus` sichert das globale Federation-Audit-Log |
| **GDPR — DSGVO-Konformität** | `CrossBorderTransferPolicy` prüft EU-Adequacy-Grenze vor Gradient-Aggregation |
| **Embedding-Anonymisierung** | `CrossShardFeedbackSync` erzwingt `shard_origin = "ANON"`, kein Klartext in Payload |
| **Privacy-Budget-Monitoring** | `DifferentialPrivacyManager::verifyPrivacyBudget(ε_total, δ)` verhindert Budget-Überschreitung |

---

## 8. Verbindung zu den Optimierungsebenen 5–10

Die sechs LLM-Optimierungsebenen aus `LLM_OPTIMIERUNGSEBENEN_MATRIX.md`
werden im Distributed-Modus qualitativ erweitert:

| Ebene | Shard-lokal (heute) | Cross-Shard (neu) |
|---|---|---|
| **E5 Tx-Semantik** | Batch-Hints je Shard | `CrossShardTransaction`-Hints via `QueryFederation` |
| **E6 Schema** | Dead-Weight-Report je Shard | Aggregierter Dead-Weight über alle Shards — verhindert Fehleinstufung saisonaler Felder |
| **E7 Security** | IntentAlert je Shard | Gossip-Propagation: Shard A erkennt Anomalie → alle Shards erhöhen `session_risk_score` sofort |
| **E8 Multi-Tenant** | WorkloadFingerprint je Shard | Cross-Shard WorkloadFingerprint-Transfer: Shard B lernt von Shard A bei ähnlichem Tenant |
| **E9 Explainability** | AIDecisionAuditor je Shard | `FederatedAIDecisionAuditor`: DBA sieht Entscheidungen **aller Shards** in einer globalen Timeline |
| **E10 Layout** | LayoutHint je Shard | LayoutHint propagiert via Gossip — shard-übergreifende Kompressionsstrategie |

### Neue Ebene 11 — Verteiltes Wissens-Sharding

Layer 11 ist nicht analog zu Ebenen 5–10 (die orthogonale semantische
Dimensionen beschreiben), sondern die **Infrastrukturebene**, die Ebenen 5–10
erst shard-übergreifend *wirksam* macht.

```
Ebenen 5–10 (Semantik, lokal)  +  Ebene 11 (Transport)
=  Distributed Self-Optimizing ThemisDB
```

---

## 9. Gesamtarchitektur-Diagramm

```
┌─────────────────────────────────────────────────────────────────────────┐
│          ThemisDB Distributed Knowledge Architecture — Ebene 11         │
│                                                                         │
│  Shard 1          Shard 2          Shard N         Global Coordinator   │
│  ─────────        ─────────        ─────────       ─────────────────    │
│                                                                         │
│  [Ebene A: Adapter-Discovery]                                           │
│  AdapterReg   ─── Gossip ───►  AdapterReg   ─────► CapabilityScore     │
│       ▲             │                                     │             │
│       └─────────────┴─────────── AdaptiveShardRouter ◄───┘             │
│                                                                         │
│  [Ebene B: Federated LoRA]                                              │
│  LoRATrainer  ──► Gradient₁                                             │
│  LoRATrainer  ──► Gradient₂ ──► LoRAFederationCoordinator              │
│  LoRATrainer  ──► GradientN        │ FedAvg + DP(ε,δ)                  │
│       ◄──────── GlobalDelta ◄──────┘                                   │
│                                                                         │
│  [Ebene C: Federated RAG]                                               │
│  RAGBridge    ─── QueryFed ──►  RAGBridge   ──► FederatedRAGMerger     │
│                                                     │ RRF + top_k=20   │
│                              LLM Prompt ◄───────────┘                  │
│                                                                         │
│  [Ebene D: Federated RLAIF]                                             │
│  FeedbackColl ─── Gossip ───►  FeedbackColl  ──► CrossShardFeedbackSync│
│  RLAIFTrainer ◄── Summary  ◄──────────────────────────────────────────  │
│                                                                         │
│  [Sicherheit durchgängig]                                               │
│  ZeroTrust ─────── mTLS ──── SphincsPlus ──── CrossBorderTransferPolicy│
│                                                                         │
│  [Ebenen 5–10 im verteilten Modus]                                      │
│  E7 IntentAlert  ──►  Gossip  ──►  session_risk_score (alle Shards)    │
│  E6 Dead-Weight  ──►  Federation ──►  globaler Dead-Weight-Report       │
│  E9 AIDecision   ──►  Audit-Log  ──►  FederatedAIDecisionAuditor        │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 10. Implementierungsreihenfolge nach ROI

| Priorität | Ebene | Geschätzter Aufwand | Sofortiger Nutzen |
|---|---|---|---|
| **1** | A — Adapter-Gossip | 2 Wochen | Domain-aware Routing ohne Training |
| **2** | C — Federated RAG Merge | 3 Wochen | LLM sieht Wissen aller Shards |
| **3** | B — Federated LoRA | 6 Wochen | Kernmechanismus verteiltes Lernen |
| **4** | D — Cross-Shard RLAIF | 3 Wochen | DBA-Feedback propagiert global |

**Begründung der Reihenfolge:**
- Ebene A ist rein konfigurativ, kein Training, sofortiger Routing-Gewinn
- Ebene C nutzt vorhandene `QueryFederation` + `RAGIngestionBridge`
- Ebene B ist der aufwändigste Kern, wird aber von Ebene A und C vorbereitet
  (korrekte Routing-Basis → bessere Gradienten)
- Ebene D ist unabhängig, aber das volle Potential entfaltet sich erst wenn
  Ebene B vollständig läuft (besseres Adapter-Fundament → bessere Preference-Pairs)

---

## 11. Offene Forschungsfragen

**RQ-DK-1** — Wie viele Trainings-Samples benötigt ein Shard mindestens,
bevor sein Gradient-Beitrag das globale Modell nicht verschlechtert?
*(Hypothese: n_k ≥ 500 basierend auf FedAvg-Konvergenzanalyse McMahan §4)*

**RQ-DK-2** — Wie groß ist der Informationsverlust durch DP-Rauschen
bei ε = 0.1, δ = 1e-5 für LoRA-Adapater mit rank=8?
*(Hypothese: < 3 % Accuracy-Delta, da LoRA-Updates per se Low-Norm-Vektoren sind)*

**RQ-DK-3** — Ist die Nearest-Neighbor-Konstruktion von RLAIF-Präferenzpaaren
aus Embeddings (ohne Klartext) ausreichend für messbare Lernverbesserung?
*(Hypothese: +8–15 % im Vergleich zu keiner Cross-Shard-Propagation; muss
empirisch validiert werden)*

**RQ-DK-4** — Verletzt Gradient-Aggregation über EU/Non-EU-Shard-Grenzen die
DSGVO-Anforderungen (Art. 44 DSGVO — Drittlandtransfer)?
*(Hypothese: Nein, da nur anonymisierte numerische Gradienten transferiert
werden, kein personenbezogener Inhalt — aber: `CrossBorderTransferPolicy`
muss Grenzfall explizit prüfen)*

**RQ-DK-5** — Wie stark ist der Spezialisierungsverlust in Ebene C, wenn
RRF domänen-spezialisierte Shard-Ergebnisse mit generischen Shard-Ergebnissen
vermischt?
*(Hypothese: Mit Spezialisierungs-Boost 1.2× wird Relevanz dominiert durch
den spezialisierten Shard; generische Beiträge ergänzen, stören nicht)*

**RQ-DK-6** — Konvergiert FedAvg bei stark heterogener Shard-Spezialisierung
(Security-Shard vs. Schema-Shard) oder divergiert das globale Modell?
*(Hypothese: FedProx mit μ = 0.01 verhindert Divergenz bei bis zu 5×
Heterogenität — basierend auf Li et al. 2020 §4.3)*

---

## 12. Laufzeit-Einflussmechanismen: Schalter · Hebel · Optimierer

> **Querbezug:** `PERFORMANCE_EXPECTATIONS.md §14.1` · Paper 2
> `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12` ·
> `config/lora/adalora_optimization_strategy.yaml` ·
> `config/ai_ml/llm/llm_optimization_strategy.yaml`

Die Erkenntnisse aus Paper 1 (FedAvg-Rank-Federation) und Paper 2
(Semantic Advisors B5–B10) ermöglichen drei klar trennbare Einflussklassen,
mit denen LLM und AdaLoRA die ThemisDB-Performance zur **Laufzeit** steuern —
ohne Neustart, Recompile oder Modell-Reload.

### 12.1 Klasse 1 — Schalter

Schalter sind **binäre Codepfad-Entscheidungen**. Ihre Aktivierung tritt
sofort in Kraft und verändert das Laufzeitverhalten deterministisch.

| Schalter | Aktivierungsbedingung | Betroffenes SLO |
|---|---|---|
| `bypass_dedup_cache_for_streaming` | Streaming-Request erkannt | TTFT −10 ms (L-1) |
| `enable_draft_kv_cache` | Draft-Modell vorhanden | Speculative Decoding aktiv (L-6/L-8) |
| `hot_swap.enabled` | `THEMIS_ENABLE_LLM=1` gesetzt | LoRA-Swap ohne Neustart (L-3 ≤ 5 s) |
| `importance_pruning.enabled` | AdaLoRA aktiv | Rank-Budget-Kompression (§39.20) |
| `federation.broadcast_importance_scores` | Feature-Flag IMPL-A3 aktiv | Shard-übergreifendes Pruning-Wissen |

### 12.2 Klasse 2 — Hebel

Hebel sind **numerische Konfigurationsparameter** mit messbarem, kontinuierlichem
Trade-off. Sie können zur Laufzeit umgeschrieben werden (Hot-Reload via SIGHUP).

| Hebel | Wertebereich | Trade-off |
|---|---|---|
| `speculative_tokens` | 3 – 10 | TTFT ↔ Acceptance-Rate (L-6) |
| `total_rank_budget` | 128 – 1024 | Memory-Footprint ↔ Modellqualität |
| `acceptance_threshold` | 0.6 – 0.9 | Inferenzgeschwindigkeit ↔ Korrektheit |
| `pruning_interval_steps` | 50 – 500 | Pruning-Overhead ↔ Adaptivität |
| `worker_threads` | 2 – 16 | Dispatch-Latenz P99 ↔ CPU-Overhead (L-5) |
| `chunked_prefill_size` | 512 – 2048 tokens | TTFT-Reduktion ↔ Decode-Interleave |

### 12.3 Klasse 3 — Optimierer

Optimierer sind **selbst-anpassende Feedback-Loops**. Sie beobachten den aktuellen
Laufzeit-Zustand und schreiben Hebel oder Schalter autonom um.

| Optimierer | Wirkt auf | Intervall | Verknüpftes Issue |
|---|---|---|---|
| `WorkloadFingerprintEngine` (B8) | AdaLoRA `total_rank_budget` | alle 100 Queries | IMPL-B8 |
| FedAvg Rank-Aggregation (`lora_federation_coordinator`) | Importance-Score-Verteilung aller Shards | pro Pruning-Step | IMPL-A3 |
| `SelfImprovementModule` | Qualitäts-Thresholds (Acceptance, Confidence) | kontinuierlich | DK-4 |
| TIES-Merge SVD (`LoRAAdapterMerger`) | Adapter-Zusammenführung ohne Checkpoint | bei Adapter-Switch | PR #4405 |
| CI SLO-Gate (P99 > 20 % Regression) | Deployment-Freigabe | jeder CI-Lauf | §23 SLO Monitor |

### 12.4 Wirkungskette

```
WorkloadFingerprintEngine (B8)
  └─ erkennt aktuellen Query-Mix
       └─ passt total_rank_budget an
            └─ AdaLoRA verteilt Rank-Budget per-Layer optimal
                 └─ lora_federation_coordinator propagiert
                    Importance-Scores shard-weit (FedAvg)
                         └─ TTFT P99 L-1 sinkt
                            Throughput L-8 steigt
                            — ohne manuelle Intervention
```

Die Differential-Privacy-Garantie (Dwork & Roth 2014, Gaussian-Mechanismus,
Ebene B §4) bleibt dabei vollständig erhalten: nur anonymisierte
Gradient-Statistiken werden zwischen Shards ausgetauscht.

---

## 13. Referenzen

**Federated Learning & Differential Privacy:**
- McMahan, H.B. et al. (2017). Communication-Efficient Learning of Deep Networks from Decentralised Data. *AISTATS 2017*.
- Li, T. et al. (2020). Federated Optimization in Heterogeneous Networks (FedProx). *MLSys 2020*.
- Dwork, C. & Roth, A. (2014). The Algorithmic Foundations of Differential Privacy. *Foundations and Trends in Theoretical Computer Science*.
- Geyer, R.C. et al. (2017). Differentially Private Federated Learning: A Client Level Perspective. *arXiv:1712.07557*.

**RLAIF & Constitutional AI:**
- Bai, Y. et al. (2022). Constitutional AI: Harmlessness from AI Feedback. *arXiv:2212.08073*.
- Lee, H. et al. (2023). RLAIF: Scaling Reinforcement Learning from Human Feedback with AI Feedback. *arXiv:2309.00267*.

**Retrieval & Re-Ranking:**
- Cormack, G.V., Clarke, C.L.A., Buettcher, S. (2009). Reciprocal Rank Fusion outperforms Condorcet and individual Rank Learning Methods. *ACM SIGIR 2009*.

**Gossip & Verteilte Systeme:**
- Demers, A. et al. (1987). Epidemic Algorithms for Replicated Database Maintenance. *ACM PODC 1987*.
- Jelasity, M. et al. (2007). Gossip-based Peer Sampling. *ACM TOCS 2007*.

**Self-Driving DBMS:**
- Pavlo, A. et al. (2017). Self-Driving Database Management Systems. *CIDR 2017*.
- Van Aken, D. et al. (2017). Automatic Database Management System Tuning Through Large-scale Machine Learning. *ACM SIGMOD*.

**ThemisDB interne Dokumente:**
- `docs/de/research/LLM_OPTIMIERUNGSEBENEN_MATRIX.md` — Ebenen 5–10
- `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md` — LoRA-Grundlagen
- `docs/en/research/THEMISDB_LORA_METRICS_AND_OVERVIEW.md` — Metriken & Loops 1–4
- `docs/de/research/MULTI_LAYER_FEEDBACK_LEARNING.md` — Feedback-Koordination
- `docs/en/sharding/RAID_LORA_IMPLEMENTATION_REPORT.md` — RAID Data-Layer Status

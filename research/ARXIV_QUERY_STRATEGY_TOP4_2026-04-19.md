# ArXiv Query Strategy (Top 4 ThemisDB Drafts)

**Status**: Ready for Search
**Date**: 2026-04-19
**Scope**: Pre-search strategy for novelty-gap mapping and citation seeding

---

## Abstract

Dieses Dokument definiert eine reproduzierbare Vorab-Strategie fuer die ArXiv-Literaturrecherche zu vier priorisierten ThemisDB-Paper-Drafts: (A) Serializable RAG Under Contention, (B) Gossip-Driven LoRA Domain Routing, (C) Continuous Batching in Database-Native LLM Pipelines und (D) Cost-Aware Hybrid Retrieval Planning in AQL. Fuer jedes Thema werden strukturierte Query-Sets (Broad/Focused/Tight), themenspezifische Ausschlusskriterien sowie Screening-Fragen zur Relevanzbeurteilung bereitgestellt. Ein einheitliches Scoring-Schema (0-10) klassifiziert Treffer als Kern-, Sekundaer- oder Hintergrundliteratur. Die Strategie zielt auf systematische Erfassung des Stands der Technik, Schaerfen der Neuheitsnische je Draft und Aufbau einer belastbaren Related-Work-Grundlage. Bekannte Einschraenkungen betreffen die ArXiv-Abdeckung (kein vollstaendiger Conference-Index), die Dynamik der Literaturbasis und die subjektive Komponente der Relevanzgewichtung.

---

## I. Einleitung

Die vier priorisierten ThemisDB-Drafts adressieren technische Problemstellungen, fuer die die Literaturbasis dynamisch und partiell unvollstaendig ist: Konsistenz und Isolation in RAG-Systemen, dezentrale Adapter-Routingprotokolle fuer LLM-Serving, scheduling-theoretische Aspekte datenbanknativ eingebetteter Inferenz-Pipelines und kostenmodellbasierte Hybridretrieval-Planung. Ohne strukturierte Vorrecherche besteht das Risiko, dass wesentliche verwandte Arbeiten uebersehen werden, Neuheitsbehauptungen nicht praezise abgegrenzt sind oder Related-Work-Sektionen wichtige Baseline-Arbeiten nicht nennen.

Die Herausforderungen auf den vier Gebieten lassen sich wie folgt skizzieren:

1. **Serializable RAG**: Konsistenzforschung bei RAG-Systemen konzentriert sich bisher auf Embedding-Qualitaet und Retrievalguete [1, 2]. Isolationssemantik im Sinne von MVCC/SI/SSI als RAG-Parameter ist bislang nicht systematisch untersucht.
2. **Gossip LoRA Routing**: Dezentrale Modellselektion via Gossip-Protokolle ist im Kontext verteilter Datenbanken und Peer-to-Peer-Systemen etabliert [8], aber noch nicht mit LoRA-Adapter-Lifecycle-Management in DB-nativer Inferenz verbunden.
3. **Continuous Batching DB-Native**: Iteration-level scheduling ist fuer dedizierte Serving-Systeme (vLLM [4], TensorRT-LLM [5]) gut untersucht. Die Wechselwirkung mit AQL-Query-Execution, MVCC-Transaktionen und LoRA-Hot-Swap in einem einheitlichen System ist eine offene Forschungsluecke.
4. **Cost-Aware Hybrid Retrieval**: Kostenmodelle fuer Hybridsuche (lexical + vector + graph) ueber mehrere Operatorpfade sind in der Literatur kaum behandelt. Bestehende Planer optimieren selten gleichzeitig Recall/NDCG und Laufzeit [6].

### Ziel dieses Dokuments

- Stand der Technik systematisch und reproduzierbar erfassen.
- Neuheitsnische (Delta) pro Draft schaerfen.
- Belastbare Related-Work-Grundlage fuer alle vier Drafts vorbereiten.
- Seed-Referenzen fuer zitierrelevante Kernarbeiten identifizieren.

---

## II. Methodik

### II.1 Allgemeines Suchprotokoll

#### Zeitfenster

- Primaer: 2022-2026 (LLM-serving, RAG, distributed inference)
- Sekundaer: 2018-2021 (DB optimizer, classical distributed query planning)

#### ArXiv-Kategorien

- `cs.DB`
- `cs.LG`
- `cs.DC`
- `cs.AI`
- `cs.CL`

#### Suchmodus

Pro Thema drei Ebenen:

1. **Broad sweep** (hohe Recall)
2. **Focused query** (balancierter Precision/Recall)
3. **Tight query** (hohe Precision, novelty check)

#### Ausschlusskriterien

Treffer zunaechst als *nicht zentral* markieren, wenn:
- kein verteiltes Systemmodell vorhanden ist
- nur Modell-Training, aber kein Serving/Execution-Design behandelt wird
- rein benchmark-orientiert ohne Architekturbeitrag
- kein Bezug zu transaktionaler Datenhaltung/DB-Integration erkennbar ist

### II.2 Priorisierte Suchreihenfolge

1. Thema C (Continuous Batching) - viele verwandte Arbeiten, schnelle Baseline-Lage.
2. Thema D (Cost-Aware Hybrid Retrieval) - hohe Paper-Dichte, schneller State-of-the-Art Vergleich.
3. Thema A (Serializable RAG) - potenziell groessere Neuheitsnische, aber duennere direkte Trefferlage.
4. Thema B (Gossip LoRA Routing) - spezialisiertes Gebiet, vermutlich weniger direkte 1:1 Treffer.

### II.3 Sofort nutzbare Query-Shortlist

- "retrieval augmented generation" serializable isolation snapshot
- "transactional RAG" mvcc ssi occ
- gossip routing lora distributed inference
- capability based routing llm serving
- continuous batching paged attention tail latency
- database native llm serving scheduler kv cache
- cost-aware hybrid retrieval lexical vector graph
- rag query optimizer cost model recall latency

---

## III. Themenspezifische Query-Sets

### III.A Thema A: Serializable RAG Under Contention

#### Keywords

- serializable rag
- snapshot isolation retrieval augmented generation
- transactional rag
- mvcc rag
- concurrency control + llm retrieval
- anomaly detection rag consistency

#### Query-Sets

**Broad**
- "retrieval augmented generation" AND (consistency OR isolation OR concurrency)
- (transactional OR serializable OR snapshot isolation) AND (RAG OR "retrieval augmented")

**Focused**
- (mvcc OR "snapshot isolation" OR serializable) AND ("retrieval augmented generation" OR "hybrid retrieval") AND (database OR dbms)
- (write skew OR phantom OR anomaly) AND (RAG OR LLM retrieval)

**Tight**
- "transactional retrieval augmented generation" AND (serializable OR SSI OR OCC)
- ("isolation level" AND "retrieval augmented generation") AND (evaluation OR benchmark)

#### Screening-Fragen

1. Gibt es ein explizites Isolationsmodell (RC/RR/SI/SZ)?
2. Werden Qualitaetsmetriken unter Konfliktlast gemessen?
3. Ist Retrieval innerhalb transaktionaler Grenzen modelliert?

#### Erwartete Delta-Story ThemisDB

- Kombination aus ACID-Isolation und RAG-Qualitaetsdrift unter Contention als primaerer Beitrag.
- Nicht nur retrieval quality, sondern correctness + anomaly behavior.
- Methodischer Bezug: `SERIALIZABLE_RAG_UNDER_CONTENTION_DRAFT.md`, ThemisDB MVCC-Transaktionskern.

---

### III.B Thema B: Gossip-Driven LoRA Domain Routing

#### Keywords

- gossip routing llm
- federated lora routing
- capability based routing inference
- decentralized model selection
- distributed adapter selection
- domain aware inference routing

#### Query-Sets

**Broad**
- (gossip OR epidemic) AND (inference routing OR model routing)
- (LoRA OR adapter) AND (distributed inference OR routing)

**Focused**
- ("capability announcement" OR "capability routing") AND (LLM OR inference)
- (federated learning AND LoRA) AND (serving OR routing)

**Tight**
- (gossip AND LoRA AND routing) AND (latency OR tail latency)
- (distributed adapter selection) AND (domain specialization)

#### Screening-Fragen

1. Erfolgt Routing dezentral oder zentral?
2. Werden Accuracy und Latenz gemeinsam als Routing-Signal genutzt?
3. Gibt es Ausfall-/Staleness-Modelle?

#### Erwartete Delta-Story ThemisDB

- Gossip-basierte Domain-Affinity-Routingentscheidung in DB-nativer Inferenzfabrik.
- Verbindung von federated adapter metadata und request scheduling.
- Methodischer Bezug: `GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md`, `src/llm/lora/`.

---

### III.C Thema C: Continuous Batching in Database-Native LLM Pipelines

#### Keywords

- continuous batching llm serving
- paged attention scheduler
- kv cache scheduling
- db-native llm serving
- queue-aware inference scheduler
- tail latency batching

#### Query-Sets

**Broad**
- ("continuous batching" OR "iteration-level scheduling") AND (LLM serving)
- ("kv cache" AND scheduler) AND (inference)

**Focused**
- (database OR dbms) AND (LLM serving OR RAG serving) AND (batch scheduling)
- (tail latency AND throughput) AND (continuous batching)

**Tight**
- ("paged attention" AND scheduler) AND (queue depth OR admission control)
- ("database-native" AND "LLM pipeline") AND (batching)

#### Screening-Fragen

1. Wird nur Model-Serving betrachtet oder auch DB-gekoppelte Pipeline?
2. Gibt es klare Scheduler-Parameter und Stabilitaetsgrenzen?
3. Sind TTFT, p95/p99 und Throughput gemeinsam berichtet?

#### Erwartete Delta-Story ThemisDB

- Scheduler-Analyse im Kontext von AQL/RAG/transactional constraints statt isolierter serving engine.
- Methodischer Bezug: `CONTINUOUS_BATCHING_DATABASE_NATIVE_LLM_DRAFT.md`, `ContinuousBatchScheduler`, `PagedKVCache`.

---

### III.D Thema D: Cost-Aware Hybrid Retrieval Planning in AQL

#### Keywords

- hybrid retrieval cost model
- lexical vector graph query optimization
- rag query planner
- vector relational optimizer
- cost-based rag planning
- multi-operator retrieval planning

#### Query-Sets

**Broad**
- (hybrid retrieval OR hybrid search) AND (cost model OR query optimizer)
- (vector search AND graph retrieval) AND (planning OR optimization)

**Focused**
- (RAG AND query planning) AND (cost-based OR optimizer)
- (lexical AND vector AND graph) AND (retrieval) AND (latency OR recall)

**Tight**
- ("cost-aware hybrid retrieval") AND (database)
- ("query optimizer" AND "retrieval augmented generation")

#### Screening-Fragen

1. Gibt es explizite Kostenfunktionen fuer hybride Operatorpfade?
2. Werden Recall/NDCG und Laufzeit gemeinsam optimiert?
3. Wird Planstabilitaet unter Workload-Shift behandelt?

#### Erwartete Delta-Story ThemisDB

- AQL-spezifischer Planner fuer lexical+vector+graph als integriertes Modell mit messbarer Planstabilitaet.
- Methodischer Bezug: `COST_AWARE_HYBRID_RETRIEVAL_PLANNING_AQL_DRAFT.md`, ThemisDB AQL-Query-Engine.

---

## IV. Evaluation der Suchstrategie

### IV.1 Relevanz-Scoring fuer Treffer (0-10)

Treffer nach folgendem Schema bewerten:

| Kriterium | Punkte |
|---|---:|
| Direktes Themen-Match (Problemstellung) | +3 |
| Vergleichbares Systemmodell (distributed + serving + data) | +2 |
| Reproduzierbare Evaluation mit klaren Metriken | +2 |
| Methodischer Beitrag (Algorithmus/Planner/Protocol) | +2 |
| Starke Bedrohungs-/Limitationsdiskussion | +1 |

**Interpretation:**

| Score | Klassifikation |
|---|---|
| 8-10 | Kernreferenz |
| 5-7 | Sekundaerreferenz |
| 0-4 | Hintergrund/optional |

### IV.2 Erfolgskriterien der Recherche

Eine erfolgreiche ArXiv-Recherche pro Thema ergibt:

- Mindestens 3 Kernreferenzen (Score 8-10) identifiziert.
- Neuheitsdelta ThemisDB in 3-5 Bullet-Points formulierbar.
- Risikoabschaetzung (inkrementell vs. substanziell neu) begruendet moeglich.
- Konkrete Anpassungen fuer Abstract + Related Work im jeweiligen Draft benennbar.

### IV.3 Ergebnisformat nach der ArXiv-Recherche

Pro Thema ein kompakter Research-Block:

1. Top-5 relevante Arbeiten (mit Relevanz-Score und kurzer Begruendung)
2. Neuheitsdelta ThemisDB in 3-5 Bulletpoints
3. Risikoabschaetzung (inkrementell vs. substanziell neu)
4. Konkrete Anpassungen fuer Abstract + Related Work im jeweiligen Draft

---

## V. Limitations / Bekannte Einschraenkungen

1. **ArXiv-Abdeckung**: ArXiv indexiert nicht alle Konferenzpaper vollstaendig. VLDB, SIGMOD, OSDI und MLSys Proceedings sind partiell in ArXiv vertreten; seltene Venue-Luecken erfordern ergaenzende Suchen in DBLP, ACM DL und IEEE Xplore.
2. **Dynamik der Literaturbasis**: Im Primaer-Zeitfenster 2022-2026 erscheinen woechentlich neue ArXiv-Preprints. Die Suche muss bei relevanten Drafts kurz vor Submission wiederholt werden.
3. **Subjektive Relevanzgewichtung**: Das Scoring-Schema ist reproduzierbar, aber die Zuordnung von +2 vs. +3 bei Grenzfaellen bleibt ein Ermessensentscheid. Peer-Review-Stufen der Drafts koennen abweichende Gewichtungen benoetigen.
4. **Englischsprachige Bias**: Die Query-Sets sind ausschliesslich auf Englisch formuliert. Bedeutsame Arbeiten in anderen Sprachen werden nicht erfasst. Fuer ThemisDB-relevante Themen ist dieser Bias gering, da internationale Konferenzen auf Englisch publizieren.
5. **Thema B (Gossip LoRA Routing) - duenne Trefferlage**: Das spezifische Kombinationsmuster Gossip + LoRA + DB-nativer Inference ist in der Literatur kaum direkt adressiert. Related-Work-Sektionen des Drafts muessen benachbarte Felder (dezentrale Modellselektion, federated adapter management, Gossip-Protokolle) getrennt aufarbeiten.
6. **Keyword-Drift**: Begriffe wie "continuous batching", "paged attention" und "cost-aware retrieval" sind nicht standardisiert und koennen in aelterer Literatur unter anderen Bezeichnungen erscheinen. Die Broad-Sweep-Ebene adressiert dieses Risiko, deckt es aber moeglicherweise nicht vollstaendig ab.

---

## VI. Fazit

Dieses Dokument stellt eine reproduzierbare Suchstrategie fuer vier priorisierte ThemisDB-Paper-Drafts bereit. Die Drei-Ebenen-Query-Struktur (Broad/Focused/Tight) und das einheitliche Scoring-Schema (0-10) ermoeglichen eine systematische und nachvollziehbare Literaturrecherche. Die Suchreihenfolge priorisiert Themen mit hoher Paperdichte (C, D) vor solchen mit erwarteter Neuheitsnische (A, B), um fruehzeitig Baselines zu fixieren und Neuheitsbehauptungen abzusichern. Erkannte Limitations (ArXiv-Abdeckung, Keyword-Drift, themenspezifisch duenne Trefferlage bei Thema B) sind explizit dokumentiert und koennen durch ergaenzende DBLP/ACM-Suchen und Wiederholung kurz vor Submission adressiert werden.

---

## Referenzen

[1] P. Lewis et al., "Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks," in *Advances in Neural Information Processing Systems (NeurIPS)*, 2020. arXiv: https://arxiv.org/abs/2005.11401

[2] S. Es et al., "RAGAS: Automated Evaluation of Retrieval Augmented Generation," in *Proceedings of the 18th Conference of the EACL*, 2024. arXiv: https://arxiv.org/abs/2309.15217

[3] W. Kwon et al., "Efficient Memory Management for Large Language Model Serving with PagedAttention," in *Proceedings of the 29th ACM SOSP*, 2023. arXiv: https://arxiv.org/abs/2309.06180

[4] H. Yu et al., "Orca: A Distributed Serving System for Transformer-Based Generative Models," in *Proceedings of the 16th USENIX OSDI*, 2022. https://www.usenix.org/conference/osdi22/presentation/yu

[5] E. J. Hu et al., "LoRA: Low-Rank Adaptation of Large Language Models," in *Proceedings of ICLR*, 2022. arXiv: https://arxiv.org/abs/2106.09685

[6] Y. A. Malkov and D. A. Yashunin, "Efficient and Robust Approximate Nearest Neighbor Search Using Hierarchical Navigable Small World Graphs," in *IEEE Transactions on Pattern Analysis and Machine Intelligence*, vol. 42, no. 4, 2020. arXiv: https://arxiv.org/abs/1603.09320

[7] G. Sheng et al., "S-LoRA: Serving Thousands of Concurrent LoRA Adapters," in *Proceedings of MLSys*, 2024. arXiv: https://arxiv.org/abs/2311.03285

[8] A. J. Demers et al., "Epidemic Algorithms for Replicated Database Maintenance," in *Proceedings of the 6th ACM PODC*, 1987. https://dl.acm.org/doi/10.1145/41840.41841

---

*Hinweis: Dieses Dokument ist ein Pre-Search-Protokoll. Es enthaelt bewusst noch keine gesicherten Literaturclaims aus abgeschlossenen Recherchen und dient als reproduzierbare Suchvorlage fuer die Drafts unter `research/`.*

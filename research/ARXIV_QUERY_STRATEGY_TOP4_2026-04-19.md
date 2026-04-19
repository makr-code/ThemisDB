# ArXiv Query Strategy (Top 4 ThemisDB Drafts)

**Status**: Ready for Search  
**Date**: 2026-04-19  
**Scope**: Pre-search strategy for novelty-gap mapping and citation seeding

---

## 1. Ziel

Dieses Dokument definiert eine reproduzierbare Vorab-Strategie fuer die ArXiv-Recherche zu vier priorisierten ThemisDB-Paper-Drafts:

1. `SERIALIZABLE_RAG_UNDER_CONTENTION_DRAFT.md`
2. `GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md`
3. `CONTINUOUS_BATCHING_DATABASE_NATIVE_LLM_DRAFT.md`
4. `COST_AWARE_HYBRID_RETRIEVAL_PLANNING_AQL_DRAFT.md`

Zweck der Recherche:
- Stand der Technik systematisch erfassen
- Neuheitsnische (Delta) pro Draft schaerfen
- belastbare Related-Work-Sektion vorbereiten

---

## 2. Allgemeines Suchprotokoll

## 2.1 Zeitfenster

- Primaer: 2022-2026 (LLM-serving, RAG, distributed inference)
- Sekundaer: 2018-2021 (DB optimizer, classical distributed query planning)

## 2.2 ArXiv-Kategorien

- `cs.DB`
- `cs.LG`
- `cs.DC`
- `cs.AI`
- `cs.CL`

## 2.3 Suchmodus

Pro Thema drei Ebenen:
1. **Broad sweep** (hohe Recall)
2. **Focused query** (balancierter Precision/Recall)
3. **Tight query** (hohe Precision, novelty check)

## 2.4 Ausschlusskriterien

Treffer zunaechst als *nicht zentral* markieren, wenn:
- kein verteiltes Systemmodell vorhanden ist
- nur Modell-Training, aber kein Serving/Execution-Design behandelt wird
- rein benchmark-orientiert ohne Architekturbeitrag
- kein Bezug zu transaktionaler Datenhaltung/DB-Integration erkennbar ist

---

## 3. Thema A: Serializable RAG Under Contention

## 3.1 Keywords

- serializable rag
- snapshot isolation retrieval augmented generation
- transactional rag
- mvcc rag
- concurrency control + llm retrieval
- anomaly detection rag consistency

## 3.2 Query-Sets

### Broad
- "retrieval augmented generation" AND (consistency OR isolation OR concurrency)
- (transactional OR serializable OR snapshot isolation) AND (RAG OR "retrieval augmented")

### Focused
- (mvcc OR "snapshot isolation" OR serializable) AND ("retrieval augmented generation" OR "hybrid retrieval") AND (database OR dbms)
- (write skew OR phantom OR anomaly) AND (RAG OR LLM retrieval)

### Tight
- "transactional retrieval augmented generation" AND (serializable OR SSI OR OCC)
- ("isolation level" AND "retrieval augmented generation") AND (evaluation OR benchmark)

## 3.3 Screening-Fragen

1. Gibt es ein explizites Isolationsmodell (RC/RR/SI/SZ)?
2. Werden Qualitaetsmetriken unter Konfliktlast gemessen?
3. Ist Retrieval innerhalb transaktionaler Grenzen modelliert?

## 3.4 Erwartete Delta-Story ThemisDB

- Kombination aus ACID-Isolation und RAG-Qualitaetsdrift unter Contention als primaerer Beitrag.
- Nicht nur retrieval quality, sondern correctness + anomaly behavior.

---

## 4. Thema B: Gossip-Driven LoRA Domain Routing

## 4.1 Keywords

- gossip routing llm
- federated lora routing
- capability based routing inference
- decentralized model selection
- distributed adapter selection
- domain aware inference routing

## 4.2 Query-Sets

### Broad
- (gossip OR epidemic) AND (inference routing OR model routing)
- (LoRA OR adapter) AND (distributed inference OR routing)

### Focused
- ("capability announcement" OR "capability routing") AND (LLM OR inference)
- (federated learning AND LoRA) AND (serving OR routing)

### Tight
- (gossip AND LoRA AND routing) AND (latency OR tail latency)
- (distributed adapter selection) AND (domain specialization)

## 4.3 Screening-Fragen

1. Erfolgt Routing dezentral oder zentral?
2. Werden Accuracy und Latenz gemeinsam als Routing-Signal genutzt?
3. Gibt es Ausfall-/Staleness-Modelle?

## 4.4 Erwartete Delta-Story ThemisDB

- Gossip-basierte Domain-Affinity-Routingentscheidung in DB-nativer Inferenzfabrik.
- Verbindung von federated adapter metadata und request scheduling.

---

## 5. Thema C: Continuous Batching in Database-Native LLM Pipelines

## 5.1 Keywords

- continuous batching llm serving
- paged attention scheduler
- kv cache scheduling
- db-native llm serving
- queue-aware inference scheduler
- tail latency batching

## 5.2 Query-Sets

### Broad
- ("continuous batching" OR "iteration-level scheduling") AND (LLM serving)
- ("kv cache" AND scheduler) AND (inference)

### Focused
- (database OR dbms) AND (LLM serving OR RAG serving) AND (batch scheduling)
- (tail latency AND throughput) AND (continuous batching)

### Tight
- ("paged attention" AND scheduler) AND (queue depth OR admission control)
- ("database-native" AND "LLM pipeline") AND (batching)

## 5.3 Screening-Fragen

1. Wird nur Model-Serving betrachtet oder auch DB-gekoppelte Pipeline?
2. Gibt es klare Scheduler-Parameter und Stabilitaetsgrenzen?
3. Sind TTFT, p95/p99 und Throughput gemeinsam berichtet?

## 5.4 Erwartete Delta-Story ThemisDB

- Scheduler-Analyse im Kontext von AQL/RAG/transactional constraints statt isolierter serving engine.

---

## 6. Thema D: Cost-Aware Hybrid Retrieval Planning in AQL

## 6.1 Keywords

- hybrid retrieval cost model
- lexical vector graph query optimization
- rag query planner
- vector relational optimizer
- cost-based rag planning
- multi-operator retrieval planning

## 6.2 Query-Sets

### Broad
- (hybrid retrieval OR hybrid search) AND (cost model OR query optimizer)
- (vector search AND graph retrieval) AND (planning OR optimization)

### Focused
- (RAG AND query planning) AND (cost-based OR optimizer)
- (lexical AND vector AND graph) AND (retrieval) AND (latency OR recall)

### Tight
- ("cost-aware hybrid retrieval") AND (database)
- ("query optimizer" AND "retrieval augmented generation")

## 6.3 Screening-Fragen

1. Gibt es explizite Kostenfunktionen fuer hybride Operatorpfade?
2. Werden Recall/NDCG und Laufzeit gemeinsam optimiert?
3. Wird Planstabilitaet unter Workload-Shift behandelt?

## 6.4 Erwartete Delta-Story ThemisDB

- AQL-spezifischer Planner fuer lexical+vector+graph als integriertes Modell mit messbarer Planstabilitaet.

---

## 7. Priorisierte Suchreihenfolge

1. Thema C (Continuous Batching) - viele verwandte Arbeiten, schnelle Baseline-Lage.
2. Thema D (Cost-Aware Hybrid Retrieval) - hohe Paper-Dichte, schneller State-of-the-Art Vergleich.
3. Thema A (Serializable RAG) - potenziell groessere Neuheitsnische, aber duennere direkte Trefferlage.
4. Thema B (Gossip LoRA Routing) - spezialisiertes Gebiet, vermutlich weniger direkte 1:1 Treffer.

---

## 8. Relevanz-Scoring fuer Treffer (0-10)

Treffer nach folgendem Schema bewerten:
- +3: direktes Themen-Match (Problemstellung)
- +2: vergleichbares Systemmodell (distributed + serving + data)
- +2: reproduzierbare Evaluation mit klaren Metriken
- +2: methodischer Beitrag (Algorithmus/Planner/Protocol)
- +1: starke Bedrohungs-/Limitationsdiskussion

Interpretation:
- 8-10: Kernreferenz
- 5-7: Sekundaerreferenz
- 0-4: Hintergrund/optional

---

## 9. Ergebnisformat nach der ArXiv-Recherche

Pro Thema ein kompakter Research-Block:

1. Top-5 relevante Arbeiten (mit kurzer Begruendung)
2. Neuheitsdelta ThemisDB in 3-5 Bulletpoints
3. Risikoabschaetzung (inkrementell vs substanziell neu)
4. Konkrete Anpassungen fuer Abstract + Related Work im jeweiligen Draft

---

## 10. Sofort nutzbare Query-Shortlist

- "retrieval augmented generation" serializable isolation snapshot
- "transactional RAG" mvcc ssi occ
- gossip routing lora distributed inference
- capability based routing llm serving
- continuous batching paged attention tail latency
- database native llm serving scheduler kv cache
- cost-aware hybrid retrieval lexical vector graph
- rag query optimizer cost model recall latency

---

*Hinweis: Dieses Dokument ist ein Pre-Search-Protokoll. Es enthaelt bewusst noch keine Literaturclaims und dient als reproduzierbare Suchvorlage.*

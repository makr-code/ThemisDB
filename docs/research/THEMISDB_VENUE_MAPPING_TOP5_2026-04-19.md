# ThemisDB: Venue-Mapping fuer wissenschaftliche Top-5-Themen (Stand 2026-04-19)

## Ziel

Dieses Dokument uebersetzt die priorisierten ThemisDB-Themen in konkrete Publikationsziele pro Venue-Familie:

- DB Systems: VLDB, SIGMOD, ICDE
- Systems fuer LLM Serving: MLSys, USENIX ATC, SOSP/OSDI Workshops
- NLP/LLM Methods: ACL/EMNLP/NAACL (fuer eval- und retrieval-nahe Arbeiten)

## Kurzuebersicht je Thema

| Thema | Primaere Venue-Familie | Bester Beitragstyp | Risiko |
|---|---|---|---|
| RAG plus RAG-Evaluation | VLDB/ICDE + ACL-Industry | End-to-end DB-native RAG Evaluation System | Mittel |
| Vektor-IR und ANN | VLDB/SIGMOD/ICDE | System+Benchmark Paper | Niedrig-Mittel |
| LoRA und QLoRA in DB-Kontext | MLSys + VLDB Industry | Infrastruktur- und Betriebs-Optimierung | Mittel |
| Effiziente LLM-Inferenz (Paged/Speculative) | MLSys/ATC + DB Workshops | Serving-System mit verteiltem DB-Bezug | Mittel-Hoch |
| Verteilte ACID Multi-Model-DB fuer AI | VLDB/SIGMOD | Kernsystems-Paper mit starker Evaluation | Mittel |

## 1) RAG plus RAG-Evaluation

### Empfohlene Venues

- Primaer: VLDB, ICDE
- Sekundaer: ACL Industry Track, EMNLP Industry Track

### Warum passend

- Starke Verbindung aus Datenbanksystem, Retrieval und LLM-Evaluation.
- Reproduzierbare Systemmetriken sind fuer DB-Venues besonders attraktiv.

### Claim-Fokus (Abstract-Kern)

- ACID-kompatible, DB-native RAG-Pipeline mit integrierter Evaluationsschicht.
- Quantifizierter Trade-off zwischen Antwortqualitaet, Latenz und Konsistenz.

### Pflichtmetriken

- Faithfulness, Answer Relevancy, Context Precision/Recall
- p50/p95/p99 Latenz
- Throughput unter Last und Contention
- Konsistenzfehlerquote unter konkurrierenden Writes

### Relevante Repo-Basis

- RAG als implemented
- RAGAS-orientierte Monitoring/Evaluationsbausteine implemented

## 2) Vektor-IR und ANN (HNSW, FAISS, Hybrid Search)

### Empfohlene Venues

- Primaer: SIGMOD, VLDB, ICDE
- Sekundaer: SISAP-umfeldnahe Tracks/Workshops

### Warum passend

- Klassisches DB/IR-Topthema mit klaren Benchmark-Protokollen.
- Gute Anschlussfaehigkeit an Hybrid-Retrieval und Kostenmodelle.

### Claim-Fokus (Abstract-Kern)

- Einheitlicher Query-Planer fuer lexical + vector + graph retrieval.
- Produktionsnahe Vergleichsstudie HNSW/FAISS/Hybrid unter realen Lastprofilen.

### Pflichtmetriken

- Recall@k, nDCG@k, MRR
- QPS und Tail-Latenz
- Speicherfussabdruck und Build-Zeit
- Kosten pro 1M Queries

### Relevante Repo-Basis

- HNSW/FAISS als Architekturentscheidung und Implementierungspfad dokumentiert
- Hybrid Search in Architektur dokumentiert

## 3) LoRA und QLoRA im Datenbank-nativen Betrieb

### Empfohlene Venues

- Primaer: MLSys
- Sekundaer: VLDB Industry/Applications

### Warum passend

- LoRA/QLoRA sind methodisch stark, der Neuheitshebel liegt im DB-nativen Betriebsmodell.

### Claim-Fokus (Abstract-Kern)

- Multi-LoRA-Adapter-Betrieb direkt im Datenbanksystem mit belastbaren SLOs.
- Lifecycle-Management (routing, hot-swap, rollback, monitoring) fuer produktive Domainen.

### Pflichtmetriken

- Adapter-Switch-Latenz
- Qualitaetsdelta pro Domane
- GPU/VRAM-Effizienz
- Failure-Recovery-Zeit nach fehlerhaften Adapter-Deployments

### Relevante Repo-Basis

- LoRA und QLoRA als implemented dokumentiert
- Training plus LLM-Layer als aktive Komponenten ausgewiesen

## 4) Effiziente LLM-Inferenz (PagedAttention, Speculative, Continuous Batching)

### Empfohlene Venues

- Primaer: MLSys, USENIX ATC
- Sekundaer: OSDI/SOSP Workshops, Datenbank-Workshops mit AI Systems Fokus

### Warum passend

- Starkes Systems-Thema; hoher Impact bei sauberer End-to-end Evaluation.

### Claim-Fokus (Abstract-Kern)

- Datenbank-native Serving-Pipeline mit paged KV-cache, continuous batching und spekulativer Dekodierung.
- Vergleich gegen nicht-integrierte Serving-Tier-Architektur.

### Pflichtmetriken

- Token/s und TTFT
- p99 End-to-end Latenz
- Acceptance-Rate bei spekulativer Dekodierung
- Degradationsverhalten bei Lastspitzen

### Relevante Repo-Basis

- PagedAttention und Speculative Decoding als implemented dokumentiert
- Architektur nennt batching und paged KV-cache explizit

## 5) Verteilte ACID Multi-Model-DB fuer AI

### Empfohlene Venues

- Primaer: VLDB, SIGMOD, ICDE
- Sekundaer: Middleware/Distributed Systems Tracks

### Warum passend

- Klassischer Kernbeitrag fuer DB-Topvenues: Konsistenz, Verteilung, Multi-Model, AI-Integration.

### Claim-Fokus (Abstract-Kern)

- Einheitliche Datenplattform fuer relationale, graph- und vektorbasierte Workloads plus LLM-Funktionen unter strengen Transaktionsgarantien.
- Quantifizierte Kosten von Konsistenz gegen AI/LLM-Serving-Performance.

### Pflichtmetriken

- Commit-Latenz und Abort-Rate unter Contention
- Cross-shard Skalierung
- Replikations- und Failover-Zeit
- Konsistenzverletzungen in Fehlerfaellen

### Relevante Repo-Basis

- Raft/Paxos/Gossip, MVCC, SAGA und Sharding in Architektur und Produktuebersicht verankert

## 6-Monats Submission-Plan (empfohlen)

### Monat 1-2

- RAG plus Evaluation als erstes, einreichungsreifes Manuskript
- Benchmark- und Reproduzierbarkeitsprotokoll fixieren

### Monat 3-4

- ANN/Hybrid Retrieval Paper mit systematischer Vergleichsstudie
- Artefakt-Paket fuer Repro-Track vorbereiten

### Monat 5

- LLM-Inferenz-Optimierung als Systems-Paper (MLSys/ATC)

### Monat 6

- Integratives DB-Systems-Paper zu ACID + AI-native Multi-Model

## Entscheidungsregel fuer schnelle Priorisierung

Wenn das Ziel eine hohe Annahmewahrscheinlichkeit in kurzer Zeit ist:

1. RAG plus Evaluation
2. ANN/Hybrid Retrieval
3. ACID Multi-Model fuer AI
4. LLM-Inferenz-Optimierung
5. LoRA/QLoRA Betriebsmodell

Wenn das Ziel maximale Neuheit im AI-Serving ist, dann 3 und 4 tauschen.

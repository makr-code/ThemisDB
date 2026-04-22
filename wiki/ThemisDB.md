<!--
  Wikipedia-Artikelentwurf: ThemisDB
  Stand: 2026-04-22
  Quellen: research/, docs/, compendium/docs/
  Zielsprachliche Wikipedia: de.wikipedia.org
-->

{{Entwurf}}

{{Infobox Software
| Name               = ThemisDB
| Logo               =
| Beschreibung       = Verteiltes Multi-Modell-Datenbankmanagementsystem mit nativer KI-Integration
| Entwickler         = The ThemisDB Authors
| Erscheinungsjahr   = 2025
| AktuelleVersion    = 1.8.1-rc1 (April 2026)
| Betriebssystem     = Linux, macOS, Windows; Container (Docker/Kubernetes)
| Programmiersprache = [[C++|C++ (ISO 17/20)]]
| Kategorie          = Datenbankverwaltungssystem
| Lizenz             = MIT License with Government Clause
| Website            = https://github.com/makr-code/ThemisDB
}}

**ThemisDB** ist ein quelloffenes, hochperformantes [[Multi-Modell-Datenbank|Multi-Modell-Datenbankverwaltungssystem]] (DBMS), das relationale, graphenbasierte, vektorielle, dokumentenorientierte, geospatiale und zeitreihenbasierte Speicherung in einem einzigen System vereint.[^readme] Das System ist in [[C++|C++17/20]] implementiert und enthält eine native Laufzeitumgebung für [[Großes Sprachmodell|große Sprachmodelle]] (Large Language Models, LLMs). ThemisDB steht unter der MIT-Lizenz mit einer zusätzlichen Government-Klausel.[^license]

---

## Inhaltsverzeichnis

1. [Entstehungsgeschichte](#entstehungsgeschichte)
2. [Systemarchitektur](#systemarchitektur)
3. [Abfragesprache AQL](#abfragesprache-aql)
4. [KI- und LLM-Integration](#ki--und-llm-integration)
5. [Verteilter Betrieb](#verteilter-betrieb)
6. [Sicherheit](#sicherheit)
7. [Editions-Modell](#editions-modell)
8. [Beobachtbarkeit und Betrieb](#beobachtbarkeit-und-betrieb)
9. [Performance](#performance)
10. [Entwicklung und Governance](#entwicklung-und-governance)
11. [Wissenschaftliche Einordnung](#wissenschaftliche-einordnung)
12. [Weblinks](#weblinks)
13. [Einzelnachweise](#einzelnachweise)

---

## Entstehungsgeschichte

ThemisDB wurde nicht als akademisches Projekt oder kommerzielles Produkt initiiert, sondern als Antwort auf eine konkrete gesellschaftliche Herausforderung der deutschen öffentlichen Verwaltung. Ausgangspunkt war die sogenannte **VCC-Initiative** (*Veritas, Covina, Clara*): ein souveränes, KI-gestütztes Assistenzsystem zur Entlastung von Fachkräften in der Verwaltung.[^genesis]

Der demografische Wandel in der deutschen Verwaltung – massiver Wissensabfluss durch Renteneintritte der Babyboomer-Generation bei gleichzeitig schwieriger Nachwuchsgewinnung – machte technologische Stützungssysteme notwendig. Das ursprüngliche Designziel war eine maschinell nutzbare Abbildung von Verwaltungsprozessen mit BPMN-2.0-Modellen (VPB, Verwaltungsprozessbank) kombiniert mit einem RAG-fähigen Assistenzsystem. Da keine bestehende Datenbank alle Anforderungen (Multi-Modell, ACID, LLM-Integration, Offline/Air-Gap-Betrieb) erfüllte, entstand ThemisDB als eigenständiges System.[^genesis]

Die drei VCC-Komponenten sind:

- **Veritas** – KI-gestützter agenten-basierter Assistent für Fachexperten, Wissensmanagement und Dokumentenverarbeitung
- **Covina** – Ingestion-Pipeline für heterogene Datenquellen
- **Clara** – Large-Language-Model-Verbesserung mittels LoRA-Feinabstimmung

Der Status *Production-Ready* wurde im November 2025 erreicht.

### UDS3-Architektur als Ausgangspunkt

Der Ausgangspunkt war die sogenannte *UDS3-Architektur* (Unified Data Storage 3), eine klassische polyglottes-Persistenz-Landschaft mit sechs verschiedenen Datenbankkomponenten (relationales DBMS, Dokumentendatenbank, Graph-Datenbank, Suchindex, Cache-System, Zeitreihendatenbank). Die operative Komplexität, fehlende Cross-System-Transaktionen und der hohe Wartungsaufwand motivierten die Konsolidierung in einem einheitlichen System.[^compendium_1]

---

## Systemarchitektur

### Schichtmodell

ThemisDB ist in vier konzeptionelle Schichten gegliedert, die in 55 Quellcodemodule unter `src/` implementiert sind:[^arch]

| Schicht | Inhalt |
|---|---|
| **API-Schicht** | REST (HTTP/1.1, HTTP/2), GraphQL, gRPC (mTLS), WebSocket, MQTT, PostgreSQL-Wire-Protokoll, eigenes Binärprotokoll (Wire v2), SSE (Server-Sent Events) |
| **Abfrageschicht** | AQL-Parser, kostenbasierter Abfrageoptimierer, Ausführungsplaner, semantischer Cache, Vektorkompilierung (SIMD/AVX-512) |
| **Speicherschicht** | [RocksDB](https://rocksdb.org/)-Backend ([LSM-Tree](https://en.wikipedia.org/wiki/Log-structured_merge-tree)), [MVCC](https://en.wikipedia.org/wiki/Multiversion_concurrency_control), Write-Ahead-Log (WAL), Sharding, Blob-Speicher, spaltenbasierter Cache |
| **Verteilungsschicht** | [Raft](https://raft.github.io/)- / [Paxos](https://en.wikipedia.org/wiki/Paxos_(computer_science))- / Gossip-Konsens, mTLS-Replikation, Auto-Failover, Change-Data-Capture (CDC), [2PC](https://en.wikipedia.org/wiki/Two-phase_commit_protocol) mit TrueTime-Zeitstempelgebung |

### Indexarchitektur

ThemisDB implementiert eine modulare, per [Dependency Injection](https://en.wikipedia.org/wiki/Dependency_injection) aufgebaute Indexschicht mit neun eigenständigen Indexfamilien, die alle eine einzige ACID-dauerhafte RocksDB-Persistenzschicht mit WriteBatch-Atomizität teilen:[^indexeval]

1. **B-Baum / Range-Sekundärindizes** – O(log N) Punktabfragen, Bereichsscans
2. **HNSW-Vektorindizes** – [ANN](https://en.wikipedia.org/wiki/Nearest_neighbor_search) mit Multi-GPU-Beschleunigung; geplant: ≥ 4× QPS vs. CPU-HNSW bei Recall@10 ≥ 0,95
3. **IVF+PQ-komprimierte ANN-Indizes** ([FAISS](https://github.com/facebookresearch/faiss)) – ≥ 16× VRAM-Reduktion (PQ m=8) bei Recall@10 ≥ 0,90
4. **Graph-Indizes** – Adjazenzlisten, Property-Graph, temporale Kanten, GNN-Vorbereitung
5. **R-Baum-Räumlichindizes** – MBR-Intersektionssuche, Morton-Code-Z-Kurven-Linearisierung für Geospatial
6. **TF–IDF / BM25-Volltextindizes** – invertierte Posting-Listen für hybride lexikale Suche
7. **Recursive Model Indexes (RMI)** – ersetzen B-Baum-Knoten durch stückweise lineare CDF-Modelle (*Learned Index*)
8. **Matryoshka Representation Learning (MRL)** – Dimensionstrunkierung; geplant: ≥ 2× QPS vs. vollständiger 768-D-Einbettung
9. **Adaptiver Index-Advisor** – workload-gesteuerter Indexempfehlungsdienst; geplant: ≥ 20 % p99-Latenzsenkung nach 10.000 Trainingsabfragen

Zum Vergleich unterstützen Systeme wie ArangoDB, OrientDB oder Weaviate jeweils nur eine Teilmenge dieser Indexfamilien (in der Regel 2–4), während ThemisDB alle neun Familien unter einem ACID-dauerhaften Backend vereinigt.[^indexeval]

| System | Relational | Vektor | Graph | Geospatial | Volltext | Learned | Advisor |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| ArangoDB 3.x | ✓ | begrenzt | ✓ | – | ✓ | – | – |
| OrientDB 3.x | ✓ | – | ✓ | – | – | – | – |
| Weaviate 1.x | begrenzt | ✓ | – | – | ✓ | – | – |
| Milvus 2.x | – | ✓ | – | – | – | – | – |
| **ThemisDB** | **✓** | **✓** | **✓** | **✓** | **✓** | **✓** | **✓** |

### Transaktionssystem

Das Transaktionssystem implementiert vier ISO-SQL-konforme Isolationsstufen mit SSI-Erweiterungen:[^acidrag]

| Stufe | Garantien |
|---|---|
| `READ_UNCOMMITTED` | keine; nur für unkritische Lesepfade |
| `READ_COMMITTED` | verhindert Dirty Reads; Standardfall für interaktive Abfragen |
| `REPEATABLE_READ / Snapshot` | MVCC-Snapshot-Isolation |
| `SERIALIZABLE` | [SSI](https://en.wikipedia.org/wiki/Snapshot_isolation#Serializable_snapshot_isolation) mit Prädikatsperrung; höchste Korrektheitsstufe |

Für shard-übergreifende Transaktionen verwendet ThemisDB ein [Zweiphasen-Commit-Protokoll (2PC)](https://en.wikipedia.org/wiki/Two-phase_commit_protocol) mit *TrueTime*-basierter externer Konsistenz: Der Koordinator weist den Commit-Zeitstempel über die TrueTime-Spätschranke zu und wartet, bis dieser sicher in der Vergangenheit liegt, bevor das COMMIT an alle Teilnehmer gesendet wird.[^disttx]

Für lang laufende verteilte Geschäftsprozesse ist das [SAGA-Muster](https://en.wikipedia.org/wiki/Compensating_transaction) mit Kompensationsworkflows implementiert.[^arch]

---

## Abfragesprache AQL

ThemisDB stellt eine eigene Abfragesprache **AQL** (*Abfragesprache*) bereit, die relationale, Graph-, Vektor- und Geospatial-Operatoren in einem einheitlichen Syntax kombiniert. Ein LLM-gestützter *NL-to-AQL-Handler* übersetzt natürlichsprachliche Anfragen automatisch in AQL-Ausdrücke.[^arch]

Ein *LLMAQLEmbeddingBridge*-Adapter (eingeführt April 2026) verbindet den AQL-Handler mit beliebigen Embedding-Providern und ermöglicht semantische Abfragen direkt aus AQL-Ausdrücken heraus.[^embbridge]

Der AQL-Referenz-Cheatsheet ist im Kompendium als eigener Anhang dokumentiert.[^compendium_aql]

---

## KI- und LLM-Integration

### Eingebettete Inferenzumgebung

ThemisDB enthält eine eingebettete LLM-Laufzeitumgebung, die über eine Plugin-Abstraktion (`ILLMPlugin`) mehrere Backends unterstützt. Der primäre Pfad ist [llama.cpp](https://github.com/ggerganov/llama.cpp) (MIT-lizenziert); weitere evaluierte Backends umfassen ONNX Runtime GenAI, PowerInfer und mistral.rs.[^llmengine]

Die Designprinzipien der LLM-Integration sind:[^llmengine]

- Kein API-zentriertes Primärmodell – ausschließlich In-Process-Engines
- Tiefe C++-Integration ohne externe Laufzeitabhängigkeiten
- Betrieb in Air-Gapped-Umgebungen (ohne Netzverbindung)
- MIT-kompatible Lizenzkette durch alle Abhängigkeiten

Evaluierte In-Process-Backends:

| Backend | Lizenz | Besonderheit |
|---|---|---|
| llama.cpp | MIT | Primärpfad; GGUF-Format, llama.cpp-CUDA/Vulkan |
| ONNX Runtime GenAI | MIT | DirectML/TensorRT-Pfade; ONNX-CLIP-Integration vorhanden |
| PowerInfer | MIT | Neuronales Sparse-Activation; niedrige VRAM-Last |
| mistral.rs | MIT | Rust-basiert; hohe Throughput-Dichte |
| Candle | MIT / Apache-2.0 | Rust; GPU via CUDA/Metal |

### RAG-Pipeline und ACID-Kopplung

Die Retrieval-Augmented-Generation (RAG)-Pipeline kombiniert vektorielle und lexikale Suche (BM25 + RRF-Rankfusion) mit ACID-Transaktionssemantik. Im Unterschied zu Standard-RAG-Frameworks (LangChain, LlamaIndex), die Retrieval und Transaktionen orthogonal behandeln, implementiert ThemisDB einen expliziten **ACID-RAG-Vertrag**:[^acidrag]

- **MVCC-Snapshots** für transaktionale Dokumentversionierung (verhindert *Stale Reads* während RAG-Abfragen)
- **Isolationsschicht-bewusstes Retriever Fusion** (BM25 + Vektorindex im gemeinsamen Konsistenzzustand)
- **Serialisierbarer LLM-Aufruf** mit Prädikats-Bereichssperrung

Gemessene Latenzrichtwerte (v1.8.2):

| Pfad | Latenz |
|---|---|
| OCC-Transaktionspfad | < 100 µs (p50) |
| SSI-Transaktionspfad | < 5 ms (p99) |
| RAG-Urteilspipeline FAST-Modus | < 150 ms |
| RAG-Urteilspipeline BALANCED-Modus | < 600 ms |

### Adaptives Fine-Tuning mit LoRA

ThemisDB behandelt LoRA-/QLoRA-Adapter nicht nur als Trainingsartefakte, sondern als **Runtime-Operationen** mit eigenen Lebenszykluszuständen: Registrierung, Hot-Swap, Canary-Rollout, Rollback und Audit.[^lorapaper]

Ein Policy-Gate verhindert unkontrollierte Promotionen und kann automatisch Rollbacks auslösen, wenn Qualitätsschwellenwerte unterschritten werden. Der *LoRAFederationCoordinator* implementiert einen **Gradient-Outlier-Filter** (L2-Norm-basiert) zur Erkennung vergifteter Gradienten im föderativen Lernbetrieb.[^fedlearning]

### Gossip-gesteuertes LoRA-Routing

In verteilten Deployments akkumulieren verschiedene Shards unterschiedliche LoRA-Spezialisierungen. ThemisDB implementiert ein **Gossip-gesteuertes Domain-Routing**, bei dem Shards kontinuierlich Adapter-Fähigkeitssignale (*Capability Announcements*) über das Gossip-Protokoll publizieren, und ein kostenbasierter Router Anfragen nach einem kombinierten Qualitäts-Latenz-Ziel weiterleitet.[^gossiplora]

### Föderative Wissensdestillation

Der *FederatedDistillationCoordinator* implementiert Lehrer-zu-Schüler-Softlabel-Transfer mit **Differential Privacy** (Gaussian-DP-Rauschen) und vollständiger ε-Budget-Buchführung pro Runde und Lebensdauer. Ein *PolicyGate* verhindert Budget-Erschöpfung; ein *RollbackTrigger* kann abgeschlossene Runden rückgängig machen.[^feddistillation]

Bedrohungsmodell (T-1 bis T-6, Threat Model v1.0.0):[^feddistillation]

| Asset | Sensitivität |
|---|---|
| Soft Labels (Lehrer-Logits) | Mittel – offenbart Entscheidungsgrenzen des Modells |
| LoRA-Gradienten | Hoch – können Trainingsdata-Strukturen enkodieren |
| Privacy-Budget (ε) | Hoch – Erschöpfung ermöglicht Rekonstruktionsangriffe |
| Studentenadapter-Gewichte | Hoch – aus privaten Daten erlernt |
| Audit-Log | Mittel – offenbart Föderations-Topologie |

### Ethische KI

Das Modul `ethics_ai` implementiert eine fünfdimensionale Entscheidungsbewertung (*EthicsEvaluator*, *EthicalDiscourseEngine*) sowie einen RAG-Kontextabruf für ethisch relevante Anfragen. Es ermöglicht die automatische Markierung problematischer LLM-Ausgaben nach konfigurierbaren Ethik-Dimensionen.[^arch]

---

## Verteilter Betrieb

### Konsens und Replikation

ThemisDB unterstützt drei Konsensalgorithmen: [Raft](https://raft.github.io/), [Paxos](https://en.wikipedia.org/wiki/Paxos_(computer_science)) und Gossip. Die shard-übergreifende Replikation erfolgt über mTLS-verschlüsselte Kanäle. Der Auto-Failover-Manager und der *DisasterRecoveryOrchestrator* ermöglichen automatische Wiederherstellung ohne manuelle Eingriffe.[^arch]

### Change-Data-Capture

Ein eigenständiges CDC-Modul (`ChangeFeed`, `ChangeBuffer`) ermöglicht das Abonnieren von Datenstromänderungen über SSE-Verbindungen. Pro Tenant und Verbindung werden Rate-Limits durchgesetzt.[^arch]

### Wissenschaftliche Grundlagen des verteilten Designs

Das Architekturdokument *ThemisDB as a Distributed ACID Multi-Model AI Database* definiert einen formalen Bewertungsrahmen für die Trade-offs zwischen Konsistenz, Skalierbarkeit und KI-Workload-Performance und wurde für arXiv (cs.DB/cs.DC) eingereicht.[^distpaper]

Forschungshypothesen (H1, H2):[^distpaper]

> H1: Mixed transaction+AI workloads can preserve strict correctness while meeting latency SLOs within a bounded coordination-overhead regime.

> H2: Transition-window overhead after failures dominates steady-state overhead and must be modeled explicitly for realistic SLO planning.

---

## Sicherheit

### Transportschicht und Authentifizierung

- TLS 1.2/1.3 auf allen Protokollpfaden
- JWT-Tokens, GSSAPI (Kerberos) und Multi-Faktor-Authentifizierung
- Rollenbasierte Zugangskontrolle (RBAC) mit per-Tenant-Ressourcenquoten
- mTLS-Pflicht für gRPC und Replikationskanäle (Fail-Closed)

### Datenverschlüsselung

- **Feldverschlüsselung** auf Basis von AES-256-GCM (ENTERPRISE/HYPERSCALER)
- **Verschlüsselter Benutzerspeicher** über GocryptFS-Backend mit Argon2id-KDF und Schlüsselrotation
- HSM-Integration für Schlüsselverwaltung
- eIDAS-konformes Zeitstempeln über TSA (Time-Stamping Authority)

### Audit und PII-Schutz

- Strukturiertes Audit-Logging mit Ereignisklassen (u. a. `UNAUTHORIZED_ACCESS`)
- PII-Erkennung und Redaktionsrichtlinien (`PiiDetector`, `PiiRedactionPolicy`)
- GPG-gestützte Modul-Signaturprüfung über fork+execvp (kein Shell-Injection-Risiko)[^secfixes]

---

## Editions-Modell

ThemisDB wird in fünf Editionen ausgeliefert, die zur CMake-Build-Zeit ausgewählt werden. Die Funktionsumfänge sind geschachtelt: MINIMAL ⊂ COMMUNITY ⊂ ENTERPRISE ⊂ HYPERSCALER.[^editions]

| Edition | Anwendungsfall | Lizenz | Binärgröße | Max. Knoten | SLA |
|---|---|---|---|---|---|
| **MINIMAL** | Eingebettet / IoT / Edge | MIT | ~30–50 MB | 1 | – |
| **COMMUNITY** | Open-Source, selbst gehostet, Startups | MIT | ~80–150 MB | 1 | – |
| **ENTERPRISE** | Kommerziell, SLA-gebunden, 2–100 Knoten | Kommerziell | ~150–250 MB | 100 | 99,9 % |
| **MILITARY** | Abgehärtet, Air-Gapped | Kommerziell | ~150–250 MB | 100 | nach Vertrag |
| **HYPERSCALER** | Cloud/OEM, Kubernetes-Operator, cross-region | Kommerziell | ~200–300 MB | unbegrenzt | 99,99 %+ |

Horizontales Sharding, Replikation und Multi-Master-Betrieb sind ausschließlich in den ENTERPRISE- und HYPERSCALER-Editionen enthalten. LLM-Integration ist ab COMMUNITY optional verfügbar; GPU-Beschleunigung mit CUDA/Vulkan/HIP/OpenCL ist in allen Editionen ab COMMUNITY vorhanden.[^editions]

Feature-Übersicht nach Edition:

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---|:---:|:---:|:---:|:---:|
| ACID-Transaktionen | ✅ | ✅ | ✅ | ✅ |
| Vector Search (CPU) | ✅ Basic | ✅ Full | ✅ Full | ✅ Advanced |
| LLM-Integration | ❌ | ✅ Optional | ✅ Optional | ✅ Full |
| GPU-Beschleunigung | ❌ | ✅ Optional | ✅ Full | ✅ Multi-GPU |
| Horizontales Sharding | ❌ | ❌ | ✅ | ✅ |
| Replikation | ❌ | ❌ | ✅ | ✅ |
| Kubernetes-Operator | ❌ | ❌ | ❌ | ✅ |
| RBAC | ❌ | ❌ | ✅ | ✅ |
| HSM-Support | ❌ | ❌ | ✅ | ✅ |
| SIEM-Integration | ❌ | ❌ | ✅ | ✅ |

---

## Beobachtbarkeit und Betrieb

ThemisDB integriert [Prometheus](https://prometheus.io/)-Metriken, [OpenTelemetry](https://opentelemetry.io/)-Tracing und Grafana-Dashboards ab der COMMUNITY-Edition. Der *DatabaseMaintenanceOrchestrator* koordiniert MVCC-Bereinigung, Storage-Kompaktierung und Backup-Zeitpläne über ein zentrales Cron-/Wartungsfenster-System.[^arch] PagerDuty- und Slack-Alerting sowie SIEM-Integration sind ab ENTERPRISE verfügbar.

---

## Performance

Die folgende Tabelle zeigt Messwerte aus dem internen Performance-Bericht (v1.8.2, April 2026, x64-Hardware, 20 Kerne @ 3,7 GHz, AVX2/AVX-512):[^perf]

| Metrik | Messwert | Ziel | Status |
|---|---|---|---|
| Graphkanten-Operationen | 1.177.000/s | 1.000.000/s | ✅ erfüllt |
| Zeitreihen-Insert | 61,0 Mio. Punkte/s | 60 Mio./s | ✅ erfüllt |
| Abfrage-p99-Latenz | 9,67 ms | < 50 ms | ✅ erfüllt |
| Write-Throughput (sustained) | ~45.000 WPS | 45.000 WPS | ✅ erfüllt |
| Read-Throughput | ~120.000 RPS | – | – |
| Sekundärindex-Insert | 254.900/s | 1.000.000/s | ❌ Lücke |
| Abfrage-Engine-Spitzendurchsatz | 796,4 Mio./s | 900 Mio./s | ❌ Lücke |

Die Benchmarks folgen dem CHIMERA-Framework (IEEE Std 2807-2022) sowie TPC-C- und YCSB-Standardlastprofilen. GPU-abhängige Workloads erfordern dedizierte Hardware und sind nicht in obigen Werten enthalten.[^perf]

---

## Entwicklung und Governance

ThemisDB wird quelloffen auf GitHub entwickelt. Die Governance-Struktur unterscheidet zwischen Nutzern, Contributors und Modul-Maintainern; strategische Entscheidungen obliegen einem Project Lead.[^gov]

Leitprinzipien:

- **Korrektheit vor Geschwindigkeit** – ACID-Garantien haben Vorrang vor Durchsatzoptimierungen
- **Rückwärtskompatibilität** – Breaking Changes erfordern MAJOR-Versionsbump und Migrationsleitfaden
- **Sicherheit-first** – Sicherheitsmeldungen werden als P0 behandelt; vertrauliche Offenlegung erwünscht
- **Dokumentationsparität** – jede API-Änderung muss zeitgleich dokumentiert werden[^gov]

Das Kompendium (`compendium/docs/`) umfasst 43 Kapitel (Kapitel 0–40 sowie Anhänge A–I) und ist als technisches Referenzwerk mit Literaturverzeichnis im IEEE-Zitierstil gestaltet. Referenzierte Standardwerke umfassen u. a. Arbeiten von Stonebraker/Cetintemel zu Multi-Modell-Datenbanken (ICDE 2005), O'Neil et al. zum LSM-Tree (1996) sowie Cahill et al. zur serialisierbaren Snapshot-Isolation (ACM TODS 2009).[^lit]

---

## Wissenschaftliche Einordnung

Mehrere Forschungsentwürfe des ThemisDB-Projekts befassen sich mit Fragestellungen, die in der Datenbankforschung bislang isoliert behandelt wurden:

| Arbeit | Venue | Kernbeitrag |
|---|---|---|
| *ThemisDB as a Distributed ACID Multi-Model AI Database* | arXiv cs.DB/cs.DC | Formaler Rahmen für ACID + Multi-Modell + KI in einer Laufzeit[^distpaper] |
| *ThemisDB: A Multi-Model Database with Individualized Index Families* | VLDB 2027 / arXiv cs.DB | 9-Indexfamilien-Taxonomie, reproduzierbares Evaluationsprotokoll[^indexeval] |
| *ACID-Constrained RAG* | VLDB/SIGMOD | Formaler ACID-RAG-Vertrag; gemessene Isolations-Overhead-Kurven[^acidrag] |
| *Cost-Aware Hybrid ANN Retrieval* | arXiv cs.DB/cs.IR | Planerauswahl über Vektor-/lexikale/Graph-Operatoren[^annpaper] |
| *Operating LoRA/QLoRA in a Multi-Model DB* | arXiv cs.DB/cs.LG/cs.DC | Adapter-Lifecycle-Protokoll unter Datenbankbedingungen[^lorapaper] |
| *Gossip-Driven LoRA Domain Routing* | Middleware / ICDCS | Gossip-gesteuertes Domain-Routing mit Capability Announcements[^gossiplora] |

---

## Weblinks

- [Offizielles Repository (GitHub)](https://github.com/makr-code/ThemisDB)
- [Projektdokumentation](https://makr-code.github.io/ThemisDB/)

---

## Einzelnachweise

[^readme]: ThemisDB Repository: `README.md`. GitHub, abgerufen April 2026.
[^license]: ThemisDB Repository: `LICENSE`. GitHub, abgerufen April 2026.
[^genesis]: ThemisDB Kompendium: `compendium/docs/chapter_00_genesis.md` – *Genese und Entwicklungsgeschichte*. Abgerufen April 2026.
[^compendium_1]: ThemisDB Kompendium: `compendium/docs/chapter_01_introduction.md` – *Die Multi-Model-Herausforderung*. Abgerufen April 2026.
[^arch]: ThemisDB Repository: `ARCHITECTURE.md`. GitHub, abgerufen April 2026.
[^indexeval]: ThemisDB Research: `research/THEMIS_MULTIMODEL_INDEX_EVALUATION_V2.md` v1.0 – arXiv-Entwurf cs.DB (VLDB 2027). Abgerufen April 2026.
[^acidrag]: ThemisDB Research: `research/ACID_CONSTRAINED_RAG_DRAFT.md` v0.1 – VLDB/SIGMOD-Entwurf. Abgerufen April 2026.
[^disttx]: ThemisDB Documentation: `docs/DISTRIBUTED_TRANSACTIONS.md`. Abgerufen April 2026.
[^embbridge]: ThemisDB Code: `include/aql/llm_aql_embedding_bridge.h` + `src/aql/llm_aql_embedding_bridge.cpp`. Commit 48a6c9efc4, 2026-04-22.
[^compendium_aql]: ThemisDB Kompendium: `compendium/docs/appendix_f_aql_cheatsheet.md`. Abgerufen April 2026.
[^llmengine]: ThemisDB Documentation: `docs/LLM_ENGINE_AUSARBEITUNG.md`. Stand 2026-04-18.
[^lorapaper]: ThemisDB Research: `research/LORA_QLORA_DATABASE_NATIVE_OPERATIONS_PAPER_DRAFT.md` v0.1 – arXiv-Entwurf cs.DB/cs.LG/cs.DC. Abgerufen April 2026.
[^fedlearning]: ThemisDB Code: `include/distributed_knowledge/lora_federation_coordinator.h`. Commit 28a6751350, 2026-04-21.
[^gossiplora]: ThemisDB Research: `research/GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md` v0.2 – Middleware/ICDCS-Entwurf. Abgerufen April 2026.
[^feddistillation]: ThemisDB Code + Docs: `include/distributed_knowledge/federated_distillation_coordinator.h`; `docs/en/security/FEDERATED_DISTILLATION_THREAT_MODEL.md`. Commit 0d906cf908, 2026-04-21.
[^distpaper]: ThemisDB Research: `research/DISTRIBUTED_ACID_MULTIMODEL_AI_DATABASE_PAPER_DRAFT.md` v0.1 – arXiv-Entwurf cs.DB/cs.DC. 2026-04-19.
[^annpaper]: ThemisDB Research: `research/HYBRID_ANN_RETRIEVAL_SYSTEMS_PAPER_DRAFT.md` v0.1 – arXiv-Entwurf cs.DB/cs.IR. 2026-04-19.
[^secfixes]: ThemisDB Code: `src/base/module_loader.cpp`. Commit 2026-04-21.
[^editions]: ThemisDB Documentation: `docs/EDITION_COMPARISON.md` v1.5.0-dev. Abgerufen April 2026.
[^perf]: ThemisDB Repository: `PERFORMANCE_EXPECTATIONS.md` v2.0. Stand 2026-04-13.
[^gov]: ThemisDB Repository: `GOVERNANCE.md` v1.0. Stand 2026-04-13.
[^lit]: ThemisDB Kompendium: `compendium/docs/appendix_literatur.md` – IEEE-Literaturverzeichnis. Abgerufen April 2026.

---

<!-- Kategorien (für de.wikipedia.org) -->
<!-- [[Kategorie:Datenbankmanagementsystem]] -->
<!-- [[Kategorie:Freie Datenbankensoftware]] -->
<!-- [[Kategorie:C++-Bibliothek]] -->
<!-- [[Kategorie:Open-Source-Software]] -->
<!-- [[Kategorie:Maschinelles Lernen]] -->
<!-- [[Kategorie:Verteiltes System]] -->

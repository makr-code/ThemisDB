# State of the Art — 2026 Q1

**Period:** January – March 2026  
**Author:** ThemisDB Research Team  
**Status:** ✅ Complete

---

## Areas Reviewed

- [x] Relational Databases & SQL Engines
- [x] Vector Search & ANN Indexes
- [x] Graph Databases & Graph Algorithms
- [x] Temporal / Timeline Databases
- [x] Process Databases & Process Mining
- [x] Verwaltungs-IT / Administrative IT (Deutschland)
- [x] LLM Integration & RAG
- [x] LoRA & Parameter-Efficient Fine-Tuning (PEFT)
- [x] Prompt Engineering
- [x] Storage Engines & LSM-Trees
- [x] Distributed Systems & Consensus
- [x] GPU-Accelerated Operations
- [x] Security & Encryption
- [x] Query Optimization

---

## Key Findings

---

### Relational Databases & SQL Engines

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Raasveldt & Mühleisen (2019) — *DuckDB: An Embeddable Analytical Database* (SIGMOD 2019) | Conference Paper | Columnar, vectorized, in-process OLAP engine outperforms Postgres/SQLite on analytical workloads by 10–100× | Inspiration for ThemisDB's analytical query path and columnar storage layout |
| Stonebraker & Hellerstein (2005) — *What Goes Around Comes Around* (MIT, Red Book 5th ed.) | Book Chapter | Historical survey of 35 years of data models; argues that relational SQL will survive all challengers | Validates ThemisDB's SQL compatibility layer |
| Neumann (2011) — *Efficiently Compiling Efficient Query Plans for Modern Hardware* (VLDB) | Conference Paper | Query compilation (LLVM JIT) vs interpretation; HyPer DBMS compiles queries to native machine code | Roadmap item: JIT-compiled query execution for ThemisDB |
| Armbrust et al. (2021) — *Lakehouse: A New Generation of Open Platforms* (CIDR 2021) | Conference Paper | Merges data lake flexibility with data warehouse performance (Delta Lake, Apache Iceberg) | ThemisDB storage layer: Delta-style MVCC file formats |
| Pavlo et al. (2017) — *Self-Driving Database Management Systems* (CIDR 2017) | Conference Paper | Autonomous tuning, cost model learning, workload forecasting — NoisePage/OtterTune prototype | ThemisDB self-tuning roadmap; adaptive cache & index selection |
| MySQL 8.x / PostgreSQL 16 release notes (2023–2024) | Documentation | Parallel query improvements, JSON path indexing, generated columns, partitioning enhancements | Compatibility targets for ThemisDB PostgreSQL wire protocol |
| Apache Arrow specification (2016–) | Specification / Website | Language-agnostic columnar in-memory format; zero-copy inter-process data exchange | ThemisDB columnar serialization for bulk export/import |
| SQLite3 WASM (2022–) — https://sqlite.org/wasm | Website | Embeddable SQL in browsers via WebAssembly | ThemisDB minimal-edition / edge deployment |

**Books:**
- *Database System Concepts* — Silberschatz, Korth & Sudarshan (7th ed., 2020), McGraw-Hill — standard reference for query processing, transaction management, storage
- *Designing Data-Intensive Applications* — Kleppmann (2017), O'Reilly — practical patterns for distributed data systems; widely cited in ThemisDB design decisions
- *The Red Book: Readings in Database Systems* (5th ed., 2015), Bailis, Hellerstein & Stonebraker (eds.) — curated collection of foundational papers

**Gaps identified:**
- ThemisDB lacks a JIT-compiled query execution path; HyPer/DuckDB style query compilation is a high-priority future enhancement
- No vectorized execution engine (batch-at-a-time model); current row-at-a-time interpretation limits analytical throughput

---

### Vector Search & ANN Indexes

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Malkov & Yashunin (2020) — *Efficient and Robust Approximate Nearest Neighbor Search Using Hierarchical Navigable Small World Graphs* (IEEE TPAMI) | Journal Paper | HNSW achieves state-of-the-art recall/throughput on billion-scale datasets; O(log n) search | Already implemented in `src/index/` |
| Subramanya et al. (2019) — *DiskANN: Fast Accurate Billion-point Nearest Neighbor Search on a Single Node* (NeurIPS 2019) | Conference Paper | SSD-resident graph index; beats HNSW on memory-constrained billion-scale search | Roadmap: SSD-resident index tier for ThemisDB |
| Guo et al. (2020) — *Accelerating Large-Scale Inference with Anisotropic Vector Quantization* (ICML 2020) — ScaNN | Conference Paper | Anisotropic quantization reduces quantization error for inner-product search by 30% | Vector compression module enhancement |
| Johnson et al. (2021) — *Billion-Scale Similarity Search with GPUs* (IEEE Big Data) — FAISS | Journal Paper | GPU-accelerated exact + approximate search; IVF-PQ as baseline for billion-scale | ThemisDB GPU vector index path |
| Babenko & Lempitsky (2014) — *Efficient Indexing of Billion-Scale Datasets of Deep Descriptors* (CVPR) | Conference Paper | Product Quantization (PQ) baselines; shows memory vs recall trade-off | Product Quantizer implementation in ThemisDB |
| Kusupati et al. (2022) — *Matryoshka Representation Learning* (NeurIPS 2022) | Conference Paper | Nested embeddings allow adaptive precision at query time (truncation to shorter dimensions) | Planned v1.4.1+: adaptive embedding truncation |
| pgvector (2021–) — https://github.com/pgvector/pgvector | Open Source | IVFFlat + HNSW extension for PostgreSQL; standard for SQL+vector hybrid queries | Compatibility reference for ThemisDB SQL+vector queries |
| Weaviate, Qdrant, Milvus white papers (2022–2025) | Technical Reports | Dedicated vector database architectures; hybrid scalar+vector filtering; multi-tenancy | Competitive analysis; ThemisDB aims to unify all data models |

**Books & Survey Papers:**
- Wang et al. (2021) — *A Comprehensive Survey and Experimental Comparison of Graph-Based Approximate Nearest Neighbor Search* (VLDB 2021) — exhaustive benchmark of ANN algorithms
- Bernhardsson (2018) — *ANN Benchmarks* — https://ann-benchmarks.com — live benchmark suite, widely referenced

**Gaps identified:**
- DiskANN-style SSD-resident index not yet implemented; critical for billion-scale deployments without GPU
- Matryoshka truncation partially planned (v1.4.1); needs implementation

---

### Graph Databases & Graph Algorithms

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Robinson, Webber & Eifrem (2015) — *Graph Databases* (2nd ed., O'Reilly) | Book | Foundational text on property graph model, Neo4j architecture, Cypher | Design reference for ThemisDB graph layer |
| Besta et al. (2023) — *Graph of Thoughts: Solving Elaborate Problems with Large Language Models* (AAAI 2024) | Conference Paper | Graphs as LLM reasoning structures; non-linear thought chains with merging/branching | ThemisDB graph+LLM integration; Graph-of-Thought RAG |
| Hamilton, Ying & Leskovec (2017) — *Inductive Representation Learning on Large Graphs* (NeurIPS) — GraphSAGE | Conference Paper | Generalizable node embeddings via neighborhood sampling; inductive (works on unseen nodes) | `src/graph/` GNN-based indexing roadmap |
| Kipf & Welling (2017) — *Semi-Supervised Classification with Graph Convolutional Networks* (ICLR) — GCN | Conference Paper | Graph Convolutional Networks; neighborhood aggregation → node classification | GNN embedding layer in ThemisDB |
| Velickovic et al. (2018) — *Graph Attention Networks* (ICLR) | Conference Paper | Attention-weighted neighbor aggregation; better than uniform GraphSAGE for heterogeneous graphs | Heterogeneous graph support in ThemisDB |
| Francis et al. (2018) — *Cypher: An Evolving Query Language for Property Graphs* (SIGMOD 2018) | Conference Paper | Formal semantics of Cypher; GQL ISO standard precursor | ThemisDB AQL design influenced by GQL/Cypher |
| ISO/IEC 39075:2024 — GQL Standard | Standard | First international standard for property graph query language (ratified 2024) | ThemisDB AQL compliance target |
| Shao et al. (2013) — *Trinity: A Distributed Graph Engine on a Memory Cloud* (SIGMOD) | Conference Paper | Distributed in-memory property graph with key-value backbone | Distributed graph query planning in ThemisDB |
| Apache TinkerPop / Gremlin (2015–) — https://tinkerpop.apache.org | Open Source Framework | De-facto standard graph traversal language; used by Amazon Neptune, JanusGraph | ThemisDB Gremlin compatibility layer consideration |
| Leskovec & Sosic (2016) — *SNAP: A General-Purpose Network Analysis and Graph-Mining Library* | Software/Paper | Community detection, centrality, link prediction algorithms | Algorithm library reference for ThemisDB analytics module |

**Conferences to monitor:** SIGMOD, VLDB, WWW (WebConf), ICDM, KDD

**Gaps identified:**
- GNN-based node embeddings are planned but not yet fully implemented
- No GQL/ISO 39075 compliance; Cypher/Gremlin compatibility is partial

---

### Temporal / Timeline Databases

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Kulkarni & Michels (2012) — *Temporal Features in SQL:2011* (SIGMOD Record) | Journal Article | Official SQL:2011 standard: period types, SYSTEM_TIME, APPLICATION_TIME, temporal predicates | Basis for ThemisDB temporal module SQL compliance |
| Snodgrass (1995) — *Developing Time-Oriented Database Applications in SQL* (Morgan Kaufmann) | Book | Comprehensive guide to bi-temporal data modeling; TSQL2 reference | Standard reference for bi-temporal implementation |
| Böhlen & Jensen (1996) — *The Consensus Glossary of Temporal Database Concepts* (LNCS) | Workshop Paper | Precise definitions: valid time, transaction time, bi-temporal | Terminology normalization in ThemisDB temporal module |
| Johnston & Weis (2010) — *Managing Time in Relational Databases* (Morgan Kaufmann) | Book | Practical bi-temporal schema patterns; sequenced vs non-sequenced semantics | Design patterns for ThemisDB temporal storage |
| TimescaleDB papers (2018–2024) — https://docs.timescale.com/about/latest/timescaledb-editions/ | Technical Documentation | Automatic partitioning (hypertables), continuous aggregates, compression for time-series data | ThemisDB timeseries module architecture reference |
| Brewer et al. (2016) — *Spanner: Becoming a SQL System* (SIGMOD 2017) — Google Spanner | Conference Paper | TrueTime API for globally consistent timestamps; external consistency at scale | ThemisDB distributed timestamp service design |
| Lomet et al. (2009) — *Transaction Time Support Inside a Database Engine* (ICDE) | Conference Paper | Embedding transaction-time versioning inside the storage engine (not as add-on) | ThemisDB MVCC + temporal integration |
| InfluxDB 3.0 (2024) — https://www.influxdata.com | Technical Documentation | Apache Arrow + DataFusion-based rewrite; InfluxQL + SQL hybrid | Competitive analysis for ThemisDB timeseries storage |

**Gaps identified:**
- SQL:2011 `FOR SYSTEM_TIME AS OF` queries not fully covered in ThemisDB temporal module
- No continuous aggregate materialization (TimescaleDB-style) yet implemented

---

### Process Databases & Process Mining

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| van der Aalst et al. (2012) — *Process Mining: Discovery, Conformance and Enhancement of Business Processes* (Springer) | Book | Definitive text on process mining; α-algorithm, conformance checking, enhancement | Algorithmic foundation for ThemisDB process module |
| van der Aalst (2016) — *Process Mining: Data Science in Action* (2nd ed., Springer) | Book | Updated coverage of Petri nets, BPMN, case management; ProM framework | ThemisDB process graph implementation reference |
| Dumas, La Rosa et al. (2018) — *Fundamentals of Business Process Management* (2nd ed., Springer) | Book | BPMN 2.0 modeling, process simulation, process analytics | BPMN 2.0 compliance for ThemisDB process-DB |
| Leemans, Fahland & van der Aalst (2013) — *Discovering Block-Structured Process Models from Event Logs* (Petri Nets) | Conference Paper | IMf (Inductive Miner infrequent) algorithm; noise-tolerant process discovery | Process discovery algorithm for ThemisDB |
| OMG BPMN 2.0.2 Standard (2014) — https://www.omg.org/spec/BPMN/2.0.2/ | Standard | International standard for business process notation; event types, gateways, subprocesses | ThemisDB process-DB schema target |
| van der Aalst et al. (2015) — *Conformance Checking: Relating Processes and Models* (Springer) | Book | Alignment-based conformance checking between event logs and process models | Conformance checking feature in ThemisDB process module |
| IEEE Task Force on Process Mining (2012) — *Process Mining Manifesto* (LNBIP) | Workshop Paper | Community consensus on process mining goals, challenges, and maturity levels | Research positioning for ThemisDB process-DB |
| ProM Framework — http://www.promtools.org | Open Source | De-facto research platform for process mining algorithms; 350+ plugins | Reference implementation for ThemisDB process algorithms |
| Burattin (2015) — *Process Mining Techniques in Business Environments* (Springer) | Book | Online process mining (streaming), scalability, IoT event streams | Streaming process analysis in ThemisDB CDC + process integration |
| Celonis Process Intelligence Platform — https://www.celonis.com | Commercial Reference | Industry leader; execution management with process graphs + ML | Competitive analysis; ThemisDB process-DB scope |

**Conferences to monitor:** BPM (Business Process Management Conference), ICPM (International Conference on Process Mining), CAISE

**Gaps identified:**
- No automated process discovery (α-algorithm / Inductive Miner) in ThemisDB yet
- BPMN 2.0 wire protocol exists but conformance checking not implemented

---

### Verwaltungs-IT / Administrative IT (Deutschland)

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Onlinezugangsgesetz (OZG) 2017 + OZG-Änderungsgesetz 2024 | Law / Standard | German federal law mandating online access to all administrative services; 575 service types | ThemisDB Verwaltungs-IT module must support OZG service schemas |
| XDOMEA 3.0 — https://www.xdomea.de | Standard | Standard for electronic records management (eAkte); XML schema for document exchange between German public authorities | ThemisDB document import/export compatibility |
| FIM — Föderales Informationsmanagement — https://fimportal.de | Standard | Standardized data field catalog for German administrative forms (FIM-Leistungen, FIM-Daten) | ThemisDB schema registry: FIM data fields as first-class schema types |
| FITKO (Föderale IT-Kooperation) — https://www.fitko.de | Organization / Website | Federal IT cooperation body coordinating OZG implementation; provides Verwaltungscloud, API standards | Integration target for ThemisDB in German e-government context |
| eID-Server BSI TR-03130 — https://www.bsi.bund.de/TR-03130 | Technical Guideline | German eID online authentication; AusweisApp2 / eID-Client protocols | ThemisDB auth module: eID integration for citizen identity |
| ELSTER Schnittstelle (ERiC) — https://www.elster.de/elsterweb/infoseite/entwickler | Technical Reference | German tax authority API for electronic tax filing; XML/SOAP based | ThemisDB ingestion: ELSTER data source connector |
| OSCI Transport 2.0 — https://www.xoev.de/osci-transport-14168 | Standard | Secure transport layer for German e-government messages; end-to-end encryption | ThemisDB network layer: OSCI compatibility |
| XÖV-Standards — https://www.xoev.de | Standard Family | Family of German administrative XML standards: XPersonenstand, XMeld, XKfz, XBau, etc. | ThemisDB schema library: XÖV data model support |
| GOVDIGI / Registermodernisierung 2025 | Law / Program | German Registermodernisierungsgesetz; unified citizen identifier (SVNR) across government registers | ThemisDB multi-source identity resolution feature |
| BITKOM (2024) — *Digitalisierungsindex der deutschen Wirtschaft* | Industry Report | Annual digital maturity report; public sector lags by 2.3 years vs private sector | Positioning context for ThemisDB in German public sector |
| BMI Nationale E-Government-Strategie (NEGS) 2030 — https://www.bmi.bund.de | Government Strategy | Federal strategy for German digital government to 2030; cloud-first, API-first, once-only principle | Strategic alignment for ThemisDB Verwaltungs-IT roadmap |
| Lucke & Reinermann (2000) — *Speyerer Definition von Electronic Government* | Academic Paper | Foundational German definition of e-government; still referenced in legal/policy contexts | Academic framing for ThemisDB Verwaltungs-IT feature descriptions |

**Key Websites:**
- https://www.verwaltung-innovativ.de — BMI program for innovation in German public administration
- https://digitalservice.bund.de — German federal digital service team; publishes open-source tools (e.g., digitalcheck)
- https://www.cio.bund.de — German federal CIO; IT planning council publications
- https://joinup.ec.europa.eu — EU joinup platform; eGovERA reference architecture; EIRA standard

**Gaps identified:**
- No OZG service schema registry in ThemisDB
- No XÖV data model import/export support
- eID authentication integration is missing
- XDOMEA document management connector absent

---

### LLM Integration & RAG

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Touvron et al. (2023) — *Llama 2: Open Foundation and Fine-Tuned Chat Models* (Meta AI) | Technical Report | Open-weight 7B–70B models; RLHF alignment; commercial license | ThemisDB default LLM backend via llama.cpp |
| Dubey et al. (2024) — *The Llama 3 Herd of Models* (Meta AI) | Technical Report | 8B/70B/405B models; grouped-query attention, RoPE, 128k context window | ThemisDB LLM upgrade path; long-context document retrieval |
| Jiang et al. (2023) — *Mistral 7B* (Mistral AI) | Technical Report | Sliding window attention, grouped-query attention; outperforms Llama 2 13B on most benchmarks | ThemisDB alternative LLM backend; efficient inference |
| Gao et al. (2023) — *Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks — Advanced Survey* (arXiv) | Survey Paper | Modular RAG architectures; query routing, reranking, iterative retrieval | ThemisDB RAG pipeline design patterns |
| Shi et al. (2023) — *REPLUG: Retrieval-Augmented Language Model Pre-Training* (NAACL) | Conference Paper | Joint optimization of retriever and LLM; REPLUG LSR improves perplexity | Advanced RAG: co-training retriever+LLM for ThemisDB domain data |
| Es et al. (2023) — *RAGAS: Automated Evaluation of Retrieval-Augmented Generation* (arXiv) | Preprint | Context precision/recall, faithfulness, answer relevance metrics | Already implemented in `src/llm/monitoring/` |
| Anthropic (2022) — *Constitutional AI: Harmlessness from AI Feedback* (arXiv) | Technical Report | RLAIF approach; principles-based safety alignment without human labelers | ThemisDB LLM safety alignment strategy |
| EU AI Act (2024) — Regulation (EU) 2024/1689 | Regulation | High-risk AI classification; transparency requirements; GPAI model obligations | ThemisDB legal compliance; model cards, audit logs required |
| Gunning et al. (2019) — *DARPA's Explainable Artificial Intelligence (XAI) Program* (AI Magazine) | Journal Article | XAI taxonomy; explanation fidelity vs. model performance trade-off | ThemisDB AI decision auditing module design |
| Lewis et al. (2020) — *Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks* (NeurIPS) | Conference Paper | Original RAG paper; DPR retriever + BART generator | Already implemented in `src/rag/` |
| ModelSpec / GGUF format (2023–) — https://github.com/ggerganov/ggml | Technical Specification | Standardized quantized model storage format for llama.cpp | ThemisDB GGUF model management |

**Conferences to monitor:** NeurIPS, ICML, ICLR, ACL, EMNLP, NAACL

**Gaps identified:**
- Co-trained retriever+LLM (REPLUG-style) not implemented; uses independent retriever
- No constitutional AI / RLAIF training pipeline yet

---

### LoRA & Parameter-Efficient Fine-Tuning (PEFT)

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Hu et al. (2022) — *LoRA: Low-Rank Adaptation of Large Language Models* (ICLR 2022) | Conference Paper | Freeze base model; inject rank-decomposition matrices A,B into attention; <1% trainable params | Already implemented in `src/llm/lora/` |
| Dettmers et al. (2023) — *QLoRA: Efficient Finetuning of Quantized LLMs* (NeurIPS 2023) | Conference Paper | 4-bit NF4 quantization + LoRA; 65B model fine-tunable on single 48GB GPU | Already implemented in `src/llm/lora/` |
| Houlsby et al. (2019) — *Parameter-Efficient Transfer Learning for NLP* (ICML 2019) — Adapter Tuning | Conference Paper | Bottleneck adapter modules inserted between Transformer layers; BERT fine-tuning with 3% of params | ThemisDB alternative PEFT strategy: adapter-based fine-tuning |
| Li & Liang (2021) — *Prefix-Tuning: Optimizing Continuous Prompts for Generation* (ACL 2021) | Conference Paper | Prepend trainable continuous vectors to key/value in attention layers; no backbone modification | ThemisDB soft-prompt tuning option |
| Liu et al. (2022) — *P-Tuning v2: Prompt Tuning Can Be Comparable to Fine-Tuning Universally* (ACL Findings) | Conference Paper | Deep prompt tuning at all layers; comparable to full fine-tuning on NLU tasks | ThemisDB prompt tuning for classification tasks |
| Pfeiffer et al. (2020) — *AdapterHub: A Framework for Adapting Transformers* (EMNLP Demo) | Conference Paper | Modular adapter composition; chain adapters for multi-task learning | ThemisDB multi-domain adapter composition |
| Ding et al. (2023) — *Delta Tuning: A Comprehensive Study of Parameter Efficient Methods for Pre-Trained Language Models* | Survey Paper | Systematic comparison: adapter vs prefix vs LoRA vs BitFit; task-type recommendations | Guides ThemisDB PEFT method selection |
| Zhao et al. (2024) — *LoRA+: Efficient Low Rank Adaptation of Large Models* (ICML 2024) | Conference Paper | Different learning rates for A and B matrices; +1–2% accuracy with same compute | ThemisDB LoRA optimizer improvement |
| Zi et al. (2023) — *Delta-LoRA: Fine-tuning High-Rank Parameters with the Delta of Low-Rank Matrices* | Preprint | Combines LoRA gradients with full-matrix weight updates; better generalization | Advanced LoRA variant for ThemisDB domain adaptation |
| HuggingFace PEFT Library — https://github.com/huggingface/peft | Open Source | Reference PEFT implementation: LoRA, QLoRA, prefix tuning, prompt tuning, AdaLoRA | ThemisDB training pipeline compatibility target |
| Databricks / MLflow (2023–) — https://mlflow.org | Open Source | Experiment tracking, model registry, LoRA adapter versioning | ThemisDB LoRA training lifecycle management |

**Books:**
- *Natural Language Processing with Transformers* — Tunstall, von Werra & Wolf (O'Reilly, 2022) — practical guide to HuggingFace; PEFT chapter

**Conferences:** NeurIPS, ICML, ICLR, ACL, EMNLP

**Gaps identified:**
- AdaLoRA (adaptive rank allocation) not implemented; could improve model quality for legal domain adaptation
- No multi-adapter composition (stacking domain-specific LoRA adapters)
- LoRA+ (asymmetric learning rates) not yet applied in ThemisDB training scripts

---

### Prompt Engineering

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Brown et al. (2020) — *Language Models are Few-Shot Learners* (NeurIPS) — GPT-3 | Conference Paper | In-context learning; few-shot prompting with k examples in context window | Already implemented in `src/prompt_engineering/` |
| Wei et al. (2022) — *Chain-of-Thought Prompting Elicits Reasoning in Large Language Models* (NeurIPS) | Conference Paper | Step-by-step reasoning in prompt; improves arithmetic/commonsense by 20–40% | CoT Builder implemented; `cot_tracer` module v1.7.0 |
| Yao et al. (2023) — *Tree of Thoughts: Deliberate Problem Solving with Large Language Models* (NeurIPS) | Conference Paper | Tree-structured search over partial solutions; backtracking; better on complex planning | ThemisDB roadmap: Tree-of-Thought reasoner |
| Yao et al. (2022) — *ReAct: Synergizing Reasoning and Acting in Language Models* (ICLR 2023) | Conference Paper | Interleaved reasoning traces + actions (tool calls); agent loop | ThemisDB ReAct grammar already implemented |
| White et al. (2023) — *A Prompt Pattern Catalog to Enhance Prompt Engineering with ChatGPT* (arXiv) | Preprint | 16 reusable prompt patterns (persona, flipped interaction, question refinement, etc.) | ThemisDB prompt pattern library |
| Zhou et al. (2022) — *Large Language Models Are Human-Level Prompt Engineers* (ICLR 2023) — APE | Conference Paper | Automatic Prompt Engineer: LLM generates + selects prompt instructions | Already implemented in `src/prompt_engineering/` |
| Pryzant et al. (2023) — *Automatic Prompt Optimization with "Gradient Descent" and Beam Search* (EMNLP) — ProTeGi | Conference Paper | Textual gradients; beam search over prompt candidates; outperforms APE | ThemisDB PromptEnhancementEngine roadmap |
| Fernando et al. (2023) — *Promptbreeder: Self-Referential Self-Improvement Via Prompt Evolution* (arXiv) | Preprint | Evolutionary algorithm over mutation/thinking-style prompts; self-improving | Advanced prompt auto-optimization for ThemisDB |
| Schulhoff et al. (2024) — *The Prompt Report: A Systematic Survey of Prompting Techniques* (arXiv) | Survey | 58 text prompting techniques catalogued; taxonomy with 33 LLM families tested | Most comprehensive survey; maps to ThemisDB prompt library |
| Dong et al. (2022) — *A Survey for In-Context Learning* (arXiv) | Survey | Analysis of demonstration selection, ordering effects, label calibration | ThemisDB few-shot example selection algorithm |
| DSPY Framework (2024) — https://dspy.ai | Open Source Framework | Declarative prompting + auto-optimization; replaces hand-written prompts | ThemisDB prompt engineering migration path |
| LangChain / LlamaIndex (2023–) | Open Source Frameworks | Popular orchestration frameworks for LLM + retrieval pipelines; hub of prompt templates | ThemisDB integration compatibility; prompt template format reference |

**Conferences:** NeurIPS, ICLR, ACL, EMNLP, NAACL

**Gaps identified:**
- Tree-of-Thoughts (ToT) reasoner not yet implemented
- ProTeGi textual-gradient optimizer not yet implemented (APE exists)
- No DSPY-compatible prompt declaration layer

---

### Storage Engines & LSM-Trees

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Dong et al. (2021) — *CrystalBall: Statistically Assisted Adaptive Compaction in RocksDB* (HotStorage) | Workshop Paper | ML-guided compaction scheduling; reduces write amplification 30% | ThemisDB RocksDB wrapper: adaptive compaction tuning |
| Lu et al. (2017) — *WiscKey: Separating Keys from Values in SSD-Conscious Storage* (FAST 2016) | Conference Paper | Key-value separation in LSM; reduces write amplification for large values | ThemisDB storage: large-value blob separation |
| Dayan & Idreos (2018) — *Dostoevsky: Better Space-Time Trade-Offs for LSM-Tree Based Key-Value Stores via Adaptive Removal of Superfluous Merging* (SIGMOD) | Conference Paper | Lazy Leveling + Fluid LSM designs; optimal space-time trade-off surface | Tuning guide for ThemisDB compaction strategy selection |
| RocksDB 9.x blog posts (2024) — https://rocksdb.org/blog | Blog / Documentation | MDBF (Multi-DB block flush), Universal Compaction improvements, remote compaction | ThemisDB RocksDB dependency upgrade target: v9.x |
| Athanassoulis et al. (2016) — *Designing Access Methods: The RUM Conjecture* (EDBT) | Conference Paper | Read/Update/Memory amplification trade-off; no free lunch in index design | Theoretical framework for ThemisDB index selection |

---

### Distributed Systems & Consensus

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Ongaro & Ousterhout (2014) — *In Search of an Understandable Consensus Algorithm* (USENIX ATC) — Raft | Conference Paper | Replicated state machine; leader election, log replication; understandable alternative to Paxos | Core ThemisDB consensus (implemented) |
| Howard, Malkhi & Spiegelman (2021) — *Flexible Paxos: Quorum Intersection Revisited* (OPODIS) | Conference Paper | Any quorum intersection is sufficient; enables asymmetric quorums for read-heavy workloads | ThemisDB Raft read-scale optimization |
| Lamport (2019) — *Paxos vs Raft: Have we reached consensus on distributed consensus?* (PaPoC) | Workshop Paper | Critical analysis of Raft correctness; cluster membership change subtleties | ThemisDB Raft correctness review |
| Shapiro et al. (2011) — *Conflict-Free Replicated Data Types* (SSS) | Conference Paper | CRDTs: eventual consistency without coordination; merge semantics | ThemisDB eventual-consistent replication mode |
| Howard (2020) — *Distributed Consensus Revised* (PhD Thesis, Cambridge) | Thesis | Generalized Raft/Paxos; epoch-based leader leases; multi-master patterns | ThemisDB future multi-leader replication |

---

### GPU-Accelerated Operations

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Dao et al. (2022) — *FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness* (NeurIPS) | Conference Paper | IO-aware tiling for attention; 2–4× speedup, sub-linear memory | Already implemented in `src/llm/` CUDA |
| Dao et al. (2023) — *FlashAttention-2: Faster Attention with Better Parallelism and Work Partitioning* (ICLR) | Conference Paper | Improved work partitioning; 2× speedup over FA-1; standard for modern LLMs | ThemisDB LLM backend upgrade target |
| NVIDIA cuVS (2024) — https://github.com/rapidsai/cuvs | Open Source | Unified vector search library for GPU (replaces FAISS GPU + RAFT); CAGRA graph-based ANN index | ThemisDB GPU vector index next-gen backend |
| Pan et al. (2024) — *CAGRA: Highly Parallel Graph Construction and Approximate Nearest Neighbor Search for GPUs* (ICDE) | Conference Paper | CAGRA GPU-native graph-based ANN; 2.5× faster than HNSW-GPU | GPU vector index upgrade for ThemisDB |

---

### Security & Encryption

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| NIST FIPS 203 (2024) — *Module-Lattice-Based Key-Encapsulation Mechanism* (ML-KEM / Kyber) | Standard | First standardized post-quantum KEM; mandated for US government systems by 2030 | ThemisDB TLS: post-quantum cipher suite planning |
| NIST FIPS 204 (2024) — *Module-Lattice-Based Digital Signature Standard* (ML-DSA / Dilithium) | Standard | First standardized post-quantum signature; replaces ECDSA in post-quantum context | ThemisDB signature verification module upgrade |
| OWASP LLM Top 10 (2023) — https://owasp.org/www-project-top-10-for-large-language-model-applications/ | Standard / Guideline | 10 most critical security risks for LLM applications: prompt injection, training data poisoning, etc. | ThemisDB LLM security baseline |
| Gentry (2009) — *A Fully Homomorphic Encryption Scheme* (PhD Thesis, Stanford) | Thesis | First viable FHE construction; bootstrapping enables unlimited computation on encrypted data | Long-term ThemisDB research: encrypted query execution |

---

### Query Optimization

**State of the Art:**

| Source | Type | Key Insight | ThemisDB Relevance |
|--------|------|-------------|-------------------|
| Marcus et al. (2019) — *Neo: A Learned Query Optimizer* (VLDB) | Conference Paper | DNN-based join ordering; outperforms PostgreSQL optimizer on JOB benchmark by 1.5–3× | ThemisDB learned query optimizer roadmap |
| Dutt et al. (2019) — *Selectivity Estimation for Range Predicates using Lightweight Models* (VLDB) | Conference Paper | Lightweight neural estimators for cardinality; replace histograms | Cardinality estimation module for ThemisDB |
| Negi et al. (2021) — *Flow-Loss: Learning Cardinality Estimates That Matter* (VLDB) | Conference Paper | Loss function directly optimizing query plan quality; not raw estimation error | ThemisDB learned cardinality: loss function design |
| Perron et al. (2019) — *Speeding up Learning-based Query Optimizers* (CIDR) | Conference Paper | Hybrid optimizer: learned model for new queries, rule-based for well-tested queries | ThemisDB hybrid optimizer design |

---

## Gap Analysis

| Finding | ThemisDB Current State | Action Required | Priority |
|---------|----------------------|-----------------|----------|
| JIT-compiled query execution | Row-at-a-time interpretation | Implement DuckDB/HyPer-style query compilation | P1 |
| DiskANN SSD-resident vector index | HNSW in-memory only | Implement DiskANN or NVQ-based SSD index | P1 |
| BPMN 2.0 conformance checking | Wire protocol only | Process discovery + conformance checking algorithms | P2 |
| SQL:2011 `FOR SYSTEM_TIME AS OF` | Partial temporal support | Complete bi-temporal query semantics | P1 |
| OZG / XÖV schema registry | No built-in Verwaltungs-IT schemas | Add OZG/FIM/XÖV schema library to ThemisDB | P2 |
| Tree-of-Thoughts reasoner | Chain-of-Thought only | Implement ToT with beam search over thought tree | P2 |
| Post-quantum TLS (ML-KEM/ML-DSA) | Classical TLS | Plan PQC migration; add Kyber/Dilithium cipher suites | P1 |
| Learned query optimizer (Neo-style) | Rule-based only | Research phase: dataset collection + cardinality model | P3 |
| LoRA+ / AdaLoRA | Standard LoRA/QLoRA | Implement adaptive rank + asymmetric learning rates | P2 |
| GQL/ISO 39075 compliance | AQL (ThemisDB-specific) | AQL → GQL mapping layer | P2 |
| Multi-adapter LoRA composition | Single adapter only | Stack domain adapters (legal + administrative) | P2 |
| Continuous aggregate materialization | On-demand only | TimescaleDB-style real-time materialized views | P2 |

---

## Action Items Created

- [x] Paper: [DuckDB — Raasveldt & Mühleisen (2019)](../papers/duckdb_olap_2019.md) — created
- [x] Paper: [HNSW — Malkov & Yashunin (2020)](../papers/hnsw_efficient_ann_2020.md) — created
- [x] Paper: [SQL:2011 Temporal — Kulkarni & Michels (2012)](../papers/temporal_sql2011_2012.md) — created
- [x] Paper: [Process Mining — van der Aalst (2012)](../papers/process_mining_van_der_aalst_2012.md) — created
- [x] Paper: [LoRA — Hu et al. (2022)](../papers/lora_low_rank_adaptation_2022.md) — created
- [x] Paper: [Prompt Pattern Catalog — White et al. (2023)](../papers/prompt_patterns_catalog_2023.md) — created
- [x] Paper: [OZG / Verwaltungs-IT Sources](../papers/verwaltungs_it_ozg_sources.md) — created
- [x] Paper: [Graph Databases — Robinson et al. (2015)](../papers/graph_databases_oreilly_2015.md) — created

---

**Completed:** 2026-03-23

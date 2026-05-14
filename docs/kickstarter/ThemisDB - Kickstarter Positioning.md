# ThemisDB - Kickstarter Positioning

## The Problem ThemisDB Solves

Modern AI applications typically need at least five separate systems at once:

a [relational database](https://en.wikipedia.org/wiki/Relational_database) (PostgreSQL)
- A [vector database](https://en.wikipedia.org/wiki/Vector_database) (Pinecone/Weaviate)
- A [graph database](https://en.wikipedia.org/wiki/Graph_database) (Neo4j)
- A [time-series](https://en.wikipedia.org/wiki/Time_series) store (InfluxDB)
- Separate LLM inference infrastructure

For startups, this often means cloud costs above $15K/month, cross-system data inconsistency, and architecture decisions that are expensive to reverse. ThemisDB solves this with a single coherent system.

## 1. HISTORY - How ThemisDB Was Built

**Headline:** "We built the database we wished we had."

As AI applications entered enterprises in the late 2020s, a paradox emerged: systems became smarter, but the underlying infrastructure became increasingly chaotic.

A typical company running AI-powered workflows needed:

- PostgreSQL for structured business data with [ACID](https://en.wikipedia.org/wiki/ACID) guarantees
- Neo4j or ArangoDB for relationship and [knowledge graphs](https://en.wikipedia.org/wiki/Knowledge_graph)
- Pinecone or Weaviate for [semantic](https://en.wikipedia.org/wiki/Semantic) vector search
- InfluxDB or TimescaleDB for [time-series](https://en.wikipedia.org/wiki/Time_series) and sensor data
- Elasticsearch for [full-text search](https://en.wikipedia.org/wiki/Full-text_search)
- OpenAI API or local LLM servers for inference
- RabbitMQ or Kafka for change-data-capture across all systems

Seven systems. Seven operating teams. Seven failure surfaces. And the hardest question: how do you preserve ACID consistency when one transaction writes into four systems at the same time?

ThemisDB started from one insight: one cohesive database system that supports all these data models natively, with true ACID guarantees across models, and treats AI inference not as a bolt-on integration but as a native part of the engine.

Development began with a clear architectural principle: no compromise on correctness. [MVCC](https://en.wikipedia.org/wiki/Multiversion_concurrency_control)-based [snapshot isolation](https://en.wikipedia.org/wiki/Snapshot_isolation), [Raft](https://en.wikipedia.org/wiki/Raft_(algorithm)) consensus for distribution, and [SAGA](https://en.wikipedia.org/wiki/Saga_(software_engineering)) orchestration for distributed transactions, all in one system.

Today, with v1.9.0-alpha, ThemisDB includes:

- 58 production-grade modules in C++17/20
- Relational, graph, vector, document, geospatial, and time-series storage in one engine
- Native LLM inference via llama.cpp and ONNX (no external API required)
- [GPU](https://en.wikipedia.org/wiki/Graphics_processing_unit) acceleration via [CUDA](https://en.wikipedia.org/wiki/CUDA), HIP, and [Vulkan](https://en.wikipedia.org/wiki/Vulkan_(API))
- Benchmarked throughput: 61 million time-series points/sec, 1.2 million graph edge operations/sec
- MIT-licensed open source, developed in public since the first commit

## 2. PERSPECTIVE - Where ThemisDB Is Headed

**Headline:** "The operating system for the data-driven AI era."

The database landscape is approaching a disruption comparable to the rise of relational databases in the 1970s.

### Trend 1: Agentic AI Needs New Data Semantics

Autonomous [LLM](https://en.wikipedia.org/wiki/Large_language_model) agents need more than a vector store. They need:

- [Knowledge graphs](https://en.wikipedia.org/wiki/Knowledge_graph) for [reasoning](https://en.wikipedia.org/wiki/Reasoning)
- Relational tables for structured decisions
- Time-series context windows
- Immediate consistency across all of it

ThemisDB is built to cover that entire stack natively.

### Trend 2: Sovereign AI Means On-Prem Inference

Enterprises and public institutions increasingly do not want AI inference in the cloud. ThemisDB brings [LLM inference](https://en.wikipedia.org/wiki/Inference_(machine_learning)) (llama.cpp, [ONNX](https://en.wikipedia.org/wiki/Open_Neural_Network_Exchange), [LoRA](https://en.wikipedia.org/wiki/LoRA_(machine_learning)) fine-tuning) directly into the database layer, fully air-gap capable and certifiable for military-grade environments.

### Trend 3: Regulation Is Catching Up with AI Infrastructure

[GDPR](https://en.wikipedia.org/wiki/General_Data_Protection_Regulation), [HIPAA](https://en.wikipedia.org/wiki/Health_Insurance_Portability_and_Accountability_Act), [ISO 27001](https://en.wikipedia.org/wiki/ISO/IEC_27001), [PCI-DSS](https://en.wikipedia.org/wiki/Payment_Card_Industry_Data_Security_Standard), [eIDAS](https://en.wikipedia.org/wiki/EIDAS_regulation): in ThemisDB, compliance is not a plugin, but a governance module embedded in the engine, including OZG/XOV/eID connectors for German public sector contexts.

### Trend 4: Edge and Resource-Constrained Environments

ThemisDB MINIMAL edition runs on [embedded systems](https://en.wikipedia.org/wiki/Embedded_system). The HYPERSCALER edition scales to thousands of nodes via a [Kubernetes](https://en.wikipedia.org/wiki/Kubernetes) operator. One binary, five deployment contexts.

### Two-Year Target Position

- Certified deployment in German federal authorities (BSI alignment, CC EAL4+ path)
- [WASM](https://en.wikipedia.org/wiki/WebAssembly) edition for browser-native AI workloads
- Managed ThemisDB Cloud as an alternative to Aurora + Pinecone + Neo4j
- Academic partnership for ethical AI decision-system research (ethics_ai module)

## 3. CONCRETE GOAL - What We Build With Your Support

**Headline:** "A database that thinks. We need you to finish it."

ThemisDB is production-ready for the COMMUNITY edition. Three critical milestones remain before delivering the full enterprise offering with market-transforming potential.

### Funding Tier 1: EUR25,000 - "Bridge to Production"

**Goal:** Benchmark suite and certification package

- Complete CHIMERA benchmark suite (IEEE Std 2807-2022) with independently verified results
- Comparative whitepaper: ThemisDB vs. [PostgreSQL + Pinecone + Neo4j] vs. [MongoDB Atlas + Weaviate]
- Docker images for MINIMAL and COMMUNITY on Docker Hub with automated CVE scanning
- External penetration test (scope: REST API, Wire Protocol V2, Auth layer)

**Why this matters:** Without verified benchmarks and an external security audit, no enterprise CTO can responsibly adopt a new database stack.

### Funding Tier 2: EUR75,000 - "Agentic AI Native"

**Goal:** Fully integrated AI agent stack

- Agentic RAG production hardening: full production tests, chaos-engineering coverage, and deployment patterns for the AgenticRAG stack (multi-hop reasoning, adaptive retrieval, DELEGATE-52 safety net)
- LLM Inference Engine v2.0: llama.cpp integration with streaming, LoRA hot-swap, and GPU batch inference without external dependencies
- Process Mining + AI bridge: connect BPMN process models directly with LLM agents for administrative workflows, compliance processes, and automated decision systems
- Managed SDK: Python, TypeScript, and Java SDKs with complete ThemisDB client libraries and ORM-like APIs across data models

**Why this matters:** No other open-source database currently offers this full stack in one system.

### Funding Tier 3: EUR150,000 - "Sovereign AI Infrastructure for Europe"

**Goal:** Establish ThemisDB as a European infrastructure component

- BSI certification path: technical documentation and audit preparation for Common Criteria EAL3+
- German e-government full stack: production-grade OZG/XDOMEA/XOV connectors, eID online ID functionality, and eIDAS timestamping
- MILITARY Edition open specification: air-gap deployment, HSM integration, authenticated boot, published as an open standard
- Academic program: partnership with two German universities on ethics_ai (AI decision traceability, DOT/Mermaid visualization of philosophical decision chains)
- Full-time core maintainer for 12 months: continuous development, community support, and issue response SLA

**Why this matters:** Europe lacks a sovereign, enterprise-ready, AI-native alternative to US cloud database stacks. ThemisDB can fill that gap.

## Kickstarter Headline Positioning Summary

| Variant | Text |
|---|---|
| Core message | "The first database that natively combines relational, graph, vector, time-series, and LLM inference - ACID-safe, open source, built in Europe." |
| Problem-focused | "Stop running 7 databases for one AI application. ThemisDB is the one database that does it all." |
| Performance-focused | "61 million time-series points/sec. 1.2 million graph operations/sec. One engine." |
| Sovereignty-focused | "AI without cloud dependency. LLM inference directly in the database - air-gap capable, GDPR-aligned, European-built." |
| Developer-focused | "One AQL query instead of 5 API calls. Multi-model database with built-in LLM - MIT licensed." |

## Competitive Positioning

| Criterion | PostgreSQL + Pinecone + Neo4j | MongoDB Atlas | ThemisDB |
|---|---|---|---|
| Native multi-model | No (3 systems) | Limited | Yes (6 models) |
| ACID across all models | No | Partial | Yes |
| Native LLM inference | No | No | Yes |
| Air-gap deployable | Partial | No | Yes |
| GPU acceleration | Partial | No | Yes (CUDA/HIP/Vulkan) |
| German e-gov stack | No | No | Yes |
| Open source | Partial | No | Yes (MIT) |
| Monthly cloud cost (100K vectors + 1M docs + graph) | ~$2,500 | ~$1,800 | $0 (self-hosted) |

## Risks and Challenges

Kickstarter prompt: "Be open and honest about the risks and challenges your project faces and how you plan to overcome them."

### 1. ThemisDB is currently a one-person project

This is the largest risk. All 58 modules, all 500,000+ lines of C++, workflows, and documentation are currently maintained by one person.

Implications:

- If I become unavailable, progress slows down
- Feature delivery speed is limited by one maintainer
- Code review, security work, and community support compete for the same time

Mitigation:

- The EUR150K target primarily funds 12 months of full-time work
- Governance already defines a co-maintainer path after three accepted pull requests
- The architecture is highly modular, enabling gradual ownership transfer per module

### 2. Exceptional technical complexity

ThemisDB combines distributed transactions ([Raft](https://en.wikipedia.org/wiki/Raft_(algorithm)) + [SAGA](https://en.wikipedia.org/wiki/Saga_(software_engineering))), six data models, GPU acceleration (CUDA/HIP/Vulkan), native LLM inference, and government-grade compliance requirements in one engine.

Example:
- Current [secondary-index](https://en.wikipedia.org/wiki/Database_index) throughput: 254,900 ops/sec
- Target: 1,000,000 ops/sec
- GPU-dependent workloads require dedicated hardware

Mitigation:
- Performance gaps and open stubs are tracked publicly in PERFORMANCE_EXPECTATIONS.md and src/STUB_INVENTORY.md
- 269 of 272 stubs are resolved; 3 remain active
- Roadmap clearly separates production-ready modules from beta modules
- Kickstarter funds are prioritized toward enterprise blockers: security audit and independently verified benchmarks

### 3. Certification timelines can slip

[[BSI](https://en.wikipedia.org/wiki/Federal_Office_for_Information_Security)] baseline certification, Common Criteria EAL3+, and ISO 27001 Type II processes can take 12-24 months and depend on external bodies.

Mitigation:
- Funding Tier 1 (EUR25K) is fully under direct execution control
- Tier 3 certification goals are explicitly positioned as long-term, not near-term promises

### 4. The market is dominated by large US vendors

AWS Aurora, Google Spanner, MongoDB Atlas, and Pinecone have major budgets, sales teams, and enterprise channels.

Mitigation:
- ThemisDB competes on sovereignty: no [telemetry](https://en.wikipedia.org/wiki/Telemetry_(software)), no [vendor lock-in](https://en.wikipedia.org/wiki/Vendor_lock-in), no lock-in
- This is a regulatory requirement, not a "nice to have," in public sector, healthcare, and critical infrastructure
- MIT licensing is intentional to maximize trust and independence

### 5. Delivery schedule risk

Software development cannot be scheduled to the exact day. Unexpected audit findings can shift timelines.

Mitigation:
- Public execution via GitHub issue tracking and commits
- Roadmap milestones with explicit status markers ([x], [~], [?])
- Monthly backer updates with concrete progress metrics

## Frequently Asked Questions (FAQ)

### What is ThemisDB?

ThemisDB is an [open-source](https://en.wikipedia.org/wiki/Open_source) database engine in [C++](https://en.wikipedia.org/wiki/C%2B%2B) six data models in one engine: document, graph, vector, time-series, key-value, and relational. It also includes native LLM inference, GPU acceleration (CUDA/HIP/Vulkan), and built-in compliance features for regulated sectors.

### Is ThemisDB already real, or just an idea?

It exists and is production-capable. 57 of 58 modules are marked production-ready. The project includes 500,000+ lines of C++, a full test suite, [CI/CD](https://en.wikipedia.org/wiki/CI/CD) workflows, a CHIMERA benchmark suite, and a public roadmap on GitHub.

### Why Kickstarter instead of venture capital?

VC funding can create pressure that often conflicts with open source, user control, and privacy goals. Kickstarter keeps governance aligned with community-backed public infrastructure.

### What license does ThemisDB use?

[MIT License](https://en.wikipedia.org/wiki/MIT_License). Fully open, commercially usable, no vendor lock-in, no call-home, no mandatory subscription. Repository: github.com/makr-code/ThemisDB.

### Who is ThemisDB for?

Primarily:

- Public sector and government organizations needing sovereign infrastructure
- Healthcare and critical infrastructure operators with strict compliance requirements
- Developers and research institutions running local AI workloads
- Companies reducing dependence on US cloud stacks

### Which databases does ThemisDB replace?

Not a single one. It replaces multi-system stacks such as PostgreSQL + MongoDB + Neo4j + Pinecone + InfluxDB + separate LLM backend with one unified engine.

### Will it run on my hardware?

Yes. It runs on standard Linux server hardware. GPU features are optional. Minimum baseline: x86-64 Linux, 4 GB RAM, 10 GB disk.

### How is the funding used?

- EUR25,000: independent security audit + verified public benchmarks
- EUR75,000: 6 months full-time development + BSI baseline preparation
- EUR150,000: 12 months full-time development + Sovereign Tech Fund application + community infrastructure

### Who develops ThemisDB?

Currently one maintainer: Martin (@makr-code). Funding enables full-time focus and contributor onboarding. Co-maintainer governance is already defined.

### Can I use ThemisDB today?

Yes. Source is public, buildable (CMake + MSVC/GCC/Clang), and documented. Binary distribution and Docker image are planned in the v2.0 roadmap.

### Is support available?

Yes, via GitHub Issues and Discussions. Kickstarter backers at EUR500+ receive prioritized issue response (target: 48h). Enterprise support contracts are planned as a stretch goal above EUR200,000.

### What happens if the funding goal is not met?

The project continues. Development proceeds at a slower pace. Security audit and benchmark verification would likely be delayed.

### Is ThemisDB GDPR-compliant?

The architecture is built for [data sovereignty](https://en.wikipedia.org/wiki/Data_sovereignty):. Formal GDPR conformity assessment by a data protection officer is planned for the EUR75K milestone.

### When is stable v1.0?

v1.5.0 is already released (Q2 2026), v1.7.0 follows, and semantic versioning applies (v1.x stable API, v2.0 with breaking changes and distribution improvements). With full-time funding, v2.0 is targeted by end of Q1 2027.

### Can I contribute as a developer?

Yes. The repository is public, issues are labeled, and contribution steps are documented in CONTRIBUTING.md. Contributors with three accepted pull requests can become co-maintainers for a module.

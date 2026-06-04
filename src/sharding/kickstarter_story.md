Why AI Infrastructure Is Broken — and How ThemisDB Fixes It

Anyone running a modern AI application today does not need a single system — they need a zoo of separate services. PostgreSQL stores business data. Neo4j manages relationships in the knowledge graph. Pinecone or Weaviate handles semantic vector search. InfluxDB processes time-series data from sensors. Elasticsearch powers full-text search. A separate LLM backend handles AI inference. Kafka or RabbitMQ keeps all of them talking to each other. Seven systems. Seven operations teams. Seven points of failure. Cloud costs exceeding ~ €15,000 per month for medium-sized deployments, despite the fact that AI inferencing is uncalcuable due to the token-usage and per-token costs scaling.



The fundamental technical problem runs deeper. The moment a single transaction — for example, creating a user profile with an AI recommendation — must write data simultaneously into four of these systems and ACID consistency may break down. ACID stands for Atomicity, Consistency, Isolation, and Durability: the four core guarantees that ensure a database never leaves behind half-finished states. What is a given in a single relational database becomes an unsolvable engineering problem in a distributed seven-system stack. Data errors emerge at system boundaries, are nearly impossible to reproduce, and cost compliance certifications or user trust in critical applications.

ThemisDB solves a little on this problem at the root. It is a single engine that natively handles all multiple data models — relational, graph, vector, document, time-series, and key-value — while maintaining true ACID guarantees across all of them. Native LLM inference (LLM = Large Language Model, the technology behind ChatGPT and similar systems) runs directly inside the database process via llama.cpp and ONNX: no external API call, no network latency, no privacy concerns from sending sensitive data to a cloud service. The development approach follows a scientific standard with measurable results, reproducible experiments, and transparent failure analysis.

The technical foundations are already implemented. ThemisDB uses RocksDB LSM-Tree as MVCC (Multiversion Concurrency Control — a technique where each database access gets its own consistent snapshot of data without blocking other accesses) as the native basis for ACID semantics inside one engine. For distributed data, the sharding layer uses Raft-based coordination and RAID-sharding style inter-database communication, so replication, shard routing, and cross-node state agreement are handled in the distributed storage layer rather than in application code. 

The SAGA pattern also exists, but mainly as a concession to long-running or pre-existing distributed database workflows that users may already have; it is not the primary integrity model of ThemisDB's native multi-model core. For compute-intensive workloads, optional GPU acceleration is available via CUDA, HIP, and Vulkan. 

With v1.9.0-alpha, the project comprises 58 modules in C++17/20, over 500,000 lines of code, and achieves a benchmarked 61 million time-series points per second and 1.2 million graph edge operations per second. The project is MIT-licensed (with governance clauses) — completely free to use, commercially and without restrictions — and is developed entirely in public on GitHub. The binaries can be found also.

Three trends make ThemisDB particularly relevant today. First, autonomous AI agents that make independent decisions need more than a vector store — they need knowledge graphs for reasoning, relational tables for structured decisions, and time-series context windows, all in consistent transactions. Second, enterprises and government agencies no longer want to outsource AI inference to US cloud services: GDPR, BSI baseline protection, and the US Cloud Act force data sovereignty. ThemisDB is fully air-gap capable — it operates without any internet connection and without a mandatory call-home mechanism; development and observability telemetry exists via optional, disableable tracing. Third, ThemisDB scales from embedded systems such as industrial controllers to large multi-node deployments — with optional Kubernetes and Helm deployment support where it is useful, but not as a requirement.

For European government agencies and regulated industries, ThemisDB also includes a native compliance module with connectors for German OZG (Onlinezugangsgesetz — Online Access Act), XDOMEA, and eID, as well as support for eIDAS timestamping. In ThemisDB, eIDAS is not an isolated checkbox: it is part of the audit and immutability model, where append-only logs, cryptographic signatures, and a blockchain-like tamper-evident chain of events work together to make changes traceable and verifiable. HIPAA, ISO 27001, and PCI-DSS are not afterthought plugins — they are anchored directly in the engine.

ThemisDB runs today. Anyone who clones the repository from GitHub and builds it with CMake gets a basic functioning database system — no additional dependencies required. Additional LLM Models (like Llama, Gemma, Mistral) has to be added manually that is running in Llama.cpp submodule.



A screen recording shows: ThemisDB starting on a standard Windows 11, accepting multi-model queries that combine documents, graphs, and time-series simultaneously, finding semantically similar entries via vector similarity search in milliseconds, and calling a locally running language model inside the same database process — without a single cloud API call. The CHIMERA benchmark suite runs directly against the live system and produces the published figures: 61 million time-series points per second and 1.2 million graph edge operations per second on the same hardware used for this project.

Capabilities found in no other open-source database system in this combination: The Whisper module (v2.0.0, production-ready) integrates Whisper.cpp speech recognition directly into the database process — WAV/MP3/OGG/FLAC via FFmpeg, Voice Activity Detection, streaming transcription, and automatic language identification as a native database operation. The Stable Diffusion module (v2.1.0) integrates stable-diffusion.cpp image generation with provenance stamps, content safety filtering, and CLIP embeddings — all stored transactionally. The Ethics AI module orchestrates ethical debates across multiple philosophical schools (Kant, Rawls, utilitarianism, and others) for auditable AI decisions — benchmarks exceed SLO targets by 6–10×. The Governance module contains production-ready rule sets for GDPR, HIPAA, ISO 27001, PCI-DSS, SOC2, and CCPA — including an Open Policy Agent adapter, data lineage tracking, and AI model governance. The RAG pipeline includes hallucination detection, faithfulness evaluation, bias detection, ontology-aware knowledge-graph retrieval, and multimodal RAG (text, audio, image). The LLM module additionally supports multimodal language models (LLaVA text+image), Grammar-Constrained Generation (structurally valid JSON/XML/CSV, 95–99 % validation rate), Speculative Decoding (2–3× faster inference), Flash Attention, and PagedAttention.

Why is this important to us?

We've spent the last 2 years building ThemisDB to prove that unified multi-model systems can deliver production-grade performance while maintaining data integrity. The database industry needs a "paradigm" shift — away from federated microservice stacks toward consolidated, scientifically-grounded systems. We're building the proof point.

Now, Then and Tomorrow

Early 2024: We started from a familiar reality: combining relational data, vector search, and LLM workflows across separate systems. In parallel, discussions in the German federal administration around practical and sovereign AI use helped shape the first principles behind ThemisDB.

September 2024: We began building the initial ThemisDB architecture and boilerplate with generative AI support.

June 2025: We released the first functional prototype (without integrated core inference at that stage).

March 10, 2026: We presented initial results at WiKKI 2026 (Wildau Conference on Artificial Intelligence 2026).

Current status: We are stabilizing v1.9.0 toward a production-ready release candidate.

WiKKI 2026

Martin (makr-code) presented our research contribution: "Convergent Data Architecture for Sovereign AI Systems" with a comparative evaluation of ThemisDB against hyperscaler-style stacks.

What This Kickstarter Project Will Create

What Your Donations Will Make Possible

This campaign funds one concrete, time-bound project: ThemisDB v1.9.0 Production-Ready Delivery Project (July 2026 to June 2027).

Project Components

We have structured this Kickstarter project around five concrete components. From our perspective, the project goal is reached when these outputs are prepared and published for backers and the wider community:

ThemisDB v1.9.0 production-ready release package (GA-ready stabilization target)
Production Hardening Report (reliability, performance regression, and security hardening summary)
Multimodal Validation Pack (Whisper + Stable Diffusion production validation on defined model sets)
Operations and Deployment Pack (install guide, runbook, monitoring baseline, incident checklist)
Final Backer Project Report (scope delivered, validation evidence, release notes, next steps)

Project Timeline

Phase 1 (Jul-Sep 2026): Core stabilization for v1.9.0, including bug fixing, test hardening, and release criteria.
Phase 2 (Oct-Dec 2026): Production-readiness validation, including multimodal validation, performance regression checks, and security hardening.
Phase 3 (Jan-Mar 2027): Operations packaging, including runbooks, deployment baselines, monitoring, and incident procedures.
Phase 4 (Apr-Jun 2027): Final release packaging, publication, and backer communication.

Project Work Packages

This campaign should be understood as one fully scoped project, not as a series of separately funded milestones. If the campaign succeeds, we plan to execute the following work packages within the overall v1.9.0 production-readiness effort:

1. Core Stabilization

v1.9.0 stabilization across core runtime paths (core, storage, query, transaction
defect reduction and closure tracking for release-candidate hardening
regression baseline for correctness, reliability, and recovery behavior
sharding/failover hardening for production-relevant cluster scenarios

2. Production-Readiness Validation

network and API hardening validation (timeouts, retry behavior, input validation)
security hardening validation for release scope (auth, error handling, abuse resistance)
reproducibility and performance-regression validation for v1.9.0 target scope
targeted subsystem validation for production-critical paths before release packaging

3. Release Delivery and Operations Packaging

full Operations and Deployment Pack for production environments
release notes, upgrade guidance, and incident checklist for v1.9.0 rollout
benchmark and reproducibility package (methodology + validated results)
audit/compliance preparation summary for internal readiness documentation
maintainer-supported delivery wrap-up through project completion

The Future Outlook

ThemisDB isn't just another database. It's maybe the beginning of something larger: a sovereignty-first, knowledge-native database for organizations that can't afford to outsource their intelligence to US cloud providers.

By 2027, we'll have proven that a single, open-source multimodal engine can replace 7+ enterprise systems with better performance, lower cost, and full transparency. By 2029, we envision ThemisDB as the foundation for European AI governance—a place where governments, enterprises, and researchers can build mission-critical systems without vendor lock-in or hidden algorithms.

The longer vision is bolder: make the EU competitive in database technology again, and prove that open source + transparency can outpace proprietary systems in both performance and trust.

Your Kickstarter contribution directly funds the engineering hours needed to make this real. Not marketing, not consultants—actual developers, security audits, and infrastructure hardening.
 

Optional Add-On Packages

Whisper and Stable Diffusion support can be offered as optional add-ons during the campaign period.

Planned add-on examples:

- Whisper Support Add-On: model compatibility validation matrix, deployment guidance, and troubleshooting notes for defined model variants.
- Stable Diffusion Support Add-On: compatibility validation matrix, deployment guidance, and operational notes for defined model variants.

Add-ons extend supporter-specific materials and guidance. They do not change the core all-or-nothing project scope for v1.9.0 production-readiness.

Reward Tiers

Kickstarter is organized around reward tiers. Our tiers are designed as community support levels with project-related digital acknowledgments and participation during the campaign period.

Current tier structure:

- Community Backer (EUR 1): inclusion in the backer list
- Early Supporter (EUR 50): backer list + feature suggestion submission during the project period
- Pro Sponsor (EUR 250): backer list + feature suggestion submission + prioritized review of project-scope issues and feedback during the campaign period
- Lead Sponsor (EUR 2,500): backer list + feature suggestion submission + prioritized review during the project period + sponsor logo / acknowledgment as a supporter

Higher tiers are intended to provide broader visibility, closer project communication, and more direct feedback channels around the v1.9.0 production-readiness scope.

How Project Scope and Reward Tiers Work Together

The project work packages describe what we plan to build and publish if the campaign succeeds. The reward tiers describe how supporters participate in that project through acknowledgments, communication, and project-related digital benefits. In short: the work packages define project scope, and the reward tiers define supporter participation.

ThemisDB remains open source under MIT license. This campaign describes our planned project scope, timeline, and publication outputs for the campaign period.

All timeline dates and outputs are based on current planning assumptions. If technical, regulatory, or platform constraints require adjustments, we will communicate updates transparently through Kickstarter.

Any certification, attestation, or formal conformity outcome depends on external auditors, assessment bodies, and regulatory requirements. Within this campaign, our focus is on technical preparation, documentation, validation, and audit-readiness support rather than guaranteeing a specific certification result.

The Future Outlook

ThemisDB is built to prove that one open, sovereign, multimodal engine can replace fragmented AI data stacks with better integrity, lower complexity, and auditable results.

By June 2027, our goal is to publish the v1.9.0 production-ready package together with a full project completion report. Beyond that, our long-term direction remains a four-layer knowledge architecture: ANN retrieval, tensor compression, graph validation, and grounded LLM generation for regulated, mission-critical AI systems in Europe.

Kickstarter support helps us execute this project plan and share progress and outcomes transparently with all backers.
 
# ThemisDB as a Distributed ACID Multi-Model AI Database: Repository-Grounded Review and Evaluation Scope

**Status**: Review Ready
**Version**: 0.2
**Last Updated**: 2026-05-14
**Target Venue**: arXiv (cs.DB / cs.DC)
**Authors**: ThemisDB Research Team

---

## Abstract

This paper reviews whether the current ThemisDB repository supports describing the system as a distributed ACID multi-model database with integrated AI capabilities. The review is intentionally claim-bounded: only statements that can be tied to current repository architecture documents, benchmark mappings, or published benchmark summaries are retained. The repository documentation consistently presents ThemisDB as a system that combines Advanced Query Language (AQL) processing, ACID transaction control, distributed coordination, multi-model storage, and AI/LLM-oriented retrieval paths in one architecture. Current empirical evidence is strongest for single-node and component-level benchmarks: the canonical performance report documents a 9.67 ms query p99, 1.177 M graph edge operations per second, and 61.0 M time-series inserts per second, while also recording unresolved gaps in secondary-index insert throughput and peak query throughput. By contrast, several distributed-sharding metrics remain proxy-mapped or hardware-gated, and no public end-to-end mixed distributed AI workload study in the repository yet justifies superiority claims over decoupled architectures. The resulting article is therefore positioned as a verified architecture-and-evidence review plus a publication-ready evaluation scope, not as a completed comparative performance paper.

## I. Introduction

Modern AI data stacks are often assembled from separate transactional databases, vector stores, graph engines, and model-serving components. That split improves local specialization, but it also creates synchronization boundaries, additional operational surfaces, and ambiguity about which consistency guarantees apply to retrieval and inference paths.

ThemisDB is explicitly documented as an attempt to collapse these boundaries into one system. The repository root describes a multi-model engine with native AI/LLM integration; the architecture documentation expands this into API, query, storage, and distributed layers; and the canonical performance report defines system-level service objectives and benchmark mappings for transaction, search, RAG, and sharding subsystems. The central review question is therefore not whether such a system is conceptually attractive, but which claims are already supported by repository evidence and which still require dedicated measurement.

This revised paper contributes three concrete outputs:

1. A repository-grounded verification of the architectural claim that ThemisDB co-locates distributed, transactional, multi-model, and AI-oriented functionality.
2. A terminology-normalized description of the current system using the repository's own canonical terms: AQL, multi-model, MVCC, OCC, SAGA, Raft/Paxos/Gossip, hybrid search, and RAG.
3. A claim boundary for publication: what is already documented or measured, what is only benchmark-mapped, and what must remain future work until live distributed experiments are published.

## II. Related Work and Positioning

The distributed-systems and database literature provides the classical building blocks for this topic: Raft and Paxos for replicated coordination [1, 2], Sagas and transaction-commit protocols for distributed transactional workflows [3, 4], and multiversion or serializable isolation techniques for correctness under concurrency [5, 6]. In information retrieval and AI systems, BM25-style lexical ranking, dense retrieval, and RAG pipelines motivate the retrieval-native AI portion of the discussion [7, 8, 9].

What distinguishes ThemisDB in the current repository is not a new consensus or retrieval algorithm. The distinguishing claim is architectural composition: the same documented system surface includes AQL query processing, multi-model operators, transactional semantics, sharding and consensus options, hybrid retrieval, and LLM-adjacent components. The paper is therefore best framed as a systems-integration review. It should not claim benchmark superiority over best-of-breed separate stacks until the repository publishes comparable end-to-end measurements.

## III. Verified Repository Snapshot

### A. Architectural Co-Presence

Table 1 summarizes the central capabilities that are simultaneously documented in the repository.

| Capability area | Repository evidence | Review conclusion |
|---|---|---|
| Query surface | `README.md`, `ARCHITECTURE.md` describe AQL plus REST, GraphQL, gRPC, WebSocket, and wire protocols | The system is documented as more than a storage engine; it exposes a unified query and API surface [10, 11] |
| Transaction model | `README.md` lists MVCC, SSI, 2PC, and SAGA; `ARCHITECTURE.md` documents MVCC, snapshot isolation, OCC, rollback, and SAGA | ACID and distributed-transaction terminology are first-class repository concepts, not incidental notes [10, 11] |
| Distributed control | `README.md` and `ARCHITECTURE.md` describe sharding, Raft, Paxos, Gossip, failover, and cross-shard execution | The repository consistently positions ThemisDB as a distributed system with selectable coordination models [10, 11] |
| Multi-model data model | Root docs describe relational, graph, vector, document, geospatial, and time-series support | "Multi-model" is a supported product-level description in the current documentation [10, 11] |
| AI / retrieval layer | Root docs and architecture docs describe hybrid search, RAG evaluation, and LLM integration | AI support is documented as part of the same runtime architecture rather than an external add-on [10, 11] |

### B. Evidence Classes Used in This Review

To keep the article reviewable, all retained claims are classified into one of four evidence classes.

| Class | Meaning | Accepted examples in this paper |
|---|---|---|
| **Documented** | Canonical repository documents describe the capability | `README.md`, `ARCHITECTURE.md` |
| **Measured** | Canonical benchmark reports publish concrete numbers | `PERFORMANCE_EXPECTATIONS.md` |
| **Mapped** | A benchmark target is tied to an implemented benchmark case | `benchmarks/benchmark_target_mapping.json` |
| **Proxy / Deferred** | Only indirect coverage exists, or the measurement has not yet been published | proxy-marked sharding and transaction targets in the benchmark mapping |

This classification removes a common problem in draft systems papers: blending architecture intentions, benchmark harness availability, and actual measured outcomes into one unsupported headline claim.

## IV. Methodology / Review Approach

### A. Claim-Verification Procedure

The review procedure is intentionally conservative.

1. A claim is kept only if it can be anchored to a canonical repository document or benchmark artefact.
2. Architectural co-presence claims are supported by `README.md` and `ARCHITECTURE.md`.
3. Quantitative performance claims are supported only by values published in `PERFORMANCE_EXPECTATIONS.md`.
4. Claims about benchmark readiness or subsystem coverage are supported by `benchmarks/benchmark_target_mapping.json` and the referenced benchmark source files.
5. Any statement that implied unpublished distributed mixed-workload results, existing benchmark artefact directories, or demonstrated superiority over external architectures has been removed or narrowed.

### B. Evaluation Dimensions for Publication-Grade Follow-Up

A future publication-grade experiment should measure ThemisDB along four dimensions that are already motivated by the repository layout and benchmark mappings.

1. **Correctness and consistency**: how MVCC/OCC/SSI and distributed commit choices affect abort behaviour and tail latency.
2. **Distributed coordination cost**: how shard routing, scatter-gather paths, failover, and topology changes affect steady-state and transition-window behaviour.
3. **Multi-model execution**: how relational, graph, vector, document, and time-series paths interact under shared load rather than isolated microbenchmarks.
4. **AI-path overhead**: how hybrid retrieval and RAG-oriented components behave when coupled to transactional and distributed pressure.

This paper keeps that evaluation scope explicit, but it does not present fabricated or placeholder results for those dimensions.

## V. Evaluation / Current Evidence

### A. What the Repository Already Supports

Table 2 lists the strongest claims that are already supportable from current public artefacts.

| Supported claim | Evidence | Status |
|---|---|---|
| ThemisDB is documented as a distributed ACID multi-model database with integrated AI/LLM capabilities | `README.md`, `ARCHITECTURE.md` | documented |
| Core query latency is already measured below the published target | `PERFORMANCE_EXPECTATIONS.md` reports query p99 = 9.67 ms against a < 50 ms target | measured [12] |
| Strong single-node baselines exist for some non-AI primitives | `PERFORMANCE_EXPECTATIONS.md` reports 1.177 M graph edge ops/s and 61.0 M time-series inserts/s | measured [12] |
| Transaction throughput benchmarking is not merely planned; direct benchmark mappings exist | `benchmark_target_mapping.json` maps TX-1, TX-2, TX-3, and TX-8 to `bench_transaction_throughput.cpp`; TX-3 records 6.4 k/s as measured v1.3.4 | mapped / partly measured [13] |
| Hybrid retrieval and search benchmarking are implemented in the repository | RAG and search targets are mapped to `bench_rag_hybrid_retriever.cpp` | mapped [13] |
| Distributed sharding benchmarking exists, but much of it is still indirect | SH-1 is directly mapped, while many SH-* targets are explicitly marked `proxy` or `not_measurable` | mapped / proxy [13] |

### B. What the Repository Explicitly Does **Not** Yet Prove

The revised article removes several unsupported statements from the earlier draft.

1. **No completed mixed distributed AI benchmark wave is publicly reported.** The canonical performance report states that benchmark implementations are production-ready, but measurement runs for several module groups are still open [12].
2. **No claim of superiority over decoupled architectures is currently supportable.** The repository contains architectural scope and benchmark infrastructure, not a published comparative study against external distributed data-plus-serving stacks.
3. **No claim is made that benchmark JSON artefacts already exist in this checkout.** The previously cited `artifacts/perf_nv/targeted_validation/` and `artifacts/perf_nv/repro_validation_20260412_211053/` paths are not present in the current tree and have therefore been removed from the evidence chain.

### C. Interpreting the Current Measurement Picture

The available evidence is strong enough for a careful architecture paper, but not for an aggressive performance paper.

- The measured baselines show that important core paths are already fast on published single-node benchmarks [12].
- The benchmark mapping file shows that transaction, sharding, RAG, and search areas are connected to specific benchmark cases rather than undefined future work [13].
- The same mapping file also makes the current limits visible: several distributed claims are backed by proxies, fallbacks, or hardware-gated measurements rather than live end-to-end cluster runs [13].

That combination supports a defensible thesis: ThemisDB is already documented and partially benchmarked as a unified distributed ACID multi-model AI system, but the public evidence base remains uneven across subsystems.

## VI. Limitations and Known Issues

The limitations of the current evidence base should be stated directly.

1. **Distributed evidence is incomplete.** The strongest public measurements are single-node or component-level; many sharding and coordination targets remain proxy-backed [12, 13].
2. **Performance gaps remain in core subsystems.** The canonical performance report still documents secondary-index insert throughput below target (254.9 k/s vs. 1.0 M/s) and query peak throughput below target (796.4 M/s vs. 900 M/s) [12].
3. **GPU-dependent conclusions are conditional.** Some benchmark targets are explicitly hardware-gated, so open-source CPU-only review cannot generalize those outcomes [12, 13].
4. **Comparative external baselines are absent.** The repository does not yet publish a controlled comparison against separate transactional-database + vector-store + model-serving stacks, so any such claim would be speculative.
5. **Architecture breadth exceeds current public measurement depth.** The documentation covers a wide feature surface; not every documented subsystem has equally mature empirical validation in public artefacts.

These limitations are not weaknesses of the paper structure; they are the central facts a credible review must preserve.

## VII. Conclusion

After repository review, the strongest defensible formulation is the following: ThemisDB is presently documented as a distributed ACID multi-model database with integrated AI capabilities, and the repository already contains measurable evidence for important query, graph, time-series, transaction, search, and retrieval-related subsystems. However, the current public evidence does not yet justify broad superiority claims for end-to-end distributed mixed AI workloads. A review-ready paper should therefore emphasize verified architectural integration, measured baseline anchors, benchmark coverage status, and explicit claim boundaries. This revised article adopts that framing and removes unsupported draft material.

## References

1. Ongaro, D., & Ousterhout, J. (2014). *In Search of an Understandable Consensus Algorithm (Extended Version).* URL: <https://raft.github.io/raft.pdf>
2. Lamport, L. (1998). "The Part-Time Parliament." *ACM Transactions on Computer Systems*, 16(2), 133-169. DOI: [10.1145/279227.279229](https://doi.org/10.1145/279227.279229)
3. Garcia-Molina, H., & Salem, K. (1987). "Sagas." *Proceedings of SIGMOD*, 249-259. DOI: [10.1145/38713.38742](https://doi.org/10.1145/38713.38742)
4. Gray, J., & Lamport, L. (2006). "Consensus on Transaction Commit." *ACM Transactions on Database Systems*, 31(1), 133-160. DOI: [10.1145/1132863.1132867](https://doi.org/10.1145/1132863.1132867)
5. Fekete, A., O'Neil, E., O'Neil, P., & Shasha, D. (2005). "Making Snapshot Isolation Serializable." *ACM Transactions on Database Systems*, 30(2), 492-528. DOI: [10.1145/1071610.1071615](https://doi.org/10.1145/1071610.1071615)
6. Cahill, M. J., Rohm, U., & Fekete, A. D. (2008). "Serializable Isolation for Snapshot Databases." *Proceedings of SIGMOD*, 729-738. DOI: [10.1145/1376616.1376690](https://doi.org/10.1145/1376616.1376690)
7. Robertson, S. E., & Walker, S. (1994). "Some Simple Effective Approximations to the 2-Poisson Model for Probabilistic Weighted Retrieval." *Proceedings of SIGIR*, 232-241. DOI: [10.1007/978-1-4471-2099-5_24](https://doi.org/10.1007/978-1-4471-2099-5_24)
8. Karpukhin, V., Oğuz, B., Min, S., Lewis, P., Wu, L., Edunov, S., Chen, D., & Yih, W.-T. (2020). "Dense Passage Retrieval for Open-Domain Question Answering." *Proceedings of EMNLP*, 6769-6781. DOI: [10.18653/v1/2020.emnlp-main.550](https://doi.org/10.18653/v1/2020.emnlp-main.550)
9. Lewis, P., Perez, E., Piktus, A., Petroni, F., Karpukhin, V., Goyal, N., Küttler, H., Lewis, M., Yih, W.-T., Rocktäschel, T., Riedel, S., & Kiela, D. (2020). "Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks." *Advances in Neural Information Processing Systems*, 33. URL: <https://papers.nips.cc/paper/2020/hash/6b493230205f780e1bc26945df7481e5-Abstract.html>
10. ThemisDB Contributors. (2026). *ThemisDB* GitHub repository. URL: <https://github.com/makr-code/ThemisDB>
11. ThemisDB Contributors. (2026). *ThemisDB Architecture Documentation.* URL: <https://github.com/makr-code/ThemisDB/blob/main/ARCHITECTURE.md>
12. ThemisDB Engineering Team. (2026). *ThemisDB Performance Evaluation: Service Level Objectives, Benchmark Methodology, and Empirical Measurement Results (v1.9.0).* URL: <https://github.com/makr-code/ThemisDB/blob/main/PERFORMANCE_EXPECTATIONS.md>
13. ThemisDB Engineering Team. (2026). *Benchmark target mapping.* URL: <https://github.com/makr-code/ThemisDB/blob/main/benchmarks/benchmark_target_mapping.json>

---

## Appendix A. Claim-to-Evidence Traceability

| Claim ID | Claim summary | Evidence |
|---|---|---|
| C1 | ThemisDB documents a unified AQL, transaction, distributed, multi-model, and AI-capable architecture. | [10], [11] |
| C2 | Published benchmark results already provide credible baseline evidence for core query, graph, and time-series paths. | [12] |
| C3 | Transaction, sharding, RAG, and search benchmark coverage exists in code and mapping metadata, but evidence maturity differs by subsystem. | [13] |
| C4 | Broad superiority claims over decoupled architectures remain unsupported in the current public repository state. | [12], [13] |

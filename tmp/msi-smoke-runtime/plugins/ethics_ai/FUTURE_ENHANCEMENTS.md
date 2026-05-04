# Ethics AI Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`ROADMAP.md`](ROADMAP.md) for committed near-term work.

---

## Scope

- Enhancements to the ethics AI plugin covering multi-philosophy discourse, explainability of ethical verdicts, regulatory compliance checking (EU AI Act, GDPR), and causal/counterfactual reasoning.
- Entry-point: `plugins/ethics_ai/CMakeLists.txt` (compatibility shim) · canonical implementation: `src/ethics_ai/`.
- Out of scope: changes to ThemisDB vector storage or LLM inference; this plugin consumes those services via defined interfaces.
- Covers ontology integration (OWL/RDF), multi-agent adversarial debate, and stakeholder-weighted evaluation.

## Design Constraints

- [ ] Ethics evaluation MUST complete within 50 ms per response (excluding RAG retrieval I/O).
- [ ] Philosophy definition files MUST be validated against a published JSON schema before loading; invalid files cause startup failure with a detailed error.
- [ ] No philosophy file may contain executable code; only declarative rule definitions are permitted.
- [ ] Discourse engine MUST support at least 4 simultaneous philosophy agents without exceeding 200 MB additional RAM.
- [ ] Compliance checker MUST map every verdict to at least one cited regulatory article.
- [ ] All evaluation results MUST be reproducible given the same input and philosophy set (deterministic scoring).

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IEthicsEvaluator` | `EthicsAIPlugin`, ThemisDB query layer | `evaluate(response, context) → EthicsVerdict` |
| `IPhilosophyLoader` | `EthicsEvaluator` | Loads/validates JSON philosophy definitions |
| `IDiscourseEngine` | `EthicsAIPlugin` | Orchestrates multi-agent debate; returns consensus score |
| `IRAGContextEngine` | `EthicsEvaluator` | Retrieves relevant ethical precedents from ThemisDB |
| `IComplianceChecker` | `EthicsAIPlugin` | Maps verdict to EU AI Act / GDPR risk tiers |

## Idea Backlog

### Reasoning & Knowledge

- [ ] **Ontology integration** – link ethical concepts to OWL/RDF ontologies (e.g., DOLCE, SUMO).
- [ ] **Causal reasoning** – model cause-effect chains in ethical arguments.
- [ ] **Counterfactual evaluation** – "What if X had not happened?" reasoning support.
- [ ] **Temporal ethics** – track how ethical assessments change over time (via ThemisDB timeline storage).

### Multi-Agent / Social

- [ ] **Adversarial debate mode** – one agent argues for, another against a proposition.
- [ ] **Stakeholder modelling** – weight decisions by affected party profiles.
- [ ] **Consensus detection** – identify when multiple philosophy schools converge.

### Explainability / Transparency

- [ ] **Audit trail export** – export full decision chain as JSON-LD for external auditors.
- [ ] **Visual argument graph** – render argument store as a graph (DOT / Mermaid).
- [ ] **Natural language report** – LLM-generated summary of an ethical evaluation.

### Compliance & Regulation

- [ ] **EU AI Act compliance checker** – map ethics scores to regulatory risk tiers.
- [ ] **GDPR data-minimisation advisor** – flag privacy-invasive data uses.

---

## Test Strategy

- Unit tests for `IEthicsEvaluator` with fixture responses covering known-harmful, edge-case, and benign categories; assert verdict within ≤ 50 ms.
- Philosophy schema validation tests: assert that malformed JSON files are rejected at load time with a non-zero exit code.
- Determinism tests: run the same input 100 times; assert identical verdict and score each time.
- Compliance mapping tests: for every EU AI Act risk tier (minimal, limited, high, unacceptable), assert at least one fixture response maps correctly.
- Multi-agent discourse tests: 4-philosophy debate on 10 fixture prompts; assert consensus score in [0.0, 1.0] and ≤ 200 ms wall time.
- RAG integration tests: seed ThemisDB with 500 ethical precedents; assert top-3 retrieved precedents are relevant for 5 curated queries.

## Performance Targets

- Ethics evaluation latency ≤ 50 ms p99 per response (CPU only, excluding RAG network I/O).
- Philosophy set loading at startup ≤ 100 ms for up to 20 philosophy definitions.
- RAG precedent retrieval ≤ 20 ms p99 for top-5 results from a 10,000-document collection.
- Discourse engine consensus computation ≤ 150 ms for 4 philosophy agents on a single response.
- Memory overhead of ethics plugin ≤ 50 MB RSS after full initialisation.

## Security / Reliability

- Philosophy files MUST be validated against JSON schema before loading; no arbitrary code execution from loaded philosophies.
- Ethics evaluation results MUST NOT include raw user input in log output at INFO level or above.
- RAG context retrieval MUST use read-only ThemisDB access credentials; the plugin must not modify the precedent store.
- Compliance checker outputs MUST be treated as advisory only; plugin MUST NOT autonomously block or modify responses without host application approval.
- Plugin failures (e.g., schema load error) MUST degrade gracefully and return a `VERDICT_UNAVAILABLE` sentinel rather than crashing the host process.

## Research / References

- L. Floridi et al., "An ethical framework for a good AI society: Opportunities, risks, principles, and recommendations," *Minds Mach.*, vol. 29, pp. 689–707, 2019. DOI: [10.1007/s11023-019-09484-4](https://doi.org/10.1007/s11023-019-09484-4)
- T. J. M. Bench-Capon and P. E. Dunne, "Argumentation in artificial intelligence," *Artif. Intell.*, vol. 171, no. 10–15, pp. 619–641, 2007. DOI: [10.1016/j.artint.2007.05.001](https://doi.org/10.1016/j.artint.2007.05.001)
- P. M. Dung, "On the acceptability of arguments and its fundamental role in nonmonotonic reasoning, logic programming and n-person games," *Artif. Intell.*, vol. 77, no. 2, pp. 321–357, 1995. DOI: [10.1016/0004-3702(94)00041-X](https://doi.org/10.1016/0004-3702(94)00041-X)
- B. Mittelstadt et al., "The ethics of algorithms: Mapping the debate," *Big Data Soc.*, vol. 3, no. 2, 2016. DOI: [10.1177/2053951716679679](https://doi.org/10.1177/2053951716679679)
- S. Russell, D. Dewey, and M. Tegmark, "Research priorities for robust and beneficial artificial intelligence," *AI Mag.*, vol. 36, no. 4, pp. 105–114, 2015. DOI: [10.1609/aimag.v36i4.2577](https://doi.org/10.1609/aimag.v36i4.2577)

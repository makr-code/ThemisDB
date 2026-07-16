# State of the Art — Index

This directory tracks the current state of the art in areas relevant to ThemisDB, updated on a quarterly basis.

## Purpose

Regular landscape reviews ensure that ThemisDB stays aligned with advances in:
- Vector search algorithms and indexing
- Storage engine design
- Distributed systems and consensus protocols
- LLM integration and RAG pipelines
- GPU-accelerated database operations

## Quarterly Reports

| Quarter | Report | Focus Areas | Status |
|---------|--------|-------------|--------|
| 2026 Q1 | [2026_q1_landscape.md](2026_q1_landscape.md) | Relational DB, Vector Search, Graph DB, Temporal DB, Process-DB, Verwaltungs-IT, LLM/RAG, LoRA/PEFT, Prompt Engineering, Storage, Distributed Systems, GPU, Security, Query Optimization | ✅ Complete |

## Update Process

See [quarterly_updates.md](quarterly_updates.md) for the review process and schedule.

## Contributing

When you discover a relevant new technique, tool, or paper that affects ThemisDB's design space:

1. Add it to the current quarterly report
2. If it directly influences an implementation decision, also create an entry in  
   [architecture_decisions/](../architecture_decisions/)
3. If you implement it, create a paper entry in [papers/](../papers/)

## See Also

- [../RESEARCH_GUIDE.md](../RESEARCH_GUIDE.md) — contributor workflow
- [../implementation_influence/README.md](../implementation_influence/README.md) — master index
- [../architecture_decisions/README.md](../architecture_decisions/README.md) — design decisions driven by landscape analysis

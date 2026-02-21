# AQL Module Roadmap

## Current Status
Production-ready for LLM-assisted AQL query generation, natural language to AQL translation, and documentation assistance. Core AQL parsing and execution are handled by the query module.

## Completed ✅
- [x] LlmAqlHandler for INFER, RAG, EMBED, MODEL, and LORA command processing
- [x] Natural language to AQL translation via LLM integration
- [x] AQL documentation assistant for function lookup and explanation
- [x] Query explanation and profiling assistance
- [x] LLM command handler infrastructure (request routing, response parsing)
- [x] Support for multi-paradigm AQL (documents, graphs, vectors, geospatial, timeseries)
- [x] Integration with OpenAI, Anthropic, Azure OpenAI, and llama.cpp providers
- [x] AQL syntax highlighting and error annotation in LLM responses (`AQLSyntaxHighlighter`)

## In Progress 🚧
- [ ] AQL query validation and linting before LLM submission (Target: Q2 2026)
- [ ] Streaming natural language responses for long AQL explanations (Target: Q2 2026)
- [ ] Few-shot example library for improved NL-to-AQL accuracy (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] AQL syntax highlighting and error annotation in LLM responses *(completed)*
- [ ] Query template library for common AQL patterns
- [ ] Interactive AQL query builder with LLM suggestions
- [ ] Batch NL-to-AQL translation for offline workloads
- [ ] Confidence scoring for generated AQL queries
- [ ] Multi-turn conversation context for iterative query refinement

### Long-term (6-12 months)
- [ ] AQL auto-complete API for editor integrations (LSP-compatible)
- [ ] AQL query migration assistant (ArangoDB AQL → ThemisDB AQL)
- [ ] Schema-aware query generation using live collection metadata
- [ ] AQL function documentation auto-generation from C++ headers
- [ ] Fine-tuned local model (LoRA adapter) for ThemisDB-specific AQL
- [ ] Integration with query optimizer for cost-aware suggestions

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests
- [ ] Performance benchmarks
- [ ] Security audit (prompt injection prevention)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- NL-to-AQL accuracy depends on LLM provider quality and prompt engineering
- No offline fallback when no LLM provider is configured
- Prompt injection is a known risk for NL-to-AQL translation; input sanitization is partial
- Schema-aware generation requires explicit schema injection into prompts

## Breaking Changes
- LLM command handler API is stable; new command types will be additive
- Confidence scoring API will be introduced as a new optional field (non-breaking)

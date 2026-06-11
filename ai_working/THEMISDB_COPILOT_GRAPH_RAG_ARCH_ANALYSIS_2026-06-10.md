# ThemisDB as Copilot Backend: Graph-RAG + MCP + Re-Ranking

## Scope
- Create a practical architecture analysis for integrating ThemisDB as coding-assistant backend in VS Code/Copilot.
- Include current state (IST), target state (SOLL), gap analysis, and implementation deep-dive.
- Assume ThemisDB can host local inference models (llama.cpp family, CodeLlama, DeepSeek, Gemma).

## Inputs Used
- ARCHITECTURE.md
- docs/architecture/FEATURE_FLAGS_REFERENCE.md
- include/llama_cpp/*
- include/search/llm_reranker.h + README
- include/rag/reranker.h + src/rag/reranker.cpp
- tools/copilot-ollama-router/*
- audit/docs/implementation-history/LLM_LORA_QLORA_INTEGRATION_AUDIT.md

## Deliverable Outline
1. Executive summary
2. IST architecture map
3. SOLL architecture map
4. SOLL-IST comparison table
5. Deep-dive by capability
6. Security and governance model
7. Rollout phases and acceptance criteria
8. Risks and mitigations

## Summary

Add adaptive retrieval depth control for RAG endpoints based on complexity and SLA profile.

## Deliverables

- Dynamic `top_k` and retrieval budget policy
- Optional rerank budget escalation for complex requests
- Timeout-aware retrieval constraints

## Tasks

- [ ] Define rag profile knobs and defaults
- [ ] Implement profile-based retrieval budget selection
- [ ] Integrate with rag endpoint flow
- [ ] Add integration tests across S0..S3 classes

## Acceptance Criteria

- RAG path uses policy-selected retrieval budget
- Low-complexity requests keep low-latency profile
- Complex requests can escalate retrieval depth safely

## Labels

- type:feature
- area:rag
- area:llm
- priority:P1
- effort:medium

---
name: rag-evaluation
description: Evaluate ThemisDB RAG retrieval and answer quality against labeled evidence and comparable baselines.
metadata:
  AI_Used: true
  AI_Models: "unknown"
  AI_Contributor: "GitHub Copilot"
  Human_Review: false
  Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

# Evaluate a RAG result set

Use this skill to review recorded results or design an evaluation. Use
`rag-retrieval` guidance when the task is to find evidence rather than score
an evaluation run.

1. Record the corpus and dataset revisions, case count, relevance labels,
   cutoff, retrieval settings, model version, and baseline. Compare runs only
   when their inputs and metric definitions match; report other differences.
2. Use Recall@k for labeled retrieval coverage and a ranking metric such as
   MRR or nDCG where appropriate. Evaluate answer faithfulness separately by
   checking whether cited passages support the claims. A relevant document
   or high similarity score is not evidence that an answer is faithful.
3. Record timeouts, missing labels, and no-answer cases separately. State
   denominators and exclusions rather than dropping failed cases silently.
   Preserve case-level evidence without exposing unnecessary private text.
4. Treat `src/rag/EVALUATION_CONTRACT_V1.md` as acceptance criteria, not as
   performance evidence. A smoke fixture cannot establish release-quality
   results. Report measured values, scope, baseline deltas, and any metric
   or threshold that was not evaluated.

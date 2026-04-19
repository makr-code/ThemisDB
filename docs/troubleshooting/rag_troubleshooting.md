# RAG Troubleshooting Guide

The `rag` module provides Retrieval-Augmented Generation capabilities for ThemisDB, including retrieval quality control, RAG judge evaluation, hallucination detection, bias detection, A/B testing, and continuous learning orchestration.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| RAG responses contain hallucinations | Hallucination detector not enabled | Enable `rag.hallucination_detector.enabled` |
| `RagJudge: model not available` | Judge LLM not configured | Set `rag.judge.model_path` |
| Retrieved context irrelevant | Retrieval quality threshold too low | Increase `rag.retrieval.min_relevance_score` |
| A/B test variant not assigned | A/B testing not enabled | Enable `rag.ab_testing.enabled: true` |
| `CoherenceEvaluator: ONNX error` | ONNX runtime not installed | Install `onnxruntime`; check `rag.evaluator.onnx_path` |
| `BiasDetector: no patterns loaded` | Bias pattern file missing | Set `rag.bias_detector.patterns_file` |
| Continuous learning not improving quality | Too few feedback samples | Collect more feedback; lower `min_feedback_samples` |
| Claim extractor returns no claims | Wrong claim extraction model | Check `rag.claim_extractor.model` |
| G-Eval score always 0 | Missing G-Eval criteria file | Provide `rag.geval.criteria_file` |
| `ContinuousLearningOrchestrator: stalled` | Upstream data pipeline paused | Check data pipeline and feedback collector |

## Common Issues

### Issue 1: Hallucination in RAG Response

**Description:** The RAG pipeline produces responses that contain factual claims not supported by retrieved documents.

**Symptoms:**
- RAG answer contains dates/names not present in any retrieved document
- Log: `HallucinationDetector: hallucination detected (confidence=0.82)`

**Cause:** Hallucination detector is disabled or threshold too permissive.

**Solution:**
```yaml
rag:
  hallucination_detector:
    enabled: true
    model: nli_cross_encoder        # Natural Language Inference model
    threshold: 0.70                 # block responses with hallucination score > 0.70
    action: block                   # "block" | "warn" | "log"
    fallback_response: "I cannot find a reliable answer in the provided context."
```

---

### Issue 2: RAG Judge LLM Not Available

**Description:** The LLM-as-judge evaluation pipeline fails because no judge model is configured.

**Symptoms:**
- Log: `RagJudge: judge model not loaded – evaluation disabled`
- Quality metrics show `N/A`

**Cause:** `rag.judge.model_path` is empty or the LLM module is not initialised.

**Solution:**
```yaml
rag:
  judge:
    enabled: true
    model_path: /var/lib/themisdb/models/rag-judge.Q4_K_M.gguf
    criteria:
      - faithfulness
      - relevance
      - completeness
    timeout_ms: 5000
    batch_size: 8
```

---

### Issue 3: Retrieved Context Is Irrelevant

**Description:** RAG retrieves documents that are not related to the query.

**Symptoms:**
- Retrieved chunks have low similarity scores
- Log: `RagQualityControl: 8/10 retrieved chunks below min_relevance_score=0.70`

**Cause:** Embedding model mismatch between query and document embeddings; threshold too low.

**Solution:**
```yaml
rag:
  retrieval:
    top_k: 10
    min_relevance_score: 0.75       # increase from 0.50
    rerank: true                    # rerank with cross-encoder
    reranker_top_k: 5               # keep top 5 after reranking
    collection: knowledge_base
    embedding_model: text-embedding-3-small
```

---

### Issue 4: ONNX Runtime Error in Evaluators

**Description:** Coherence and completeness evaluators fail because ONNX runtime is not configured.

**Symptoms:**
- Log: `CoherenceEvaluator: ONNX inference failed: provider not found`
- Quality metrics show errors

**Cause:** ONNX runtime not installed or library path wrong.

**Solution:**
```bash
# Install ONNX runtime
pip install onnxruntime-gpu==1.17.0   # or onnxruntime for CPU

# Check library path
ldconfig -p | grep onnxruntime
```
```yaml
rag:
  evaluator:
    onnx_path: /usr/local/lib/python3.11/dist-packages/onnxruntime/capi
    use_gpu: true
    device_id: 0
```

---

### Issue 5: Bias Detector Loads No Patterns

**Description:** The bias detector runs but reports 0 patterns loaded.

**Symptoms:**
- Log: `BiasDetector: loaded 0 patterns from /etc/themisdb/rag/bias_patterns.yaml`
- No bias alerts ever fire

**Cause:** Patterns file path is wrong or file is empty.

**Solution:**
```bash
# Check patterns file
ls -la /etc/themisdb/rag/bias_patterns.yaml
cat /etc/themisdb/rag/bias_patterns.yaml | head -20
```
```yaml
rag:
  bias_detector:
    enabled: true
    patterns_file: /etc/themisdb/rag/bias_patterns.yaml
    categories:
      - gender_bias
      - racial_bias
      - age_bias
    threshold: 0.80
```

---

### Issue 6: A/B Testing Variant Not Assigned

**Description:** A/B testing framework assigns all requests to the control variant.

**Symptoms:**
- All responses use `variant=control`
- Log: `AbTestingFramework: A/B testing disabled`

**Cause:** A/B testing is not enabled; no experiments are defined.

**Solution:**
```yaml
rag:
  ab_testing:
    enabled: true
    experiments:
      rag_reranker_test:
        enabled: true
        traffic_split:
          control: 0.5
          treatment: 0.5
        control:
          reranker: bm25
        treatment:
          reranker: cross_encoder
        min_samples: 1000
        confidence_level: 0.95
```

---

### Issue 7: Continuous Learning Not Improving Quality

**Description:** Despite user feedback being collected, RAG quality does not improve over time.

**Symptoms:**
- `rag.quality_score` metric is flat over weeks
- Log: `ContinuousLearningOrchestrator: insufficient feedback (n=45 < min=100)`

**Cause:** Not enough feedback samples collected; feedback not reaching the orchestrator.

**Solution:**
```yaml
rag:
  continuous_learning:
    enabled: true
    feedback_collection: true
    min_feedback_samples: 50        # reduce from 100 to start earlier
    retrain_interval_hours: 24
    improvement_threshold: 0.02     # trigger retraining if quality drops 2%
```
```bash
# Check feedback collection
themisdb-admin rag feedback-stats

# Manually trigger learning cycle
themisdb-admin rag continuous-learning trigger
```

---

### Issue 8: G-Eval Score Always Returns 0

**Description:** G-Eval evaluation always returns 0 for all criteria.

**Symptoms:**
- All G-Eval criteria scored as 0
- Log: `GEval: criteria file not found – using empty criteria`

**Cause:** G-Eval criteria file not configured.

**Solution:**
```yaml
rag:
  geval:
    enabled: true
    criteria_file: /etc/themisdb/rag/geval_criteria.yaml
    judge_model: rag-judge
    scale: 1_to_5
```
```yaml
# /etc/themisdb/rag/geval_criteria.yaml
criteria:
  faithfulness:
    prompt: "Rate how faithful the answer is to the provided context (1-5)"
    weight: 0.4
  relevance:
    prompt: "Rate how relevant the answer is to the question (1-5)"
    weight: 0.3
  completeness:
    prompt: "Rate how completely the answer addresses the question (1-5)"
    weight: 0.3
```

## Diagnostic Commands

```bash
# RAG pipeline health
themisdb-admin rag status

# Quality metrics
themisdb-admin rag quality-report --last 7d

# Hallucination stats
themisdb-admin rag hallucination-stats

# A/B test results
themisdb-admin rag ab-test results --experiment rag_reranker_test

# Feedback statistics
themisdb-admin rag feedback-stats

# Live RAG metrics
curl -s http://localhost:9100/metrics | grep themisdb_rag

# Tail RAG logs
journalctl -u themisdb -f | grep -E "rag|hallucin|judge|retriev|bias|geval"
```

## Configuration Reference

```yaml
rag:
  enabled: true
  retrieval:
    top_k: 10
    min_relevance_score: 0.70
    rerank: true
  hallucination_detector:
    enabled: true
    threshold: 0.70
    action: block
  judge:
    enabled: false
    model_path: ""
  bias_detector:
    enabled: true
    patterns_file: /etc/themisdb/rag/bias_patterns.yaml
  ab_testing:
    enabled: false
  continuous_learning:
    enabled: true
    min_feedback_samples: 100
    retrain_interval_hours: 24
```

## Known Limitations

- Hallucination detection requires an NLI model; adds 100–300ms latency per response.
- G-Eval requires a capable judge LLM (≥7B parameters); smaller models produce unreliable scores.
- A/B testing traffic split is approximate; exact 50/50 split requires sticky session routing.
- Continuous learning improvements apply to the retrieval configuration, not the underlying LLM weights.

## Related Documentation

- [RAG Module ROADMAP](../../src/rag/ROADMAP.md)
- [RAG Ethics Implementation](../llm_orchestration/RAG_ETHICS_IMPLEMENTATION.md)
- [RAG Judge LLM Integration](../llm_orchestration/RAG_JUDGE_LLM_INTEGRATION.md)
- [RAG Judge Phase 1 Verification](../ARCHIVED/implementation-summaries/RAG_JUDGE_PHASE1_VERIFICATION.md)
- [LLM Roadmap](../llm_roadmap.md)

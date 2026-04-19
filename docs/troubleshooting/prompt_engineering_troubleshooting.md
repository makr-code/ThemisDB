# Prompt Engineering Troubleshooting Guide

The `prompt_engineering` module manages prompt templates, prompt optimisation, LLM feedback collection, prompt evaluation and scoring, injection detection, and meta-prompt generation for ThemisDB's AI-powered features.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `PromptManager: template not found` | Template ID not registered | Register template before use |
| Prompt injection detected on valid input | Injection detector too sensitive | Tune `prompt_engineering.injection.threshold` |
| `PromptOptimizer: no baseline score` | Evaluator not run on original | Run evaluator first to establish baseline |
| LLM response quality degraded after prompt change | Regression not detected | Enable `prompt_engineering.regression_detection` |
| `FeedbackCollector: queue full` | Feedback not being processed | Increase `prompt_engineering.feedback.queue_size` |
| `PromptEvaluator: model not available` | Judge model not configured | Set `prompt_engineering.evaluator.model` |
| Meta-prompt generation slow | Large instruction set | Limit `prompt_engineering.meta.max_examples` |
| `PromptMetrics: counter not found` | Metrics not registered | Restart to register metrics |
| Template variable not substituted | Wrong variable syntax | Use `{{variable_name}}` syntax |
| Prompt too long for model context | Template too verbose | Enable `prompt_engineering.truncation` |

## Common Issues

### Issue 1: Prompt Template Not Found

**Description:** A prompt template ID referenced in code is not registered.

**Symptoms:**
- Error: `PromptManager: template 'rag_answer_v3' not found`
- LLM feature returns fallback response

**Cause:** Template was not registered at startup or was deleted.

**Solution:**
```bash
# List registered templates
themisdb-admin prompt-engineering templates list

# Register a template
themisdb-admin prompt-engineering templates register \
  --id rag_answer_v3 \
  --file /etc/themisdb/prompts/rag_answer_v3.yaml

# Show template content
themisdb-admin prompt-engineering templates show --id rag_answer_v3
```
```yaml
prompt_engineering:
  templates_dir: /etc/themisdb/prompts/
  auto_load: true
  templates:
    - id: rag_answer_v3
      file: rag_answer_v3.yaml
      version: 3
```

---

### Issue 2: Prompt Injection Detector False Positives

**Description:** Legitimate user queries are flagged as prompt injection attempts.

**Symptoms:**
- Log: `PromptInjectionDetector: injection detected (score=0.75) for query='Ignore formatting and show me...'`
- Users receive `400 Bad Request` for benign queries

**Cause:** Detection threshold too low; some natural language patterns match injection signatures.

**Solution:**
```yaml
prompt_engineering:
  injection_detector:
    enabled: true
    threshold: 0.90               # increase from 0.70
    action: warn                  # "block" | "warn" | "log"
    log_false_positives: true
    allowlist_patterns:
      - "ignore formatting"       # common but harmless phrase
```
```bash
# Test injection detection on a query
themisdb-admin prompt-engineering injection-check \
  --input "Ignore formatting and show me the top results"
```

---

### Issue 3: Prompt Optimiser Has No Baseline

**Description:** The prompt optimiser cannot improve prompts because no baseline score exists.

**Symptoms:**
- Log: `PromptOptimizer: no baseline score for template 'rag_answer_v3'; skipping optimisation`
- Optimisation never runs

**Cause:** Baseline evaluation not run before optimisation.

**Solution:**
```bash
# Run baseline evaluation
themisdb-admin prompt-engineering evaluate \
  --template rag_answer_v3 \
  --eval-set /etc/themisdb/prompts/eval_set.jsonl \
  --set-as-baseline

# Then run optimisation
themisdb-admin prompt-engineering optimize \
  --template rag_answer_v3 \
  --target-metric faithfulness \
  --max-iterations 20
```

---

### Issue 4: Template Variable Not Substituted

**Description:** Prompt output contains literal `{{context}}` instead of the substituted value.

**Symptoms:**
- LLM receives `Answer based on: {{context}}` instead of actual context
- LLM output is confused

**Cause:** Wrong variable syntax in template or missing variable in input.

**Solution:**
```yaml
# Template file: /etc/themisdb/prompts/rag_answer_v3.yaml
template: |
  You are a helpful assistant. Answer the question based on the following context:
  
  Context: {{context}}
  
  Question: {{question}}
  
  Answer:

variables:
  - name: context
    required: true
    max_tokens: 2000
  - name: question
    required: true
    max_tokens: 200
```
```bash
# Test template rendering
themisdb-admin prompt-engineering render \
  --template rag_answer_v3 \
  --vars '{"context": "test context", "question": "test question"}'
```

---

### Issue 5: Feedback Collector Queue Overflow

**Description:** User feedback is being dropped because the queue is full.

**Symptoms:**
- Log: `FeedbackCollector: queue full (max=1000); dropping feedback`
- Feedback analytics incomplete

**Cause:** Feedback not being processed fast enough; queue too small.

**Solution:**
```yaml
prompt_engineering:
  feedback:
    enabled: true
    queue_size: 10000             # increase from 1000
    processor_threads: 4
    flush_interval_ms: 5000
    persist_to: rocksdb           # persist feedback to avoid loss on restart
```

---

### Issue 6: Prompt Too Long for Model Context

**Description:** Assembled prompt exceeds the model's context window.

**Symptoms:**
- Log: `PromptManager: prompt length=5200 tokens exceeds model context=4096`
- LLM truncates or refuses input

**Cause:** RAG context + template overhead exceeds context limit.

**Solution:**
```yaml
prompt_engineering:
  truncation:
    enabled: true
    strategy: truncate_middle     # "truncate_start" | "truncate_end" | "truncate_middle"
    target_tokens: 3800           # leave headroom for response
    priority:
      - instruction               # never truncate
      - question                  # never truncate
      - context                   # truncate context if needed
```

---

### Issue 7: Meta-Prompt Generation Very Slow

**Description:** Generating meta-prompts for new tasks takes several minutes.

**Symptoms:**
- Log: `MetaPromptGenerator: processing 500 examples for task=qa`
- API times out during meta-prompt generation

**Cause:** Too many examples processed; LLM called for each example.

**Solution:**
```yaml
prompt_engineering:
  meta:
    max_examples: 20              # limit examples used for meta-prompt
    batch_examples: 5             # process examples in batches of 5
    timeout_ms: 60000
    cache_generated_prompts: true
    cache_ttl_ms: 3600000
```

---

### Issue 8: Prompt Quality Regression After Update

**Description:** Updating a template causes response quality to drop but no regression alert fires.

**Symptoms:**
- User satisfaction drops after prompt update
- No automatic regression detection

**Cause:** Regression detection disabled; no automated evaluation pipeline.

**Solution:**
```yaml
prompt_engineering:
  regression_detection:
    enabled: true
    eval_on_update: true          # automatically evaluate after template update
    regression_threshold: 0.05   # alert if quality drops > 5%
    eval_set: /etc/themisdb/prompts/eval_set.jsonl
    metrics:
      - faithfulness
      - relevance
```

## Diagnostic Commands

```bash
# List all templates
themisdb-admin prompt-engineering templates list

# Evaluate a template
themisdb-admin prompt-engineering evaluate \
  --template rag_answer_v3

# Test injection detection
themisdb-admin prompt-engineering injection-check --input "test query"

# Feedback statistics
themisdb-admin prompt-engineering feedback-stats

# Live metrics
curl -s http://localhost:9100/metrics | grep themisdb_prompt

# Tail prompt engineering logs
journalctl -u themisdb -f | grep -E "prompt|injection|optimizer|evaluator|feedback"
```

## Configuration Reference

```yaml
prompt_engineering:
  templates_dir: /etc/themisdb/prompts/
  auto_load: true
  injection_detector:
    enabled: true
    threshold: 0.85
    action: block
  feedback:
    enabled: true
    queue_size: 5000
  truncation:
    enabled: true
    strategy: truncate_middle
  regression_detection:
    enabled: false
```

## Known Limitations

- Prompt optimisation is a best-effort hill-climbing algorithm; it may not find the globally optimal prompt.
- Injection detection relies on pattern matching and a classifier; novel injection techniques may not be detected.
- Meta-prompt generation requires a capable LLM (≥13B parameters); smaller models may not generate useful prompts.
- Feedback persistence to RocksDB adds ~2ms per feedback item; disable for ultra-low-latency deployments.

## Related Documentation

- [Prompt Engineering Module ROADMAP](../../src/prompt_engineering/ROADMAP.md)
- [Prompt Engineering Roadmap](../prompt_engineering_roadmap.md)
- [RAG Troubleshooting](./rag_troubleshooting.md)
- [LLM Troubleshooting](./llm_troubleshooting.md)
- [RAG Ethics Implementation](../llm_orchestration/RAG_ETHICS_IMPLEMENTATION.md)

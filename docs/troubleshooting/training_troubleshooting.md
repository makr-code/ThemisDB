# Training Troubleshooting Guide

The `training` module provides model fine-tuning infrastructure for ThemisDB, including incremental LoRA training, automatic data labelling, knowledge graph enrichment, training data selection, and pipeline orchestration.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `TrainingPipeline: base model not found` | Base model not downloaded | Download base model first |
| `IncrementalLoraTuner: OOM` | Batch size too large for VRAM | Reduce `training.batch_size` |
| `AutoLabeler: confidence too low` | Labelling model uncertain | Increase `training.auto_label.min_confidence` |
| LoRA training diverges | Learning rate too high | Reduce `training.lora.learning_rate` |
| `KnowledgeGraphEnricher: graph not found` | Graph collection not specified | Set `training.knowledge_graph.collection` |
| Training data selection quality poor | Selection strategy mismatch | Use `training.selection.strategy: diversity` |
| Training takes too long | No GPU or wrong batch size | Enable GPU; increase batch size |
| `LoraDataSelection: insufficient samples` | Too few training examples | Collect more data; lower `min_samples` |
| Validation loss not decreasing | Wrong hyperparameters | Run hyperparameter search |
| Training checkpoint not saved | Checkpoint dir not writable | Fix checkpoint directory permissions |

## Common Issues

### Issue 1: Training OOM on GPU

**Description:** LoRA training runs out of GPU memory.

**Symptoms:**
- Log: `IncrementalLoraTuner: CUDA OOM during forward pass (batch_size=32)`
- Training aborts

**Cause:** Batch size too large for available VRAM.

**Solution:**
```yaml
training:
  batch_size: 4                   # reduce from 32
  gradient_accumulation_steps: 8  # effective batch size = 4 × 8 = 32
  gradient_checkpointing: true    # trade compute for memory
  lora:
    r: 8                          # reduce LoRA rank to save memory
    alpha: 16
    dropout: 0.05
  mixed_precision: bf16           # use bfloat16 to halve memory
```

---

### Issue 2: Auto-Labeller Low Confidence

**Description:** The automatic data labeller rejects most examples due to low confidence.

**Symptoms:**
- Log: `AutoLabeler: 80% of samples below min_confidence=0.85; labelling failed`
- Training dataset is too small

**Cause:** Labelling model not suited for the domain; confidence threshold too high.

**Solution:**
```yaml
training:
  auto_label:
    enabled: true
    model: domain-specific-classifier   # use domain-specific model
    min_confidence: 0.70               # lower from 0.85
    fallback_to_manual: true           # queue low-confidence samples for human review
    human_review_queue: training_review
```

---

### Issue 3: LoRA Training Diverges

**Description:** Training loss increases instead of decreasing.

**Symptoms:**
- Loss curve shows divergence (loss > 10 after epoch 1)
- Log: `IncrementalLoraTuner: loss spike detected at step=100`

**Cause:** Learning rate too high; warmup not configured.

**Solution:**
```yaml
training:
  lora:
    learning_rate: 2e-4            # reduce from 1e-3
    warmup_steps: 100
    scheduler: cosine              # "linear" | "cosine" | "constant"
    max_grad_norm: 1.0             # clip gradients to prevent explosion
    weight_decay: 0.01
  early_stopping:
    enabled: true
    patience: 3                    # stop if no improvement for 3 epochs
    min_delta: 0.001
```

---

### Issue 4: Training Data Selection Poor Quality

**Description:** Selected training data does not represent the target task well.

**Symptoms:**
- Model performs poorly on target task despite many training examples
- Log: `LoraDataSelection: using random strategy – diversity may be low`

**Cause:** Random selection picks redundant examples; diversity-based selection not used.

**Solution:**
```yaml
training:
  selection:
    strategy: diversity            # "random" | "diversity" | "uncertainty" | "curriculum"
    diversity_metric: cosine_distance
    cluster_count: 100             # cluster examples; pick from each cluster
    min_samples: 500
    max_samples: 10000
    balance_classes: true
```

---

### Issue 5: Knowledge Graph Enrichment Fails

**Description:** Training data enrichment with knowledge graph facts fails.

**Symptoms:**
- Log: `KnowledgeGraphEnricher: collection 'knowledge_base' not found`
- Training examples missing entity-level facts

**Cause:** Knowledge graph collection not specified or does not exist.

**Solution:**
```yaml
training:
  knowledge_graph:
    enabled: true
    collection: knowledge_base      # ThemisDB collection with KG data
    entity_field: entities
    relation_field: relations
    max_hops: 2                     # traverse up to 2 hops for enrichment
    enrichment_fields:
      - definition
      - related_concepts
```
```bash
# Check collection existence
themisdb-admin collection list | grep knowledge_base

# Create if missing
themisdb-admin collection create --name knowledge_base
```

---

### Issue 6: Training Checkpoint Not Saved

**Description:** Training runs but no checkpoint files are written.

**Symptoms:**
- No files in checkpoint directory after training
- Log: `TrainingPipeline: checkpoint write failed: permission denied`

**Cause:** Checkpoint directory is not writable by the ThemisDB service user.

**Solution:**
```bash
# Fix permissions
mkdir -p /var/lib/themisdb/training/checkpoints/
chown -R themisdb:themisdb /var/lib/themisdb/training/
chmod 755 /var/lib/themisdb/training/checkpoints/
```
```yaml
training:
  checkpoint:
    enabled: true
    path: /var/lib/themisdb/training/checkpoints/
    save_every_n_steps: 100
    keep_last_n: 5
    save_best_only: true
    metric: eval_loss
```

---

### Issue 7: Incremental Training Not Improving Base Model

**Description:** LoRA adapter trained incrementally does not improve on the validation set.

**Symptoms:**
- Eval loss on new data is worse than base model
- Log: `IncrementalLoraTuner: incremental training: no improvement detected`

**Cause:** New training data too different from original training distribution; catastrophic forgetting.

**Solution:**
```yaml
training:
  incremental:
    replay_buffer_size: 1000       # include old examples to prevent forgetting
    replay_ratio: 0.2              # 20% of each batch from replay buffer
    elastic_weight_consolidation:
      enabled: true
      lambda: 5000                 # EWC regularisation strength
```

## Diagnostic Commands

```bash
# Training pipeline status
themisdb-admin training status

# List training jobs
themisdb-admin training jobs list

# Start a training job
themisdb-admin training start \
  --base-model llama-3-8b \
  --data-collection qa_pairs \
  --output-adapter /var/lib/themisdb/adapters/qa-v2.gguf

# Training metrics
curl -s http://localhost:9100/metrics | grep themisdb_training

# Tail training logs
journalctl -u themisdb -f | grep -E "training|lora|auto.label|knowledge.graph|selection"
```

## Configuration Reference

```yaml
training:
  enabled: false
  batch_size: 8
  gradient_accumulation_steps: 4
  mixed_precision: bf16
  lora:
    r: 16
    alpha: 32
    dropout: 0.05
    learning_rate: 2e-4
    warmup_steps: 50
  auto_label:
    enabled: false
    min_confidence: 0.75
  selection:
    strategy: diversity
    min_samples: 100
  checkpoint:
    enabled: true
    save_every_n_steps: 100
```

## Known Limitations

- LoRA training requires a GPU with ≥8 GB VRAM for 7B parameter models at minimum batch size 1.
- Incremental training can suffer from catastrophic forgetting without EWC or replay buffer.
- Auto-labelling accuracy depends heavily on the labelling model quality; always validate a sample manually.
- Training pipeline does not support multi-node distributed training in the current implementation.

## Related Documentation

- [Training Module ROADMAP](../../src/training/ROADMAP.md)
- [LLM Troubleshooting](./llm_troubleshooting.md)
- [Multi-LoRA Fusion Guide](../MULTI_LORA_FUSION_GUIDE.md)
- [LoRA Merge Strategies](../LORA_MERGE_STRATEGIES.md)
- [Paged Optimizer Guide](../llm_orchestration/PAGED_OPTIMIZER_GUIDE.md)

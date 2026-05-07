# ProcessTransformer: Predictive Business Process Monitoring with Transformer Network

**Metadaten:**
- Author(en): Zaharah A. Bukhsh, Aaqib Saeed, Remco M. Dijkman
- Konferenz/Journal: arXiv preprint arXiv:2104.00721; accepted at ICPM 2021
- Jahr: 2021
- Link: [arXiv:2104.00721](https://arxiv.org/abs/2104.00721)
- Zitierweise: `bukhsh2021processtransformer`
- Tags: `process-mining`, `predictive-monitoring`, `transformer`, `next-activity-prediction`, `remaining-time-prediction`, `event-log`
- ThemisDB-Versionen: v1.9.0+; planned in `src/process/` (P10: ProcessTransformer Vorhersage, Q1 2027)
- Status: [ ] Not Started · planned Q1 2027

## 📋 Executive Summary

ProcessTransformer applies the Transformer architecture (self-attention over sequences of process events) to predictive process monitoring — specifically next-activity prediction and remaining time prediction from ongoing process execution traces. Unlike LSTM-based methods (the prior state of the art), ProcessTransformer captures long-range dependencies in process traces and generalizes better across process variants. For ThemisDB, this enables real-time SLA alerts: "In 4 steps, this Bauantrag case will miss its 30-day deadline with 87% confidence."

Directly referenced in `src/process/FUTURE_ENHANCEMENTS.md` (P10: ProcessTransformer Vorhersage, Target Q1 2027, Priority: High).

## 🎯 Key Findings

- **Next-activity prediction accuracy**: ProcessTransformer achieves 10–15 pp higher accuracy than LSTM on 7 of 9 benchmark event logs (BPIC 2012, 2013, 2017, Helpdesk).
- **Remaining time prediction**: Mean Absolute Error (MAE) 15–30% lower than BiLSTM on remaining case duration prediction.
- **Transfer learning**: Pre-trained on similar process logs; fine-tuned on target process in < 10 epochs — enables ThemisDB to fine-tune on customer process data without full retraining.
- **Self-attention interpretability**: Attention weights visualize which past events the model considers most predictive — directly usable for ThemisDB audit explanations.
- **Multi-task learning**: Single model jointly predicts next activity + remaining time — reduces inference overhead vs. two separate models.
- **Tokenization**: Activity names tokenized as integer IDs; case attributes encoded as embedding vectors concatenated with activity embeddings.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Process module → `src/process/` (planned `process_predictor.cpp`)
- [x] Training module → `src/training/` (ProcessTransformer training on event logs extracted from ThemisDB)
- [x] Analytics module → `src/analytics/` (SLA breach prediction integrated with CEP engine)
- [x] CDC module → `src/cdc/` (real-time event stream as input to predictor)

### What Was Adopted?

1. **ProcessTransformer architecture**: Transformer encoder (causal masking for sequential event prediction) with 4 heads, 2 layers, d_model=64 for small event logs; scales to d_model=256 for large logs.
2. **Tokenization scheme**: Activity IDs as integers; case-level attributes (priority, responsible_role, citizen_category) as categorical embeddings.
3. **Multi-task output heads**: Separate classification head (next-activity softmax) and regression head (remaining time MSE).
4. **SLA integration**: `ProcessPredictor::predictBreachRisk(instance_id)` → `{breach_probability, predicted_remaining_seconds}` — triggers SLA alert dispatch in `ProcessGraphRag`.

### How Was It Adapted?

| ProcessTransformer Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Static training on full event log | Incremental LoRA fine-tuning via `src/training/` | ThemisDB processes evolve; model must adapt to new process variants without full retraining |
| BPIC benchmark event logs | XDOMEA / FIM process execution logs | German administrative context; activity names in German |
| Python (PyTorch) implementation | ONNX export + ONNX Runtime inference | ThemisDB is C++; ONNX Runtime (`src/onnx_clip/`) provides portable inference |
| Per-process model | Shared model with process-type embeddings | Single model for all Verwaltungsvorgang types; process type as a categorical input feature |

### Performance Impact

| Metric | Paper Claim | ThemisDB Target | Status |
|--------|-------------|-----------------|--------|
| Next-activity prediction accuracy | +10–15 pp vs. LSTM | +8 pp vs. rule-based | ⏳ Planned Q1 2027 |
| Remaining time MAE | -15–30% vs. BiLSTM | -20% vs. deadline-based heuristic | ⏳ Planned |
| Inference latency (ONNX) | Not reported | <50 ms p99 | ⏳ Planned |

## ⚠️ Limitations & Open Questions

- ProcessTransformer requires sufficient training data per process type (>500 completed cases).
  - ThemisDB solution: Pre-train on synthetic BPMN-generated traces; fine-tune on real cases as they accumulate.
- Categorical embeddings assume stable activity vocabulary; new activities require retraining.
  - ThemisDB solution: Reserve an `<UNK>` token; new activities map to UNK until model is retrained.
- ONNX export requires careful handling of dynamic sequence lengths.
  - ThemisDB solution: Pad all input sequences to `max_trace_length=128`; mask padding positions in attention.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests written (next-activity prediction on synthetic traces)
- [ ] Benchmark executed (BPIC 2017 event log)
- [ ] Documentation updated
- [ ] Module README linked (`src/process/README.md`)
- [ ] implementation_influence index updated

## 📚 Related Work

- [ProcessGPT — Busch et al. (2023)](processgpt_busch_2023.md)
- [GraphRAG — Edge et al. (2024)](graphrag_edge_2024.md)
- [Process Mining — van der Aalst (2012)](process_mining_van_der_aalst_2012.md)
- [`src/process/FUTURE_ENHANCEMENTS.md`](../../../src/process/FUTURE_ENHANCEMENTS.md) (P10)
- [ICPM — International Conference on Process Mining](https://icpmconference.org/)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-12-31

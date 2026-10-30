---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Analytics Research and Evaluation

## Purpose and scope

This note is for engineers evaluating algorithm changes in `src/analytics`.
It records which method families are present and how to validate them; it does
not claim a research benchmark result or prescribe a new model. The runtime
spans time-series prediction, anomaly scoring, process discovery, text analysis,
process-pattern similarity, streaming, and model-serving integration.

## Method assumptions and trade-offs

| Surface | Implemented method families | Main assumption or limit |
|---|---|---|
| Forecasting | Linear regression, exponential smoothing, Holt-Winters, ARIMA, SARIMA, Prophet-style trend/seasonality, weighted ensemble | The caller supplies timestamped observations; method/configuration and training data govern suitability. Evaluate held-out series instead of inferring quality from a successful fit. |
| Anomaly detection | Z-score, modified z-score/MAD, IQR, approximate isolation forest, LOF, and weighted ensemble | Thresholds and training distribution matter. A configured score threshold labels events; the detector does not establish business impact or deduplicate alerts. |
| Process mining | Directly-follows graphs, process discovery, variant analysis/clustering, and conformance checks | Event-log quality depends on usable case IDs, activities, and timestamps. Missing required fields are skipped during extraction. |
| Process-pattern similarity | Graph, vector, behavioral, and weighted hybrid scores | The built-in vector representation is a normalized character-trigram hash representation, not a learned semantic embedding. |
| Text analysis | Tokenization, stop words, stemming/lemmatization, keyword TF-IDF, lexicon sentiment, and regular-expression entity patterns | These are deterministic heuristics and dictionaries; they are not a general language model or learned named-entity recognizer. |

These method descriptions are supported by the public contracts in
[`forecasting.h`](../../include/analytics/forecasting.h),
[`anomaly_detection.h`](../../include/analytics/anomaly_detection.h),
[`process_mining.h`](../../include/analytics/process_mining.h),
[`process_pattern_matcher.cpp`](./process_pattern_matcher.cpp), and
[`nlp_text_analyzer.cpp`](./nlp_text_analyzer.cpp).

## Evaluation path

1. Define the input distribution, ordering, missing-value policy, and target
   outcome before changing an algorithm.
2. Use the focused regression suites in
   [`tests/analytics`](../../tests/analytics/): forecasting,
   `test_anomaly_detection.cpp`, process discovery/conformance, and
   streaming-window or pattern-matcher tests as applicable.
3. For forecasts, evaluate a time-ordered holdout with `ForecastModel::evaluate`
   and report MAE, RMSE, MAPE, and sMAPE; do not randomly shuffle a series when
   that would leak future observations.
4. For anomaly detection, report threshold, method, training sample count,
   feature set, and the resulting false-positive/false-negative counts on a
   labeled evaluation set. No universal accuracy target is defined here.
5. For process matching, retain the graph/vector/behavioral score breakdown,
   the configured hybrid weights, and representative positive and negative
   cases.
6. For performance claims, run the direct benchmark cases in
   [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md). The
   representative-hardware document says authoritative profile execution
   evidence is still pending; benchmark wiring alone is not a measured result.

## Reproducibility and limits

Record source revision, configuration, input fixture identity, build options,
and optional backend availability with each comparison. Test both supported
backend-present and backend-unavailable paths when an optional serving or
export dependency is involved. Keep operational release thresholds in the
benchmark baseline artifacts rather than copying transient results into this
research note.

## Related contracts

- [Model families and input constraints](./MODELING.md)
- [Embedding and feature-vector boundaries](./EMBEDDING.md)
- [Performance gates and pending hardware evidence](./PERFORMANCE_EXPECTATIONS.md)
- [Public interfaces](./API.md)

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

# Analytics Modeling

## Purpose

This document helps developers choose and validate the analytics model
interfaces. It describes the implemented families, their inputs, and important
failure conditions; it does not assert that a particular family is suitable
for every dataset.

## Forecast models

`ForecastModel` in
[`forecasting.h`](../../include/analytics/forecasting.h) fits a
`TimeSeries` of `(timestamp_ms, value)` points and predicts a requested number
of steps. `ForecastConfig` controls smoothing coefficients, seasonality,
ARIMA/SARIMA orders, Prophet-style changepoints/Fourier terms, confidence
intervals, and ensemble weights. Supported `ForecastMethod` values are
linear regression, exponential smoothing, Holt-Winters, ARIMA, ensemble,
SARIMA, and Prophet-style trend/seasonality.

Use the same method/configuration and an explicit time-ordered holdout when
comparing models. Inspect `ForecastModel::info()` for training range, point
count, median interval, fitted state, and in-sample RMSE. The API contract
rejects NaN/Inf input, requires an explicitly loaded model handle in serving
paths, and specifies deterministic predictions for identical model/input/seed
combinations; callers should propagate the corresponding errors rather than
retry invalid input. Serialized forecast models carry a CRC-32 check in the
current implementation. CRC detects accidental corruption; it is not a
cryptographic authenticity mechanism.

## Anomaly models

`AnomalyDetector` accepts `DataPoint` records whose fields can contain null,
boolean, integer, floating-point, or string values. Numeric feature extraction
is explicit; record the selected numeric fields and their ordering with every
evaluation. The implemented methods are Z-score, modified Z-score, IQR,
approximate Isolation Forest, LOF, and a weighted ensemble. `DetectorConfig`
sets contamination, threshold, tree/sample counts, neighbor count, and
ensemble weights.

`score >= threshold` determines `is_anomaly`; zero is valid and fires for every
event, while a negative threshold is invalid under the shared API contract.
Training is not thread-safe for `AnomalyDetector`; prediction/explanation are
safe for concurrent calls. `StreamingAnomalyDetector` maintains a bounded
training window and exposes window statistics, but its warm-up state must be
accounted for before interpreting an absent result as a normal event.

## Process models and pattern classifiers

`ProcessMining` builds event logs from configured case-ID, activity, and
timestamp fields, then exposes discovery, variant analysis, clustering, and
conformance results. Select `MiningAlgorithm` and thresholds through
`MiningConfig`; inspect `Status` before consuming a result. The public structures
also permit optional activity embeddings, but that does not imply a
module-provided pretrained model.

`ProcessPatternMatcher` can combine graph, vector, and behavioral similarity;
the default hybrid weights are 0.4, 0.3, and 0.3. Set weights deliberately and
validate threshold behavior on known traces. Its generated vector features are
character-trigram hash vectors (see [EMBEDDING.md](./EMBEDDING.md)).
`LoRAPatternClassifier` uses an injected inference callback when configured and
otherwise uses a statistical fallback; a fallback result is marked with
`used_fallback`.

## Serving and validation

`ModelServingEngine` registers named/versioned in-process AutoML models and
exposes single and batch prediction, metadata, health metrics, serialization,
and loading. `MLServingClient` dispatches to optional ONNX Runtime or TensorFlow
Serving backends; backend availability is a runtime/build capability, not a
model-quality guarantee. Validate input tensor names, shapes, and types and
handle `UNAVAILABLE`, `INVALID_INPUT`, `BACKEND_ERROR`, `TIMEOUT`, and
`POLICY_REJECTED` distinctly.

Recommended evidence is collected in
[`tests/analytics/test_forecasting.cpp`](../../tests/analytics/test_forecasting.cpp),
[`test_anomaly_detection.cpp`](../../tests/analytics/test_anomaly_detection.cpp),
[`test_model_serving.cpp`](../../tests/analytics/test_model_serving.cpp), and
[`test_ml_serving.cpp`](../../tests/analytics/test_ml_serving.cpp). See
[`RESEARCH.md`](./RESEARCH.md) for evaluation methodology and
[`API.md`](./API.md) for interface ownership.

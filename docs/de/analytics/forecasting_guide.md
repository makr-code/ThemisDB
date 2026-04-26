# Predictive Analytics & Time-Series Forecasting Guide

**Version:** v1.7.0  
**Status:** ✅ Production-Ready  
**Last Updated:** 2026-04-06

---

## Overview

ThemisDB's Predictive Analytics & Time-Series Forecasting engine provides pure C++17
multi-step-ahead forecasting with no external ML dependencies.  It is designed to integrate
directly with the Analytics module for sales forecasting, demand prediction, capacity
planning, and trend analysis.

---

## Key Features

### Supported Algorithms

| Algorithm | Enum | Description |
| --- | --- | --- |
| Ordinary Least Squares | `LINEAR_REGRESSION` | Trend-line extrapolation |
| Simple Exponential Smoothing | `EXP_SMOOTHING` | Level-only smoothing (SES/ETS-ANN) |
| Holt-Winters | `HOLT_WINTERS` | Triple exponential smoothing with trend + seasonality (additive or multiplicative) |
| ARIMA | `ARIMA` | AR(p) + I(d) + MA(q) via Yule–Walker / Levinson–Durbin |
| Ensemble | `ENSEMBLE` | Weighted combination of all four models above |

### Capabilities

- **Multi-step ahead forecasting** — predict any number of future steps
- **Confidence intervals** — empirical 95 % (or custom level) bounds on every forecast point
- **Seasonal decomposition** — additive and multiplicative trend / seasonal / residual split
- **Automatic hyperparameter tuning** — grid-search of α/β/γ via `auto_tune = true`
- **Accuracy metrics** — MAE, RMSE, MAPE, sMAPE via `evaluate()` and `computeMetrics()`
- **Serialisation round-trip** — `serialize()` / `deserialize()` with full IEEE-754 precision

---

## Usage Examples

### Basic Linear Trend Forecast

```cpp
#include "analytics/forecasting.h"
using namespace themisdb::analytics;

TimeSeries ts;
for (int i = 0; i < 30; ++i)
    ts.push(static_cast<int64_t>(i) * 86400000LL, 100.0 + 2.5 * i);  // daily data

ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
model.fit(ts);

auto forecast = model.predict(7);   // next 7 days
for (const auto& fp : forecast)
    std::cout << fp.timestamp_ms << ": " << fp.value
              << " [" << fp.lower << ", " << fp.upper << "]\n";
```

### Seasonal Forecast with Holt-Winters

```cpp
ForecastConfig cfg;
cfg.seasonality  = 12;    // monthly data → yearly season
cfg.alpha        = 0.3;
cfg.beta         = 0.1;
cfg.gamma        = 0.1;
cfg.multiplicative = false;   // additive seasonality

ForecastModel model(ForecastMethod::HOLT_WINTERS);
model.fit(ts, cfg);

auto forecast = model.predict(12);  // forecast next year
```

### ARIMA Forecast

```cpp
ForecastConfig cfg;
cfg.ar_order   = 2;   // AR(2)
cfg.diff_order = 1;   // I(1) — first differencing
cfg.ma_order   = 1;   // MA(1)

ForecastModel model(ForecastMethod::ARIMA);
model.fit(ts, cfg);
auto forecast = model.predict(10);
```

### Ensemble with Custom Weights

```cpp
ForecastConfig cfg;
// Weights for [LINEAR_REGRESSION, EXP_SMOOTHING, HOLT_WINTERS, ARIMA]
cfg.ensemble_weights = {0.4, 0.2, 0.3, 0.1};

ForecastModel model(ForecastMethod::ENSEMBLE);
model.fit(ts, cfg);
auto forecast = model.predict(5);
```

### Train / Test Split and Accuracy Evaluation

```cpp
auto [train, test] = ts.trainTestSplit(0.8);

ForecastModel model(ForecastMethod::HOLT_WINTERS);
model.fit(train, cfg);

ForecastMetrics m = model.evaluate(test);
std::cout << "MAE:  " << m.mae  << "\n";
std::cout << "RMSE: " << m.rmse << "\n";
std::cout << "MAPE: " << m.mape << " %\n";
```

### Seasonal Decomposition

```cpp
// Requires a fitted model
auto dr = model.decompose(/*multiplicative=*/false);

for (size_t i = 0; i < ts.size(); ++i)
    std::cout << "trend=" << dr.trend[i]
              << " seasonal=" << dr.seasonal[i]
              << " residual=" << dr.residual[i] << "\n";
```

### Auto-Tune and Model Serialisation

```cpp
ForecastConfig cfg;
cfg.auto_tune = true;   // grid-search α ∈ {0.1, …, 0.9}

ForecastModel model(ForecastMethod::EXP_SMOOTHING);
model.fit(ts, cfg);

// Persist the fitted model
std::string state = model.serialize();
// ... store state to DB / file ...

// Restore later
ForecastModel restored = ForecastModel::deserialize(state);
auto forecast = restored.predict(10);
```

---

## TimeSeries Container

```cpp
TimeSeries ts;

// Append observations (sorted insertion)
ts.push(timestamp_ms, value);

// Construct from vector (will be sorted)
std::vector<TimeSeriesPoint> pts = {{1000, 10.0}, {2000, 20.0}};
TimeSeries ts2(std::move(pts));

// Statistics
double mean   = ts.mean();
double stddev = ts.stddev();
double minVal = ts.min();
double maxVal = ts.max();

// Slice by timestamp range [from_ms, to_ms)
auto slice = ts.slice(start_ms, end_ms);

// 80/20 train-test split
auto [train, test] = ts.trainTestSplit(0.8);
```

---

## ForecastConfig Reference

| Field | Default | Description |
| --- | --- | --- |
| `alpha` | 0.3 | Level smoothing factor (0 < α < 1) |
| `beta` | 0.1 | Trend smoothing factor (0 < β < 1) |
| `gamma` | 0.1 | Seasonal smoothing factor (0 < γ < 1) |
| `seasonality` | 0 | Seasonal period (0 = no seasonality) |
| `multiplicative` | false | `true` = multiplicative, `false` = additive |
| `ar_order` | 2 | ARIMA autoregressive order p |
| `diff_order` | 1 | ARIMA differencing order d (0 or 1) |
| `ma_order` | 1 | ARIMA moving-average order q |
| `include_confidence` | true | Compute CI bounds in every ForecastPoint |
| `confidence_level` | 0.95 | CI level, e.g. 0.95 → 95 % |
| `ensemble_weights` | {} | Weights `[lr, ses, hw, arima]`; empty → equal |
| `auto_tune` | false | Grid-search α/β/γ before fitting |

---

## Accuracy Metrics

`computeMetrics(actual, predicted)` and `ForecastModel::evaluate(test_ts)` both return
`ForecastMetrics`:

| Field | Formula |
| --- | --- |
| `mae` | Mean Absolute Error |
| `rmse` | Root Mean Squared Error |
| `mape` | Mean Absolute Percentage Error (%) |
| `smape` | Symmetric MAPE (%) |
| `n` | Number of evaluation points |

---

## Thread Safety

- `ForecastModel::fit()` — **not** thread-safe; use one model per thread for training.
- `ForecastModel::predict()`, `evaluate()`, `decompose()`, `serialize()` — **thread-safe**
  (read-only after fitting).

---

## Related Documentation

- [OLAP Analytics Guide](./olap_guide.md)
- [Process Mining Guide](./process_mining_guide.md)
- [API Future Enhancements](../../../include/analytics/FUTURE_ENHANCEMENTS.md)
- [Implementation Roadmap](../../../src/analytics/ROADMAP.md)

See full API at `include/analytics/forecasting.h`.

---

**Last Updated:** 2026-04-06  
**Version:** v1.7.0  
**Status:** ✅ Production-Ready

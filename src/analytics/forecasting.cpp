/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            forecasting.cpp                                    ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-23                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     ~900                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Predictive Analytics & Time-Series Forecasting Engine – Implementation
 *
 * Algorithms (pure C++17, no external ML dependencies):
 *
 *   LINEAR_REGRESSION
 *       Ordinary least-squares fit: value = α + β·t.
 *       Forecast = α + β·(t_last + k·Δt).
 *       Confidence interval from residual standard error.
 *
 *   EXP_SMOOTHING (SES)
 *       s_t = α·y_t + (1-α)·s_{t-1}
 *       All future predictions equal the last smoothed value.
 *       CI from residual MAD.
 *
 *   HOLT_WINTERS (triple exponential smoothing)
 *       Additive:       ŷ_{t+k} = (L_t + k·T_t) + S_{t+k-m}
 *       Multiplicative: ŷ_{t+k} = (L_t + k·T_t) · S_{t+k-m}
 *       where m = seasonality period.
 *       Initial values by simple decomposition.
 *       If seasonality == 0 or < 2 full seasons, falls back to Holt's (no seasonal).
 *
 *   ARIMA (AR(p) + I(d) + MA(q))
 *       Yule–Walker equations for AR coefficients.
 *       Differencing order d (0 or 1) applied before fitting.
 *       MA(q) correction using residuals.
 *       Forecast integrates back if d == 1.
 *
 *   ENSEMBLE
 *       Weighted average of LINEAR_REGRESSION, EXP_SMOOTHING,
 *       HOLT_WINTERS, and ARIMA forecasts (equal weights unless overridden).
 *       CI is the weighted average of individual CIs.
 *
 * Confidence intervals:
 *       Linear / ARIMA / HW: CI = ŷ ± z * σ_res * sqrt(k)
 *       where z = inverse-normal(1-(1-level)/2), σ_res = residual std-dev,
 *       k = forecast step.
 */

#include "analytics/forecasting.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace themisdb {
namespace analytics {

// ============================================================================
// TimeSeries implementation
// ============================================================================

TimeSeries::TimeSeries(std::vector<TimeSeriesPoint> pts)
    : points_(std::move(pts))
{
    std::sort(points_.begin(), points_.end());
}

void TimeSeries::push(int64_t ts_ms, double value) {
    TimeSeriesPoint p{ts_ms, value};
    auto it = std::lower_bound(points_.begin(), points_.end(), p);
    points_.insert(it, p);
}

void TimeSeries::push(const TimeSeriesPoint& p) {
    push(p.timestamp_ms, p.value);
}

std::vector<double> TimeSeries::values() const {
    std::vector<double> v;
    v.reserve(points_.size());
    for (const auto& p : points_) v.push_back(p.value);
    return v;
}

std::vector<int64_t> TimeSeries::timestamps() const {
    std::vector<int64_t> t;
    t.reserve(points_.size());
    for (const auto& p : points_) t.push_back(p.timestamp_ms);
    return t;
}

TimeSeries TimeSeries::slice(int64_t from_ms, int64_t to_ms) const {
    TimeSeries result;
    for (const auto& p : points_) {
        if (p.timestamp_ms >= from_ms && p.timestamp_ms < to_ms)
            result.points_.push_back(p);
    }
    return result;
}

std::pair<TimeSeries, TimeSeries> TimeSeries::trainTestSplit(double ratio) const {
    if (ratio <= 0.0 || ratio >= 1.0)
        throw std::invalid_argument("train_ratio must be in (0, 1)");
    size_t split = static_cast<size_t>(std::round(ratio * static_cast<double>(points_.size())));
    split = std::max(size_t{1}, std::min(split, points_.size() - 1));
    TimeSeries train, test;
    train.points_.assign(points_.begin(), points_.begin() + static_cast<ptrdiff_t>(split));
    test.points_.assign(points_.begin() + static_cast<ptrdiff_t>(split), points_.end());
    return {train, test};
}

double TimeSeries::mean() const {
    if (points_.empty()) return 0.0;
    double s = 0.0;
    for (const auto& p : points_) s += p.value;
    return s / static_cast<double>(points_.size());
}

double TimeSeries::stddev() const {
    if (points_.size() < 2) return 0.0;
    double m = mean();
    double acc = 0.0;
    for (const auto& p : points_) { double d = p.value - m; acc += d * d; }
    return std::sqrt(acc / static_cast<double>(points_.size() - 1));
}

double TimeSeries::min() const {
    if (points_.empty()) return 0.0;
    double m = points_[0].value;
    for (const auto& p : points_) m = std::min(m, p.value);
    return m;
}

double TimeSeries::max() const {
    if (points_.empty()) return 0.0;
    double m = points_[0].value;
    for (const auto& p : points_) m = std::max(m, p.value);
    return m;
}

// ============================================================================
// Free helper: computeMetrics
// ============================================================================

ForecastMetrics computeMetrics(const std::vector<double>& actual,
                               const std::vector<double>& predicted)
{
    ForecastMetrics m;
    size_t n = std::min(actual.size(), predicted.size());
    if (n == 0) return m;
    m.n = n;
    double sum_abs  = 0.0;
    double sum_sq   = 0.0;
    double sum_mape = 0.0;
    double sum_smape = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double err = actual[i] - predicted[i];
        sum_abs  += std::abs(err);
        sum_sq   += err * err;
        if (std::abs(actual[i]) > 1e-10)
            sum_mape += std::abs(err) / std::abs(actual[i]);
        double denom = (std::abs(actual[i]) + std::abs(predicted[i]));
        if (denom > 1e-10)
            sum_smape += 2.0 * std::abs(err) / denom;
    }
    double dn = static_cast<double>(n);
    m.mae   = sum_abs  / dn;
    m.rmse  = std::sqrt(sum_sq / dn);
    m.mape  = (sum_mape  / dn) * 100.0;
    m.smape = (sum_smape / dn) * 100.0;
    return m;
}

// ============================================================================
// Anonymous namespace – shared algorithm helpers
// ============================================================================

namespace {

// Normal-distribution quantile (inverse CDF) via rational approximation
// (Beasley-Springer-Moro algorithm).
double normalQuantile(double p) {
    static const double a[] = {-3.969683028665376e+01, 2.209460984245205e+02,
                                -2.759285104469687e+02, 1.383577518672690e+02,
                                -3.066479806614716e+01, 2.506628277459239e+00};
    static const double b[] = {-5.447609879822406e+01, 1.615858368580409e+02,
                                -1.556989798598866e+02, 6.680131188771972e+01,
                                -1.328068155288572e+01};
    static const double c[] = {-7.784894002430293e-03, -3.223964580411365e-01,
                                -2.400758277161838e+00, -2.549732539343734e+00,
                                4.374664141464968e+00,  2.938163982698783e+00};
    static const double d[] = {7.784695709041462e-03, 3.224671290700398e-01,
                                2.445134137142996e+00, 3.754408661907416e+00};
    double q;
    if (p <= 0.0 || p >= 1.0) return (p <= 0.0) ? -1e38 : 1e38;
    if (p < 0.02425) {
        double t = std::sqrt(-2.0 * std::log(p));
        q = (((((c[0]*t+c[1])*t+c[2])*t+c[3])*t+c[4])*t+c[5]) /
            ((((d[0]*t+d[1])*t+d[2])*t+d[3])*t+1.0);
    } else if (p <= 0.97575) {
        double u = p - 0.5;
        double r = u * u;
        q = u * (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5]) /
               (((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1.0);
    } else {
        double t = std::sqrt(-2.0 * std::log(1.0 - p));
        q = -(((((c[0]*t+c[1])*t+c[2])*t+c[3])*t+c[4])*t+c[5]) /
             ((((d[0]*t+d[1])*t+d[2])*t+d[3])*t+1.0);
    }
    return q;
}

double zScore(double confidence) {
    return normalQuantile(0.5 + confidence * 0.5);
}

double computeMean(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size());
}

/// Compute the median of a SORTED vector.
double medianSorted(const std::vector<double>& sorted) {
    if (sorted.empty()) return 0.0;
    size_t n = sorted.size();
    return (n % 2 == 1) ? sorted[n / 2]
                        : 0.5 * (sorted[n / 2 - 1] + sorted[n / 2]);
}

/// Median interval between consecutive observations.
int64_t medianInterval(const std::vector<int64_t>& timestamps) {
    if (timestamps.size() < 2) return 1;
    std::vector<double> diffs;
    diffs.reserve(timestamps.size() - 1);
    for (size_t i = 1; i < timestamps.size(); ++i)
        diffs.push_back(static_cast<double>(timestamps[i] - timestamps[i - 1]));
    std::sort(diffs.begin(), diffs.end());
    double med = medianSorted(diffs);
    return static_cast<int64_t>(std::max(1.0, med));
}

// ------------------------------------------------------------------
// Linear regression
// ------------------------------------------------------------------

struct LinearParams { double alpha, beta, residual_stddev; };

LinearParams fitLinear(const std::vector<double>& y) {
    size_t n = y.size();
    if (n < 2) return {y.empty() ? 0.0 : y[0], 0.0, 0.0};

    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double xi = static_cast<double>(i);
        sx  += xi;
        sy  += y[i];
        sxx += xi * xi;
        sxy += xi * y[i];
    }
    double dn = static_cast<double>(n);
    double denom = dn * sxx - sx * sx;
    LinearParams p{};
    if (std::abs(denom) < 1e-12) {
        p.alpha = sy / dn;
        p.beta  = 0.0;
    } else {
        p.beta  = (dn * sxy - sx * sy) / denom;
        p.alpha = (sy - p.beta * sx) / dn;
    }
    // Residual std-dev
    double acc = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double res = y[i] - (p.alpha + p.beta * static_cast<double>(i));
        acc += res * res;
    }
    p.residual_stddev = (n > 2) ? std::sqrt(acc / static_cast<double>(n - 2)) : 0.0;
    return p;
}

// ------------------------------------------------------------------
// Exponential smoothing (SES)
// ------------------------------------------------------------------

struct SESParams {
    double alpha, last_level, residual_stddev;
};

SESParams fitSES(const std::vector<double>& y, double alpha) {
    if (y.empty()) return {alpha, 0.0, 0.0};
    double level = y[0];
    double ss = 0.0;
    size_t n = y.size();
    for (size_t i = 0; i < n; ++i) {
        double pred = level;
        double res  = y[i] - pred;
        ss   += res * res;
        level = alpha * y[i] + (1.0 - alpha) * level;
    }
    double residual_stddev = (n > 1) ? std::sqrt(ss / static_cast<double>(n - 1)) : 0.0;
    return {alpha, level, residual_stddev};
}

// ------------------------------------------------------------------
// Holt-Winters (triple exponential smoothing)
// ------------------------------------------------------------------

struct HoltWintersParams {
    double alpha, beta, gamma;
    double L, T;                    ///< final level and trend
    std::vector<double> S;          ///< seasonal indices (length == m)
    int    m;                       ///< seasonal period
    bool   multiplicative;
    double residual_stddev;
};

HoltWintersParams fitHoltWinters(const std::vector<double>& y,
                                  double alpha, double beta, double gamma,
                                  int m, bool multiplicative)
{
    size_t n = y.size();
    HoltWintersParams p{alpha, beta, gamma, 0.0, 0.0, {}, m, multiplicative, 0.0};

    // Need at least 2 full seasons for seasonal initialisation
    bool has_season = (m >= 2) && (static_cast<int>(n) >= 2 * m);

    if (!has_season) {
        // Fall back to Holt's (linear trend, no seasonality)
        if (n < 2) { p.L = y.empty() ? 0.0 : y[0]; return p; }
        double L0 = y[0];
        double T0 = y[1] - y[0];
        double L  = L0, T = T0;
        double ss = 0.0;
        for (size_t i = 1; i < n; ++i) {
            double pred = L + T;
            double res  = y[i] - pred;
            ss += res * res;
            double Lnew = alpha * y[i] + (1.0 - alpha) * (L + T);
            T = beta * (Lnew - L) + (1.0 - beta) * T;
            L = Lnew;
        }
        p.L = L; p.T = T;
        p.residual_stddev = (n > 2) ? std::sqrt(ss / static_cast<double>(n - 2)) : 0.0;
        return p;
    }

    // Seasonal initialisation: average of first m seasons
    int im = static_cast<int>(m);
    std::vector<double> season_avgs;
    int num_complete = static_cast<int>(n) / im;
    season_avgs.reserve(static_cast<size_t>(num_complete));
    for (int s = 0; s < num_complete; ++s) {
        double avg = 0.0;
        for (int j = 0; j < im; ++j) avg += y[static_cast<size_t>(s * im + j)];
        season_avgs.push_back(avg / static_cast<double>(im));
    }

    // L0 = mean of first full season
    double L = season_avgs[0];
    // T0 = slope from first to last complete season avg
    double T = (num_complete > 1)
        ? (season_avgs.back() - season_avgs.front()) / static_cast<double>((num_complete - 1) * im)
        : 0.0;

    // Initial seasonal components
    std::vector<double> S(static_cast<size_t>(im), 1.0);
    for (int j = 0; j < im; ++j) {
        double acc = 0.0;
        int cnt = 0;
        for (int s = 0; s < num_complete; ++s) {
            double base = season_avgs[s];
            if (base == 0.0) base = 1e-10;
            acc += multiplicative ? (y[static_cast<size_t>(s * im + j)] / base)
                                  : (y[static_cast<size_t>(s * im + j)] - base);
            ++cnt;
        }
        S[static_cast<size_t>(j)] = (cnt > 0) ? (acc / static_cast<double>(cnt)) : (multiplicative ? 1.0 : 0.0);
    }

    // Iterate
    double ss = 0.0;
    size_t forecast_count = 0;
    for (size_t i = 0; i < n; ++i) {
        int si = static_cast<int>(i) % im;
        double pred = multiplicative ? (L + T) * S[static_cast<size_t>(si)]
                                     : (L + T) + S[static_cast<size_t>(si)];
        double res = y[i] - pred;
        if (i > 0) { ss += res * res; ++forecast_count; }

        double Lnew, Tnew;
        double Snew;
        if (multiplicative) {
            double s_val = (S[static_cast<size_t>(si)] != 0.0) ? S[static_cast<size_t>(si)] : 1e-10;
            Lnew = alpha * (y[i] / s_val)       + (1.0 - alpha) * (L + T);
            Tnew = beta  * (Lnew - L)            + (1.0 - beta)  * T;
            Snew = gamma * (y[i] / (Lnew != 0.0 ? Lnew : 1e-10)) + (1.0 - gamma) * S[static_cast<size_t>(si)];
        } else {
            Lnew = alpha * (y[i] - S[static_cast<size_t>(si)]) + (1.0 - alpha) * (L + T);
            Tnew = beta  * (Lnew - L)                          + (1.0 - beta)  * T;
            Snew = gamma * (y[i] - Lnew)                       + (1.0 - gamma) * S[static_cast<size_t>(si)];
        }
        L = Lnew; T = Tnew;
        S[static_cast<size_t>(si)] = Snew;
    }
    p.L = L; p.T = T; p.S = S;
    p.residual_stddev = (forecast_count > 1)
        ? std::sqrt(ss / static_cast<double>(forecast_count - 1)) : 0.0;
    return p;
}

// ------------------------------------------------------------------
// ARIMA AR(p) + I(d) + MA(q) via Yule–Walker
// ------------------------------------------------------------------

struct ArimaParams {
    std::vector<double> ar_coeffs;  ///< AR coefficients φ_1…φ_p
    std::vector<double> ma_coeffs;  ///< MA coefficients θ_1…θ_q
    double              mean_diff;  ///< mean of (possibly differenced) series
    double              last_obs;   ///< last original observation
    std::vector<double> last_window; ///< last p values (differenced)
    std::vector<double> last_resid;  ///< last q residuals
    int                 d;
    double              residual_stddev;
};

/// Solve Yule-Walker equations using Levinson–Durbin recursion.
std::vector<double> yuleWalker(const std::vector<double>& y, int p) {
    size_t n = y.size();
    if (n == 0 || p <= 0) return {};

    double mean_y = computeMean(y);
    // Autocovariances r[0..p]
    std::vector<double> r(static_cast<size_t>(p + 1), 0.0);
    for (int k = 0; k <= p; ++k) {
        for (size_t i = static_cast<size_t>(k); i < n; ++i)
            r[static_cast<size_t>(k)] += (y[i] - mean_y) * (y[i - static_cast<size_t>(k)] - mean_y);
        r[static_cast<size_t>(k)] /= static_cast<double>(n);
    }

    if (r[0] < 1e-15) return std::vector<double>(static_cast<size_t>(p), 0.0);

    // Levinson–Durbin recursion
    std::vector<double> phi(static_cast<size_t>(p), 0.0);
    std::vector<double> phi_prev(static_cast<size_t>(p), 0.0);
    double err = r[0];

    for (int k = 1; k <= p; ++k) {
        double lambda = r[static_cast<size_t>(k)];
        for (int j = 1; j < k; ++j)
            lambda -= phi_prev[static_cast<size_t>(j - 1)] * r[static_cast<size_t>(k - j)];
        lambda /= err;
        phi[static_cast<size_t>(k - 1)] = lambda;
        for (int j = 1; j < k; ++j)
            phi[static_cast<size_t>(j - 1)] = phi_prev[static_cast<size_t>(j - 1)]
                                              - lambda * phi_prev[static_cast<size_t>(k - j - 1)];
        err *= (1.0 - lambda * lambda);
        phi_prev = phi;
        if (err < 1e-15) break;
    }
    return phi;
}

ArimaParams fitARIMA(const std::vector<double>& y, int p, int d, int q) {
    ArimaParams params{};
    params.d = d;
    if (y.empty()) return params;
    params.last_obs = y.back();

    // Differencing
    std::vector<double> yd = y;
    if (d == 1 && y.size() > 1) {
        std::vector<double> diff(y.size() - 1);
        for (size_t i = 1; i < y.size(); ++i) diff[i - 1] = y[i] - y[i - 1];
        yd = diff;
    }
    params.mean_diff = computeMean(yd);

    // Demean for AR fitting
    std::vector<double> yc = yd;
    for (double& v : yc) v -= params.mean_diff;

    // AR coefficients via Yule-Walker
    int actual_p = std::min(p, static_cast<int>(yc.size()) - 1);
    if (actual_p > 0) params.ar_coeffs = yuleWalker(yc, actual_p);
    else              params.ar_coeffs = {};

    // Compute AR residuals
    size_t n = yc.size();
    size_t ap = params.ar_coeffs.size();
    std::vector<double> residuals(n, 0.0);
    for (size_t i = ap; i < n; ++i) {
        double pred = 0.0;
        for (size_t j = 0; j < ap; ++j) pred += params.ar_coeffs[j] * yc[i - 1 - j];
        residuals[i] = yc[i] - pred;
    }

    // Simple MA(q) coefficients: regress residuals on lagged residuals (OLS)
    int actual_q = std::min(q, static_cast<int>(n) - static_cast<int>(ap) - 1);
    if (actual_q > 0 && n > ap + static_cast<size_t>(actual_q)) {
        // MA via iterative regression (simplified: one OLS pass)
        std::vector<double> ma_phi(static_cast<size_t>(actual_q), 0.0);
        for (int qi = 0; qi < actual_q; ++qi) {
            double sxy = 0.0, sxx = 0.0;
            for (size_t i = ap + static_cast<size_t>(qi) + 1; i < n; ++i) {
                sxy += residuals[i] * residuals[i - static_cast<size_t>(qi) - 1];
                sxx += residuals[i - static_cast<size_t>(qi) - 1] * residuals[i - static_cast<size_t>(qi) - 1];
            }
            ma_phi[static_cast<size_t>(qi)] = (sxx > 1e-12) ? sxy / sxx : 0.0;
        }
        params.ma_coeffs = ma_phi;
    }

    // Last window (for multi-step ahead)
    size_t win_size = static_cast<size_t>(std::max(actual_p, 1));
    if (n >= win_size) {
        params.last_window.assign(yc.end() - static_cast<ptrdiff_t>(win_size), yc.end());
    } else {
        params.last_window = yc;
    }

    // Last residuals
    size_t res_size = static_cast<size_t>(std::max(actual_q, 1));
    if (n >= res_size) {
        params.last_resid.assign(residuals.end() - static_cast<ptrdiff_t>(res_size), residuals.end());
    } else {
        params.last_resid = residuals;
    }

    // Residual std-dev
    size_t cnt = 0;
    double ss = 0.0;
    for (size_t i = ap; i < n; ++i) {
        double full_res = residuals[i];
        // subtract MA contribution
        for (size_t j = 0; j < params.ma_coeffs.size() && j < i; ++j)
            full_res -= params.ma_coeffs[j] * residuals[i - 1 - j];
        ss += full_res * full_res; ++cnt;
    }
    params.residual_stddev = (cnt > 1) ? std::sqrt(ss / static_cast<double>(cnt - 1)) : 0.0;
    return params;
}

} // anonymous namespace

// ============================================================================
// ForecastModel::Impl
// ============================================================================

struct ForecastModel::Impl {
    ForecastMethod  method;
    ForecastConfig  config;
    bool            fitted = false;

    // Training series (needed for decompose / evaluate)
    std::vector<double>  train_y;
    std::vector<int64_t> train_ts;

    // Fitted parameters
    LinearParams      linear_p{};
    SESParams         ses_p{};
    HoltWintersParams hw_p{};
    ArimaParams       arima_p{};

    // In-sample RMSE
    double in_sample_rmse = 0.0;

    // ---- fit helpers ----
    void fitLinear() {
        linear_p = ::themisdb::analytics::fitLinear(train_y);
    }

    void fitSES() {
        ses_p = ::themisdb::analytics::fitSES(train_y, config.alpha);
    }

    void fitHW() {
        hw_p = fitHoltWinters(train_y, config.alpha, config.beta, config.gamma,
                              config.seasonality, config.multiplicative);
    }

    void fitAR() {
        arima_p = fitARIMA(train_y, config.ar_order, config.diff_order, config.ma_order);
    }

    // ---- single-model predict ----

    std::vector<double> predictLinear(int steps) const {
        std::vector<double> out;
        out.reserve(static_cast<size_t>(steps));
        // Use train_ts.size() so this also works after deserialization
        // (train_y is not persisted but train_ts is).
        size_t n = train_ts.empty() ? train_y.size() : train_ts.size();
        double n_last = static_cast<double>(n) - 1.0;
        for (int k = 1; k <= steps; ++k)
            out.push_back(linear_p.alpha + linear_p.beta * (n_last + static_cast<double>(k)));
        return out;
    }

    std::vector<double> predictSES(int steps) const {
        // All future values equal the last smoothed level
        return std::vector<double>(static_cast<size_t>(steps), ses_p.last_level);
    }

    std::vector<double> predictHW(int steps) const {
        std::vector<double> out;
        out.reserve(static_cast<size_t>(steps));
        double L = hw_p.L;
        double T = hw_p.T;
        const auto& S = hw_p.S;
        int m = hw_p.m;
        bool has_season = (m >= 2) && !S.empty();
        // Use train_ts.size() so this also works after deserialization
        size_t n = train_ts.empty() ? train_y.size() : train_ts.size();
        int train_n = static_cast<int>(n);

        for (int k = 1; k <= steps; ++k) {
            double val;
            if (has_season) {
                int si = (train_n - 1 + k) % m;
                if (si < 0) si += m;
                val = hw_p.multiplicative
                    ? (L + static_cast<double>(k) * T) * S[static_cast<size_t>(si)]
                    : (L + static_cast<double>(k) * T) + S[static_cast<size_t>(si)];
            } else {
                val = L + static_cast<double>(k) * T;
            }
            out.push_back(val);
        }
        return out;
    }

    std::vector<double> predictARIMA(int steps) const {
        std::vector<double> out;
        out.reserve(static_cast<size_t>(steps));

        // Multi-step AR forecast (iterated one-step-ahead)
        std::vector<double> window = arima_p.last_window;
        std::vector<double> resid  = arima_p.last_resid;
        size_t ap = arima_p.ar_coeffs.size();
        size_t mq = arima_p.ma_coeffs.size();
        double last_val = arima_p.last_obs;

        for (int k = 0; k < steps; ++k) {
            // AR contribution (demeaned)
            double ar_contrib = 0.0;
            for (size_t j = 0; j < ap && j < window.size(); ++j)
                ar_contrib += arima_p.ar_coeffs[j] * window[window.size() - 1 - j];
            // MA contribution (residuals set to 0 for future steps)
            double ma_contrib = 0.0;
            for (size_t j = 0; j < mq && j < resid.size(); ++j)
                ma_contrib += arima_p.ma_coeffs[j] * resid[resid.size() - 1 - j];
            // Future residuals are 0
            double pred_diff = arima_p.mean_diff + ar_contrib + ma_contrib;

            // Integrate if d == 1
            double pred_val;
            if (arima_p.d == 1) {
                pred_val = last_val + pred_diff;
                last_val = pred_val;
            } else {
                pred_val = pred_diff;
            }
            out.push_back(pred_val);

            // Update window with this forecast (demeaned)
            window.push_back(pred_diff - arima_p.mean_diff);
            if (window.size() > ap + 10) window.erase(window.begin());
            // No new residual: set to 0
            resid.push_back(0.0);
            if (resid.size() > mq + 10) resid.erase(resid.begin());
        }
        return out;
    }

    std::vector<double> predictEnsemble(int steps) const {
        // Equal weights unless config provides them
        std::vector<std::vector<double>> forecasts = {
            predictLinear(steps),
            predictSES(steps),
            predictHW(steps),
            predictARIMA(steps)
        };
        std::vector<double> weights(4, 1.0);
        if (config.ensemble_weights.size() == 4)
            weights = config.ensemble_weights;
        double wsum = 0.0;
        for (double w : weights) wsum += w;
        if (wsum < 1e-12) { wsum = 4.0; std::fill(weights.begin(), weights.end(), 1.0); }

        std::vector<double> out(static_cast<size_t>(steps), 0.0);
        for (size_t m = 0; m < 4; ++m)
            for (int k = 0; k < steps; ++k)
                out[static_cast<size_t>(k)] += weights[m] * forecasts[m][static_cast<size_t>(k)];
        for (double& v : out) v /= wsum;
        return out;
    }

    std::vector<double> predict(int steps) const {
        switch (method) {
            case ForecastMethod::LINEAR_REGRESSION: return predictLinear(steps);
            case ForecastMethod::EXP_SMOOTHING:     return predictSES(steps);
            case ForecastMethod::HOLT_WINTERS:      return predictHW(steps);
            case ForecastMethod::ARIMA:             return predictARIMA(steps);
            case ForecastMethod::ENSEMBLE:          return predictEnsemble(steps);
        }
        return predictLinear(steps);
    }

    double residualStddev() const {
        switch (method) {
            case ForecastMethod::LINEAR_REGRESSION: return linear_p.residual_stddev;
            case ForecastMethod::EXP_SMOOTHING:     return ses_p.residual_stddev;
            case ForecastMethod::HOLT_WINTERS:      return hw_p.residual_stddev;
            case ForecastMethod::ARIMA:             return arima_p.residual_stddev;
            case ForecastMethod::ENSEMBLE: {
                // Average residual std-devs
                double s = linear_p.residual_stddev + ses_p.residual_stddev
                         + hw_p.residual_stddev      + arima_p.residual_stddev;
                return s / 4.0;
            }
        }
        return 0.0;
    }
};

// ============================================================================
// ForecastModel – public interface
// ============================================================================

ForecastModel::ForecastModel(ForecastMethod method)
    : impl_(std::make_unique<Impl>())
{
    impl_->method = method;
}

ForecastModel::ForecastModel(const ForecastConfig& config, ForecastMethod method)
    : impl_(std::make_unique<Impl>())
{
    impl_->method = method;
    impl_->config = config;
}

ForecastModel::~ForecastModel() = default;
ForecastModel::ForecastModel(ForecastModel&&) noexcept = default;
ForecastModel& ForecastModel::operator=(ForecastModel&&) noexcept = default;

void ForecastModel::fit(const TimeSeries& ts) {
    fit(ts, impl_->config);
}

void ForecastModel::fit(const TimeSeries& ts, const ForecastConfig& config) {
    if (ts.size() < 2)
        throw std::invalid_argument("TimeSeries must have at least 2 points to fit");

    impl_->config   = config;
    impl_->train_y  = ts.values();
    impl_->train_ts = ts.timestamps();

    // Auto-tune alpha/beta/gamma if requested (simple grid search)
    if (config.auto_tune && impl_->method != ForecastMethod::LINEAR_REGRESSION
                         && impl_->method != ForecastMethod::ARIMA) {
        // Grid-search alpha over {0.1, 0.2, …, 0.9}
        double best_rmse = std::numeric_limits<double>::max();
        double best_alpha = config.alpha;
        for (int ai = 1; ai <= 9; ++ai) {
            double a = 0.1 * ai;
            // Compute in-sample RMSE for this alpha
            double lvl = impl_->train_y[0];
            double ss = 0.0; size_t cnt = 0;
            for (size_t i = 1; i < impl_->train_y.size(); ++i) {
                double err = impl_->train_y[i] - lvl;
                ss += err * err; ++cnt;
                lvl = a * impl_->train_y[i] + (1.0 - a) * lvl;
            }
            double rmse = (cnt > 0) ? std::sqrt(ss / static_cast<double>(cnt)) : 1e38;
            if (rmse < best_rmse) { best_rmse = rmse; best_alpha = a; }
        }
        impl_->config.alpha = best_alpha;
    }

    // Fit all sub-models (needed for ENSEMBLE, and for decompose)
    impl_->fitLinear();
    impl_->fitSES();
    impl_->fitHW();
    impl_->fitAR();
    impl_->fitted = true;

    // In-sample RMSE
    auto preds = impl_->predict(static_cast<int>(impl_->train_y.size()) - 1);
    double ss = 0.0;
    for (size_t i = 0; i < preds.size(); ++i) {
        double err = impl_->train_y[i + 1] - preds[i];
        ss += err * err;
    }
    impl_->in_sample_rmse = preds.empty() ? 0.0
        : std::sqrt(ss / static_cast<double>(preds.size()));
}

bool ForecastModel::isFitted() const noexcept {
    return impl_->fitted;
}

std::vector<ForecastPoint> ForecastModel::predict(int steps) const {
    if (!impl_->fitted)
        throw std::runtime_error("ForecastModel: call fit() before predict()");
    if (steps <= 0) return {};

    auto values = impl_->predict(steps);

    // Determine next timestamp and interval
    int64_t last_ts = impl_->train_ts.empty() ? 0 : impl_->train_ts.back();
    int64_t interval = medianInterval(impl_->train_ts);

    double sigma  = impl_->residualStddev();
    double z      = (impl_->config.include_confidence && impl_->config.confidence_level > 0.0)
                    ? zScore(impl_->config.confidence_level) : 0.0;

    std::vector<ForecastPoint> result;
    result.reserve(static_cast<size_t>(steps));
    for (int k = 0; k < steps; ++k) {
        ForecastPoint fp;
        fp.timestamp_ms = last_ts + static_cast<int64_t>(k + 1) * interval;
        fp.value        = values[static_cast<size_t>(k)];
        double margin   = z * sigma * std::sqrt(static_cast<double>(k + 1));
        fp.lower        = fp.value - margin;
        fp.upper        = fp.value + margin;
        result.push_back(fp);
    }
    return result;
}

ForecastMetrics ForecastModel::evaluate(const TimeSeries& test_ts) const {
    if (!impl_->fitted)
        throw std::runtime_error("ForecastModel: call fit() before evaluate()");
    if (test_ts.empty()) return {};

    int steps = static_cast<int>(test_ts.size());
    auto preds = impl_->predict(steps);
    return computeMetrics(test_ts.values(), preds);
}

DecompositionResult ForecastModel::decompose(bool multiplicative) const {
    if (!impl_->fitted)
        throw std::runtime_error("ForecastModel: call fit() before decompose()");

    const auto& y = impl_->train_y;
    size_t n = y.size();
    DecompositionResult dr;
    dr.multiplicative = multiplicative;
    dr.trend.resize(n, 0.0);
    dr.seasonal.resize(n, multiplicative ? 1.0 : 0.0);
    dr.residual.resize(n, 0.0);

    if (n < 4) {
        for (size_t i = 0; i < n; ++i) dr.residual[i] = y[i];
        return dr;
    }

    // Trend: centred moving average of window = min(seasonality, n/3)
    int m = (impl_->config.seasonality >= 2) ? impl_->config.seasonality
                                              : static_cast<int>(std::max(size_t{3}, n / 5));
    m = std::min(m, static_cast<int>(n) - 1);
    for (size_t i = 0; i < n; ++i) {
        int lo = static_cast<int>(i) - m / 2;
        int hi = static_cast<int>(i) + m / 2;
        lo = std::max(lo, 0);
        hi = std::min(hi, static_cast<int>(n) - 1);
        double acc = 0.0;
        for (int j = lo; j <= hi; ++j) acc += y[static_cast<size_t>(j)];
        dr.trend[i] = acc / static_cast<double>(hi - lo + 1);
    }

    // Seasonal: average detrended values per period
    if (impl_->config.seasonality >= 2) {
        int period = impl_->config.seasonality;
        std::vector<double> season_acc(static_cast<size_t>(period), 0.0);
        std::vector<int>    season_cnt(static_cast<size_t>(period), 0);
        for (size_t i = 0; i < n; ++i) {
            int si = static_cast<int>(i) % period;
            double base = dr.trend[i];
            season_acc[static_cast<size_t>(si)] += multiplicative
                ? (base != 0.0 ? y[i] / base : 1.0)
                : (y[i] - base);
            ++season_cnt[static_cast<size_t>(si)];
        }
        for (size_t i = 0; i < n; ++i) {
            int si = static_cast<int>(i) % period;
            dr.seasonal[i] = (season_cnt[static_cast<size_t>(si)] > 0)
                ? season_acc[static_cast<size_t>(si)] / static_cast<double>(season_cnt[static_cast<size_t>(si)])
                : (multiplicative ? 1.0 : 0.0);
        }
    }

    // Residual
    for (size_t i = 0; i < n; ++i) {
        dr.residual[i] = multiplicative
            ? (dr.seasonal[i] != 0.0 && dr.trend[i] != 0.0
               ? y[i] / (dr.trend[i] * dr.seasonal[i]) : 1.0)
            : (y[i] - dr.trend[i] - dr.seasonal[i]);
    }
    return dr;
}

std::string ForecastModel::serialize() const {
    std::ostringstream oss;
    oss << "method=" << static_cast<int>(impl_->method) << "\n";
    oss << "fitted=" << (impl_->fitted ? 1 : 0) << "\n";
    oss << "alpha=" << impl_->config.alpha << "\n";
    oss << "beta=" << impl_->config.beta << "\n";
    oss << "gamma=" << impl_->config.gamma << "\n";
    oss << "seasonality=" << impl_->config.seasonality << "\n";
    oss << "multiplicative=" << (impl_->config.multiplicative ? 1 : 0) << "\n";
    oss << "ar_order=" << impl_->config.ar_order << "\n";
    oss << "diff_order=" << impl_->config.diff_order << "\n";
    oss << "ma_order=" << impl_->config.ma_order << "\n";
    oss << "in_sample_rmse=" << impl_->in_sample_rmse << "\n";
    // Linear params
    oss << "lin_alpha=" << impl_->linear_p.alpha << "\n";
    oss << "lin_beta=" << impl_->linear_p.beta << "\n";
    oss << "lin_sigma=" << impl_->linear_p.residual_stddev << "\n";
    // SES params
    oss << "ses_alpha=" << impl_->ses_p.alpha << "\n";
    oss << "ses_level=" << impl_->ses_p.last_level << "\n";
    oss << "ses_sigma=" << impl_->ses_p.residual_stddev << "\n";
    // HW params
    oss << "hw_L=" << impl_->hw_p.L << "\n";
    oss << "hw_T=" << impl_->hw_p.T << "\n";
    oss << "hw_sigma=" << impl_->hw_p.residual_stddev << "\n";
    oss << "hw_S_size=" << impl_->hw_p.S.size() << "\n";
    for (size_t i = 0; i < impl_->hw_p.S.size(); ++i)
        oss << "hw_S_" << i << "=" << impl_->hw_p.S[i] << "\n";
    // Training timestamps
    oss << "train_n=" << impl_->train_ts.size() << "\n";
    for (size_t i = 0; i < impl_->train_ts.size(); ++i)
        oss << "ts_" << i << "=" << impl_->train_ts[i] << "\n";
    return oss.str();
}

ForecastModel ForecastModel::deserialize(const std::string& data) {
    ForecastModel model;
    std::istringstream iss(data);
    std::string line;
    auto read = [&](const std::string& key) -> std::string {
        std::string prefix = key + "=";
        std::istringstream ss2(data);
        std::string l2;
        while (std::getline(ss2, l2)) {
            if (l2.rfind(prefix, 0) == 0)
                return l2.substr(prefix.size());
        }
        return "";
    };
    auto readD = [&](const std::string& key) -> double {
        std::string v = read(key);
        return v.empty() ? 0.0 : std::stod(v);
    };
    auto readI = [&](const std::string& key) -> int {
        std::string v = read(key);
        return v.empty() ? 0 : std::stoi(v);
    };

    model.impl_->method = static_cast<ForecastMethod>(readI("method"));
    model.impl_->fitted = (readI("fitted") == 1);
    model.impl_->config.alpha         = readD("alpha");
    model.impl_->config.beta          = readD("beta");
    model.impl_->config.gamma         = readD("gamma");
    model.impl_->config.seasonality   = readI("seasonality");
    model.impl_->config.multiplicative = (readI("multiplicative") == 1);
    model.impl_->config.ar_order      = readI("ar_order");
    model.impl_->config.diff_order    = readI("diff_order");
    model.impl_->config.ma_order      = readI("ma_order");
    model.impl_->in_sample_rmse       = readD("in_sample_rmse");
    model.impl_->linear_p.alpha       = readD("lin_alpha");
    model.impl_->linear_p.beta        = readD("lin_beta");
    model.impl_->linear_p.residual_stddev = readD("lin_sigma");
    model.impl_->ses_p.alpha          = readD("ses_alpha");
    model.impl_->ses_p.last_level     = readD("ses_level");
    model.impl_->ses_p.residual_stddev = readD("ses_sigma");
    model.impl_->hw_p.L               = readD("hw_L");
    model.impl_->hw_p.T               = readD("hw_T");
    model.impl_->hw_p.residual_stddev = readD("hw_sigma");
    model.impl_->hw_p.alpha           = model.impl_->config.alpha;
    model.impl_->hw_p.beta            = model.impl_->config.beta;
    model.impl_->hw_p.gamma           = model.impl_->config.gamma;
    model.impl_->hw_p.m               = model.impl_->config.seasonality;
    model.impl_->hw_p.multiplicative  = model.impl_->config.multiplicative;
    int hw_s_size = readI("hw_S_size");
    model.impl_->hw_p.S.resize(static_cast<size_t>(hw_s_size));
    for (int i = 0; i < hw_s_size; ++i)
        model.impl_->hw_p.S[static_cast<size_t>(i)] = readD("hw_S_" + std::to_string(i));
    int train_n = readI("train_n");
    model.impl_->train_ts.resize(static_cast<size_t>(train_n));
    for (int i = 0; i < train_n; ++i)
        model.impl_->train_ts[static_cast<size_t>(i)] = static_cast<int64_t>(readD("ts_" + std::to_string(i)));

    (void)line;
    return model;
}

ForecastModel::ModelInfo ForecastModel::info() const {
    ModelInfo mi;
    mi.method           = impl_->method;
    mi.fitted           = impl_->fitted;
    mi.training_points  = impl_->train_y.size();
    mi.in_sample_rmse   = impl_->in_sample_rmse;
    if (!impl_->train_ts.empty()) {
        mi.train_start_ms = impl_->train_ts.front();
        mi.train_end_ms   = impl_->train_ts.back();
        mi.median_interval_ms = medianInterval(impl_->train_ts);
    }
    return mi;
}

ForecastMethod ForecastModel::method() const noexcept {
    return impl_->method;
}

const ForecastConfig& ForecastModel::config() const noexcept {
    return impl_->config;
}

} // namespace analytics
} // namespace themisdb

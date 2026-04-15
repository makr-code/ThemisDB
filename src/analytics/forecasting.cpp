/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            forecasting.cpp                                    ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:15:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1488                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d1e63d24c0  2026-04-13  perf(analytics): O(1) incremental OLS update + forecastin... ║
    • a15f06cbdd  2026-03-25  feat(analytics): batch prediction, update(), parallel aut... ║
    • 971a3c49d5  2026-03-20  Build/test fixes and auth role mapping refactor ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • 89af7a908e  2026-03-17  perf(analytics): cache AVX-512 CPUID check in static cons... ║
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

#include <array>
#include <cmath>
#include <future>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// SIMD intrinsics — guarded so non-SIMD platforms compile cleanly.
#if defined(__AVX512F__)
#  include <immintrin.h>
#elif defined(__AVX2__)
#  include <immintrin.h>
#endif

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

// ---------------------------------------------------------------------------
// SIMD-accelerated autocovariance kernel for the Yule–Walker equations.
//
// acov0_avx2  : AVX2  path — processes 4 doubles/cycle.
// acov0_avx512: AVX-512 path — processes 8 doubles/cycle (2× AVX2).
//
// Both compute sum_{i=lag}^{n-1} (y[i] - mean) * (y[i-lag] - mean),
// which is the (unnormalized) autocovariance at a given lag.
// lag == 0 reduces to the sum of squares (variance).
//
// The AVX-512 path is additionally protected by __builtin_cpu_supports() so
// that a binary compiled with -mavx512f still runs on AVX2-only hardware.
// ---------------------------------------------------------------------------

// Cache the AVX-512 runtime support check — avoids repeated CPUID calls when
// yuleWalker() is invoked in a loop (e.g., batch prediction or ARIMA search).
// Initialized once at first use; thread-safe under C++11 static initialisation.
#if defined(__AVX512F__)
static const bool kHasAVX512 = __builtin_cpu_supports("avx512f");
#endif

#if defined(__AVX2__)
static double acov0_avx2(const double* y, size_t n,
                         double mean, int lag) noexcept {
    const size_t start = static_cast<size_t>(lag);
    double acc = 0.0;
    size_t i = start;

    __m256d vmean = _mm256_set1_pd(mean);
    __m256d vacc  = _mm256_setzero_pd();

    for (; i + 3 < n; i += 4) {
        __m256d vi   = _mm256_loadu_pd(y + i);
        __m256d vi_k = _mm256_loadu_pd(y + i - start);
        __m256d di   = _mm256_sub_pd(vi,   vmean);
        __m256d di_k = _mm256_sub_pd(vi_k, vmean);
        vacc = _mm256_add_pd(vacc, _mm256_mul_pd(di, di_k));
    }
    double lane[4];
    _mm256_storeu_pd(lane, vacc);
    acc = lane[0] + lane[1] + lane[2] + lane[3];

    for (; i < n; ++i)
        acc += (y[i] - mean) * (y[i - start] - mean);

    return acc;
}
#endif  // __AVX2__

#if defined(__AVX512F__)
static double acov0_avx512(const double* y, size_t n,
                            double mean, int lag) noexcept {
    const size_t start = static_cast<size_t>(lag);
    double acc = 0.0;
    size_t i = start;

    __m512d vmean = _mm512_set1_pd(mean);
    __m512d vacc  = _mm512_setzero_pd();

    for (; i + 7 < n; i += 8) {
        __m512d vi   = _mm512_loadu_pd(y + i);
        __m512d vi_k = _mm512_loadu_pd(y + i - start);
        __m512d di   = _mm512_sub_pd(vi,   vmean);
        __m512d di_k = _mm512_sub_pd(vi_k, vmean);
        vacc = _mm512_add_pd(vacc, _mm512_mul_pd(di, di_k));
    }
    acc = _mm512_reduce_add_pd(vacc);

    for (; i < n; ++i)
        acc += (y[i] - mean) * (y[i - start] - mean);

    return acc;
}
#endif  // __AVX512F__

// Dispatch: pick the best available autocovariance kernel at runtime.
static double computeAutocovariance(const double* y, size_t n,
                                    double mean, int lag) noexcept {
#if defined(__AVX512F__)
    if (n >= 8 && kHasAVX512)
        return acov0_avx512(y, n, mean, lag);
#endif
#if defined(__AVX2__)
    if (n >= 4)
        return acov0_avx2(y, n, mean, lag);
#endif
    // Scalar fallback
    const size_t start = static_cast<size_t>(lag);
    double acc = 0.0;
    for (size_t i = start; i < n; ++i)
        acc += (y[i] - mean) * (y[i - start] - mean);
    return acc;
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
    // Autocovariances r[0..p] — inner loop accelerated by AVX-512 / AVX2.
    std::vector<double> r(static_cast<size_t>(p + 1), 0.0);
    for (int k = 0; k <= p; ++k) {
        r[static_cast<size_t>(k)] =
            computeAutocovariance(y.data(), n, mean_y, k) / static_cast<double>(n);
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

    // ---- Running OLS moments for O(1) incremental linear regression update ----
    // Populated by fitLinear() and updated O(1) by update().
    // x values are 0-based integer indices (0, 1, 2, …, n-1).
    double   lin_sx  = 0.0; ///< Σ x_i
    double   lin_sy  = 0.0; ///< Σ y_i
    double   lin_sxx = 0.0; ///< Σ x_i²
    double   lin_sxy = 0.0; ///< Σ x_i·y_i
    size_t   lin_n   = 0;   ///< number of observations in running sums

    // ---- fit-result cache ------------------------------------------------
    // Key: FNV-1a 64-bit hash over (training_data_bytes + config_bytes).
    // On cache hit we skip re-fitting and reuse the cached params.
    struct FitCacheKey {
        uint64_t data_hash   = 0;
        uint64_t config_hash = 0;
        bool operator==(const FitCacheKey& o) const noexcept {
            return data_hash == o.data_hash && config_hash == o.config_hash;
        }
    };
    struct FitCacheKeyHash {
        size_t operator()(const FitCacheKey& k) const noexcept {
            // Combine two 64-bit values using the Boost hash_combine pattern.
            // 0x9e3779b97f4a7c15 is the 64-bit golden-ratio increment constant.
            static constexpr uint64_t kGoldenRatio = 0x9e3779b97f4a7c15ULL;
            size_t h = static_cast<size_t>(k.data_hash);
            h ^= static_cast<size_t>(k.config_hash) + kGoldenRatio + (h << 6) + (h >> 2);
            return h;
        }
    };
    struct FitCacheEntry {
        LinearParams      linear_p{};
        SESParams         ses_p{};
        HoltWintersParams hw_p{};
        ArimaParams       arima_p{};
        double            in_sample_rmse = 0.0;
        ForecastConfig    config{};
        // Running OLS moments (for O(1) incremental update after cache restore)
        double   lin_sx  = 0.0;
        double   lin_sy  = 0.0;
        double   lin_sxx = 0.0;
        double   lin_sxy = 0.0;
        size_t   lin_n   = 0;
    };
    // Single-entry cache (last fit)
    bool            cache_valid = false;
    FitCacheKey     cache_key{};
    FitCacheEntry   cache_entry{};

    // FNV-1a 64-bit hash over arbitrary bytes
    static uint64_t fnv1a64(const void* data, size_t len) noexcept {
        uint64_t hash = 14695981039346656037ULL;
        const uint8_t* p = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < len; ++i) {
            hash ^= static_cast<uint64_t>(p[i]);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    FitCacheKey computeCacheKey(const std::vector<double>& y,
                                const std::vector<int64_t>& ts,
                                const ForecastConfig& cfg) const noexcept {
        FitCacheKey k;
        // Hash the value vector bytes
        k.data_hash = fnv1a64(y.data(), y.size() * sizeof(double));
        k.data_hash = fnv1a64(ts.data(), ts.size() * sizeof(int64_t)) ^ k.data_hash;
        // Hash the config fields that affect fitting
        uint64_t cfg_h = fnv1a64(&cfg.alpha, sizeof(cfg.alpha));
        cfg_h ^= fnv1a64(&cfg.beta,            sizeof(cfg.beta));
        cfg_h ^= fnv1a64(&cfg.gamma,           sizeof(cfg.gamma));
        cfg_h ^= fnv1a64(&cfg.seasonality,     sizeof(cfg.seasonality));
        cfg_h ^= fnv1a64(&cfg.ar_order,        sizeof(cfg.ar_order));
        cfg_h ^= fnv1a64(&cfg.diff_order,      sizeof(cfg.diff_order));
        cfg_h ^= fnv1a64(&cfg.ma_order,        sizeof(cfg.ma_order));
        uint8_t flags = (cfg.auto_tune ? 1 : 0)
                      | (cfg.multiplicative ? 2 : 0)
                      | (cfg.include_confidence ? 4 : 0);
        cfg_h ^= static_cast<uint64_t>(flags);
        k.config_hash = cfg_h;
        return k;
    }

    // ---- fit helpers ----
    void fitLinear() {
        linear_p = ::themisdb::analytics::fitLinear(train_y);
        // Populate running OLS moments for O(1) incremental update().
        lin_n = train_y.size();
        lin_sx = 0.0; lin_sy = 0.0; lin_sxx = 0.0; lin_sxy = 0.0;
        for (size_t i = 0; i < lin_n; ++i) {
            double xi = static_cast<double>(i);
            lin_sx  += xi;
            lin_sy  += train_y[i];
            lin_sxx += xi * xi;
            lin_sxy += xi * train_y[i];
        }
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

    // ---- fit-result cache check ----------------------------------------
    auto ck = impl_->computeCacheKey(impl_->train_y, impl_->train_ts, config);
    if (impl_->cache_valid && impl_->cache_key == ck) {
        // Restore cached parameters — skip all expensive fitting
        impl_->linear_p      = impl_->cache_entry.linear_p;
        impl_->ses_p         = impl_->cache_entry.ses_p;
        impl_->hw_p          = impl_->cache_entry.hw_p;
        impl_->arima_p       = impl_->cache_entry.arima_p;
        impl_->in_sample_rmse = impl_->cache_entry.in_sample_rmse;
        impl_->config        = impl_->cache_entry.config;
        impl_->lin_sx  = impl_->cache_entry.lin_sx;
        impl_->lin_sy  = impl_->cache_entry.lin_sy;
        impl_->lin_sxx = impl_->cache_entry.lin_sxx;
        impl_->lin_sxy = impl_->cache_entry.lin_sxy;
        impl_->lin_n   = impl_->cache_entry.lin_n;
        impl_->fitted        = true;
        return;
    }

    // Auto-tune alpha/beta/gamma if requested — parallelised over 9 grid points
    if (config.auto_tune && impl_->method != ForecastMethod::LINEAR_REGRESSION
                         && impl_->method != ForecastMethod::ARIMA) {
        const auto& y_ref = impl_->train_y;
        // Launch 9 async tasks (one per alpha value)
        std::array<std::future<std::pair<double,double>>, 9> futures;
        for (int ai = 1; ai <= 9; ++ai) {
            double a = 0.1 * static_cast<double>(ai);
            futures[static_cast<size_t>(ai - 1)] = std::async(
                std::launch::async,
                [a, &y_ref]() -> std::pair<double,double> {
                    double lvl = y_ref[0];
                    double ss  = 0.0;
                    size_t cnt = 0;
                    for (size_t i = 1; i < y_ref.size(); ++i) {
                        double err = y_ref[i] - lvl;
                        ss += err * err; ++cnt;
                        lvl = a * y_ref[i] + (1.0 - a) * lvl;
                    }
                    double rmse = (cnt > 0)
                        ? std::sqrt(ss / static_cast<double>(cnt)) : 1e38;
                    return {rmse, a};
                });
        }
        double best_rmse  = std::numeric_limits<double>::max();
        double best_alpha = config.alpha;
        for (auto& f : futures) {
            auto [rmse, a] = f.get();
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

    // ---- populate cache ------------------------------------------------
    impl_->cache_entry.linear_p       = impl_->linear_p;
    impl_->cache_entry.ses_p          = impl_->ses_p;
    impl_->cache_entry.hw_p           = impl_->hw_p;
    impl_->cache_entry.arima_p        = impl_->arima_p;
    impl_->cache_entry.in_sample_rmse = impl_->in_sample_rmse;
    impl_->cache_entry.config         = impl_->config;
    impl_->cache_entry.lin_sx  = impl_->lin_sx;
    impl_->cache_entry.lin_sy  = impl_->lin_sy;
    impl_->cache_entry.lin_sxx = impl_->lin_sxx;
    impl_->cache_entry.lin_sxy = impl_->lin_sxy;
    impl_->cache_entry.lin_n   = impl_->lin_n;
    impl_->cache_key   = ck;
    impl_->cache_valid = true;
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

std::vector<std::vector<ForecastPoint>> ForecastModel::predictBatch(
    const std::vector<TimeSeries>& batch, int steps) const
{
    if (steps < 1)
        throw std::invalid_argument("ForecastModel::predictBatch: steps must be >= 1");

    std::vector<std::vector<ForecastPoint>> results;
    results.reserve(batch.size());

    // Reuse this model's config and method for each series in the batch.
    // Each series gets its own lightweight ForecastModel so that the
    // caller's fitted state is not modified.
    for (const auto& ts : batch) {
        ForecastModel m(impl_->config, impl_->method);
        m.fit(ts);
        results.push_back(m.predict(steps));
    }
    return results;
}

void ForecastModel::update(double new_value) {
    if (!impl_->fitted) return;  // no-op if not fitted

    // ---- append to training series ----
    int64_t interval = medianInterval(impl_->train_ts);
    int64_t next_ts  = impl_->train_ts.empty()
                     ? 0
                     : impl_->train_ts.back() + interval;
    impl_->train_ts.push_back(next_ts);
    impl_->train_y.push_back(new_value);

    // Invalidate fit cache (data changed)
    impl_->cache_valid = false;

    const double y = new_value;

    // ---- LINEAR_REGRESSION: O(1) incremental OLS update via running moments ----
    // Running sums (lin_sx, lin_sy, lin_sxx, lin_sxy, lin_n) were populated
    // during fitLinear() and are updated here in O(1) per new observation.
    // Residual std-dev is tracked with an exponential moving average (EMA) so
    // the confidence-interval width stays approximately current without O(n) refit.
    {
        auto& lp = impl_->linear_p;
        if (impl_->lin_n > 0) {
            double x_new = static_cast<double>(impl_->lin_n); // next 0-based index
            impl_->lin_sx  += x_new;
            impl_->lin_sy  += y;
            impl_->lin_sxx += x_new * x_new;
            impl_->lin_sxy += x_new * y;
            impl_->lin_n   += 1;
            double dn    = static_cast<double>(impl_->lin_n);
            double denom = dn * impl_->lin_sxx - impl_->lin_sx * impl_->lin_sx;
            if (std::abs(denom) > 1e-12) {
                lp.beta  = (dn * impl_->lin_sxy - impl_->lin_sx * impl_->lin_sy) / denom;
                lp.alpha = (impl_->lin_sy - lp.beta * impl_->lin_sx) / dn;
            } else {
                lp.alpha = impl_->lin_sy / dn;
                lp.beta  = 0.0;
            }
            // EMA update for residual std-dev (O(1) approximation)
            double res = y - (lp.alpha + lp.beta * x_new);
            lp.residual_stddev = std::sqrt(
                0.9 * lp.residual_stddev * lp.residual_stddev + 0.1 * res * res);
        } else {
            // Fallback: running sums not initialized (model was deserialized
            // without running-sum state).  Full refit is correct but O(n).
            lp = ::themisdb::analytics::fitLinear(impl_->train_y);
            // Reinitialize running sums from the freshly fitted series.
            impl_->lin_n = impl_->train_y.size();
            impl_->lin_sx = 0.0; impl_->lin_sy = 0.0;
            impl_->lin_sxx = 0.0; impl_->lin_sxy = 0.0;
            for (size_t i = 0; i < impl_->lin_n; ++i) {
                double xi = static_cast<double>(i);
                impl_->lin_sx  += xi;
                impl_->lin_sy  += impl_->train_y[i];
                impl_->lin_sxx += xi * xi;
                impl_->lin_sxy += xi * impl_->train_y[i];
            }
        }
    }

    // ---- EXP_SMOOTHING (SES): O(1) level update ----
    {
        auto& sp = impl_->ses_p;
        sp.last_level = sp.alpha * y + (1.0 - sp.alpha) * sp.last_level;
    }

    // ---- HOLT_WINTERS: O(1) level/trend/seasonal update ----
    {
        auto& hp = impl_->hw_p;
        int m = hp.m;
        bool has_season = (m >= 2) && !hp.S.empty();
        int n_prev = static_cast<int>(impl_->train_y.size()) - 1; // index before this obs

        double L_prev = hp.L;
        double T_prev = hp.T;

        if (has_season) {
            int si = (n_prev) % m;
            if (si < 0) si += m;
            double S_t = hp.S[static_cast<size_t>(si)];
            double L_new, T_new, S_new;
            if (hp.multiplicative) {
                L_new = hp.alpha * (y / (S_t > 1e-12 ? S_t : 1e-12))
                      + (1.0 - hp.alpha) * (L_prev + T_prev);
                T_new = hp.beta * (L_new - L_prev) + (1.0 - hp.beta) * T_prev;
                S_new = hp.gamma * (y / (L_new > 1e-12 ? L_new : 1e-12))
                      + (1.0 - hp.gamma) * S_t;
            } else {
                L_new = hp.alpha * (y - S_t)
                      + (1.0 - hp.alpha) * (L_prev + T_prev);
                T_new = hp.beta * (L_new - L_prev) + (1.0 - hp.beta) * T_prev;
                S_new = hp.gamma * (y - L_new) + (1.0 - hp.gamma) * S_t;
            }
            hp.L = L_new;
            hp.T = T_new;
            hp.S[static_cast<size_t>(si)] = S_new;
        } else {
            // Holt (no seasonal)
            double L_new = hp.alpha * y + (1.0 - hp.alpha) * (L_prev + T_prev);
            double T_new = hp.beta * (L_new - L_prev) + (1.0 - hp.beta) * T_prev;
            hp.L = L_new;
            hp.T = T_new;
        }
    }

    // ---- ARIMA: shift window by one, append new value ----
    {
        auto& ap = impl_->arima_p;
        // Compute differenced value (d==1): need at least 2 points (the previous
        // training value is at train_y.size()-2 since we just pushed the new value).
        double y_diff = (ap.d == 1 && impl_->train_y.size() >= 2)
                      ? (y - impl_->train_y[impl_->train_y.size() - 2])
                      : y;
        // Update last window
        if (!ap.last_window.empty()) {
            ap.last_window.erase(ap.last_window.begin());
            ap.last_window.push_back(y_diff - ap.mean_diff);
        }
        // Update last_obs (used for integration in multi-step predict)
        ap.last_obs = y;
        // Recompute residual for last step and shift residual buffer
        if (!ap.last_resid.empty()) {
            ap.last_resid.erase(ap.last_resid.begin());
            ap.last_resid.push_back(0.0);  // future residual unknown → 0
        }
    }
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
    // Use full IEEE-754 double precision (17 sig-figs) to ensure exact round-trip.
    // std::fixed is intentionally NOT used here so integers (e.g. timestamps) and
    // small fractions both serialise without unnecessary padding.
    oss << std::setprecision(std::numeric_limits<double>::max_digits10);
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
    // ARIMA params (needed for ARIMA and ENSEMBLE predict after deserialization)
    oss << "ar_mean_diff=" << impl_->arima_p.mean_diff << "\n";
    oss << "ar_last_obs=" << impl_->arima_p.last_obs << "\n";
    oss << "ar_d=" << impl_->arima_p.d << "\n";
    oss << "ar_sigma=" << impl_->arima_p.residual_stddev << "\n";
    oss << "ar_coeffs_n=" << impl_->arima_p.ar_coeffs.size() << "\n";
    for (size_t i = 0; i < impl_->arima_p.ar_coeffs.size(); ++i)
        oss << "ar_c_" << i << "=" << impl_->arima_p.ar_coeffs[i] << "\n";
    oss << "ma_coeffs_n=" << impl_->arima_p.ma_coeffs.size() << "\n";
    for (size_t i = 0; i < impl_->arima_p.ma_coeffs.size(); ++i)
        oss << "ma_c_" << i << "=" << impl_->arima_p.ma_coeffs[i] << "\n";
    oss << "ar_win_n=" << impl_->arima_p.last_window.size() << "\n";
    for (size_t i = 0; i < impl_->arima_p.last_window.size(); ++i)
        oss << "ar_w_" << i << "=" << impl_->arima_p.last_window[i] << "\n";
    oss << "ar_res_n=" << impl_->arima_p.last_resid.size() << "\n";
    for (size_t i = 0; i < impl_->arima_p.last_resid.size(); ++i)
        oss << "ar_r_" << i << "=" << impl_->arima_p.last_resid[i] << "\n";
    // Training timestamps
    oss << "train_n=" << impl_->train_ts.size() << "\n";
    for (size_t i = 0; i < impl_->train_ts.size(); ++i)
        oss << "ts_" << i << "=" << impl_->train_ts[i] << "\n";
    return oss.str();
}

ForecastModel ForecastModel::deserialize(const std::string& data) {
    ForecastModel model;

    // Parse the entire string into a map once for O(1) per-key lookup.
    std::unordered_map<std::string, std::string> kv;
    {
        std::istringstream ss(data);
        std::string line;
        while (std::getline(ss, line)) {
            auto eq = line.find('=');
            if (eq != std::string::npos)
                kv.emplace(line.substr(0, eq), line.substr(eq + 1));
        }
    }
    // Sentinel returned when a key is missing; declared in outer scope (not static
    // inside the lambda) to avoid any question about concurrent initialisation.
    const std::string kEmpty;
    auto readS = [&](const std::string& key) -> const std::string& {
        auto it = kv.find(key);
        return (it != kv.end()) ? it->second : kEmpty;
    };
    auto readD = [&](const std::string& key) -> double {
        const auto& v = readS(key);
        return v.empty() ? 0.0 : std::stod(v);
    };
    auto readI = [&](const std::string& key) -> int {
        const auto& v = readS(key);
        return v.empty() ? 0 : std::stoi(v);
    };
    auto readL = [&](const std::string& key) -> int64_t {
        const auto& v = readS(key);
        return v.empty() ? int64_t{0} : static_cast<int64_t>(std::stoll(v));
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
    // Linear
    model.impl_->linear_p.alpha       = readD("lin_alpha");
    model.impl_->linear_p.beta        = readD("lin_beta");
    model.impl_->linear_p.residual_stddev = readD("lin_sigma");
    // SES
    model.impl_->ses_p.alpha          = readD("ses_alpha");
    model.impl_->ses_p.last_level     = readD("ses_level");
    model.impl_->ses_p.residual_stddev = readD("ses_sigma");
    // HW
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
    // ARIMA
    model.impl_->arima_p.mean_diff        = readD("ar_mean_diff");
    model.impl_->arima_p.last_obs         = readD("ar_last_obs");
    model.impl_->arima_p.d                = readI("ar_d");
    model.impl_->arima_p.residual_stddev  = readD("ar_sigma");
    int ar_n = readI("ar_coeffs_n");
    model.impl_->arima_p.ar_coeffs.resize(static_cast<size_t>(ar_n));
    for (int i = 0; i < ar_n; ++i)
        model.impl_->arima_p.ar_coeffs[static_cast<size_t>(i)] = readD("ar_c_" + std::to_string(i));
    int ma_n = readI("ma_coeffs_n");
    model.impl_->arima_p.ma_coeffs.resize(static_cast<size_t>(ma_n));
    for (int i = 0; i < ma_n; ++i)
        model.impl_->arima_p.ma_coeffs[static_cast<size_t>(i)] = readD("ma_c_" + std::to_string(i));
    int win_n = readI("ar_win_n");
    model.impl_->arima_p.last_window.resize(static_cast<size_t>(win_n));
    for (int i = 0; i < win_n; ++i)
        model.impl_->arima_p.last_window[static_cast<size_t>(i)] = readD("ar_w_" + std::to_string(i));
    int res_n = readI("ar_res_n");
    model.impl_->arima_p.last_resid.resize(static_cast<size_t>(res_n));
    for (int i = 0; i < res_n; ++i)
        model.impl_->arima_p.last_resid[static_cast<size_t>(i)] = readD("ar_r_" + std::to_string(i));
    // Training timestamps
    int train_n = readI("train_n");
    model.impl_->train_ts.resize(static_cast<size_t>(train_n));
    for (int i = 0; i < train_n; ++i)
        model.impl_->train_ts[static_cast<size_t>(i)] = readL("ts_" + std::to_string(i));

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

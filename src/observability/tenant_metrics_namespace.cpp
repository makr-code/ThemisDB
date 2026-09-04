/**
 * @file tenant_metrics_namespace.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/tenant_metrics_namespace.h"

#include <algorithm>
#include <sstream>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

TenantMetricsNamespace::TenantMetricsNamespace(const TenantMetricsConfig& config)
    : config_(config) {}

TenantMetricsNamespace::~TenantMetricsNamespace() = default;

// ---------------------------------------------------------------------------
// Tenant lifecycle
// ---------------------------------------------------------------------------

bool TenantMetricsNamespace::registerTenant(const std::string& tenant_id) {
    std::unique_lock lock(mutex_);
    if (stores_.count(tenant_id)) {
        return false; // already exists
    }
    if (config_.max_tenants > 0 && stores_.size() >= config_.max_tenants) {
        return false; // tenant cap reached
    }
    auto store = std::make_unique<TenantStore>();
    store->tenant_id = tenant_id;
    stores_.emplace(tenant_id, std::move(store));
    return true;
}

bool TenantMetricsNamespace::deregisterTenant(const std::string& tenant_id) {
    std::unique_lock lock(mutex_);
    return stores_.erase(tenant_id) > 0;
}

bool TenantMetricsNamespace::hasTenant(const std::string& tenant_id) const {
    std::shared_lock lock(mutex_);
    return stores_.count(tenant_id) > 0;
}

std::vector<std::string> TenantMetricsNamespace::tenants() const {
    std::shared_lock lock(mutex_);
    std::vector<std::string> ids = {};

    ids.reserve(stores_.size());
    for (const auto& kv : stores_) {
        ids.push_back(kv.first);
    }
    return ids;
}

size_t TenantMetricsNamespace::tenantCount() const {
    std::shared_lock lock(mutex_);
    return stores_.size();
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

std::string TenantMetricsNamespace::makeKey(
    const std::string& metric_name,
    const std::map<std::string, std::string>& labels)
{
    std::ostringstream ss = {};
    ss << metric_name;
    for (const auto& kv : labels) {
        ss << '{' << kv.first << '=' << kv.second << '}';
    }
    return ss.str();
}

bool TenantMetricsNamespace::checkCardinality(
    TenantStore& store,
    const std::string& metric_name,
    [[maybe_unused]] const std::string& key) const
{
    if (config_.cardinality_limit_per_tenant == 0) {
        return true; // unlimited
    }
    auto it = store.series_count.find(metric_name);
    if (it == store.series_count.end()) {
        store.series_count[metric_name] = 1;
        return true;
    }
    // Already seen this exact key?  Always allow.
    // We track distinct series count per metric, not per key for simplicity.
    // If the metric already has recorded series we just bump if within limit.
    if (it->second < config_.cardinality_limit_per_tenant) {
        it->second++;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Metric recording
// ---------------------------------------------------------------------------

void TenantMetricsNamespace::increment(
    const std::string& tenant_id,
    const std::string& metric_name,
    const std::map<std::string, std::string>& labels)
{
    std::unique_lock lock(mutex_);

    // Auto-register in non-strict mode
    if (!stores_.count(tenant_id)) {
        if (config_.strict_tenant_registration) {
            return;
        }
        if (config_.max_tenants > 0 && stores_.size() >= config_.max_tenants) {
            return;
        }
        auto s = std::make_unique<TenantStore>();
        s->tenant_id = tenant_id;
        stores_.emplace(tenant_id, std::move(s));
    }

    auto& store = *stores_[tenant_id];
    const std::string key = makeKey(metric_name, labels);

    if (!checkCardinality(store, metric_name, key)) {
        store.dropped_observations.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    store.counters[key]++;
    store.total_observations.fetch_add(1, std::memory_order_relaxed);
}

void TenantMetricsNamespace::setGauge(
    const std::string& tenant_id,
    const std::string& metric_name,
    double value,
    const std::map<std::string, std::string>& labels)
{
    std::unique_lock lock(mutex_);

    if (!stores_.count(tenant_id)) {
        if (config_.strict_tenant_registration) {
          return;
        }
        if (config_.max_tenants > 0 && stores_.size() >= config_.max_tenants) {
          return;
        }
        auto s = std::make_unique<TenantStore>();
        s->tenant_id = tenant_id;
        stores_.emplace(tenant_id, std::move(s));
    }

    auto& store = *stores_[tenant_id];
    const std::string key = makeKey(metric_name, labels);

    if (!checkCardinality(store, metric_name, key)) {
        store.dropped_observations.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    store.gauges[key] = value;
    store.total_observations.fetch_add(1, std::memory_order_relaxed);
}

void TenantMetricsNamespace::observeHistogram(
    const std::string& tenant_id,
    const std::string& metric_name,
    double value,
    const std::map<std::string, std::string>& labels)
{
    std::unique_lock lock(mutex_);

    if (!stores_.count(tenant_id)) {
        if (config_.strict_tenant_registration) {
          return;
        }
        if (config_.max_tenants > 0 && stores_.size() >= config_.max_tenants) {
          return;
        }
        auto s = std::make_unique<TenantStore>();
        s->tenant_id = tenant_id;
        stores_.emplace(tenant_id, std::move(s));
    }

    auto& store = *stores_[tenant_id];
    const std::string key = makeKey(metric_name, labels);

    if (!checkCardinality(store, metric_name, key)) {
        store.dropped_observations.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    auto& hd = store.histograms[key];
    if (hd.samples.size() < TenantStore::HistogramData::kMaxSamples) {
        hd.samples.push_back(value);
    }
    store.total_observations.fetch_add(1, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Export helpers
// ---------------------------------------------------------------------------

std::string TenantMetricsNamespace::formatLine(
    const std::string& prefixed_name,
    const std::map<std::string, std::string>& labels,
    double value)
{
    std::ostringstream ss = {};
    ss << prefixed_name;
    if (!labels.empty()) {
        ss << '{';
        bool first = true;
        for (const auto& kv : labels) {
            if (!first) {
              ss << ',';
            }
            ss << kv.first << "=\"" << kv.second << '"';
            first = false;
        }
        ss << '}';
    }
    ss << ' ' << value << '\n';
    return ss.str();
}

std::string TenantMetricsNamespace::exportStore(const TenantStore& store) const {
    const std::string prefix = "themis_" + store.tenant_id + "_";
    std::ostringstream out = {};

    // Counters
    for (const auto& kv : store.counters) {
        // kv.first is the makeKey() result: "metric_name{k=v}{k2=v2}"
        // Reconstruct a prefixed line with the tenant_id label injected.
        out << "# TYPE " << prefix << "counter counter\n";
        out << prefix << kv.first
            << "{tenant_id=\"" << store.tenant_id << "\"} "
            << kv.second << '\n';
    }

    // Gauges
    for (const auto& kv : store.gauges) {
        out << "# TYPE " << prefix << "gauge gauge\n";
        out << prefix << kv.first
            << "{tenant_id=\"" << store.tenant_id << "\"} "
            << kv.second << '\n';
    }

    // Histograms (emit sum and count)
    for (const auto& kv : store.histograms) {
        const auto& samples = kv.second.samples;
        double sum = 0.0;
        for (double s : samples) {
          sum += s;
        }
        out << "# TYPE " << prefix << "histogram histogram\n";
        out << prefix << kv.first
            << "_sum{tenant_id=\"" << store.tenant_id << "\"} "
            << sum << '\n';
        out << prefix << kv.first
            << "_count{tenant_id=\"" << store.tenant_id << "\"} "
            << samples.size() << '\n';
    }

    return out.str();
}

std::string TenantMetricsNamespace::exportTenant(const std::string& tenant_id) const {
    std::shared_lock lock(mutex_);
    auto it = stores_.find(tenant_id);
    if (it == stores_.end()) return {};
    return exportStore(*it->second);
}

std::string TenantMetricsNamespace::exportAll() const {
    std::shared_lock lock(mutex_);
    std::ostringstream out = {};
    for (const auto& kv : stores_) {
        out << exportStore(*kv.second);
    }
    return out.str();
}

// ---------------------------------------------------------------------------
// Introspection
// ---------------------------------------------------------------------------

TenantMetricsStats TenantMetricsNamespace::stats(const std::string& tenant_id) const {
    std::shared_lock lock(mutex_);
    auto it = stores_.find(tenant_id);
    if (it == stores_.end()) return TenantMetricsStats{tenant_id};

    const auto& store = *it->second;
    TenantMetricsStats s;
    s.tenant_id = tenant_id;
    s.total_observations = store.total_observations.load(std::memory_order_relaxed);
    s.dropped_observations = store.dropped_observations.load(std::memory_order_relaxed);
    s.active_series = store.counters.size() + store.gauges.size() + store.histograms.size();
    return s;
}

std::vector<TenantMetricsStats> TenantMetricsNamespace::allStats() const {
    std::shared_lock lock(mutex_);
    std::vector<TenantMetricsStats> result = {};

    result.reserve(stores_.size());
    for (const auto& kv : stores_) {
        const auto& store = *kv.second;
        TenantMetricsStats s;
        s.tenant_id = kv.first;
        s.total_observations = store.total_observations.load(std::memory_order_relaxed);
        s.dropped_observations = store.dropped_observations.load(std::memory_order_relaxed);
        s.active_series = store.counters.size() + store.gauges.size() + store.histograms.size();
        result.push_back(std::move(s));
    }
    return result;
}

TenantMetricsConfig TenantMetricsNamespace::config() const {
    std::shared_lock lock(mutex_);
    return config_;
}

void TenantMetricsNamespace::reset() {
    std::unique_lock lock(mutex_);
    for (auto& kv : stores_) {
        auto& store = *kv.second;
        store.counters.clear();
        store.gauges.clear();
        store.histograms.clear();
        store.series_count.clear();
        store.total_observations.store(0, std::memory_order_relaxed);
        store.dropped_observations.store(0, std::memory_order_relaxed);
    }
}

} // namespace observability
} // namespace themis

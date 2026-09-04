/**
 * @file materialized_cte.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * Incremental View Maintenance for Materialized CTEs — Implementation
 *
 * Converts between the query module's JSON-based CTEDataChange records and
 * the analytics module's variant-typed ChangeRecord, then delegates all
 * delta maintenance to analytics::IncrementalView.
 *
 * Type mapping (nlohmann::json → analytics::FieldValue):
 *   null    → nullptr_t
 *   boolean → bool
 *   integer → int64_t
 *   float   → double
 *   string  → std::string
 *   other   → std::string (JSON-serialized)
 */

#include "query/materialized_cte.h"

#include <spdlog/spdlog.h>

namespace themis {
namespace query {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/** Convert a single nlohmann::json value to analytics::FieldValue. */
themisdb::analytics::FieldValue jsonToFieldValue(const nlohmann::json& v) {
    if (v.is_null())    return themisdb::analytics::FieldValue{nullptr};
    if (v.is_boolean()) return themisdb::analytics::FieldValue{v.get<bool>()};
    if (v.is_number_integer()) {
        return themisdb::analytics::FieldValue{v.get<int64_t>()};
    }
    if (v.is_number_float()) {
        return themisdb::analytics::FieldValue{v.get<double>()};
    }
    if (v.is_string()) {
        return themisdb::analytics::FieldValue{v.get<std::string>()};
    }
    // Arrays / objects: serialize to string representation
    return themisdb::analytics::FieldValue{v.dump()};
}

/** Convert analytics::FieldValue back to nlohmann::json. */
nlohmann::json fieldValueToJson(const themisdb::analytics::FieldValue& fv) {
    if (std::holds_alternative<std::nullptr_t>(fv)) {
      return nullptr;
    }
    if (auto* b = std::get_if<bool>(&fv)) {
      return *b;
    }
    if (auto* i = std::get_if<int64_t>(&fv)) {
      return *i;
    }
    if (auto* d = std::get_if<double>(&fv)) {
      return *d;
    }
    if (auto* s = std::get_if<std::string>(&fv)) {
      return *s;
    }
    return nullptr;
}

} // anonymous namespace

// ============================================================================
// MaterializedCTEView — static helpers
// ============================================================================

themisdb::analytics::ViewAggFunc
MaterializedCTEView::toViewAggFunc(CTEAggFunc f) {
    switch (f) {
        case CTEAggFunc::COUNT:          return themisdb::analytics::ViewAggFunc::COUNT;
        case CTEAggFunc::SUM:            return themisdb::analytics::ViewAggFunc::SUM;
        case CTEAggFunc::AVG:            return themisdb::analytics::ViewAggFunc::AVG;
        case CTEAggFunc::MIN:            return themisdb::analytics::ViewAggFunc::MIN;
        case CTEAggFunc::MAX:            return themisdb::analytics::ViewAggFunc::MAX;
        case CTEAggFunc::COUNT_DISTINCT: return themisdb::analytics::ViewAggFunc::COUNT_DISTINCT;
    }
    return themisdb::analytics::ViewAggFunc::COUNT;
}

themisdb::analytics::ViewFilter::Op
MaterializedCTEView::toViewFilterOp(CTEBaseFilter::Op op) {
    switch (op) {
        case CTEBaseFilter::Op::EQ:          return themisdb::analytics::ViewFilter::Op::EQ;
        case CTEBaseFilter::Op::NE:          return themisdb::analytics::ViewFilter::Op::NE;
        case CTEBaseFilter::Op::LT:          return themisdb::analytics::ViewFilter::Op::LT;
        case CTEBaseFilter::Op::LE:          return themisdb::analytics::ViewFilter::Op::LE;
        case CTEBaseFilter::Op::GT:          return themisdb::analytics::ViewFilter::Op::GT;
        case CTEBaseFilter::Op::GE:          return themisdb::analytics::ViewFilter::Op::GE;
        case CTEBaseFilter::Op::IS_NULL:     return themisdb::analytics::ViewFilter::Op::IS_NULL;
        case CTEBaseFilter::Op::IS_NOT_NULL: return themisdb::analytics::ViewFilter::Op::IS_NOT_NULL;
    }
    return themisdb::analytics::ViewFilter::Op::EQ;
}

themisdb::analytics::ChangeRecord::Row
MaterializedCTEView::jsonToRow(const nlohmann::json& json_row) {
    themisdb::analytics::ChangeRecord::Row row;
    if (!json_row.is_object()) {
      return row;
    }
    for (auto it = json_row.begin(); it != json_row.end(); ++it) {
        row[it.key()] = jsonToFieldValue(it.value());
    }
    return row;
}

themisdb::analytics::ChangeRecord
MaterializedCTEView::toChangeRecord(const CTEDataChange& change) {
    themisdb::analytics::ChangeRecord rec;
    rec.collection  = change.collection;
    rec.change_time = std::chrono::system_clock::now();

    switch (change.type) {
        case CTEChangeType::INSERT:
            rec.type       = themisdb::analytics::ChangeType::INSERT;
            rec.after_row  = jsonToRow(change.after_row);
            break;
        case CTEChangeType::DELETE:
            rec.type        = themisdb::analytics::ChangeType::DELETE;
            rec.before_row  = jsonToRow(change.before_row);
            break;
        case CTEChangeType::UPDATE:
            rec.type        = themisdb::analytics::ChangeType::UPDATE;
            rec.before_row  = jsonToRow(change.before_row);
            rec.after_row   = jsonToRow(change.after_row);
            break;
    }
    return rec;
}

themisdb::analytics::ViewDefinition
MaterializedCTEView::buildViewDef(const MaterializedCTEDef& def) {
    themisdb::analytics::ViewDefinition vd;
    vd.name               = def.name;
    vd.source_collection  = def.source_collection;
    vd.dimensions         = def.dimensions;
    vd.staleness_seconds  = def.staleness_seconds;

    for (const auto& agg : def.aggregations) {
        themisdb::analytics::ViewAggSpec spec;
        spec.output_name  = agg.output_name;
        spec.func         = toViewAggFunc(agg.func);
        spec.source_field = agg.source_field;
        vd.aggregations.push_back(spec);
    }

    for (const auto& bf : def.base_filters) {
        themisdb::analytics::ViewFilter vf;
        vf.field = bf.field;
        vf.op    = toViewFilterOp(bf.op);
        vf.value = jsonToFieldValue(bf.value);
        vd.base_filters.push_back(vf);
    }

    return vd;
}

MaterializedCTEResult
MaterializedCTEView::fromViewQueryResult(
    const themisdb::analytics::ViewQueryResult& vqr,
    const MaterializedCTEDef& def)
{
    MaterializedCTEResult result;
    result.is_stale    = vqr.is_stale;
    result.total_rows  = vqr.total_rows;
    result.last_update = vqr.last_update;

    result.rows.reserve(vqr.rows.size());
    for (const auto& vrow : vqr.rows) {
        nlohmann::json obj = nlohmann::json::object();

        // Dimension key fields
        for (const auto& [dim, val] : vrow.group_key) {
            obj[dim] = val;
        }

        // Aggregate values
        for (const auto& agg : def.aggregations) {
            auto it = vrow.values.find(agg.output_name);
            if (it != vrow.values.end()) {
                obj[agg.output_name] = fieldValueToJson(it->second);
            }
        }

        result.rows.push_back({std::move(obj)});
    }
    return result;
}

// ============================================================================
// MaterializedCTEView — lifecycle
// ============================================================================

MaterializedCTEView::MaterializedCTEView(const MaterializedCTEDef& def)
    : def_(def)
    , view_(std::make_unique<themisdb::analytics::IncrementalView>(buildViewDef(def)))
{}

MaterializedCTEView::~MaterializedCTEView() = default;

// ============================================================================
// MaterializedCTEView — mutation
// ============================================================================

bool MaterializedCTEView::applyChange(const CTEDataChange& change) {
    return view_->applyChange(toChangeRecord(change));
}

int MaterializedCTEView::applyChanges(const std::vector<CTEDataChange>& changes) {
    std::vector<themisdb::analytics::ChangeRecord> recs;
    recs.reserve(changes.size());
    for (const auto& c : changes) {
        recs.push_back(toChangeRecord(c));
    }
    return view_->applyChanges(recs);
}

// ============================================================================
// MaterializedCTEView — query
// ============================================================================

MaterializedCTEResult MaterializedCTEView::query(
    int64_t limit, int64_t offset) const
{
    auto vqr = view_->query({}, limit, offset);
    return fromViewQueryResult(vqr, def_);
}

// ============================================================================
// MaterializedCTEView — accessors
// ============================================================================

bool MaterializedCTEView::isDirty()      const { return view_->isDirty(); }
bool MaterializedCTEView::isStale()      const { return view_->isStale(); }
int64_t MaterializedCTEView::groupCount() const { return view_->groupCount(); }
uint64_t MaterializedCTEView::changeCount() const { return view_->changeCount(); }

void MaterializedCTEView::clear() { view_->clear(); }

// ============================================================================
// MaterializedCTERegistry — lifecycle
// ============================================================================

MaterializedCTERegistry::MaterializedCTERegistry()  = default;
MaterializedCTERegistry::~MaterializedCTERegistry() = default;

// ============================================================================
// MaterializedCTERegistry — management
// ============================================================================

bool MaterializedCTERegistry::registerCTE(const MaterializedCTEDef& def) {
    if (def.name.empty()) {
        spdlog::warn("MaterializedCTERegistry: CTE name must not be empty");
        return false;
    }
    if (def.source_collection.empty()) {
        spdlog::warn("MaterializedCTERegistry: CTE '{}' source_collection must not be empty", def.name);
        return false;
    }
    std::unique_lock lk(registry_mutex_);
    if (views_.count(def.name)) {
        spdlog::warn("MaterializedCTERegistry: CTE '{}' already registered", def.name);
        return false;
    }
    views_[def.name] = std::make_shared<MaterializedCTEView>(def);
    spdlog::debug("MaterializedCTERegistry: registered CTE '{}'", def.name);
    return true;
}

bool MaterializedCTERegistry::unregisterCTE(const std::string& name) {
    std::unique_lock lk(registry_mutex_);
    if (!views_.count(name)) {
      return false;
    }
    views_.erase(name);
    spdlog::debug("MaterializedCTERegistry: unregistered CTE '{}'", name);
    return true;
}

bool MaterializedCTERegistry::hasCTE(const std::string& name) const {
    std::shared_lock lk(registry_mutex_);
    return views_.count(name) > 0;
}

std::vector<std::string> MaterializedCTERegistry::listCTEs() const {
    std::shared_lock lk(registry_mutex_);
    std::vector<std::string> names;
    names.reserve(views_.size());
    for (const auto& [n, _] : views_) {
      names.push_back(n);
    }
    return names;
}

std::shared_ptr<MaterializedCTEView>
MaterializedCTERegistry::getView(const std::string& name) const {
    std::shared_lock lk(registry_mutex_);
    auto it = views_.find(name);
    return (it != views_.end()) ? it->second : nullptr;
}

// ============================================================================
// MaterializedCTERegistry — change dispatch
// ============================================================================

void MaterializedCTERegistry::applyChange(const CTEDataChange& change) {
    std::shared_lock lk(registry_mutex_);
    uint64_t applied = 0;
    for (const auto& [name, view] : views_) {
        if (view->applyChange(change)) {
          ++applied;
        }
    }
    total_changes_ += applied;
}

void MaterializedCTERegistry::applyChanges(
    const std::vector<CTEDataChange>& changes)
{
    std::shared_lock lk(registry_mutex_);
    uint64_t applied = 0;
    for (const auto& [name, view] : views_) {
        applied += static_cast<uint64_t>(view->applyChanges(changes));
    }
    total_changes_ += applied;
}

// ============================================================================
// MaterializedCTERegistry — query
// ============================================================================

MaterializedCTEResult MaterializedCTERegistry::query(
    const std::string& cte_name,
    int64_t limit,
    int64_t offset) const
{
    std::shared_lock lk(registry_mutex_);
    auto it = views_.find(cte_name);
    if (it == views_.end()) {
        spdlog::warn("MaterializedCTERegistry: CTE '{}' not found", cte_name);
        return {};
    }
    return it->second->query(limit, offset);
}

} // namespace query
} // namespace themis

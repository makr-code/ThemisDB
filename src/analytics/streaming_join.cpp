/**
 * @file streaming_join.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=24, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "analytics/streaming_join.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace themisdb {
namespace analytics {

// ============================================================================
// Helpers (file-local)
// ============================================================================

namespace {

/// Serialize a ColumnValue to a string for use as a composite hash-map key.
/// Format: type_tag ':' value '\0' (null separator prevents ambiguity).
std::string encodeValue(const ColumnValue &v) {
    return std::visit(
        [](auto &&arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return std::string("\x00\x00", 2); // null sentinel
            } else if constexpr (std::is_same_v<T, bool>) {
                char buf[2] = {'B', arg ? '1' : '0'};
                return std::string(buf, 2);
            } else if constexpr (std::is_same_v<T, int64_t>) {
                char buf[9];
                buf[0] = 'I';
                std::memcpy(buf + 1, &arg, 8); // little-endian OK (key comparison only)
                return std::string(buf, 9);
            } else if constexpr (std::is_same_v<T, double>) {
                char buf[9];
                buf[0] = 'D';
                std::memcpy(buf + 1, &arg, 8);
                return std::string(buf, 9);
            } else {
                // std::string
                std::string s;
                s.reserve(2 + arg.size());
                s += 'S';
                s += arg;
                s += '\x01'; // terminator
                return s;
            }
        },
        v);
}

/// Get the column index for a column name in a ColumnBatch.
/// Returns SIZE_MAX if not found.
size_t findColumnIndex(const ColumnBatch &batch, const std::string &name) {
    for (size_t i = 0; i < batch.columnCount(); ++i) {
        if (batch.getColumnAt(i)->name() == name) {
            return i;
        }
    }
    return SIZE_MAX;
}

/// Make a composite key from given columns at a specific row.
std::string makeCompositeKey(const std::vector<std::shared_ptr<Column>> &cols, const std::vector<size_t> &key_indices,
                             size_t row) {
    std::string key;
    key.reserve(key_indices.size() * 10);
    for (size_t ki : key_indices) {
        key += encodeValue(cols[ki]->get(row));
        key += '\xFF'; // separator
    }
    return key;
}

/// Append row `src_row` from `src_col` to `dst_col`.
void appendRow(Column &dst, const Column &src, size_t src_row) {
    if (src.isNull(src_row)) {
        dst.appendNull();
        return;
    }
    switch (src.type()) {
        case ColumnType::Int64:
            dst.appendInt64(src.int64Data()[src_row]);
            break;
        case ColumnType::Double:
            dst.appendDouble(src.doubleData()[src_row]);
            break;
        case ColumnType::String:
            dst.appendString(src.stringData()[src_row]);
            break;
        case ColumnType::Bool:
            dst.appendBool(src.boolData()[src_row]);
            break;
        default:
            dst.appendNull();
            break;
    }
}

} // anonymous namespace

// ============================================================================
// HashJoin
// ============================================================================

HashJoin::HashJoin(Config config) : cfg_(std::move(config)) {
    if (cfg_.join_keys.empty()) {
        throw std::invalid_argument("HashJoin: at least one join key is required");
    }
}

bool HashJoin::addBuildBatch(const ColumnBatch &batch) {
    if (batch.rowCount() == 0) {
        return true;
    }

    // On first batch: initialize build_columns_ schema.
    if (build_columns_.empty()) {
        if (cfg_.build_select.empty()) {
            for (size_t i = 0; i < batch.columnCount(); ++i) {
                auto col = batch.getColumnAt(i);
                build_columns_.push_back(std::make_shared<Column>(col->name(), col->type()));
                build_column_names_.push_back(col->name());
            }
        } else {
            for (const auto &n : cfg_.build_select) {
                auto col = batch.getColumn(n);
                if (!col) {
                    throw std::invalid_argument("HashJoin: build column not found: " + n);
                }
                build_columns_.push_back(std::make_shared<Column>(col->name(), col->type()));
                build_column_names_.push_back(n);
            }
        }
    }

    // Resolve key column indices in this batch.
    const size_t n_rows = batch.rowCount();

    // Get the columns to store (respecting build_select).
    std::vector<std::shared_ptr<Column>> src_cols;
    src_cols.reserve(build_column_names_.size());
    for (const auto &n : build_column_names_) {
        auto c = batch.getColumn(n);
        if (!c) {
            throw std::invalid_argument("HashJoin: build column not found: " + n);
        }
        src_cols.push_back(c);
    }

    // Resolve key indices in src_cols.
    std::vector<size_t> key_indices = {};

    key_indices.reserve(cfg_.join_keys.size());
    for (const auto &kn : cfg_.join_keys) {
        bool found = false;
        for (size_t i = 0; i < build_column_names_.size(); ++i) {
            if (build_column_names_[i] == kn) {
                key_indices.push_back(i);
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::invalid_argument("HashJoin: join key column not found in build: " + kn);
        }
    }

    for (size_t r = 0; r < n_rows; ++r) {
        if (cfg_.max_build_rows > 0 && build_row_count_ >= cfg_.max_build_rows) {
            return false;
        }
        auto key = makeCompositeKey(src_cols, key_indices, r);
        hash_table_[key].push_back(build_row_count_);

        for (size_t ci = 0; ci < build_columns_.size(); ++ci) {
            appendRow(*build_columns_[ci], *src_cols[ci], r);
        }
        ++build_row_count_;
    }
    return true;
}

ColumnBatch HashJoin::probe(const ColumnBatch &probe_batch) {
    // Determine probe columns to project.
    std::vector<std::shared_ptr<Column>> probe_cols;
    std::vector<std::string> probe_col_names = {};

    if (cfg_.probe_select.empty()) {
        probe_cols.reserve(probe_batch.columnCount());
        for (size_t i = 0; i < probe_batch.columnCount(); ++i) {
            probe_cols.push_back(probe_batch.getColumnAt(i));
            probe_col_names.push_back(probe_cols.back()->name());
        }
    } else {
        probe_cols.reserve(cfg_.probe_select.size());
        for (const auto &n : cfg_.probe_select) {
            auto c = probe_batch.getColumn(n);
            if (!c) {
                throw std::invalid_argument("HashJoin: probe column not found: " + n);
            }
            probe_cols.push_back(c);
            probe_col_names.push_back(n);
        }
    }

    // Resolve probe key indices.
    std::vector<size_t> probe_key_indices = {};

    probe_key_indices.reserve(cfg_.join_keys.size());
    for (const auto &kn : cfg_.join_keys) {
        bool found = false;
        for (size_t i = 0; i < probe_col_names.size(); ++i) {
            if (probe_col_names[i] == kn) {
                probe_key_indices.push_back(i);
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::invalid_argument("HashJoin: join key column not found in probe: " + kn);
        }
    }

    // Build output schema: probe columns + build columns (excluding dup keys).
    std::vector<std::string> build_non_key_names = {};

    for (size_t ci = 0; ci < build_column_names_.size(); ++ci) {
        const auto &n = build_column_names_[ci];
        bool is_key   = false;
        for (const auto &kn : cfg_.join_keys) {
            if (kn == n) {
                is_key = true;
                break;
            }
        }
        if (!is_key) {
            build_non_key_names.push_back(n);
        }
    }

    // Create output columns.
    std::vector<std::shared_ptr<Column>> out_cols;
    out_cols.reserve(probe_cols.size() + build_non_key_names.size());
    for (const auto &c : probe_cols) {
        out_cols.push_back(std::make_shared<Column>(c->name(), c->type()));
    }
    for (const auto &n : build_non_key_names) {
        auto it   = std::find(build_column_names_.begin(), build_column_names_.end(), n);
        size_t ci = static_cast<size_t>(it - build_column_names_.begin());
        out_cols.push_back(std::make_shared<Column>(n, build_columns_[ci]->type()));
    }

    const size_t n_probe_cols = probe_cols.size();
    size_t out_row_count      = 0;

    for (size_t r = 0; r < probe_batch.rowCount(); ++r) {
        auto key = makeCompositeKey(probe_cols, probe_key_indices, r);
        auto it  = hash_table_.find(key);

        if (it == hash_table_.end()) {
            // No match.
            if (cfg_.join_type == JoinType::LeftOuter) {
                // Emit probe row with nulls for build columns.
                for (size_t ci = 0; ci < n_probe_cols; ++ci) {
                    appendRow(*out_cols[ci], *probe_cols[ci], r);
                }
                for (size_t ci = n_probe_cols; ci < out_cols.size(); ++ci) {
                    out_cols[ci]->appendNull();
                }
                ++out_row_count;
            }
        } else {
            for (size_t build_row : it->second) {
                // Emit probe columns.
                for (size_t ci = 0; ci < n_probe_cols; ++ci) {
                    appendRow(*out_cols[ci], *probe_cols[ci], r);
                }
                // Emit non-key build columns.
                size_t out_ci = n_probe_cols;
                for (const auto &n : build_non_key_names) {
                    auto bit   = std::find(build_column_names_.begin(), build_column_names_.end(), n);
                    size_t bci = static_cast<size_t>(bit - build_column_names_.begin());
                    appendRow(*out_cols[out_ci++], *build_columns_[bci], build_row);
                }
                ++out_row_count;
            }
        }
    }

    ColumnBatch result(out_row_count);
    for (auto &c : out_cols) {
        result.addColumn(c);
    }
    return result;
}

void HashJoin::reset() {
    hash_table_.clear();
    build_columns_.clear();
    build_column_names_.clear();
    build_row_count_ = 0;
}

HashJoin::CompositeKey HashJoin::makeKey(const std::vector<std::shared_ptr<Column>> &cols,
                                         const std::vector<size_t> &key_col_indices, size_t row) const {
    return makeCompositeKey(cols, key_col_indices, row);
}

ColumnValue HashJoin::getVal(const Column &col, size_t row) const {
    return col.get(row);
}

// ============================================================================
// IntervalJoin
// ============================================================================

IntervalJoin::IntervalJoin(Config config) : cfg_(std::move(config)) {
    if (cfg_.time_column.empty()) {
        throw std::invalid_argument("IntervalJoin: time_column must be specified");
    }
}

void IntervalJoin::addBuildBatch(const ColumnBatch &batch) {
    if (batch.rowCount() == 0) {
        return;
    }

    // Initialize column names on first batch.
    if (build_col_names_.empty()) {
        if (cfg_.build_select.empty()) {
            for (size_t i = 0; i < batch.columnCount(); ++i) {
                build_col_names_.push_back(batch.getColumnAt(i)->name());
            }
        } else {
            build_col_names_ = cfg_.build_select;
        }
    }

    // Find time column index in this batch.
    size_t time_idx = findColumnIndex(batch, cfg_.time_column);
    if (time_idx == SIZE_MAX) {
        throw std::invalid_argument("IntervalJoin: time column not found: " + cfg_.time_column);
    }
    const auto &time_col = *batch.getColumnAt(time_idx);

    for (size_t r = 0; r < batch.rowCount(); ++r) {
        int64_t ts = time_col.int64Data()[r];
        std::vector<ColumnValue> vals = {};

        vals.reserve(build_col_names_.size());
        for (const auto &n : build_col_names_) {
            auto c = batch.getColumn(n);
            if (!c) {
                throw std::invalid_argument("IntervalJoin: build column not found: " + n);
            }
            vals.push_back(c->get(r));
        }
        build_buffer_.push_back({ts, std::move(vals)});
    }
    build_sorted_ = false;
}

size_t IntervalJoin::buildSideSize() const noexcept {
    return build_buffer_.size();
}

void IntervalJoin::sortBuildBuffer() {
    if (!build_sorted_) {
        std::sort(build_buffer_.begin(), build_buffer_.end(),
                  [](const BuildRow &a, const BuildRow &b) { return a.timestamp_ms < b.timestamp_ms; });
        build_sorted_ = true;
    }
}

void IntervalJoin::pruneBuildBuffer(int64_t min_keep_ms) {
    auto it = std::lower_bound(build_buffer_.begin(), build_buffer_.end(), min_keep_ms,
                               [](const BuildRow &row, int64_t val) { return row.timestamp_ms < val; });
    if (it != build_buffer_.begin()) {
        build_buffer_.erase(build_buffer_.begin(), it);
    }
}

ColumnBatch IntervalJoin::probe(const ColumnBatch &probe_batch) {
    if (probe_batch.rowCount() == 0) {
        return ColumnBatch{0};
    }

    sortBuildBuffer();

    // Find probe time column.
    size_t probe_time_idx = findColumnIndex(probe_batch, cfg_.time_column);
    if (probe_time_idx == SIZE_MAX) {
        throw std::invalid_argument("IntervalJoin: probe time column not found: " + cfg_.time_column);
    }

    // Determine probe columns.
    std::vector<std::shared_ptr<Column>> probe_cols;
    std::vector<std::string> probe_col_names = {};

    if (cfg_.probe_select.empty()) {
        for (size_t i = 0; i < probe_batch.columnCount(); ++i) {
            probe_cols.push_back(probe_batch.getColumnAt(i));
            probe_col_names.push_back(probe_cols.back()->name());
        }
    } else {
        for (const auto &n : cfg_.probe_select) {
            auto c = probe_batch.getColumn(n);
            if (!c) {
                throw std::invalid_argument("IntervalJoin: probe column not found: " + n);
            }
            probe_cols.push_back(c);
            probe_col_names.push_back(n);
        }
    }

    // Resolve join key indices on both sides.
    std::vector<size_t> probe_key_indices, build_key_indices;
    for (const auto &kn : cfg_.join_keys) {
        // Probe side.
        bool found = false;
        for (size_t i = 0; i < probe_col_names.size(); ++i) {
            if (probe_col_names[i] == kn) {
                probe_key_indices.push_back(i);
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::invalid_argument("IntervalJoin: join key not in probe: " + kn);
        }

        // Build side.
        found = false;
        for (size_t i = 0; i < build_col_names_.size(); ++i) {
            if (build_col_names_[i] == kn) {
                build_key_indices.push_back(i);
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::invalid_argument("IntervalJoin: join key not in build: " + kn);
        }
    }

    // Determine which build columns to output (excluding key duplicates).
    std::vector<size_t> build_output_indices;
    std::vector<std::string> build_output_names = {};

    for (size_t i = 0; i < build_col_names_.size(); ++i) {
        const auto &n = build_col_names_[i];
        bool is_key   = false;
        for (const auto &kn : cfg_.join_keys) {
            if (kn == n) {
                is_key = true;
                break;
            }
        }
        if (!is_key) {
            build_output_indices.push_back(i);
            build_output_names.push_back(n);
        }
    }

    // Create output columns (probe first, then non-key build columns).
    // We need the types for build columns — infer from first build row if available.
    std::vector<std::shared_ptr<Column>> out_cols;
    out_cols.reserve(probe_cols.size() + build_output_names.size());
    for (const auto &c : probe_cols) {
        out_cols.push_back(std::make_shared<Column>(c->name(), c->type()));
    }
    // For build output columns we don't have type info at this point (stored as ColumnValue).
    // Use Int64 as default; actual appending uses appendNull / correct typed append via ColumnValue.
    // We'll accumulate output rows as ColumnValue vectors and materialise at end.

    const size_t n_probe_cols   = probe_cols.size();
    const size_t n_build_out    = build_output_names.size();
    const auto &probe_time_data = probe_batch.getColumnAt(probe_time_idx)->int64Data();

    // Collect output rows as variant vectors.
    struct OutRow {
        std::vector<ColumnValue> probe_vals;
        std::vector<ColumnValue> build_vals; // empty → null build side
    };
    std::vector<OutRow> out_rows;
    out_rows.reserve(probe_batch.rowCount());

    int64_t min_probe_ts = INT64_MAX;

    for (size_t r = 0; r < probe_batch.rowCount(); ++r) {
        int64_t probe_ts = probe_time_data[r];
        min_probe_ts     = std::min(min_probe_ts, probe_ts);

        int64_t lo = probe_ts - cfg_.before_ms;
        int64_t hi = probe_ts + cfg_.after_ms;

        // Binary-search lower bound.
        auto it_lo = std::lower_bound(build_buffer_.begin(), build_buffer_.end(), lo,
                                      [](const BuildRow &row, int64_t val) { return row.timestamp_ms < val; });

        // Collect probe column values once.
        std::vector<ColumnValue> probe_vals;
        probe_vals.reserve(n_probe_cols);
        for (size_t ci = 0; ci < n_probe_cols; ++ci) {
            probe_vals.push_back(probe_cols[ci]->get(r));
        }

        // Build composite probe key.
        std::string probe_key;
        for (size_t ki : probe_key_indices) {
            probe_key += encodeValue(probe_vals[ki]);
            probe_key += '\xFF';
        }

        bool any_match = false;
        for (auto it = it_lo; it != build_buffer_.end() && it->timestamp_ms <= hi; ++it) {
            // Check join keys.
            if (!cfg_.join_keys.empty()) {
                std::string build_key;
                for (size_t ki : build_key_indices) {
                    build_key += encodeValue(it->values[ki]);
                    build_key += '\xFF';
                }
                if (build_key != probe_key) {
                    continue;
                }
            }

            std::vector<ColumnValue> build_out_vals;
            build_out_vals.reserve(n_build_out);
            for (size_t bi : build_output_indices) {
                build_out_vals.push_back(it->values[bi]);
            }
            out_rows.push_back({probe_vals, std::move(build_out_vals)});
            any_match = true;
        }

        if (!any_match && cfg_.join_type == JoinType::LeftOuter) {
            std::vector<ColumnValue> null_build(n_build_out, nullptr);
            out_rows.push_back({probe_vals, std::move(null_build)});
        }
    }

    // Prune build buffer beyond the retention window.
    if (min_probe_ts != INT64_MAX) {
        pruneBuildBuffer(min_probe_ts - cfg_.before_ms - cfg_.slack_ms);
    }

    // Materialise output ColumnBatch.
    const size_t n_out = out_rows.size();
    ColumnBatch result(n_out);

    if (n_out == 0) {
        return result;
    }

    // Determine column types from first non-null output row.
    std::vector<ColumnType> probe_types(n_probe_cols, ColumnType::Null);
    std::vector<ColumnType> build_types(n_build_out, ColumnType::Null);

    for (const auto &row : out_rows) {
        for (size_t i = 0; i < n_probe_cols && probe_types[i] == ColumnType::Null; ++i) {
            std::visit(
                [&]([[maybe_unused]] auto &&v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, int64_t>) {
                        probe_types[i] = ColumnType::Int64;
                    } else if constexpr (std::is_same_v<T, double>) {
                        probe_types[i] = ColumnType::Double;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        probe_types[i] = ColumnType::String;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        probe_types[i] = ColumnType::Bool;
                    }
                },
                row.probe_vals[i]);
        }
        for (size_t i = 0; i < n_build_out && build_types[i] == ColumnType::Null; ++i) {
            std::visit(
                [&]([[maybe_unused]] auto &&v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, int64_t>) {
                        build_types[i] = ColumnType::Int64;
                    } else if constexpr (std::is_same_v<T, double>) {
                        build_types[i] = ColumnType::Double;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        build_types[i] = ColumnType::String;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        build_types[i] = ColumnType::Bool;
                    }
                },
                row.build_vals[i]);
        }
    }

    // Create and populate output columns.
    std::vector<std::shared_ptr<Column>> out_cols_mat;
    out_cols_mat.reserve(n_probe_cols + n_build_out);
    for (size_t i = 0; i < n_probe_cols; ++i) {
        auto t = probe_types[i] == ColumnType::Null ? ColumnType::String : probe_types[i];
        out_cols_mat.push_back(std::make_shared<Column>(probe_col_names[i], t));
    }
    for (size_t i = 0; i < n_build_out; ++i) {
        auto t = build_types[i] == ColumnType::Null ? ColumnType::String : build_types[i];
        out_cols_mat.push_back(std::make_shared<Column>(build_output_names[i], t));
    }

    auto appendVal = [](Column &col, const ColumnValue &v) {
        std::visit(
            [&]([[maybe_unused]] auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::nullptr_t>) {
                    col.appendNull();
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    col.appendInt64(arg);
                } else if constexpr (std::is_same_v<T, double>) {
                    col.appendDouble(arg);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    col.appendString(arg);
                } else if constexpr (std::is_same_v<T, bool>) {
                    col.appendBool(arg);
                }
            },
            v);
    };

    for (const auto &row : out_rows) {
        for (size_t i = 0; i < n_probe_cols; ++i) {
            appendVal(*out_cols_mat[i], row.probe_vals[i]);
        }
        for (size_t i = 0; i < n_build_out; ++i) {
            appendVal(*out_cols_mat[n_probe_cols + i], row.build_vals[i]);
        }
    }

    ColumnBatch final_result(n_out);
    for (auto &c : out_cols_mat) {
        final_result.addColumn(c);
    }
    return final_result;
}

void IntervalJoin::reset() {
    build_buffer_.clear();
    build_col_names_.clear();
    build_sorted_ = false;
}

ColumnValue IntervalJoin::getVal(const Column &col, size_t row) const {
    return col.get(row);
}

} // namespace analytics
} // namespace themisdb

/**
 * @file temporal_support.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/temporal_support.h"
#include <algorithm>
#include <set>
#include <sstream>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static const std::set<std::string> VALID_TIME_FROM_HINTS{
    "valid_from", "valid_start", "effective_date", "effective_from",
    "start_date", "begin_date", "period_start"
};
static const std::set<std::string> VALID_TIME_TO_HINTS{
    "valid_to", "valid_end", "expiry_date", "expiration_date",
    "end_date", "finish_date", "period_end"
};
static const std::set<std::string> TX_TIME_FROM_HINTS{
    "created_at", "inserted_at", "sys_period_start", "transaction_start",
    "row_created"
};
static const std::set<std::string> TX_TIME_TO_HINTS{
    "updated_at", "deleted_at", "sys_period_end", "transaction_end",
    "row_updated"
};

bool TemporalDatabaseSupport::isValidTimeColumn(const std::string& col) {
    return VALID_TIME_FROM_HINTS.count(col) || VALID_TIME_TO_HINTS.count(col);
}

bool TemporalDatabaseSupport::isTransactionTimeColumn(const std::string& col) {
    return TX_TIME_FROM_HINTS.count(col) || TX_TIME_TO_HINTS.count(col);
}

std::string TemporalDatabaseSupport::temporalModelToString(TemporalModel m) {
    switch (m) {
        case TemporalModel::VALID_TIME:        return "VALID_TIME";
        case TemporalModel::TRANSACTION_TIME:  return "TRANSACTION_TIME";
        case TemporalModel::BI_TEMPORAL:       return "BI_TEMPORAL";
        default:                               return "NONE";
    }
}

// ---------------------------------------------------------------------------
// detectTemporalDimensions
// ---------------------------------------------------------------------------

std::vector<TemporalDatabaseSupport::TemporalSchema>
TemporalDatabaseSupport::detectTemporalDimensions(
    const std::vector<InferenceTableSchema>& schemas)
{
    std::vector<TemporalSchema> result;

    for (const auto& schema : schemas) {
        TemporalSchema ts;
        ts.table_name = schema.name;

        bool has_valid    = false;
        bool has_tx       = false;

        for (const auto& col : schema.columns) {
            if (VALID_TIME_FROM_HINTS.count(col)) {
                ts.valid_from_column = col;
                has_valid = true;
            } else if (VALID_TIME_TO_HINTS.count(col)) {
                ts.valid_to_column = col;
                has_valid = true;
            } else if (TX_TIME_FROM_HINTS.count(col)) {
                ts.transaction_from_column = col;
                has_tx = true;
            } else if (TX_TIME_TO_HINTS.count(col)) {
                ts.transaction_to_column = col;
                has_tx = true;
            }
        }

        if (has_valid && has_tx) {
            ts.temporal_model = TemporalModel::BI_TEMPORAL;
        } else if (has_valid) {
            ts.temporal_model = TemporalModel::VALID_TIME;
        } else if (has_tx) {
            ts.temporal_model = TemporalModel::TRANSACTION_TIME;
        } else {
            continue; // no temporal dimension detected
        }

        result.push_back(std::move(ts));
    }

    return result;
}

// ---------------------------------------------------------------------------
// TemporalQueryBuilder
// ---------------------------------------------------------------------------

std::string
TemporalDatabaseSupport::TemporalQueryBuilder::buildPointInTimeQuery(
    const TemporalSchema& temporal,
    const std::string& timestamp)
{
    std::ostringstream sql = {};
    sql << "SELECT * FROM " << temporal.table_name;
    sql << "\nWHERE";

    bool first = true;

    if (!temporal.valid_from_column.empty()) {
        sql << "\n  " << temporal.valid_from_column << " <= '" << timestamp << "'";
        first = false;
    }
    if (!temporal.valid_to_column.empty()) {
        if (!first) {
          sql << "\n  AND ";
        }
        sql << "(" << temporal.valid_to_column << " IS NULL"
            << " OR " << temporal.valid_to_column << " > '" << timestamp << "')";
        first = false;
    }
    if (!temporal.transaction_from_column.empty()) {
        if (!first) {
          sql << "\n  AND ";
        }
        sql << temporal.transaction_from_column << " <= '" << timestamp << "'";
        first = false;
    }
    if (!temporal.transaction_to_column.empty()) {
        if (!first) {
          sql << "\n  AND ";
        }
        sql << "(" << temporal.transaction_to_column << " IS NULL"
            << " OR " << temporal.transaction_to_column << " > '" << timestamp << "')";
    }

    if (first) {
        // No temporal columns detected; return unfiltered query
        return "SELECT * FROM " + temporal.table_name;
    }

    return sql.str();
}

std::string
TemporalDatabaseSupport::TemporalQueryBuilder::buildSystemTimeQuery(
    const TemporalSchema& temporal,
    const std::string& timestamp)
{
    // SQL:2011 FOR SYSTEM_TIME AS OF
    std::ostringstream sql = {};
    sql << "SELECT * FROM " << temporal.table_name
        << "\nFOR SYSTEM_TIME AS OF TIMESTAMP '" << timestamp << "'";
    return sql.str();
}

} // namespace importers
} // namespace themis

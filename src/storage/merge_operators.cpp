/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            merge_operators.cpp                                ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:52:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     167                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • dbc9bfed9f  2026-04-13  Add CI/CD workflows and scripts for release management ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • dd319b9918  2026-04-13  Add CI/CD workflows and scripts for release management ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/merge_operators.h"
#include <spdlog/spdlog.h>
#include <charconv>
#include <set>
#include <sstream>
#include <algorithm>

namespace themis {

// CounterMergeOperator Implementation
bool CounterMergeOperator::Merge([[maybe_unused]] const rocksdb::Slice& key,
                                  const rocksdb::Slice* existing_value,
                                  const rocksdb::Slice& value,
                                  std::string* new_value,
                                  [[maybe_unused]] rocksdb::Logger* logger) const {
    // Parse new value as integer
    int64_t delta = 0;
    auto result = std::from_chars(value.data(), value.data() + value.size(), delta);
    if (result.ec != std::errc()) {
        spdlog::warn("CounterMergeOperator: Failed to parse value as integer");
        return false;
    }

    // If existing value exists, add it
    int64_t current = 0;
    if (existing_value) {
        auto existing_result = std::from_chars(
            existing_value->data(), 
            existing_value->data() + existing_value->size(), 
            current
        );
        if (existing_result.ec != std::errc()) {
            spdlog::warn("CounterMergeOperator: Failed to parse existing value as integer");
            return false;
        }
    }

    // Calculate new sum
    int64_t sum = current + delta;
    *new_value = std::to_string(sum);
    return true;
}

// AppendMergeOperator Implementation
AppendMergeOperator::AppendMergeOperator(std::string delimiter) 
    : delimiter_(std::move(delimiter)) {}

bool AppendMergeOperator::Merge([[maybe_unused]] const rocksdb::Slice& key,
                                 const rocksdb::Slice* existing_value,
                                 const rocksdb::Slice& value,
                                 std::string* new_value,
                                 [[maybe_unused]] rocksdb::Logger* logger) const {
    if (existing_value) {
        new_value->reserve(existing_value->size() + delimiter_.size() + value.size());
        new_value->assign(existing_value->data(), existing_value->size());
        new_value->append(delimiter_);
        new_value->append(value.data(), value.size());
    } else {
        new_value->assign(value.data(), value.size());
    }
    return true;
}

// SetMergeOperator Implementation
bool SetMergeOperator::Merge([[maybe_unused]] const rocksdb::Slice& key,
                              const rocksdb::Slice* existing_value,
                              const rocksdb::Slice& value,
                              std::string* new_value,
                              [[maybe_unused]] rocksdb::Logger* logger) const {
    // Parse existing set
    std::set<std::string> unique_values;
    
    if (existing_value) {
        std::string existing_str(existing_value->data(), existing_value->size());
        std::stringstream ss(existing_str);
        std::string item;
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) {
                unique_values.insert(item);
            }
        }
    }

    // Parse and add new values (comma-separated)
    std::string value_str(value.data(), value.size());
    std::stringstream ss(value_str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) {
            unique_values.insert(item);
        }
    }

    // Serialize back to comma-separated string
    new_value->clear();
    bool first = true;
    for (const auto& val : unique_values) {
        if (!first) {
            new_value->append(",");
        }
        new_value->append(val);
        first = false;
    }
    
    return true;
}

// MaxMergeOperator Implementation
bool MaxMergeOperator::Merge([[maybe_unused]] const rocksdb::Slice& key,
                              const rocksdb::Slice* existing_value,
                              const rocksdb::Slice& value,
                              std::string* new_value,
                              [[maybe_unused]] rocksdb::Logger* logger) const {
    // Parse new value as double
    double new_val = 0.0;
    auto result = std::from_chars(value.data(), value.data() + value.size(), new_val);
    if (result.ec != std::errc()) {
        spdlog::warn("MaxMergeOperator: Failed to parse value as double");
        return false;
    }

    // If existing value exists, compare
    double max_val = new_val;
    if (existing_value) {
        double existing_val = 0.0;
        auto existing_result = std::from_chars(
            existing_value->data(), 
            existing_value->data() + existing_value->size(), 
            existing_val
        );
        if (existing_result.ec != std::errc()) {
            spdlog::warn("MaxMergeOperator: Failed to parse existing value as double");
            return false;
        }
        max_val = std::max(existing_val, new_val);
    }

    // Convert back to string
    *new_value = std::to_string(max_val);
    return true;
}

} // namespace themis

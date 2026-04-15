/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            merge_operators.h                                  ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     93                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <rocksdb/merge_operator.h>
#include <string>
#include <string_view>
#include <memory>

namespace themis {

/// CounterMergeOperator - Atomic numeric increments
/// Use Case: Query statistics, counters
/// Example: "stats:query:count:+5" + "stats:query:count:+3" → "+8"
class CounterMergeOperator : public rocksdb::AssociativeMergeOperator {
public:
    bool Merge(const rocksdb::Slice& key,
               const rocksdb::Slice* existing_value,
               const rocksdb::Slice& value,
               std::string* new_value,
               rocksdb::Logger* logger) const override;

    const char* Name() const override { return "CounterMergeOperator"; }
};

/// AppendMergeOperator - Concatenate values with delimiter
/// Use Case: Append-only logs, event streams
/// Example: "log:events:event1" + "log:events:event2" → "event1|event2"
class AppendMergeOperator : public rocksdb::AssociativeMergeOperator {
public:
    explicit AppendMergeOperator(std::string delimiter = "|");

    bool Merge(const rocksdb::Slice& key,
               const rocksdb::Slice* existing_value,
               const rocksdb::Slice& value,
               std::string* new_value,
               rocksdb::Logger* logger) const override;

    const char* Name() const override { return "AppendMergeOperator"; }

private:
    std::string delimiter_;
};

/// SetMergeOperator - Union of unique values
/// Use Case: Unique value aggregation for sets
/// Example: "set:users:{id1}" + "set:users:{id2}" → "{id1,id2}"
class SetMergeOperator : public rocksdb::AssociativeMergeOperator {
public:
    bool Merge(const rocksdb::Slice& key,
               const rocksdb::Slice* existing_value,
               const rocksdb::Slice& value,
               std::string* new_value,
               rocksdb::Logger* logger) const override;

    const char* Name() const override { return "SetMergeOperator"; }
};

/// MaxMergeOperator - Keep maximum numeric value
/// Use Case: Track maximum values efficiently
/// Example: "max:temperature:25.5" + "max:temperature:26.3" → "26.3"
class MaxMergeOperator : public rocksdb::AssociativeMergeOperator {
public:
    bool Merge(const rocksdb::Slice& key,
               const rocksdb::Slice* existing_value,
               const rocksdb::Slice& value,
               std::string* new_value,
               rocksdb::Logger* logger) const override;

    const char* Name() const override { return "MaxMergeOperator"; }
};

} // namespace themis

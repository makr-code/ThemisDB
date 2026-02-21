/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            merge_operators.h                                  ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     97                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

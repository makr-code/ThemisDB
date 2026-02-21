/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            retention.h                                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     34                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_RETENTION_H
#define THEMIS_RETENTION_H

#include <string>
#include <unordered_map>
#include <chrono>

namespace rocksdb { class TransactionDB; class ColumnFamilyHandle; }

namespace themis {

class TSStore;

struct RetentionPolicy {
    // Retention per metric in seconds (0 or missing means ignore)
    std::unordered_map<std::string, std::chrono::seconds> per_metric;
};

class RetentionManager {
public:
    RetentionManager(TSStore* store, RetentionPolicy policy)
        : store_(store), policy_(std::move(policy)) {}

    // Apply retention for now()
    size_t apply();

private:
    TSStore* store_;
    RetentionPolicy policy_;
};

} // namespace themis

#endif // THEMIS_RETENTION_H

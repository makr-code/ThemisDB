/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wal_grpc_service.h                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>

namespace themis {
namespace sharding {
class WALApplier;
}

namespace server {

// gRPC WAL Apply service wrapper; returns nullptr if gRPC stubs are unavailable
class WalGrpcService {
public:
    explicit WalGrpcService(std::shared_ptr<sharding::WALApplier> wal_applier);
    ~WalGrpcService();

    // Returns grpc::Service* when gRPC is available, else nullptr
    void* service();

private:
    std::shared_ptr<sharding::WALApplier> wal_applier_;

    // Impl is only instantiated when shard_rpc gRPC headers are available
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace server
} // namespace themis

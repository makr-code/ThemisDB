/**
 * @file bench_replication_geo_wal_baselines.cpp
 * @brief Minimal compile benchmark contract for geo placement and WAL shipper APIs.
 */

#include "replication/async_wal_shipper.h"
#include "replication/geo_placement.h"

#include <chrono>
#include <iostream>

using namespace themisdb::replication;

int main() {
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "dc-b:9000";
    cfg.local_dc_id = "dc-a";

    AsyncWalShipper shipper(cfg);
    WalSegment seg;
    seg.sequence_number = 1;
    seg.data = "payload";
    seg.enqueue_time = std::chrono::steady_clock::now();
    seg.target_dc = "dc-b";
    (void)shipper.enqueueSegment(std::move(seg));

    GeoReplicaPlacementManager placement;
    PlacementConstraints constraints;
    std::vector<ReplicaInfo> replicas;
    auto validation = placement.validatePlacement(replicas, constraints);

    std::cout << "validation.is_valid=" << (validation.is_valid ? "true" : "false") << "\n";
    return 0;
}

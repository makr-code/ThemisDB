#include "distributed_tensor/local_projection_publisher.h"
#include "sharding/shard_artifact_fragment_store.h"
#include "storage/rocksdb_wrapper.h"
#include "tensor/adapter_repository.h"

#include <iostream>
#include <memory>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        return 2;
    }

    themis::RocksDBWrapper::Config config;
    config.db_path = argv[2];
    auto db = std::make_shared<themis::RocksDBWrapper>(config);
    if (!db->open()) {
        std::cerr << "unable to open smoke-test database\n";
        return 3;
    }

    const std::string mode = argv[1];
    if (mode == "seed") {
        auto backend = std::make_shared<themis::storage::RocksDBTensorBackend>(db);
        themis::tensor::AdapterRepository repository(backend, "tenant-a");

        themis::storage::TTTrain train;
        themis::storage::TTCore core;
        core.r_left = 1;
        core.n = 2;
        core.r_right = 1;
        core.data = {1.0f, 2.0f};
        train.cores.push_back(core);
        train.mode_sizes = {2};

        if (!repository.store("Alpha", "Alpha", train) ||
            !repository.store("alpha", "model-a", train)) {
            std::cerr << "unable to seed smoke-test adapters\n";
            return 4;
        }
    } else if (mode == "verify") {
        themis::distributed_tensor::LocalProjectionPublisher publisher(
            db, "shard-a", "node2",
            themis::sharding::ShardArtifactFragmentStore::kDefaultMaxFragmentBytes);
        const auto manifest = publisher.load("principal-a", argv[2]);
        if (!manifest || manifest->artifact_id() != argv[2]) {
            std::cerr << "startup manifest was not persisted\n";
            return 5;
        }
        themis::sharding::ShardArtifactFragmentStore fragments(db);
        const auto payload = fragments.read(
            "principal-a", "shard-a", argv[2],
            themis::sharding::ShardArtifactFragmentStore::kDefaultMaxFragmentBytes);
        if (!payload || payload->bytes.empty()) {
            std::cerr << "startup projection fragment was not persisted\n";
            return 6;
        }
        std::cout << manifest->artifact_id() << "\n";
    } else {
        return 7;
    }

    db->close();
    return 0;
}

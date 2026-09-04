#if __has_include("sharding/shard_rpc.grpc.pb.h")

#include <benchmark/benchmark.h>
#include <grpcpp/grpcpp.h>
#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "server/wal_grpc_service.h"
#include "sharding/wal_applier.h"
#include "utils/zstd_codec.h"

using themis::sharding::LSN;
using themis::sharding::WALEntry;
using themis::sharding::WALEntryType;
using themis::sharding::WALApplier;
using themis::sharding::WALApplierConfig;

namespace {

class WalGrpcApplyFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        entry_count_ = static_cast<size_t>(state.range(0));
        use_compressed_ = state.range(1) != 0;

        wal_applier_ = std::make_shared<WALApplier>(WALApplierConfig{.replica_id = "bench"});
        wal_applier_->setApplyHandler([this](const WALEntry& e) {
            applied_entries_.fetch_add(1, std::memory_order_relaxed);
            applied_bytes_.fetch_add(e.size(), std::memory_order_relaxed);
            return true;
        });

        service_ = std::make_unique<themis::server::WalGrpcService>(wal_applier_);
        auto* svc = static_cast<grpc::Service*>(service_->service());
        if (!svc) {
            state.SkipWithError("Shard gRPC stubs unavailable");
            return;
        }

        grpc::ServerBuilder builder;
        builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port_);
        builder.RegisterService(svc);
        builder.SetMaxReceiveMessageSize(100 * 1024 * 1024);
        builder.SetMaxSendMessageSize(100 * 1024 * 1024);
        server_ = builder.BuildAndStart();
        if (!server_) {
            state.SkipWithError("Failed to start gRPC server");
            return;
        }

        auto addr = "127.0.0.1:" + std::to_string(port_);
        channel_ = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
        stub_ = themis::sharding::proto::ShardService::NewStub(channel_);

        buildRequests(state);
    }

    void TearDown(const benchmark::State&) override {
        if (server_) {
            server_->Shutdown();
        }
        stub_.reset();
        channel_.reset();
        server_.reset();
        service_.reset();
        wal_applier_.reset();
    }

    bool ready() const {
        if (!stub_) {
          return false;
        }
        if (use_compressed_ && compressed_request_.entries_compressed().empty()) {
          return false;
        }
        return true;
    }

    void resetLSN() {
        wal_applier_->setCurrentLSN(LSN{0, 0});
        applied_entries_.store(0, std::memory_order_relaxed);
        applied_bytes_.store(0, std::memory_order_relaxed);
    }

    const themis::sharding::proto::ApplyWalBatchRequest& request() const {
        return use_compressed_ ? compressed_request_ : raw_request_;
    }

    size_t entryCount() const { return entry_count_; }

private:
    void buildRequests(const benchmark::State& state) {
        raw_request_.Clear();
        nlohmann::json array_payload = nlohmann::json::array();

        for (size_t i = 0; i < entry_count_; ++i) {
            auto* e = raw_request_.add_entries();
            auto lsn_str = std::string("1/") + std::to_string(i);
            e->set_lsn(lsn_str);
            e->set_type(themis::sharding::proto::WalEntryType::WAL_INSERT);
            e->set_timestamp(1000 + static_cast<uint64_t>(i));
            e->set_transaction_id("txn_" + std::to_string(i));
            nlohmann::json payload{{"key", "k" + std::to_string(i)}, {"value", { {"n", i} }}};
            auto payload_str = payload.dump();
            e->set_data_json(payload_str);

            array_payload.push_back({
                {"lsn", lsn_str},
                {"type", static_cast<int>(WALEntryType::INSERT)},
                {"timestamp", e->timestamp()},
                {"transaction_id", e->transaction_id()},
                {"data", payload}
            });
        }

        if (use_compressed_) {
            auto blob = array_payload.dump();
            auto compressed = themis::utils::zstd_compress(blob);
            if (compressed.empty()) {
                state.SkipWithError("ZSTD not available; compressed payload missing");
                return;
            }
            compressed_request_.set_entries_compressed(compressed.data(), compressed.size());
        }
    }

    size_t entry_count_{0};
    bool use_compressed_{false};
    std::shared_ptr<WALApplier> wal_applier_;
    std::unique_ptr<themis::server::WalGrpcService> service_;
    std::unique_ptr<grpc::Server> server_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<themis::sharding::proto::ShardService::Stub> stub_;
    int port_{0};

    themis::sharding::proto::ApplyWalBatchRequest raw_request_;
    themis::sharding::proto::ApplyWalBatchRequest compressed_request_;

    std::atomic<uint64_t> applied_entries_{0};
    std::atomic<uint64_t> applied_bytes_{0};
};

BENCHMARK_DEFINE_F(WalGrpcApplyFixture, ApplyWalBatch)(benchmark::State& state) {
    if (!ready()) {
        state.SkipWithError("Benchmark setup incomplete");
        return;
    }

    for (auto _ : state) {
        state.PauseTiming();
        resetLSN();
        state.ResumeTiming();

        themis::sharding::proto::ApplyWalBatchResponse resp;
        grpc::ClientContext ctx;
        auto status = stub_->ApplyWalBatch(&ctx, request(), &resp);
        if (!status.ok()) {
            state.SkipWithError(status.error_message().c_str());
            break;
        }
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * entryCount()));
    state.counters["entries_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * entryCount()), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(WalGrpcApplyFixture, ApplyWalBatch)
    ->Args({1, 0})   // 1 entry, raw
    ->Args({10, 0})  // 10 entries, raw
    ->Args({50, 0})  // 50 entries, raw
    ->Args({1, 1})   // 1 entry, zstd
    ->Args({10, 1})  // 10 entries, zstd
    ->Args({50, 1})  // 50 entries, zstd
    ->Unit(benchmark::kMicrosecond);

#else
#include <cstdio>

// No main function here - let Google Benchmark or test framework provide it
// If building without gRPC, this file will not be linked
#endif

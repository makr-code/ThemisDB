#include <gtest/gtest.h>
#include <grpcpp/grpcpp.h>
#include <atomic>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>

#include "server/wal_grpc_service.h"
#include "sharding/wal_applier.h"
#include "sharding/wal_manager.h"
#include "utils/zstd_codec.h"

#if __has_include("sharding/shard_rpc.grpc.pb.h")
#include "sharding/shard_rpc.grpc.pb.h"
#include "shard_rpc.pb.h"
#endif

using themis::sharding::WALEntry;
using themis::sharding::WALEntryType;
using themis::sharding::WALApplier;
using themis::sharding::WALApplierConfig;
using themis::sharding::LSN;

namespace {

struct EnvGuard {
    std::string name;
    std::string previous;
    bool had_previous{false};

    EnvGuard(std::string var_name, std::string value) : name(std::move(var_name)) {
        const char* existing = std::getenv(name.c_str());
        had_previous = (existing != nullptr);
        if (had_previous) {
            previous = existing;
        }
#ifdef _WIN32
        _putenv_s(name.c_str(), value.c_str());
#else
        ::setenv(name.c_str(), value.c_str(), 1);
#endif
    }

    ~EnvGuard() {
#ifdef _WIN32
        if (had_previous) {
            _putenv_s(name.c_str(), previous.c_str());
        } else {
            _putenv_s(name.c_str(), "");
        }
#else
        if (had_previous) {
            ::setenv(name.c_str(), previous.c_str(), 1);
        } else {
            ::unsetenv(name.c_str());
        }
#endif
    }
};

struct EnvUnsetGuard {
    std::string name;
    std::string previous;
    bool had_previous{false};

    explicit EnvUnsetGuard(std::string var_name) : name(std::move(var_name)) {
        const char* existing = std::getenv(name.c_str());
        had_previous = (existing != nullptr);
        if (had_previous) {
            previous = existing;
        }
#ifdef _WIN32
        _putenv_s(name.c_str(), "");
#else
        ::unsetenv(name.c_str());
#endif
    }

    ~EnvUnsetGuard() {
#ifdef _WIN32
        if (had_previous) {
            _putenv_s(name.c_str(), previous.c_str());
        } else {
            _putenv_s(name.c_str(), "");
        }
#else
        if (had_previous) {
            ::setenv(name.c_str(), previous.c_str(), 1);
        } else {
            ::unsetenv(name.c_str());
        }
#endif
    }
};

class WalGrpcApplyTest : public ::testing::Test {
protected:
    void SetUp() override {
#if __has_include("sharding/shard_rpc.grpc.pb.h")
        wal_applier_ = std::make_shared<WALApplier>(WALApplierConfig{.replica_id = "test"});
        wal_applier_->setApplyHandler([this](const WALEntry& e) {
            std::lock_guard<std::mutex> lock(mu_);
            applied_.insert(e.lsn.toString());
            return true;
        });

        service_ = std::make_unique<themis::server::WalGrpcService>(wal_applier_);
        auto* svc = service_->service();
        ASSERT_NE(svc, nullptr) << "gRPC stubs not generated; build with THEMIS_ENABLE_GRPC and protoc";

        grpc::ServerBuilder builder;
        std::string addr = "127.0.0.1:0";
        int selected_port = 0;
        builder.AddListeningPort(addr, grpc::InsecureServerCredentials(), &selected_port);
        builder.RegisterService(static_cast<grpc::Service*>(svc));
        builder.SetMaxReceiveMessageSize(100 * 1024 * 1024);
        builder.SetMaxSendMessageSize(100 * 1024 * 1024);
        server_ = builder.BuildAndStart();
        ASSERT_TRUE(server_) << "failed to start in-process gRPC server";

        std::string real_addr = "127.0.0.1:" + std::to_string(selected_port);
        channel_ = grpc::CreateChannel(real_addr, grpc::InsecureChannelCredentials());
        stub_ = themis::sharding::ShardService::NewStub(channel_);
#else
        GTEST_SKIP() << "Shard gRPC stubs not available";
#endif
    }

    void TearDown() override {
#if __has_include("sharding/shard_rpc.grpc.pb.h")
        if (server_) server_->Shutdown();
        stub_.reset();
        channel_.reset();
        service_.reset();
        wal_applier_.reset();
#endif
    }

#if __has_include("sharding/shard_rpc.grpc.pb.h")
    std::unique_ptr<themis::sharding::ShardService::Stub> stub_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<themis::server::WalGrpcService> service_;
    std::shared_ptr<WALApplier> wal_applier_;
    std::mutex mu_;
    std::set<std::string> applied_;
#endif
};

TEST_F(WalGrpcApplyTest, ApplyBatchRawSuccess) {
#if __has_include("sharding/shard_rpc.grpc.pb.h")
    themis::sharding::proto::ApplyWalBatchRequest req;
    auto* e1 = req.add_entries();
    e1->set_lsn("1/0");
    e1->set_type(themis::sharding::proto::WalEntryType::WAL_INSERT);
    e1->set_timestamp(123);
    e1->set_transaction_id("txn1");
    e1->set_data_json("{\"key\":\"k1\",\"value\":{\"v\":1}}");

    auto* e2 = req.add_entries();
    e2->set_lsn("1/1");
    e2->set_type(themis::sharding::proto::WalEntryType::WAL_DELETE);
    e2->set_timestamp(124);
    e2->set_transaction_id("txn2");
    e2->set_data_json("{\"key\":\"k2\"}");

    themis::sharding::proto::ApplyWalBatchResponse resp;
    grpc::ClientContext ctx;
    auto status = stub_->ApplyWalBatch(&ctx, req, &resp);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(resp.success());
    EXPECT_EQ(resp.entries_applied(), 2u);
    EXPECT_EQ(resp.last_applied_lsn(), "1/1");

    EXPECT_EQ(applied_.count("1/0"), 1u);
    EXPECT_EQ(applied_.count("1/1"), 1u);
#endif
}

TEST_F(WalGrpcApplyTest, ApplyBatchCompressedSuccess) {
#if __has_include("sharding/shard_rpc.grpc.pb.h")
    nlohmann::json arr = nlohmann::json::array({
        {
            {"lsn", "2/0"},
            {"type", static_cast<int>(WALEntryType::INSERT)},
            {"timestamp", 200},
            {"transaction_id", "txn3"},
            {"data", nlohmann::json{{"key", "k3"}, {"value", {"v", 3}}}}
        }
    });
    auto payload = arr.dump();
    auto compressed = themis::utils::zstd_compress(payload);
    if (compressed.empty()) {
        GTEST_SKIP() << "zstd compress failed (zstd likely unavailable)";
    }

    themis::sharding::proto::ApplyWalBatchRequest req;
    req.set_entries_compressed(compressed.data(), compressed.size());

    themis::sharding::proto::ApplyWalBatchResponse resp;
    grpc::ClientContext ctx;
    auto status = stub_->ApplyWalBatch(&ctx, req, &resp);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(resp.success());
    EXPECT_EQ(resp.entries_applied(), 1u);
    EXPECT_EQ(resp.last_applied_lsn(), "2/0");
    EXPECT_EQ(applied_.count("2/0"), 1u);
#endif
}

TEST_F(WalGrpcApplyTest, RejectsMissingPayload) {
#if __has_include("sharding/shard_rpc.grpc.pb.h")
    themis::sharding::proto::ApplyWalBatchRequest req;
    themis::sharding::proto::ApplyWalBatchResponse resp;
    grpc::ClientContext ctx;
    auto status = stub_->ApplyWalBatch(&ctx, req, &resp);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
#endif
}

#if !__has_include("sharding/shard_rpc.grpc.pb.h")
TEST(WalGrpcServiceStubGuardTest, StubRefusesProductionModeByDefault) {
    EnvGuard production("THEMIS_PRODUCTION_MODE", "1");
    EnvUnsetGuard allow_stub("THEMIS_ALLOW_WAL_GRPC_STUB");

    EXPECT_THROW(
        (void)std::make_unique<themis::server::WalGrpcService>(nullptr),
        std::runtime_error
    );
}

TEST(WalGrpcServiceStubGuardTest, StubCanBeExplicitlyAllowed) {
    EnvGuard production("THEMIS_PRODUCTION_MODE", "1");
    EnvGuard allow_stub("THEMIS_ALLOW_WAL_GRPC_STUB", "1");

    auto service = std::make_unique<themis::server::WalGrpcService>(nullptr);
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->service(), nullptr);
}
#endif

} // namespace

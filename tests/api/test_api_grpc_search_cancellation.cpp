#include <gtest/gtest.h>

#include "api/themisdb_grpc_service.h"
#include "themis/base/interfaces/index_interface.h"
#include "themis/base/interfaces/query_interface.h"

#include <grpcpp/grpcpp.h>
#include "themisdb.grpc.pb.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

class CooperativeVectorIndex final : public themis::IVectorIndex {
public:
    [[nodiscard]] bool insert(std::string_view, const std::vector<float>&) override {
        return true;
    }

    [[nodiscard]] bool remove(std::string_view) override { return true; }

    [[nodiscard]] std::vector<themis::VectorSearchResult> search(
        const std::vector<float>&,
        uint32_t,
        const themis::IExpressionEvaluator*) const override {
        return {themis::VectorSearchResult("result", 0.25f)};
    }

    [[nodiscard]] themis::VectorSearchOutcome search(
        const std::vector<float>&,
        uint32_t,
        const themis::IExpressionEvaluator*,
        const themis::query::RequestContext& request) const override {
        ++cooperative_search_calls;
        entered.store(true, std::memory_order_release);
        while (!request.shouldStop()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        return {{}, request.stopReason()};
    }

    [[nodiscard]] std::vector<themis::VectorSearchResult> rangeSearch(
        const std::vector<float>&,
        float,
        const themis::IExpressionEvaluator*) const override {
        return {};
    }

    [[nodiscard]] std::string getName() const override { return "cooperative"; }
    [[nodiscard]] uint32_t getDimension() const override { return 1; }
    [[nodiscard]] std::string getStatistics() const override { return "{}"; }

    mutable std::atomic<unsigned int> cooperative_search_calls{0};
    mutable std::atomic<bool> entered{false};
};

/// Query engine used to exercise the AQL stages of FullTextSearch/HybridSearch.
/// cooperative=true overrides the context-aware execute() and polls the request;
/// cooperative=false only implements the legacy overload (checkpoint-only default).
class StubQueryEngine final : public themis::IQueryEngine {
public:
    explicit StubQueryEngine(bool cooperative) : cooperative_(cooperative) {}

    using themis::IQueryEngine::execute;

    [[nodiscard]] themis::Result<std::string> execute(const std::string&) override {
        ++legacy_calls;
        return std::string("[{\"_key\":\"doc1\"}]");
    }

    [[nodiscard]] themis::Result<std::string> execute(
        const std::string& query,
        const std::string& bind_vars_json,
        const themis::query::RequestContext& request,
        themis::query::StopReason& stop_reason) override {
        if (!cooperative_) {
            return themis::IQueryEngine::execute(query, bind_vars_json, request, stop_reason);
        }
        ++cooperative_calls;
        entered.store(true, std::memory_order_release);
        while (!request.shouldStop()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        stop_reason = request.stopReason();
        return themis::Err<std::string>(
            themis::errors::ErrorCode::ERR_QUERY_CANCELLED, "stopped in flight");
    }

    [[nodiscard]] themis::Result<void> validate(const std::string&) const override {
        return {};
    }
    [[nodiscard]] themis::Result<std::unique_ptr<themis::IExpressionEvaluator>>
    createExpressionEvaluator() const override {
        return themis::Err<std::unique_ptr<themis::IExpressionEvaluator>>(
            themis::errors::ErrorCode::ERR_UNKNOWN, "unused");
    }
    [[nodiscard]] themis::Result<std::string> explainQuery(const std::string&) const override {
        return std::string("{}");
    }

    std::atomic<unsigned int> legacy_calls{0};
    std::atomic<unsigned int> cooperative_calls{0};
    std::atomic<bool> entered{false};

private:
    bool cooperative_;
};

/// Engine whose legacy execute() blocks until released, to show that the default
/// checkpoint-only overload cannot interrupt a running call but discards the result.
class BlockingLegacyEngine final : public themis::IQueryEngine {
public:
    [[nodiscard]] themis::Result<std::string> execute(const std::string&) override {
        entered.store(true, std::memory_order_release);
        while (!release.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        return std::string("[{\"_key\":\"late\"}]");
    }
    [[nodiscard]] themis::Result<void> validate(const std::string&) const override {
        return {};
    }
    [[nodiscard]] themis::Result<std::unique_ptr<themis::IExpressionEvaluator>>
    createExpressionEvaluator() const override {
        return themis::Err<std::unique_ptr<themis::IExpressionEvaluator>>(
            themis::errors::ErrorCode::ERR_UNKNOWN, "unused");
    }
    [[nodiscard]] themis::Result<std::string> explainQuery(const std::string&) const override {
        return std::string("{}");
    }
    std::atomic<bool> entered{false};
    std::atomic<bool> release{false};
};

void setValidVector(themis::api::Vector* vector) {
    vector->add_values(0.0f);
}

class RunningSearchServer {
public:
    explicit RunningSearchServer(std::shared_ptr<CooperativeVectorIndex> index,
                                 std::shared_ptr<themis::IQueryEngine> engine = nullptr)
        : owner_(nullptr, nullptr, std::move(engine), std::move(index)) {
        grpc::ServerBuilder builder;
        builder.AddListeningPort("127.0.0.1:0",
                                 grpc::InsecureServerCredentials(),
                                 &port_);
        builder.RegisterService(
            static_cast<themis::api::ThemisDBService::Service*>(owner_.service()));
        server_ = builder.BuildAndStart();
        if (server_ && port_ > 0) {
            channel_ = grpc::CreateChannel(
                "127.0.0.1:" + std::to_string(port_),
                grpc::InsecureChannelCredentials());
            stub_ = themis::api::ThemisDBService::NewStub(channel_);
        }
    }

    [[nodiscard]] bool ready() const noexcept {
        return server_ && stub_;
    }

    [[nodiscard]] themis::api::ThemisDBService::Stub& stub() const {
        return *stub_;
    }

private:
    themis::api::ThemisDBGrpcService owner_;
    int port_{0};
    std::unique_ptr<grpc::Server> server_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<themis::api::ThemisDBService::Stub> stub_;
};

template <typename Request, typename Response, typename Rpc>
void expectCooperativeCancellationInFlight(
    const std::shared_ptr<CooperativeVectorIndex>& index,
    Request request,
    Response response,
    Rpc rpc) {
    RunningSearchServer server(index);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    grpc::Status status;
    std::thread rpc_thread([&] {
        status = rpc(server.stub(), context, request, response);
    });

    const auto wait_deadline = std::chrono::steady_clock::now() +
                               std::chrono::seconds{2};
    while (!index->entered.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < wait_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    const bool search_entered = index->entered.load(std::memory_order_acquire);
    context.TryCancel();
    rpc_thread.join();

    EXPECT_TRUE(search_entered);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.hits_size(), 0);
}

} // namespace

TEST(GrpcSearchCancellation, AlreadyCancelledCallsReturnCancelledForAllVectorRoutes) {
    auto index = std::make_shared<CooperativeVectorIndex>();
    RunningSearchServer server(index);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext vector_context;
    vector_context.TryCancel();
    themis::api::VectorSearchRequest vector_request;
    setValidVector(vector_request.mutable_query_vector());
    themis::api::VectorSearchResponse vector_response;
    const auto vector_status = server.stub().VectorSearch(
        &vector_context, vector_request, &vector_response);
    EXPECT_EQ(vector_status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(vector_response.success());
    EXPECT_EQ(vector_response.hits_size(), 0);

    grpc::ClientContext filtered_context;
    filtered_context.TryCancel();
    themis::api::FilteredVectorSearchRequest filtered_request;
    setValidVector(filtered_request.mutable_query_vector());
    themis::api::VectorSearchResponse filtered_response;
    const auto filtered_status = server.stub().FilteredVectorSearch(
        &filtered_context, filtered_request, &filtered_response);
    EXPECT_EQ(filtered_status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(filtered_response.success());
    EXPECT_EQ(filtered_response.hits_size(), 0);

    grpc::ClientContext hybrid_context;
    hybrid_context.TryCancel();
    themis::api::HybridSearchRequest hybrid_request;
    setValidVector(hybrid_request.mutable_dense_vector());
    themis::api::HybridSearchResponse hybrid_response;
    const auto hybrid_status = server.stub().HybridSearch(
        &hybrid_context, hybrid_request, &hybrid_response);
    EXPECT_EQ(hybrid_status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(hybrid_response.success());
    EXPECT_EQ(hybrid_response.hits_size(), 0);
    EXPECT_EQ(index->cooperative_search_calls.load(), 0u);
}

TEST(GrpcSearchCancellation, ExpiredDeadlineReturnsDeadlineExceededForAllVectorRoutes) {
    auto index = std::make_shared<CooperativeVectorIndex>();
    RunningSearchServer server(index);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext vector_context;
    vector_context.set_deadline(std::chrono::system_clock::now() -
                                std::chrono::milliseconds{1});
    themis::api::VectorSearchRequest vector_request;
    setValidVector(vector_request.mutable_query_vector());
    themis::api::VectorSearchResponse vector_response;
    const auto vector_status = server.stub().VectorSearch(
        &vector_context, vector_request, &vector_response);
    EXPECT_EQ(vector_status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
    EXPECT_FALSE(vector_response.success());

    grpc::ClientContext filtered_context;
    filtered_context.set_deadline(std::chrono::system_clock::now() -
                                  std::chrono::milliseconds{1});
    themis::api::FilteredVectorSearchRequest filtered_request;
    setValidVector(filtered_request.mutable_query_vector());
    themis::api::VectorSearchResponse filtered_response;
    const auto filtered_status = server.stub().FilteredVectorSearch(
        &filtered_context, filtered_request, &filtered_response);
    EXPECT_EQ(filtered_status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
    EXPECT_FALSE(filtered_response.success());

    grpc::ClientContext hybrid_context;
    hybrid_context.set_deadline(std::chrono::system_clock::now() -
                                std::chrono::milliseconds{1});
    themis::api::HybridSearchRequest hybrid_request;
    setValidVector(hybrid_request.mutable_dense_vector());
    themis::api::HybridSearchResponse hybrid_response;
    const auto hybrid_status = server.stub().HybridSearch(
        &hybrid_context, hybrid_request, &hybrid_response);
    EXPECT_EQ(hybrid_status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
    EXPECT_FALSE(hybrid_response.success());
    EXPECT_EQ(index->cooperative_search_calls.load(), 0u);
}

TEST(GrpcSearchCancellation, CooperativeIndexObservesCancellationInFlight) {
    auto index = std::make_shared<CooperativeVectorIndex>();
    themis::api::VectorSearchRequest request;
    setValidVector(request.mutable_query_vector());
    themis::api::VectorSearchResponse response;
    expectCooperativeCancellationInFlight(
        index, std::move(request), std::move(response),
        [](auto& stub, auto& context, auto& req, auto& resp) {
            return stub.VectorSearch(&context, req, &resp);
        });
}

TEST(GrpcSearchCancellation, CooperativeIndexObservesCancellationInFilteredSearch) {
    auto index = std::make_shared<CooperativeVectorIndex>();
    themis::api::FilteredVectorSearchRequest request;
    setValidVector(request.mutable_query_vector());
    themis::api::VectorSearchResponse response;
    expectCooperativeCancellationInFlight(
        index, std::move(request), std::move(response),
        [](auto& stub, auto& context, auto& req, auto& resp) {
            return stub.FilteredVectorSearch(&context, req, &resp);
        });
}

TEST(GrpcSearchCancellation, CooperativeIndexObservesCancellationInHybridSearch) {
    auto index = std::make_shared<CooperativeVectorIndex>();
    themis::api::HybridSearchRequest request;
    setValidVector(request.mutable_dense_vector());
    themis::api::HybridSearchResponse response;
    expectCooperativeCancellationInFlight(
        index, std::move(request), std::move(response),
        [](auto& stub, auto& context, auto& req, auto& resp) {
            return stub.HybridSearch(&context, req, &resp);
        });
}

TEST(GrpcSearchCancellation, CooperativeIndexObservesDeadlineInFlight) {
    auto index = std::make_shared<CooperativeVectorIndex>();
    RunningSearchServer server(index);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() +
                         std::chrono::seconds{1});
    themis::api::VectorSearchRequest request;
    setValidVector(request.mutable_query_vector());
    themis::api::VectorSearchResponse response;

    const auto status = server.stub().VectorSearch(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
    EXPECT_EQ(index->cooperative_search_calls.load(), 1u);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.hits_size(), 0);
}

TEST(GrpcSearchCancellation, FullTextAlreadyCancelledDoesNotInvokeEngine) {
    auto engine = std::make_shared<StubQueryEngine>(false);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    context.TryCancel();
    themis::api::FullTextSearchRequest request;
    request.set_query("term");
    request.set_collection("docs");
    themis::api::FullTextSearchResponse response;
    const auto status = server.stub().FullTextSearch(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.hits_size(), 0);
    EXPECT_EQ(engine->legacy_calls.load(), 0u);
}

TEST(GrpcSearchCancellation, FullTextExpiredDeadlineDoesNotInvokeEngine) {
    auto engine = std::make_shared<StubQueryEngine>(false);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() -
                         std::chrono::milliseconds{1});
    themis::api::FullTextSearchRequest request;
    request.set_query("term");
    request.set_collection("docs");
    themis::api::FullTextSearchResponse response;
    const auto status = server.stub().FullTextSearch(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.hits_size(), 0);
    EXPECT_EQ(engine->legacy_calls.load(), 0u);
}

TEST(GrpcSearchCancellation, FullTextCooperativeEngineObservesCancellationInFlight) {
    auto engine = std::make_shared<StubQueryEngine>(true);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    themis::api::FullTextSearchRequest request;
    request.set_query("term");
    request.set_collection("docs");
    themis::api::FullTextSearchResponse response;
    grpc::Status status;
    std::thread rpc_thread([&] {
        status = server.stub().FullTextSearch(&context, request, &response);
    });
    const auto wait_deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!engine->entered.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < wait_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    const bool entered = engine->entered.load(std::memory_order_acquire);
    context.TryCancel();
    rpc_thread.join();

    EXPECT_TRUE(entered);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.hits_size(), 0);
}

TEST(GrpcSearchCancellation, FullTextCooperativeEngineObservesDeadlineInFlight) {
    auto engine = std::make_shared<StubQueryEngine>(true);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds{1});
    themis::api::FullTextSearchRequest request;
    request.set_query("term");
    request.set_collection("docs");
    themis::api::FullTextSearchResponse response;
    const auto status = server.stub().FullTextSearch(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
    EXPECT_EQ(engine->cooperative_calls.load(), 1u);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.hits_size(), 0);
}

TEST(GrpcSearchCancellation, FullTextNonOverridingEngineDiscardsResultAfterStop) {
    // Default overload is checkpoint-only: the blocking call is not interrupted,
    // but its successful result is discarded once the stop is observed.
    auto engine = std::make_shared<BlockingLegacyEngine>();
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    themis::api::FullTextSearchRequest request;
    request.set_query("term");
    request.set_collection("docs");
    themis::api::FullTextSearchResponse response;
    grpc::Status status;
    std::thread rpc_thread([&] {
        status = server.stub().FullTextSearch(&context, request, &response);
    });
    const auto wait_deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!engine->entered.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < wait_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    const bool engine_entered = engine->entered.load(std::memory_order_acquire);
    context.TryCancel();
    // Allow the server to observe the cancellation before the engine returns.
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    engine->release.store(true, std::memory_order_release);
    rpc_thread.join();

    EXPECT_TRUE(engine_entered);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.hits_size(), 0);
}

TEST(GrpcSearchCancellation, HybridSparseStageCooperativeCancellationClearsPartialHits) {
    auto index = std::make_shared<CooperativeVectorIndex>();
    auto engine = std::make_shared<StubQueryEngine>(true);
    RunningSearchServer server(index, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    themis::api::HybridSearchRequest request;
    request.set_collection("docs");
    request.set_sparse_query("term");  // no dense vector: only sparse AQL stage runs
    themis::api::HybridSearchResponse response;
    grpc::Status status;
    std::thread rpc_thread([&] {
        status = server.stub().HybridSearch(&context, request, &response);
    });
    const auto wait_deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!engine->entered.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < wait_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    const bool entered = engine->entered.load(std::memory_order_acquire);
    context.TryCancel();
    rpc_thread.join();

    EXPECT_TRUE(entered);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.hits_size(), 0);
}

namespace {

/// Legacy-only engine returning a large JSON array, for StreamAQL row emission.
class ManyRowsEngine final : public themis::IQueryEngine {
public:
    explicit ManyRowsEngine(std::size_t rows) {
        payload_ = "[";
        for (std::size_t i = 0; i < rows; ++i) {
            if (i) payload_ += ",";
            payload_ += "{\"_key\":\"" + std::to_string(i) + "\",\"pad\":\"" +
                        std::string(1024, 'x') + "\"}";
        }
        payload_ += "]";
    }
    [[nodiscard]] themis::Result<std::string> execute(const std::string&) override {
        ++calls;
        return payload_;
    }
    [[nodiscard]] themis::Result<void> validate(const std::string&) const override {
        return {};
    }
    [[nodiscard]] themis::Result<std::unique_ptr<themis::IExpressionEvaluator>>
    createExpressionEvaluator() const override {
        return themis::Err<std::unique_ptr<themis::IExpressionEvaluator>>(
            themis::errors::ErrorCode::ERR_UNKNOWN, "unused");
    }
    [[nodiscard]] themis::Result<std::string> explainQuery(const std::string&) const override {
        return std::string("{}");
    }
    std::atomic<unsigned int> calls{0};

private:
    std::string payload_;
};

void waitForEngine(const std::atomic<bool>& entered) {
    const auto wait_deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!entered.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < wait_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
}

} // namespace

TEST(GrpcAqlCancellation, ExecuteAqlAlreadyCancelledDoesNotInvokeEngine) {
    auto engine = std::make_shared<StubQueryEngine>(true);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    context.TryCancel();
    themis::api::AQLQueryRequest request;
    request.set_query("FOR d IN docs RETURN d");
    themis::api::AQLQueryResponse response;
    const auto status = server.stub().ExecuteAQL(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.rows_size(), 0);
    EXPECT_EQ(engine->cooperative_calls.load(), 0u);
    EXPECT_EQ(engine->legacy_calls.load(), 0u);
}

TEST(GrpcAqlCancellation, ExecuteAqlCooperativeEngineObservesCancellationInFlight) {
    auto engine = std::make_shared<StubQueryEngine>(true);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    themis::api::AQLQueryRequest request;
    request.set_query("FOR d IN docs RETURN d");
    themis::api::AQLQueryResponse response;
    grpc::Status status;
    std::thread rpc_thread([&] {
        status = server.stub().ExecuteAQL(&context, request, &response);
    });
    waitForEngine(engine->entered);
    const bool entered = engine->entered.load(std::memory_order_acquire);
    context.TryCancel();
    rpc_thread.join();

    EXPECT_TRUE(entered);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.rows_size(), 0);
}

TEST(GrpcAqlCancellation, ExecuteAqlCooperativeEngineObservesDeadlineInFlight) {
    auto engine = std::make_shared<StubQueryEngine>(true);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds{1});
    themis::api::AQLQueryRequest request;
    request.set_query("FOR d IN docs RETURN d");
    themis::api::AQLQueryResponse response;
    const auto status = server.stub().ExecuteAQL(&context, request, &response);

    // Client-side deadline and server-side stop both map to DEADLINE_EXCEEDED.
    EXPECT_EQ(status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
    EXPECT_EQ(engine->cooperative_calls.load(), 1u);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.rows_size(), 0);
}

TEST(GrpcAqlCancellation, ExecuteAqlLegacyEngineDiscardsResultAfterStop) {
    // Checkpoint-only default: the blocking call is not interrupted, its result is dropped.
    auto engine = std::make_shared<BlockingLegacyEngine>();
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    themis::api::AQLQueryRequest request;
    request.set_query("FOR d IN docs RETURN d");
    themis::api::AQLQueryResponse response;
    grpc::Status status;
    std::thread rpc_thread([&] {
        status = server.stub().ExecuteAQL(&context, request, &response);
    });
    waitForEngine(engine->entered);
    const bool entered = engine->entered.load(std::memory_order_acquire);
    context.TryCancel();
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    engine->release.store(true, std::memory_order_release);
    rpc_thread.join();

    EXPECT_TRUE(entered);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_FALSE(response.success());
    EXPECT_EQ(response.rows_size(), 0);
}

TEST(GrpcAqlCancellation, StreamAqlAlreadyCancelledDoesNotInvokeEngine) {
    auto engine = std::make_shared<StubQueryEngine>(true);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    context.TryCancel();
    themis::api::AQLQueryRequest request;
    request.set_query("FOR d IN docs RETURN d");
    auto reader = server.stub().StreamAQL(&context, request);
    themis::api::AQLRow row;
    std::size_t rows = 0;
    while (reader->Read(&row)) ++rows;
    const auto status = reader->Finish();

    EXPECT_EQ(rows, 0u);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_EQ(engine->cooperative_calls.load(), 0u);
}

TEST(GrpcAqlCancellation, StreamAqlCooperativeEngineObservesCancellationInFlight) {
    auto engine = std::make_shared<StubQueryEngine>(true);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    themis::api::AQLQueryRequest request;
    request.set_query("FOR d IN docs RETURN d");
    auto reader = server.stub().StreamAQL(&context, request);
    waitForEngine(engine->entered);
    const bool entered = engine->entered.load(std::memory_order_acquire);
    context.TryCancel();
    themis::api::AQLRow row;
    std::size_t rows = 0;
    while (reader->Read(&row)) ++rows;
    const auto status = reader->Finish();

    EXPECT_TRUE(entered);
    EXPECT_EQ(rows, 0u);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
}

TEST(GrpcAqlCancellation, StreamAqlCooperativeEngineObservesDeadlineInFlight) {
    auto engine = std::make_shared<StubQueryEngine>(true);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds{1});
    themis::api::AQLQueryRequest request;
    request.set_query("FOR d IN docs RETURN d");
    auto reader = server.stub().StreamAQL(&context, request);
    themis::api::AQLRow row;
    std::size_t rows = 0;
    while (reader->Read(&row)) ++rows;
    const auto status = reader->Finish();

    EXPECT_EQ(rows, 0u);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
    EXPECT_EQ(engine->cooperative_calls.load(), 1u);
}

TEST(GrpcAqlCancellation, StreamAqlStopsEmittingRowsAfterClientCancel) {
    // ~8 MiB of rows exceeds transport flow-control windows, so the server is still
    // emitting when the client cancels after the first row.
    constexpr std::size_t kRows = 8000;
    auto engine = std::make_shared<ManyRowsEngine>(kRows);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    themis::api::AQLQueryRequest request;
    request.set_query("FOR d IN docs RETURN d");
    auto reader = server.stub().StreamAQL(&context, request);
    themis::api::AQLRow row;
    std::size_t rows = 0;
    if (reader->Read(&row)) {
        ++rows;
        context.TryCancel();
    }
    while (reader->Read(&row)) ++rows;
    const auto status = reader->Finish();

    EXPECT_GE(rows, 1u);
    EXPECT_LT(rows, kRows);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    EXPECT_EQ(engine->calls.load(), 1u);
}

TEST(GrpcAqlCancellation, StreamAqlUncancelledStreamsAllRows) {
    auto engine = std::make_shared<ManyRowsEngine>(5);
    RunningSearchServer server(nullptr, engine);
    ASSERT_TRUE(server.ready());

    grpc::ClientContext context;
    themis::api::AQLQueryRequest request;
    request.set_query("FOR d IN docs RETURN d");
    auto reader = server.stub().StreamAQL(&context, request);
    themis::api::AQLRow row;
    std::size_t rows = 0;
    while (reader->Read(&row)) ++rows;
    const auto status = reader->Finish();

    EXPECT_TRUE(status.ok());
    EXPECT_EQ(rows, 5u);
}

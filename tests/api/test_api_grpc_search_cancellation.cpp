#include <gtest/gtest.h>

#include "api/themisdb_grpc_service.h"
#include "themis/base/interfaces/index_interface.h"

#include <grpcpp/grpcpp.h>
#include "themisdb.grpc.pb.h"

#include <atomic>
#include <chrono>
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

void setValidVector(themis::api::Vector* vector) {
    vector->add_values(0.0f);
}

class RunningSearchServer {
public:
    explicit RunningSearchServer(std::shared_ptr<CooperativeVectorIndex> index)
        : owner_(nullptr, nullptr, nullptr, std::move(index)) {
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

/**
 * @file test_mtls_client_cancellation.cpp
 * @brief In-flight cancellation / deadline tests for MTLSClient's asynchronous
 *        RequestContext overloads and RemoteExecutor propagation.
 *
 * A "black hole" server (listening socket that completes the TCP handshake in
 * the kernel backlog but never speaks TLS) keeps the client blocked inside the
 * TLS handshake, which is exactly the state a synchronous Beast call cannot be
 * interrupted from. Key/cert are generated in-process (no external files).
 */

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include "sharding/mtls_client.h"
#include "sharding/remote_executor.h"
#include "content/text_processor.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"

namespace themis::sharding {
namespace {

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

int findAvailablePort() {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return 0;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        ::close(fd);
        return 0;
    }
    socklen_t length = sizeof(address);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &length) != 0) {
        ::close(fd);
        return 0;
    }
    const int port = ntohs(address.sin_port);
    ::close(fd);
    return port;
}

struct TestPki {
    std::filesystem::path dir;
    std::string cert;
    std::string key;
    bool ok = false;

    TestPki() {
        dir = std::filesystem::temp_directory_path() /
              ("themis_mtls_cancel_" + std::to_string(::getpid()) + "_" +
               std::to_string(Clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(dir);
        cert = (dir / "cert.pem").string();
        key = (dir / "key.pem").string();

        EVP_PKEY* pkey = EVP_RSA_gen(2048);
        X509* x509 = X509_new();
        if (!pkey || !x509) return;
        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
        X509_gmtime_adj(X509_getm_notBefore(x509), 0);
        X509_gmtime_adj(X509_getm_notAfter(x509), 3600);
        X509_set_pubkey(x509, pkey);
        X509_NAME* name = X509_get_subject_name(x509);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char*>("localhost"), -1, -1, 0);
        X509_set_issuer_name(x509, name);
        if (!X509_sign(x509, pkey, EVP_sha256())) return;

        FILE* kf = std::fopen(key.c_str(), "wb");
        FILE* cf = std::fopen(cert.c_str(), "wb");
        if (kf && cf) {
            ok = PEM_write_PrivateKey(kf, pkey, nullptr, nullptr, 0, nullptr, nullptr) == 1 &&
                 PEM_write_X509(cf, x509) == 1;
        }
        if (kf) std::fclose(kf);
        if (cf) std::fclose(cf);
        X509_free(x509);
        EVP_PKEY_free(pkey);
    }
    ~TestPki() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
};

/// Listening socket that is never accept()ed or read: TLS handshake hangs.
class BlackHole {
public:
    BlackHole() {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
            ::listen(fd_, 16) != 0) {
            ::close(fd_);
            fd_ = -1;
            return;
        }
        socklen_t len = sizeof(addr);
        ::getsockname(fd_, reinterpret_cast<sockaddr*>(&addr), &len);
        port_ = ntohs(addr.sin_port);
    }
    ~BlackHole() { close(); }
    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    bool valid() const { return fd_ >= 0; }
    std::string endpoint() const { return "https://127.0.0.1:" + std::to_string(port_); }
    int port() const { return port_; }

private:
    int fd_ = -1;
    int port_ = 0;
};

/// Local TLS peer that stalls before reading a large request or mid-response.
class StallingTlsPeer {
public:
    enum class Mode { DoNotReadRequest, PartialResponse };

    StallingTlsPeer(const TestPki& pki, Mode mode)
        : ssl_context_(ssl::context::tlsv12_server),
          acceptor_(ioc_, tcp::endpoint(net::ip::make_address("127.0.0.1"), 0)),
          mode_(mode) {
        ssl_context_.use_certificate_chain_file(pki.cert);
        ssl_context_.use_private_key_file(pki.key, ssl::context::pem);
        port_ = acceptor_.local_endpoint().port();
        worker_ = std::thread([this] { serve(); });
    }

    ~StallingTlsPeer() {
        beast::error_code ec;
        acceptor_.close(ec);
        if (worker_.joinable()) worker_.join();
    }

    std::string endpoint() const {
        return "https://127.0.0.1:" + std::to_string(port_);
    }

private:
    void serve() {
        beast::error_code ec;
        tcp::socket socket(ioc_);
        acceptor_.accept(socket, ec);
        if (ec) return;
        socket.set_option(net::socket_base::receive_buffer_size(1024), ec);
        ssl::stream<tcp::socket> stream(std::move(socket), ssl_context_);
        stream.handshake(ssl::stream_base::server, ec);
        if (ec) return;

        if (mode_ == Mode::DoNotReadRequest) {
            std::this_thread::sleep_for(500ms);
            return;
        }

        beast::flat_buffer request_buffer;
        http::request<http::string_body> request;
        http::read(stream, request_buffer, request, ec);
        if (ec) return;

        http::response<http::empty_body> response{http::status::ok, 11};
        response.set(http::field::content_type, "application/json");
        response.content_length(1024);
        http::response_serializer<http::empty_body> serializer{response};
        http::write_header(stream, serializer, ec);
        if (!ec) {
            net::write(stream, net::buffer("{"), ec);
            std::this_thread::sleep_for(500ms);
        }
    }

    net::io_context ioc_;
    ssl::context ssl_context_;
    tcp::acceptor acceptor_;
    Mode mode_;
    int port_ = 0;
    std::thread worker_;
};

class MtlsCancellationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { pki_ = std::make_unique<TestPki>(); }
    static void TearDownTestSuite() { pki_.reset(); }

    void SetUp() override {
        ASSERT_TRUE(pki_->ok) << "failed to generate test certificate";
        ASSERT_TRUE(hole_.valid());
    }

    static MTLSClient::Config clientConfig() {
        MTLSClient::Config c;
        c.cert_path = pki_->cert;
        c.key_path = pki_->key;
        c.ca_cert_path = pki_->cert;
        c.max_retries = 0;
        c.connect_timeout_ms = 5000;
        c.request_timeout_ms = 30000;
        c.use_connection_pool = false;
        c.verify_hostname = false;
        return c;
    }

    static std::unique_ptr<TestPki> pki_;
    BlackHole hole_;
};

std::unique_ptr<TestPki> MtlsCancellationTest::pki_;

}  // namespace

TEST_F(MtlsCancellationTest, TokenCancelAbortsInFlightHandshakePromptly) {
    MTLSClient client(clientConfig());
    auto token = std::make_shared<query::QueryCancellationToken>();
    std::thread canceller([&] {
        std::this_thread::sleep_for(100ms);
        token->cancel();
    });

    const auto t0 = Clock::now();
    const auto resp = client.post(hole_.endpoint(), "/search/hybrid", {{"q", "x"}},
                                  query::RequestContext(token));
    const auto elapsed = Clock::now() - t0;
    canceller.join();

    EXPECT_TRUE(resp.cancelled);
    EXPECT_FALSE(resp.timed_out);
    EXPECT_FALSE(resp.success);
    EXPECT_NE(resp.error.find("cancelled"), std::string::npos) << resp.error;
    EXPECT_GE(elapsed, 90ms);
    EXPECT_LT(elapsed, 2s);  // far below the 5 s connect / 30 s request timeouts
}

TEST_F(MtlsCancellationTest, RequestDeadlineAbortsInFlightHandshake) {
    MTLSClient client(clientConfig());
    const auto t0 = Clock::now();
    const auto resp = client.get(hole_.endpoint(), "/x",
                                 query::RequestContext::withTimeout(150ms));
    const auto elapsed = Clock::now() - t0;

    EXPECT_TRUE(resp.timed_out);
    EXPECT_FALSE(resp.cancelled);
    EXPECT_FALSE(resp.success);
    EXPECT_LT(elapsed, 2s);
}

TEST_F(MtlsCancellationTest, StageTimeoutIsReportedAsTimeoutWithoutRequestDeadline) {
    auto cfg = clientConfig();
    cfg.connect_timeout_ms = 120;  // applies to resolve/connect/handshake
    MTLSClient client(cfg);

    const auto t0 = Clock::now();
    const auto resp = client.get(hole_.endpoint(), "/x", query::RequestContext{});
    const auto elapsed = Clock::now() - t0;

    EXPECT_TRUE(resp.stage_timed_out);
    EXPECT_FALSE(resp.timed_out);  // request deadline did not expire
    EXPECT_FALSE(resp.cancelled);
    EXPECT_NE(resp.error.find("handshake"), std::string::npos) << resp.error;
    EXPECT_LT(elapsed, 2s);
}

TEST_F(MtlsCancellationTest, PreCancelledRequestFailsWithoutNetworkIo) {
    MTLSClient client(clientConfig());
    auto token = std::make_shared<query::QueryCancellationToken>();
    token->cancel();

    const auto t0 = Clock::now();
    const auto resp = client.get(hole_.endpoint(), "/x", query::RequestContext(token));
    EXPECT_TRUE(resp.cancelled);
    EXPECT_LT(Clock::now() - t0, 100ms);
}

TEST_F(MtlsCancellationTest, ConnectionRefusedIsPlainFailureNotCancelOrTimeout) {
    const auto endpoint = hole_.endpoint();
    hole_.close();  // port now refuses connections
    MTLSClient client(clientConfig());

    const auto resp = client.get(endpoint, "/x", query::RequestContext::withTimeout(5s));
    EXPECT_FALSE(resp.success);
    EXPECT_FALSE(resp.cancelled);
    EXPECT_FALSE(resp.timed_out);
    EXPECT_FALSE(resp.error.empty());
}

TEST_F(MtlsCancellationTest, RemoteExecutorPropagatesContextIntoTransport) {
    RemoteExecutor::Config cfg;
    cfg.cert_path = pki_->cert;
    cfg.key_path = pki_->key;
    cfg.ca_cert_path = pki_->cert;
    cfg.local_shard_id = "local";
    cfg.enable_signing = false;
    cfg.max_retries = 0;
    RemoteExecutor executor(cfg);

    ShardInfo shard{};
    shard.shard_id = "shard_x";
    shard.primary_endpoint = hole_.endpoint();
    shard.is_healthy = true;

    const auto resp = executor.post(shard, "/search/hybrid", {{"q", "x"}},
                                    query::RequestContext::withTimeout(150ms));
    EXPECT_FALSE(resp.success);
    EXPECT_TRUE(resp.timed_out);
    EXPECT_FALSE(resp.stage_timed_out);
    EXPECT_FALSE(resp.cancelled);

    auto token = std::make_shared<query::QueryCancellationToken>();
    token->cancel();
    const auto pre = executor.post(shard, "/search/hybrid", {{"q", "x"}},
                                   query::RequestContext(token));
    EXPECT_TRUE(pre.cancelled);
}

TEST_F(MtlsCancellationTest, PutAndDeleteHonorInFlightRequestDeadline) {
    MTLSClient client(clientConfig());
    const auto ctx = query::RequestContext::withTimeout(150ms);

    const auto put = client.put(hole_.endpoint(), "/resource", {{"value", 1}}, ctx);
    EXPECT_TRUE(put.timed_out);
    EXPECT_FALSE(put.cancelled);
    EXPECT_FALSE(put.stage_timed_out);

    const auto del = client.del(hole_.endpoint(), "/resource",
                                query::RequestContext::withTimeout(150ms));
    EXPECT_TRUE(del.timed_out);
    EXPECT_FALSE(del.cancelled);
    EXPECT_FALSE(del.stage_timed_out);
}

TEST_F(MtlsCancellationTest, AuthenticatedPostHonorsInFlightRequestDeadline) {
    MTLSClient client(clientConfig());
    const auto response = client.post(
        hole_.endpoint(), "/authenticated", {{"value", 1}}, "******",
        query::RequestContext::withTimeout(150ms));
    EXPECT_TRUE(response.timed_out);
    EXPECT_FALSE(response.cancelled);
    EXPECT_FALSE(response.stage_timed_out);
}

TEST_F(MtlsCancellationTest, RemoteExecutorPropagatesContextForPutDeleteAndBinaryPost) {
    RemoteExecutor::Config cfg;
    cfg.cert_path = pki_->cert;
    cfg.key_path = pki_->key;
    cfg.ca_cert_path = pki_->cert;
    cfg.local_shard_id = "local";
    cfg.enable_signing = false;
    cfg.max_retries = 0;
    RemoteExecutor executor(cfg);

    ShardInfo shard{};
    shard.shard_id = "shard_x";
    shard.primary_endpoint = hole_.endpoint();
    shard.is_healthy = true;
    const auto ctx = query::RequestContext::withTimeout(150ms);

    const auto put = executor.put(shard, "/resource", {{"value", 1}}, ctx);
    EXPECT_TRUE(put.timed_out);
    const auto del = executor.del(shard, "/resource",
                                  query::RequestContext::withTimeout(150ms));
    EXPECT_TRUE(del.timed_out);
    const std::uint8_t payload[] = {1, 2, 3};
    const auto binary = executor.postBinary(shard, "/binary", payload,
                                            sizeof(payload),
                                            query::RequestContext::withTimeout(150ms));
    EXPECT_TRUE(binary.timed_out);

    const auto query_result = executor.executeQuery(
        shard, "FOR d IN documents RETURN d",
        query::RequestContext::withTimeout(150ms));
    EXPECT_TRUE(query_result.timed_out);
}

TEST_F(MtlsCancellationTest, RequestDeadlineAbortsMidResponseAndJoinsHandlers) {
    StallingTlsPeer peer(*pki_, StallingTlsPeer::Mode::PartialResponse);
    MTLSClient client(clientConfig());

    const auto started = Clock::now();
    const auto response = client.get(
        peer.endpoint(), "/slow-search",
        query::RequestContext::withTimeout(150ms));
    const auto elapsed = Clock::now() - started;

    EXPECT_FALSE(response.success);
    EXPECT_TRUE(response.timed_out);
    EXPECT_FALSE(response.cancelled);
    EXPECT_FALSE(response.stage_timed_out);
    EXPECT_NE(response.error.find("read"), std::string::npos) << response.error;
    EXPECT_LT(elapsed, 2s);
}

TEST_F(MtlsCancellationTest, RequestDeadlineAbortsMidWriteWithoutDetachedClientWork) {
    StallingTlsPeer peer(*pki_, StallingTlsPeer::Mode::DoNotReadRequest);
    MTLSClient client(clientConfig());
    const std::string large_body(16 * 1024 * 1024, 'x');
    const nlohmann::json body{{"payload", large_body}};

    auto ctx = query::RequestContext::withTimeout(150ms);
    const auto started = Clock::now();
    const auto response = client.post(peer.endpoint(), "/slow-search", body, ctx);
    const auto elapsed = Clock::now() - started;

    EXPECT_FALSE(response.success);
    EXPECT_TRUE(response.timed_out);
    EXPECT_FALSE(response.cancelled);
    EXPECT_FALSE(response.stage_timed_out);
    EXPECT_NE(response.error.find("write"), std::string::npos) << response.error;
    EXPECT_LT(elapsed, 2s);
}

TEST_F(MtlsCancellationTest, RealHttpsShardSearchDeadlineReturnsAfterWorkerCleanup) {
    const int port = findAvailablePort();
    ASSERT_GT(port, 0);

    const auto db_path = pki_->dir / "real_http_shard_db";
    themis::RocksDBWrapper::Config storage_config;
    storage_config.db_path = db_path.string();
    auto storage = std::make_shared<themis::RocksDBWrapper>(storage_config);
    ASSERT_TRUE(storage->open());

    auto secondary = std::make_shared<themis::SecondaryIndexManager>(*storage);
    auto graph = std::make_shared<themis::GraphIndexManager>(*storage);
    auto vectors = std::make_shared<themis::VectorIndexManager>(*storage);
    auto transactions = std::make_shared<themis::TransactionManager>(
        *storage, *secondary, *graph, *vectors);

    themis::server::HttpServer::Config server_config;
    server_config.host = "127.0.0.1";
    server_config.port = static_cast<std::uint16_t>(port);
    server_config.num_threads = 1;
    server_config.request_timeout_ms = 500;
    server_config.enable_tls = true;
    server_config.tls_cert_path = pki_->cert;
    server_config.tls_key_path = pki_->key;
    server_config.tls_min_version = "TLSv1.2";
    server_config.health_error_service_enabled = false;
    themis::server::HttpServer server(
        server_config, storage, secondary, graph, vectors, transactions);
    ASSERT_NO_THROW(server.start());

    RemoteExecutor::Config executor_config;
    executor_config.cert_path = pki_->cert;
    executor_config.key_path = pki_->key;
    executor_config.ca_cert_path = pki_->cert;
    executor_config.local_shard_id = "local";
    executor_config.enable_signing = false;
    executor_config.enable_circuit_breaker = false;
    executor_config.connect_timeout_ms = 3000;
    executor_config.request_timeout_ms = 5000;
    executor_config.max_retries = 0;
    RemoteExecutor executor(executor_config);

    ShardInfo shard{};
    shard.shard_id = "real_local_shard";
    shard.primary_endpoint = "https://localhost:" + std::to_string(port);
    shard.is_healthy = true;

    themis::content::TextProcessor text_processor;
    const auto embedding = text_processor.generateEmbedding("seed");
    const auto seed = executor.post(
        shard, "/content/import",
        {{"content", {{"id", "qry07-seed"}, {"mime_type", "text/plain"}}},
         {"chunks", {{{"id", "qry07-seed-chunk"},
                      {"seq_num", 0},
                      {"chunk_type", "text"},
                      {"text", "seed"},
                      {"embedding", embedding}}}}},
        query::RequestContext::withTimeout(10s));
    ASSERT_TRUE(seed.success) << seed.error << " status=" << seed.http_status;

    // The real TextProcessor fallback polls the request during tokenization
    // and per-token embedding work, returning no partial vector after stop.
    std::string slow_query;
    slow_query.reserve(5000000);
    for (size_t i = 0; i < 1000000; ++i) {
        slow_query += "slow ";
    }
    const auto search_started = Clock::now();
    const auto search = executor.post(
        shard, "/search/hybrid",
        {{"query", slow_query}, {"k", 1}, {"expand", {{"hops", 0}}}},
        query::RequestContext::withTimeout(5s));
    const auto search_elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - search_started).count();

    ASSERT_FALSE(search.success) << search.data.dump();
    ASSERT_EQ(search.http_status, 504) << search.error;
    EXPECT_EQ(search.data.value("status_code", 0), 504);
    EXPECT_FALSE(search.timed_out);
    EXPECT_FALSE(search.cancelled);

    // With one server worker, a successful follow-up request demonstrates
    // that the synchronous search handler returned and released its worker.
    const auto health = executor.get(
        shard, "/health/live", query::RequestContext::withTimeout(5s));
    ASSERT_TRUE(health.success) << health.error;

    server.stop();
    const auto after_stop = executor.get(
        shard, "/health/live", query::RequestContext::withTimeout(2s));
    EXPECT_FALSE(after_stop.success);
    EXPECT_EQ(after_stop.http_status, 0);
    EXPECT_FALSE(after_stop.timed_out);
    EXPECT_FALSE(after_stop.cancelled);
    EXPECT_FALSE(after_stop.error.empty());

    RecordProperty(
        "qry07_trace",
        "real_https_shard: import=200; slow_search=504(server deadline); "
        "single_worker_health=200; server_stop=joined; followup_after_stop=connection_failure");
    RecordProperty("slow_search_elapsed_ms", std::to_string(search_elapsed));
    EXPECT_GE(search_elapsed, 500);
    EXPECT_LT(search_elapsed, 3000);
    storage->close();
}

TEST_F(MtlsCancellationTest, ClientAbortCancelsSearchAndReleasesHttpWorker) {
    const int port = findAvailablePort();
    ASSERT_GT(port, 0);

    const auto db_path = pki_->dir / "disconnect_http_search_db";
    themis::RocksDBWrapper::Config storage_config;
    storage_config.db_path = db_path.string();
    auto storage = std::make_shared<themis::RocksDBWrapper>(storage_config);
    ASSERT_TRUE(storage->open());

    auto secondary = std::make_shared<themis::SecondaryIndexManager>(*storage);
    auto graph = std::make_shared<themis::GraphIndexManager>(*storage);
    auto vectors = std::make_shared<themis::VectorIndexManager>(*storage);
    auto transactions = std::make_shared<themis::TransactionManager>(
        *storage, *secondary, *graph, *vectors);

    themis::server::HttpServer::Config server_config;
    server_config.host = "127.0.0.1";
    server_config.port = static_cast<std::uint16_t>(port);
    server_config.num_threads = 1;
    server_config.request_timeout_ms = 30000;
    server_config.enable_tls = false;
    server_config.health_error_service_enabled = false;
    themis::server::HttpServer server(
        server_config, storage, secondary, graph, vectors, transactions);
    ASSERT_NO_THROW(server.start());

    net::io_context client_ioc;
    tcp::socket client(client_ioc);
    client.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"),
                                 static_cast<std::uint16_t>(port)));

    std::string slow_query;
    slow_query.reserve(10000000);
    for (size_t i = 0; i < 2000000; ++i) {
        slow_query += "slow ";
    }
    http::request<http::string_body> request{http::verb::post, "/search/hybrid", 11};
    request.set(http::field::host, "127.0.0.1");
    request.set(http::field::content_type, "application/json");
    request.body() = nlohmann::json{
        {"query", slow_query}, {"k", 1}, {"expand", {{"hops", 0}}}}.dump();
    request.prepare_payload();
    ASSERT_NO_THROW(http::write(client, request));
    std::this_thread::sleep_for(100ms);

    linger abortive_close{1, 0};
    ASSERT_EQ(::setsockopt(client.native_handle(), SOL_SOCKET, SO_LINGER,
                           &abortive_close, sizeof(abortive_close)), 0);
    beast::error_code close_error;
    client.close(close_error);
    ASSERT_FALSE(close_error);

    const auto health_started = Clock::now();
    tcp::socket health_socket(client_ioc);
    health_socket.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"),
                                        static_cast<std::uint16_t>(port)));
    http::request<http::empty_body> health_request{
        http::verb::get, "/health/live", 11};
    health_request.set(http::field::host, "127.0.0.1");
    http::write(health_socket, health_request);
    beast::flat_buffer health_buffer;
    http::response<http::string_body> health_response;
    http::read(health_socket, health_buffer, health_response);
    const auto health_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - health_started).count();

    EXPECT_EQ(health_response.result(), http::status::ok);
    EXPECT_LT(health_elapsed, 5000);
    RecordProperty(
        "qry07_disconnect_trace",
        "plain_http_search: request_body_sent; client_abortive_close; "
        "single_worker_health=200; response_to_aborted_client_suppressed");
    RecordProperty("disconnect_health_elapsed_ms", std::to_string(health_elapsed));

    server.stop();
    storage->close();
}

}  // namespace themis::sharding

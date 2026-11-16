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

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include "sharding/mtls_client.h"
#include "sharding/remote_executor.h"

namespace themis::sharding {
namespace {

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

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

    EXPECT_TRUE(resp.timed_out);
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
    EXPECT_FALSE(resp.cancelled);

    auto token = std::make_shared<query::QueryCancellationToken>();
    token->cancel();
    const auto pre = executor.post(shard, "/search/hybrid", {{"q", "x"}},
                                   query::RequestContext(token));
    EXPECT_TRUE(pre.cancelled);
}

}  // namespace themis::sharding

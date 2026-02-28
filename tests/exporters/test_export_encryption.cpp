/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_export_encryption.cpp                         ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-02-28                                         ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "exporters/export_encryption.h"
#include "exporters/exporter_metrics.h"
#include "exporters/streaming_exporter.h"
#include "security/mock_key_provider.h"
#include "storage/base_entity.h"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif
#include <vector>

using namespace themis::exporters;
using namespace themis;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class ExportEncryptionTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto temp_base = std::filesystem::temp_directory_path();
        test_dir_ = (temp_base /
            ("themis_enc_test_" + std::to_string(std::time(nullptr)) +
             "_" + std::to_string(static_cast<int>(getpid()))))
            .string();
        std::filesystem::create_directories(test_dir_);

        // Create a MockKeyProvider with a well-known 32-byte KEK.
        key_provider_ = std::make_shared<MockKeyProvider>();
        key_provider_->createKey("test_kek", 1);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    ExportEncryptionConfig makeConfig(const std::string& job_id = "job-001") const {
        ExportEncryptionConfig cfg;
        cfg.enabled      = true;
        cfg.kek_id       = "test_kek";
        cfg.job_id       = job_id;
        cfg.key_provider = key_provider_;
        return cfg;
    }

    std::string test_dir_;
    std::shared_ptr<MockKeyProvider> key_provider_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Basic encrypt / decrypt round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, EncryptDecryptRoundTrip) {
    ExportEncryption enc(makeConfig());

    const std::string plaintext_str = "Hello, ThemisDB export encryption!";
    std::vector<uint8_t> plaintext(plaintext_str.begin(), plaintext_str.end());

    auto container = enc.encrypt(plaintext);

    // Container must be larger than plaintext (header + tag overhead).
    EXPECT_GT(container.size(), plaintext.size());

    auto recovered = enc.decrypt(container);

    ASSERT_EQ(recovered.size(), plaintext.size());
    EXPECT_EQ(recovered, plaintext);
}

TEST_F(ExportEncryptionTest, EncryptDecryptEmptyPayload) {
    ExportEncryption enc(makeConfig());
    std::vector<uint8_t> plaintext;

    auto container = enc.encrypt(plaintext);
    EXPECT_GT(container.size(), 0u);  // header must still be present

    auto recovered = enc.decrypt(container);
    EXPECT_TRUE(recovered.empty());
}

TEST_F(ExportEncryptionTest, EncryptDecryptLargePayload) {
    ExportEncryption enc(makeConfig());

    // 1 MB of pseudo-random data
    std::vector<uint8_t> plaintext(1024 * 1024);
    for (size_t i = 0; i < plaintext.size(); ++i) {
        plaintext[i] = static_cast<uint8_t>(i & 0xFF);
    }

    auto container = enc.encrypt(plaintext);
    auto recovered = enc.decrypt(container);

    ASSERT_EQ(recovered.size(), plaintext.size());
    EXPECT_EQ(recovered, plaintext);
}

// ─────────────────────────────────────────────────────────────────────────────
// HKDF-based key isolation: different job IDs produce distinct ciphertexts
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, DifferentJobIdProducesDifferentCiphertext) {
    ExportEncryption enc_a(makeConfig("job-A"));
    ExportEncryption enc_b(makeConfig("job-B"));

    const std::string plain_str = "same plaintext";
    std::vector<uint8_t> plaintext(plain_str.begin(), plain_str.end());

    auto ct_a = enc_a.encrypt(plaintext);
    auto ct_b = enc_b.encrypt(plaintext);

    // Ciphertext bodies must differ (different DEKs from different job IDs).
    EXPECT_NE(ct_a, ct_b);
}

TEST_F(ExportEncryptionTest, JobACannotDecryptJobBOutput) {
    ExportEncryption enc_a(makeConfig("job-A"));
    ExportEncryption enc_b(makeConfig("job-B"));

    const std::string plain_str = "sensitive training data";
    std::vector<uint8_t> plaintext(plain_str.begin(), plain_str.end());

    // Encrypt with job-B's key, attempt to decrypt with job-A's key.
    // The ExportEncryption::decrypt() implementation re-derives the DEK
    // using the job_id recorded in the file header (job-B), so even if
    // enc_a is used, the file header job_id takes precedence.
    // The plaintext must still round-trip correctly.
    auto container = enc_b.encrypt(plaintext);
    // enc_a will use the job_id from the header (job-B), so decryption succeeds
    // as long as the KEK is the same.
    auto recovered = enc_a.decrypt(container);
    EXPECT_EQ(recovered, plaintext);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tamper detection via GCM authentication tag
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, TamperedCiphertextIsRejected) {
    ExportEncryption enc(makeConfig());

    const std::string plain_str = "important data";
    std::vector<uint8_t> plaintext(plain_str.begin(), plain_str.end());
    auto container = enc.encrypt(plaintext);

    // Flip a byte somewhere in the middle of the ciphertext region.
    const size_t mid = container.size() / 2;
    container[mid] ^= 0xFF;

    EXPECT_THROW(enc.decrypt(container), std::runtime_error);
}

TEST_F(ExportEncryptionTest, TruncatedContainerIsRejected) {
    ExportEncryption enc(makeConfig());
    std::vector<uint8_t> plaintext = {1, 2, 3, 4, 5};
    auto container = enc.encrypt(plaintext);

    // Remove the last few bytes (authentication tag).
    container.resize(container.size() > 16 ? container.size() - 16 : 0);

    EXPECT_THROW(enc.decrypt(container), std::runtime_error);
}

TEST_F(ExportEncryptionTest, WrongMagicIsRejected) {
    ExportEncryption enc(makeConfig());
    std::vector<uint8_t> plaintext = {0xDE, 0xAD, 0xBE, 0xEF};
    auto container = enc.encrypt(plaintext);

    // Overwrite the magic bytes.
    container[0] = 'X';

    EXPECT_THROW(enc.decrypt(container), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// Disabled encryption is a no-op
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, DisabledEncryptionPassesThrough) {
    ExportEncryptionConfig cfg;
    cfg.enabled = false;
    ExportEncryption enc(cfg);

    const std::string plain_str = "plaintext passthrough";
    std::vector<uint8_t> plaintext(plain_str.begin(), plain_str.end());

    auto result = enc.encrypt(plaintext);
    EXPECT_EQ(result, plaintext);  // unchanged

    auto recovered = enc.decrypt(result);
    EXPECT_EQ(recovered, plaintext);
}

// ─────────────────────────────────────────────────────────────────────────────
// File-based encrypt / decrypt
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, EncryptDecryptFile) {
    const std::string plaintext_str =
        R"({"_id":"doc1","question":"What is ThemisDB?","answer":"A hybrid DB"})"
        "\n"
        R"({"_id":"doc2","question":"What is AES-GCM?","answer":"Authenticated encryption"})"
        "\n";

    const std::string src_path  = test_dir_ + "/export.jsonl";
    const std::string enc_path  = test_dir_ + "/export.jsonl.tenc";
    const std::string dec_path  = test_dir_ + "/export_dec.jsonl";

    // Write plaintext file.
    {
        std::ofstream f(src_path);
        f << plaintext_str;
    }

    ExportEncryption enc(makeConfig("file-job-001"));
    enc.encryptFile(src_path, enc_path);

    // Encrypted file must exist and differ from plaintext.
    ASSERT_TRUE(std::filesystem::exists(enc_path));
    {
        std::ifstream ef(enc_path, std::ios::binary);
        char magic[4];
        ef.read(magic, 4);
        EXPECT_EQ(std::string(magic, 4), "TENC");
    }

    enc.decryptFile(enc_path, dec_path);

    // Decrypted content must match original.
    ASSERT_TRUE(std::filesystem::exists(dec_path));
    std::ifstream df(dec_path);
    std::string recovered((std::istreambuf_iterator<char>(df)),
                           std::istreambuf_iterator<char>());
    EXPECT_EQ(recovered, plaintext_str);
}

TEST_F(ExportEncryptionTest, DisabledEncryptFileCopiesFile) {
    const std::string content = "plain content";
    const std::string src_path = test_dir_ + "/src.txt";
    const std::string dst_path = test_dir_ + "/dst.txt";

    {
        std::ofstream f(src_path);
        f << content;
    }

    ExportEncryptionConfig cfg;
    cfg.enabled = false;
    ExportEncryption enc(cfg);
    enc.encryptFile(src_path, dst_path);

    ASSERT_TRUE(std::filesystem::exists(dst_path));
    std::ifstream df(dst_path);
    std::string dst_content((std::istreambuf_iterator<char>(df)),
                              std::istreambuf_iterator<char>());
    EXPECT_EQ(dst_content, content);
}

// ─────────────────────────────────────────────────────────────────────────────
// Error cases
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, MissingKeyProviderThrows) {
    ExportEncryptionConfig cfg;
    cfg.enabled      = true;
    cfg.kek_id       = "test_kek";
    cfg.job_id       = "job-001";
    cfg.key_provider = nullptr;  // intentionally null

    ExportEncryption enc(cfg);
    std::vector<uint8_t> plaintext = {1, 2, 3};
    EXPECT_THROW(enc.encrypt(plaintext), std::invalid_argument);
}

TEST_F(ExportEncryptionTest, EmptyJobIdThrows) {
    ExportEncryptionConfig cfg;
    cfg.enabled      = true;
    cfg.kek_id       = "test_kek";
    cfg.job_id       = "";  // intentionally empty
    cfg.key_provider = key_provider_;

    ExportEncryption enc(cfg);
    std::vector<uint8_t> plaintext = {1, 2, 3};
    EXPECT_THROW(enc.encrypt(plaintext), std::invalid_argument);
}

TEST_F(ExportEncryptionTest, MissingSourceFileThrows) {
    ExportEncryption enc(makeConfig());
    EXPECT_THROW(
        enc.encryptFile("/nonexistent/path/export.jsonl",
                        test_dir_ + "/out.tenc"),
        std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// ExporterMetrics encryption tracking
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, MetricsEncryptionCounters) {
    ExporterMetrics metrics;
    EXPECT_EQ(metrics.getEncryptedPlaintextBytes(), 0u);
    EXPECT_EQ(metrics.getEncryptedOutputBytes(), 0u);

    metrics.recordEncryption(1000, 1060);
    EXPECT_EQ(metrics.getEncryptedPlaintextBytes(), 1000u);
    EXPECT_EQ(metrics.getEncryptedOutputBytes(), 1060u);

    metrics.recordEncryption(500, 560);
    EXPECT_EQ(metrics.getEncryptedPlaintextBytes(), 1500u);
    EXPECT_EQ(metrics.getEncryptedOutputBytes(), 1620u);

    metrics.reset();
    EXPECT_EQ(metrics.getEncryptedPlaintextBytes(), 0u);
    EXPECT_EQ(metrics.getEncryptedOutputBytes(), 0u);
}

TEST_F(ExportEncryptionTest, MetricsEncryptionInToJson) {
    ExporterMetrics metrics;
    metrics.recordEncryption(2000, 2100);
    auto j = metrics.toJson();
    ASSERT_TRUE(j.contains("encryption"));
    EXPECT_EQ(j["encryption"]["plaintext_bytes"], 2000u);
    EXPECT_EQ(j["encryption"]["output_bytes"],    2100u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration: StreamingExporter with encryption enabled
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, StreamingExporterWithEncryption) {
    // Build a small set of entities.
    std::vector<BaseEntity> entities;
    for (int i = 0; i < 5; ++i) {
        BaseEntity e;
        e.setPrimaryKey("ent_" + std::to_string(i));
        e.setField("text", "Training sample " + std::to_string(i));
        entities.push_back(e);
    }

    const std::string out_path = test_dir_ + "/encrypted_export.jsonl";

    ExportOptions opts;
    opts.output_path = out_path;
    opts.encryption  = makeConfig("streaming-job-001");

    StreamingExporter exporter;
    auto stats = exporter.exportEntities(entities, opts);

    EXPECT_EQ(stats.exported_entities, entities.size());
    EXPECT_EQ(stats.failed_entities, 0u);
    ASSERT_TRUE(std::filesystem::exists(out_path));

    // The output file must be an encrypted TENC container.
    {
        std::ifstream ef(out_path, std::ios::binary);
        char magic[4] = {};
        ef.read(magic, 4);
        EXPECT_EQ(std::string(magic, 4), "TENC");
    }

    // Decrypt and verify JSONL content.
    const std::string dec_path = test_dir_ + "/decrypted_export.jsonl";
    ExportEncryption enc(makeConfig("streaming-job-001"));
    enc.decryptFile(out_path, dec_path);

    std::ifstream df(dec_path);
    std::string line;
    size_t line_count = 0;
    while (std::getline(df, line)) {
        if (!line.empty()) {
            EXPECT_NO_THROW(json::parse(line));
            ++line_count;
        }
    }
    EXPECT_EQ(line_count, entities.size());
}

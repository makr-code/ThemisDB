/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_export_encryption.cpp                         ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-02-28                                         ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "exporters/export_encryption.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_interface.h"
#include "exporters/exporter_metrics.h"
#include "exporters/jsonl_llm_exporter.h"
#include "exporters/streaming_exporter.h"
#include "exporters/incremental_exporter.h"
#include "security/mock_key_provider.h"
#include "storage/base_entity.h"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#ifdef _WIN32
#  include <process.h>
#  define getpid _getpid
#else
#  include <unistd.h>
#endif

using namespace themis::exporters;
using namespace themis;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Helper utilities
// ─────────────────────────────────────────────────────────────────────────────

static std::shared_ptr<MockKeyProvider> makeProvider(const std::string& kek_id) {
    auto provider = std::make_shared<MockKeyProvider>();
    provider->createKey(kek_id, 1);
    return provider;
}

static ExportEncryptionConfig makeConfig(const std::string& kek_id,
                                          std::shared_ptr<KeyProvider> kp,
                                          const std::string& job_id = "test-job-001") {
    ExportEncryptionConfig cfg;
    cfg.kek_id       = kek_id;
    cfg.job_id       = job_id;
    cfg.key_provider = std::move(kp);
    return cfg;
}

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
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    // Write @p content to a file and return the path.
    std::string writePlainFile(const std::string& filename,
                                const std::string& content) {
        const std::string path = test_dir_ + "/" + filename;
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        f.write(content.data(), static_cast<std::streamsize>(content.size()));
        return path;
    }

    // Read the full binary content of @p path.
    std::string readFile(const std::string& path) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f.is_open()) return {};
        const auto sz = static_cast<size_t>(f.tellg());
        f.seekg(0);
        std::string buf(sz, '\0');
        f.read(buf.data(), static_cast<std::streamsize>(sz));
        return buf;
    }

    // Build a minimal set of test entities for exporter integration tests.
    std::vector<BaseEntity> makeEntities(int count = 5) {
        std::vector<BaseEntity> entities;
        for (int i = 0; i < count; ++i) {
            BaseEntity e;
            e.setPrimaryKey("ent_" + std::to_string(i));
            e.setField("question", "Question " + std::to_string(i) + "?");
            e.setField("answer",   "Answer "   + std::to_string(i));
            e.setField("_seq",     static_cast<int64_t>(i + 1));
            entities.push_back(e);
        }
        return entities;
    }

    std::string test_dir_;
};

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptionConfig
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, ConfigEmptyByDefault) {
    ExportEncryptionConfig cfg;
    EXPECT_TRUE(cfg.empty());
    EXPECT_TRUE(cfg.kek_id.empty());
    EXPECT_EQ(cfg.key_provider, nullptr);
}

TEST_F(ExportEncryptionTest, ConfigNotEmptyWhenSet) {
    auto kp = makeProvider("test-kek");
    auto cfg = makeConfig("test-kek", kp);
    EXPECT_FALSE(cfg.empty());
}

TEST_F(ExportEncryptionTest, ConfigEmptyWithoutKeyProvider) {
    ExportEncryptionConfig cfg;
    cfg.kek_id = "test-kek";
    // key_provider is null → still empty
    EXPECT_TRUE(cfg.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor: basic round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, RoundTripSmallFile) {
    const std::string plaintext = "Hello, ThemisDB export encryption!\n";
    const std::string plain_path = writePlainFile("plain.jsonl", plaintext);
    const std::string enc_path   = test_dir_ + "/plain.jsonl.enc";
    const std::string dec_path   = test_dir_ + "/plain.jsonl.dec";

    auto kp  = makeProvider("my-kek");
    auto cfg = makeConfig("my-kek", kp);
    ExportEncryptor encryptor(cfg);

    // Encrypt
    const size_t enc_bytes = encryptor.encryptFile(plain_path, enc_path);
    EXPECT_GT(enc_bytes, plaintext.size())
        << "Encrypted file must be larger than plaintext (header + tag overhead)";

    // Ciphertext must not equal plaintext
    const std::string cipher_content = readFile(enc_path);
    EXPECT_NE(cipher_content, plaintext);

    // The magic must be present at the start
    ASSERT_GE(cipher_content.size(), 4u);
    EXPECT_EQ(cipher_content.substr(0, 4), "TMEX");

    // Decrypt and verify
    const size_t dec_bytes = encryptor.decryptFile(enc_path, dec_path);
    EXPECT_EQ(dec_bytes, plaintext.size());
    EXPECT_EQ(readFile(dec_path), plaintext);
}

TEST_F(ExportEncryptionTest, RoundTripEmptyFile) {
    const std::string plain_path = writePlainFile("empty.jsonl", "");
    const std::string enc_path   = test_dir_ + "/empty.enc";
    const std::string dec_path   = test_dir_ + "/empty.dec";

    auto kp  = makeProvider("kek-empty");
    auto cfg = makeConfig("kek-empty", kp);
    ExportEncryptor encryptor(cfg);

    encryptor.encryptFile(plain_path, enc_path);
    const size_t dec_bytes = encryptor.decryptFile(enc_path, dec_path);
    EXPECT_EQ(dec_bytes, 0u);
    EXPECT_EQ(readFile(dec_path), "");
}

TEST_F(ExportEncryptionTest, RoundTripLargerThanOneChunk) {
    // Generate >64 KiB of data to exercise the streaming path
    std::string big_content;
    big_content.reserve(128 * 1024);
    for (int i = 0; i < 2000; ++i) {
        big_content += "{\"id\":\"" + std::to_string(i) + "\",\"data\":\"" +
                        std::string(32, 'A' + (i % 26)) + "\"}\n";
    }

    const std::string plain_path = writePlainFile("big.jsonl", big_content);
    const std::string enc_path   = test_dir_ + "/big.enc";
    const std::string dec_path   = test_dir_ + "/big.dec";

    auto kp  = makeProvider("kek-big");
    auto cfg = makeConfig("kek-big", kp);
    ExportEncryptor encryptor(cfg);

    encryptor.encryptFile(plain_path, enc_path);
    encryptor.decryptFile(enc_path, dec_path);
    EXPECT_EQ(readFile(dec_path), big_content);
}

TEST_F(ExportEncryptionTest, DifferentJobIdProducesDifferentCiphertext) {
    const std::string plaintext  = "sensitive training data\n";
    const std::string plain_path = writePlainFile("same_plain.jsonl", plaintext);

    auto kp = makeProvider("shared-kek");

    auto cfg1 = makeConfig("shared-kek", kp, "job-001");
    auto cfg2 = makeConfig("shared-kek", kp, "job-002");
    ExportEncryptor enc1(cfg1), enc2(cfg2);

    const std::string enc_path1 = test_dir_ + "/out1.enc";
    const std::string enc_path2 = test_dir_ + "/out2.enc";
    enc1.encryptFile(plain_path, enc_path1);
    enc2.encryptFile(plain_path, enc_path2);

    // Different job IDs → different DEK + different IV → different ciphertext
    EXPECT_NE(readFile(enc_path1), readFile(enc_path2));
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor: authentication tag verification
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, TamperedCiphertextFailsAuthentication) {
    const std::string plaintext  = "must not be recovered if tampered\n";
    const std::string plain_path = writePlainFile("tamper_in.jsonl", plaintext);
    const std::string enc_path   = test_dir_ + "/tamper.enc";
    const std::string dec_path   = test_dir_ + "/tamper.dec";

    auto kp  = makeProvider("kek-tamper");
    auto cfg = makeConfig("kek-tamper", kp);
    ExportEncryptor encryptor(cfg);

    encryptor.encryptFile(plain_path, enc_path);

    // Flip a byte in the middle of the ciphertext
    {
        std::fstream f(enc_path, std::ios::binary | std::ios::in | std::ios::out);
        ASSERT_TRUE(f.is_open());
        f.seekp(0, std::ios::end);
        const auto file_size = static_cast<size_t>(f.tellp());
        // Flip a byte roughly in the middle (past the header)
        const auto flip_pos = static_cast<std::streamoff>(file_size / 2);
        f.seekp(flip_pos);
        char c = 0;
        f.get(c);
        f.seekp(flip_pos);
        f.put(static_cast<char>(c ^ 0xFF));
    }

    // Decryption must fail with a DecryptionException
    EXPECT_THROW(encryptor.decryptFile(enc_path, dec_path), DecryptionException);

    // Partial output must have been cleaned up
    EXPECT_FALSE(std::filesystem::exists(dec_path));
}

TEST_F(ExportEncryptionTest, WrongJobIdFailsAuthentication) {
    const std::string plaintext  = "aad matters for authenticity\n";
    const std::string plain_path = writePlainFile("aad_in.jsonl", plaintext);
    const std::string enc_path   = test_dir_ + "/aad.enc";
    const std::string dec_path   = test_dir_ + "/aad.dec";

    auto kp  = makeProvider("kek-aad");

    // Encrypt with job-A
    {
        auto cfg = makeConfig("kek-aad", kp, "job-A");
        ExportEncryptor encryptor(cfg);
        encryptor.encryptFile(plain_path, enc_path);
    }

    // Attempt decrypt with job-B (different AAD → authentication fails)
    {
        auto cfg = makeConfig("kek-aad", kp, "job-B");
        ExportEncryptor encryptor(cfg);
        EXPECT_THROW(encryptor.decryptFile(enc_path, dec_path), DecryptionException);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor: error conditions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, MissingInputFileThrows) {
    auto kp  = makeProvider("kek-err");
    auto cfg = makeConfig("kek-err", kp);
    ExportEncryptor encryptor(cfg);

    EXPECT_THROW(
        encryptor.encryptFile(test_dir_ + "/nonexistent.jsonl",
                               test_dir_ + "/out.enc"),
        ExportIOException);
}

TEST_F(ExportEncryptionTest, EmptyConfigThrows) {
    ExportEncryptionConfig cfg;  // kek_id empty, no key_provider
    ExportEncryptor encryptor(cfg);

    const std::string plain_path = writePlainFile("x.jsonl", "data");
    EXPECT_THROW(
        encryptor.encryptFile(plain_path, test_dir_ + "/x.enc"),
        EncryptionException);
}

TEST_F(ExportEncryptionTest, NoKeyProviderForDecryptThrows) {
    // Build a valid encrypted file first
    const std::string plain_path = writePlainFile("y.jsonl", "data");
    const std::string enc_path   = test_dir_ + "/y.enc";

    auto kp  = makeProvider("kek-nodec");
    auto cfg = makeConfig("kek-nodec", kp);
    ExportEncryptor encryptor(cfg);
    encryptor.encryptFile(plain_path, enc_path);

    // Attempt decrypt with no key_provider
    ExportEncryptionConfig bad_cfg;
    bad_cfg.kek_id = "kek-nodec";
    // bad_cfg.key_provider remains null
    ExportEncryptor bad_enc(bad_cfg);
    EXPECT_THROW(
        bad_enc.decryptFile(enc_path, test_dir_ + "/y.dec"),
        DecryptionException);
}

TEST_F(ExportEncryptionTest, InvalidMagicThrowsOnDecrypt) {
    // Write a file that does not start with "TMEX"
    const std::string bad_path = writePlainFile("bad.enc", "NOT_ENCRYPTED_DATA");
    auto kp  = makeProvider("kek-magic");
    auto cfg = makeConfig("kek-magic", kp);
    ExportEncryptor encryptor(cfg);

    EXPECT_THROW(
        encryptor.decryptFile(bad_path, test_dir_ + "/bad.dec"),
        DecryptionException);
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor: raw key material must not appear in encrypted output
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, EncryptedFileDoesNotContainKekId_InPlaintext) {
    // The KEK ID is stored in the header, which is fine.
    // Verify that the plaintext content is not visible in the ciphertext.
    const std::string sensitive = "secret training sample: password=hunter2";
    const std::string plain_path = writePlainFile("sensitive.jsonl", sensitive);
    const std::string enc_path   = test_dir_ + "/sensitive.enc";

    auto kp  = makeProvider("kek-leak");
    auto cfg = makeConfig("kek-leak", kp);
    ExportEncryptor encryptor(cfg);

    encryptor.encryptFile(plain_path, enc_path);

    const std::string cipher_content = readFile(enc_path);
    EXPECT_EQ(cipher_content.find("password=hunter2"), std::string::npos)
        << "Sensitive plaintext must not appear verbatim in the encrypted file";
    EXPECT_EQ(cipher_content.find("secret training"), std::string::npos)
        << "Sensitive plaintext must not appear verbatim in the encrypted file";
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration: ExportOptions::encryption_config with JSONLLLMExporter
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, JSONLLLMExporterEncryptsOutput) {
    auto entities = makeEntities(5);
    const std::string out_path = test_dir_ + "/lora.jsonl";

    ExportOptions opts;
    opts.output_path = out_path;
    auto kp  = makeProvider("kek-jsonl");
    opts.encryption_config = makeConfig("kek-jsonl", kp, "jsonl-job");

    JSONLLLMConfig cfg;
    JSONLLLMExporter exporter(cfg);
    const auto stats = exporter.exportEntities(entities, opts);

    EXPECT_GT(stats.exported_entities, 0u);

    // The output file must start with the TMEX magic (i.e., it is encrypted)
    const std::string content = readFile(out_path);
    ASSERT_GE(content.size(), 4u);
    EXPECT_EQ(content.substr(0, 4), "TMEX")
        << "JSONLLLMExporter must encrypt the output file when encryption_config is set";

    // Verify round-trip decryption recovers valid JSONL
    const std::string dec_path = test_dir_ + "/lora_dec.jsonl";
    ExportEncryptor encryptor(*opts.encryption_config);
    encryptor.decryptFile(out_path, dec_path);

    std::ifstream dec_f(dec_path);
    std::string line;
    int line_count = 0;
    while (std::getline(dec_f, line)) {
        if (line.empty()) continue;
        EXPECT_NO_THROW(json::parse(line)) << "Decrypted line is not valid JSON";
        ++line_count;
    }
    EXPECT_GT(line_count, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration: ExportOptions::encryption_config with StreamingExporter
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, StreamingExporterEncryptsOutput) {
    auto entities = makeEntities(8);
    const std::string out_path = test_dir_ + "/streaming.jsonl";

    ExportOptions opts;
    opts.output_path = out_path;
    auto kp  = makeProvider("kek-stream");
    opts.encryption_config = makeConfig("kek-stream", kp, "stream-job");

    StreamingExporter exporter;
    const auto stats = exporter.exportEntities(entities, opts);

    EXPECT_GT(stats.exported_entities, 0u);

    const std::string content = readFile(out_path);
    ASSERT_GE(content.size(), 4u);
    EXPECT_EQ(content.substr(0, 4), "TMEX");

    // Decrypt and check content
    const std::string dec_path = test_dir_ + "/streaming_dec.jsonl";
    ExportEncryptor encryptor(*opts.encryption_config);
    encryptor.decryptFile(out_path, dec_path);

    std::ifstream dec_f(dec_path);
    std::string line;
    int line_count = 0;
    while (std::getline(dec_f, line)) {
        if (line.empty()) continue;
        EXPECT_NO_THROW(json::parse(line));
        ++line_count;
    }
    EXPECT_GT(line_count, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration: ExportOptions::encryption_config with IncrementalExporter
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, IncrementalExporterEncryptsOutput) {
    auto entities = makeEntities(6);
    const std::string out_path = test_dir_ + "/incremental.jsonl";

    ExportOptions opts;
    opts.output_path = out_path;
    auto kp  = makeProvider("kek-incr");
    opts.encryption_config = makeConfig("kek-incr", kp, "incr-job");

    IncrementalExporter exporter;
    const auto stats = exporter.exportEntities(entities, opts);

    EXPECT_GT(stats.exported_entities, 0u);

    const std::string content = readFile(out_path);
    ASSERT_GE(content.size(), 4u);
    EXPECT_EQ(content.substr(0, 4), "TMEX");
}

// ─────────────────────────────────────────────────────────────────────────────
// ExporterMetrics: encryption metrics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, MetricsTrackEncryptedBytes) {
    auto entities = makeEntities(3);
    const std::string out_path = test_dir_ + "/metrics.jsonl";

    ExportOptions opts;
    opts.output_path = out_path;
    auto kp  = makeProvider("kek-metrics");
    opts.encryption_config = makeConfig("kek-metrics", kp, "metrics-job");

    JSONLLLMConfig cfg;
    JSONLLLMExporter exporter(cfg);
    exporter.exportEntities(entities, opts);

    const auto metrics = exporter.getMetrics();
    ASSERT_NE(metrics, nullptr);

    // After an encrypted export the counter must be non-zero
    EXPECT_GT(metrics->getEncryptedBytesWritten(), 0u)
        << "exporter_encrypted_bytes_written_total must be incremented";
}

TEST_F(ExportEncryptionTest, MetricsZeroWithoutEncryption) {
    auto entities = makeEntities(3);
    const std::string out_path = test_dir_ + "/noenc.jsonl";

    ExportOptions opts;
    opts.output_path = out_path;
    // No encryption_config

    JSONLLLMConfig cfg;
    JSONLLLMExporter exporter(cfg);
    exporter.exportEntities(entities, opts);

    const auto metrics = exporter.getMetrics();
    ASSERT_NE(metrics, nullptr);
    EXPECT_EQ(metrics->getEncryptedBytesWritten(), 0u)
        << "Encrypted bytes counter must remain 0 when encryption is not configured";
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportOptions backward compatibility
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExportEncryptionTest, ExportOptionsDefaultHasNoEncryption) {
    ExportOptions opts;
    EXPECT_FALSE(opts.encryption_config.has_value())
        << "encryption_config must be std::nullopt by default for backward compatibility";
}

TEST_F(ExportEncryptionTest, ExportWithNoEncryptionProducesPlaintextJsonl) {
    auto entities = makeEntities(3);
    const std::string out_path = test_dir_ + "/plain_out.jsonl";

    ExportOptions opts;
    opts.output_path = out_path;
    // No encryption

    JSONLLLMConfig cfg;
    JSONLLLMExporter exporter(cfg);
    exporter.exportEntities(entities, opts);

    // File must be plain JSONL — first char must be '{' not 'T'
    const std::string content = readFile(out_path);
    ASSERT_FALSE(content.empty());
    EXPECT_NE(content.substr(0, 4), "TMEX")
        << "Without encryption_config the output must be plain JSONL";
}

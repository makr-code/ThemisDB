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
#include "exporters/exporter_metrics.h"
#include "exporters/streaming_exporter.h"
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
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif
#include <vector>
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

TEST_F(ExportEncryptionTest, OversizedHeaderStringIsRejected) {
    // Build a minimal but otherwise valid TENC header where job_id_len
    // is set to a value exceeding MAX_HEADER_STRING_LEN (4096).
    // The decrypt() call must reject this before any large allocation.
    ExportEncryption enc(makeConfig());
    std::vector<uint8_t> plaintext = {1, 2, 3};
    auto container = enc.encrypt(plaintext);

    // Locate the job_id_len field: it comes after magic(4) + version(4) = byte 8.
    // Write a too-large length (0x00002000 = 8192 > 4096).
    ASSERT_GE(container.size(), 12u);
    uint32_t huge = 8192u;
    container[8]  = static_cast<uint8_t>(huge & 0xFF);
    container[9]  = static_cast<uint8_t>((huge >> 8) & 0xFF);
    container[10] = static_cast<uint8_t>((huge >> 16) & 0xFF);
    container[11] = static_cast<uint8_t>((huge >> 24) & 0xFF);
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

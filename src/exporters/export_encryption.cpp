/**
 * @file export_encryption.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity
 * metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0,
 * C=4, H=16, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "exporters/export_encryption.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <stdexcept>

#include "exporters/exporter_errors.h"
#include "utils/hkdf_helper.h"
#include "utils/logger.h"

namespace themis::exporters {

// ─────────────────────────────────────────────────────────────────────────────
// File-format constants
// ─────────────────────────────────────────────────────────────────────────────

static constexpr uint8_t MAGIC[4] = {'T', 'E', 'N', 'C'};
static constexpr uint32_t FORMAT_VER = 1;
static constexpr size_t IV_LEN = 12;  // AES-GCM recommended nonce size
static constexpr size_t TAG_LEN = 16; // Full GCM authentication tag
static constexpr size_t KEY_LEN = 32; // AES-256

// ─────────────────────────────────────────────────────────────────────────────
// Little-endian I/O helpers
// ─────────────────────────────────────────────────────────────────────────────

static void writeU32(std::vector<uint8_t> &buf, uint32_t v) {
  buf.push_back(static_cast<uint8_t>(v & 0xFFu));
  buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
  buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
  buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
}

static void writeU64(std::vector<uint8_t> &buf, uint64_t v) {
  for (int i = 0; i < 8; ++i) {
    buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFFu));
  }
}

static void writeBytes(std::vector<uint8_t> &buf, const uint8_t *data,
                       size_t len) {
  buf.insert(buf.end(), data, data + len);
}

static void writeString(std::vector<uint8_t> &buf, const std::string &s) {
  writeU32(buf, static_cast<uint32_t>(s.size()));
  writeBytes(buf, reinterpret_cast<const uint8_t *>(s.data()),static_cast<int>(s.size()));
}

static uint32_t readU32(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

static uint64_t readU64(const uint8_t *p) {
  uint64_t v = 0;
  for (int i = 0; i < 8; ++i) {
    v |= (static_cast<uint64_t>(p[i]) << (i * 8));
  }
  return v;
}

// Maximum length accepted for job_id and kek_id strings in the TENC header.
// Prevents allocation of enormous strings from malformed or malicious files.
static constexpr uint32_t MAX_HEADER_STRING_LEN = 4096;

static std::string readString(const uint8_t *buf, size_t buf_size,
                              size_t &offset) {
  if (offset + 4 > buf_size) {
    throw std::runtime_error("ExportEncryption: truncated string length field");
  }
  uint32_t len = readU32(buf + offset);
  offset += 4;
  if (len > MAX_HEADER_STRING_LEN) {
    throw std::runtime_error(
        "ExportEncryption: header string exceeds maximum length (" +
        std::to_string(len) + " > " + std::to_string(MAX_HEADER_STRING_LEN) +
        ")");
  }
  if (offset + len > buf_size) {
    throw std::runtime_error("ExportEncryption: truncated string data");
  }
  std::string s(reinterpret_cast<const char *>(buf + offset), len);
  offset += len;
  return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryption
// ─────────────────────────────────────────────────────────────────────────────

ExportEncryption::ExportEncryption(const ExportEncryptionConfig &config)
    : config_(config) {}

// Build the AAD buffer authenticated by the GCM tag.
// Covers job_id, kek_id, key_version, and IV so that any header
// tampering is detected at decryption time.
/*static*/
std::vector<uint8_t>
ExportEncryption::buildAAD(const std::string &job_id, const std::string &kek_id,
                           uint32_t key_version,
                           const std::vector<uint8_t> &iv) {
  std::vector<uint8_t> aad = {};

  aad.reserve(8 + static_cast<int>(job_id.size()) + 4 + static_cast<int>(kek_id.size()) + 4 + IV_LEN);
  writeString(aad, job_id);
  writeString(aad, kek_id);
  writeU32(aad, key_version);
  writeBytes(aad, iv.data(),static_cast<int>(iv.size()));
  return aad;
}

// Derive a 32-byte per-job DEK using HKDF-SHA256.
// The IKM is the raw KEK bytes from the KeyProvider; the info string
// is the export job_id so each job produces a distinct key.
std::vector<uint8_t>
ExportEncryption::deriveJobDEK(uint32_t key_version) const {
  if (!config_.key_provider) {
    throw std::invalid_argument("ExportEncryption: key_provider is null");
  }
  if (config_.kek_id.empty()) {
    throw std::invalid_argument("ExportEncryption: kek_id is empty");
  }

  // Fetch KEK – referenced by ID only; raw bytes are never logged.
  // Serialise concurrent callers at the KEK-fetch boundary; key material
  // is not copied outside the lock scope.
  std::vector<uint8_t> kek;
  {
    std::lock_guard<std::mutex> lk(key_provider_mutex_);
    kek = config_.key_provider->getKey(config_.kek_id, key_version);
  }
  if (static_cast<int>(kek.size()) != KEY_LEN) {
    throw std::runtime_error(
        "ExportEncryption: KEK must be 32 bytes (AES-256)");
  }

  // HKDF-SHA256: IKM = KEK, info = job_id, no explicit salt
  std::vector<uint8_t> dek = themis::utils::HKDFHelper::derive(
      kek, {} /*salt*/, config_.job_id, KEY_LEN);

  // Securely zero the KEK copy held on the stack once DEK is derived.
  OPENSSL_cleanse(kek.data(),static_cast<int>(kek.size()));

  return dek;
}

std::vector<uint8_t>
ExportEncryption::encrypt(const std::vector<uint8_t> &plaintext) const {
  // FIXED: Protect ALL config_ reads with mutex to prevent data races
  bool enabled = {};
  std::string kek_id = {};
  std::string job_id = {};
  std::shared_ptr<themis::KeyProvider> key_provider;

  {
    std::lock_guard<std::mutex> lk(key_provider_mutex_);
    enabled = config_.enabled;
    kek_id = config_.kek_id;
    job_id = config_.job_id;
    key_provider = config_.key_provider;
  }

  if (!enabled) {
    return plaintext;
  }
  if (kek_id.empty() || !key_provider) {
    throw std::invalid_argument(
        "ExportEncryption: encryption enabled but kek_id/key_provider missing");
  }
  if (job_id.empty()) {
    throw std::invalid_argument("ExportEncryption: job_id must not be empty");
  }

  // Fetch key metadata to record the version we used.
  // Serialise concurrent callers at the KEK-metadata boundary.
  uint32_t key_version = 0;
  {
    std::lock_guard<std::mutex> lk(key_provider_mutex_);
    key_version = key_provider->getKeyMetadata(kek_id).version;
  }

  // Create a temporary config with captured values for deriveJobDEK
  ExportEncryptionConfig temp_cfg;
  temp_cfg.enabled = true;
  temp_cfg.kek_id = kek_id;
  temp_cfg.job_id = job_id;
  temp_cfg.key_provider = key_provider;
  ExportEncryption temp_helper(temp_cfg);
  auto dek = temp_helper.deriveJobDEK(key_version);

  // Generate a random 12-byte IV.
  std::vector<uint8_t> iv(IV_LEN);
  if (RAND_bytes(iv.data(), static_cast<int>(IV_LEN)) != 1) {
    OPENSSL_cleanse(dek.data(),static_cast<int>(dek.size()));
    throw std::runtime_error("ExportEncryption: failed to generate random IV");
  }

  // Build AAD.
  auto aad = buildAAD(job_id, kek_id, key_version, iv);

  // AES-256-GCM encrypt.
  std::vector<uint8_t> ciphertext(plaintext.size());
  std::vector<uint8_t> tag(TAG_LEN);

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    OPENSSL_cleanse(dek.data(),static_cast<int>(dek.size()));
    throw std::runtime_error(
        "ExportEncryption: failed to create cipher context");
  }

  try {
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) !=
        1) {
      throw std::runtime_error("ExportEncryption: EncryptInit failed");
    }
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN,
                            static_cast<int>(IV_LEN), nullptr) != 1) {
      throw std::runtime_error("ExportEncryption: set IV length failed");
    }
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, dek.data(), iv.data()) != 1) {
      throw std::runtime_error("ExportEncryption: set key/IV failed");
    }

    // Feed AAD.
    int out_len = 0;
    if (!aad.empty() && EVP_EncryptUpdate(ctx, nullptr, &out_len, aad.data(),
                                          static_cast<int>(aad.size())) != 1) {
      throw std::runtime_error("ExportEncryption: AAD feed failed");
    }

    // Encrypt plaintext.
    if (!plaintext.empty()) {
      // Guard against truncation in the OpenSSL int API
      if (static_cast<int>(plaintext.size()) > static_cast<size_t>(INT_MAX)) {
        throw std::runtime_error("ExportEncryption: plaintext exceeds maximum "
                                 "supported size (INT_MAX)");
      }
      if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len, plaintext.data(),
                            static_cast<int>(plaintext.size())) != 1) {
        throw std::runtime_error("ExportEncryption: EncryptUpdate failed");
      }
    }
    int final_len = 0;
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len, &final_len) !=
        1) {
      throw std::runtime_error("ExportEncryption: EncryptFinal failed");
    }
    // For GCM with no padding, final_len is always 0.

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG,
                            static_cast<int>(TAG_LEN), tag.data()) != 1) {
      throw std::runtime_error("ExportEncryption: get tag failed");
    }
  } catch (...) {
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(dek.data(),static_cast<int>(dek.size()));
    throw;
  }
  EVP_CIPHER_CTX_free(ctx);
  OPENSSL_cleanse(dek.data(),static_cast<int>(dek.size()));

  // Assemble the container:
  //   [magic][format_ver][job_id][kek_id][key_version][iv][ct_len][ct][tag]
  std::vector<uint8_t> container = {};

  container.reserve(4 + 4 + 4 + static_cast<int>(job_id.size()) + 4 + static_cast<int>(kek_id.size()) + 4 + IV_LEN +
                    8 + static_cast<int>(plaintext.size()) + TAG_LEN);

  writeBytes(container, MAGIC, 4);
  writeU32(container, FORMAT_VER);
  writeString(container, job_id);
  writeString(container, kek_id);
  writeU32(container, key_version);
  writeBytes(container, iv.data(), IV_LEN);
  writeU64(container, static_cast<uint64_t>(ciphertext.size()));
  writeBytes(container, ciphertext.data(),static_cast<int>(ciphertext.size()));
  writeBytes(container, tag.data(), TAG_LEN);

  THEMIS_INFO("ExportEncryption: encrypted {} bytes -> {} bytes "
              "(job_id={}, kek_id={}, key_ver={})",
              plaintext.size(),static_cast<int>(container.size()), job_id, kek_id, key_version);

  return container;
}

std::vector<uint8_t>
ExportEncryption::decrypt(const std::vector<uint8_t> &container) const {
  // FIXED: Protect config_.enabled read with mutex
  bool enabled = {};
  std::shared_ptr<themis::KeyProvider> key_provider;
  {
    std::lock_guard<std::mutex> lk(key_provider_mutex_);
    enabled = config_.enabled;
    key_provider = config_.key_provider;
  }

  if (!enabled) {
    return container;
  }

  // ── Parse header ──────────────────────────────────────────────────────
  size_t offset = 0;
  const size_t total = container.size();
  const uint8_t *buf = container.data();

  // Magic
  if (total < 4 || std::memcmp(buf, MAGIC, 4) != 0) {
    throw std::runtime_error(
        "ExportEncryption: invalid magic; not a TENC file");
  }
  offset += 4;

  // Format version
  if (offset + 4 > total) {
    throw std::runtime_error("ExportEncryption: truncated header (version)");
  }
  const uint32_t ver = readU32(buf + offset);
  offset += 4;
  if (ver != FORMAT_VER) {
    throw std::runtime_error(
        "ExportEncryption: unsupported file format version " +
        std::to_string(ver));
  }

  // job_id
  std::string file_job_id = readString(buf, total, offset);

  // kek_id
  std::string file_kek_id = readString(buf, total, offset);

  // key_version
  if (offset + 4 > total) {
    throw std::runtime_error(
        "ExportEncryption: truncated header (key_version)");
  }
  const uint32_t key_version = readU32(buf + offset);
  offset += 4;

  // IV
  if (offset + IV_LEN > total) {
    throw std::runtime_error("ExportEncryption: truncated header (iv)");
  }
  std::vector<uint8_t> iv(buf + offset, buf + offset + IV_LEN);
  offset += IV_LEN;

  // ciphertext length
  if (offset + 8 > total) {
    throw std::runtime_error("ExportEncryption: truncated header (ct_len)");
  }
  const uint64_t ct_len = readU64(buf + offset);
  offset += 8;

  // ciphertext
  if (offset + ct_len > total) {
    throw std::runtime_error("ExportEncryption: truncated ciphertext");
  }
  const uint8_t *ct_ptr = buf + offset;
  offset += ct_len;

  // tag
  if (offset + TAG_LEN > total) {
    throw std::runtime_error("ExportEncryption: truncated authentication tag");
  }
  std::vector<uint8_t> tag(buf + offset, buf + offset + TAG_LEN);

  // ── Derive DEK ────────────────────────────────────────────────────────
  // Temporarily override the config job_id / kek_id with values read
  // from the file header so that the DEK derivation matches encryption.
  if (!key_provider) {
    throw std::invalid_argument("ExportEncryption: key_provider is null");
  }
  if (file_kek_id.empty()) {
    throw std::runtime_error("ExportEncryption: file header has empty kek_id");
  }

  // Use the job_id from the file; fall back to config_.job_id if the
  // header job_id matches (normal case) or the caller deliberately sets
  // it. Build a temporary config just for derivation.
  ExportEncryptionConfig dec_cfg;
  dec_cfg.enabled = true;
  dec_cfg.kek_id = file_kek_id;
  dec_cfg.job_id = file_job_id;
  dec_cfg.key_provider = key_provider;
  ExportEncryption dec_helper(dec_cfg);
  auto dek = dec_helper.deriveJobDEK(key_version);

  // ── Build AAD ─────────────────────────────────────────────────────────
  auto aad = buildAAD(file_job_id, file_kek_id, key_version, iv);

  // ── AES-256-GCM decrypt ───────────────────────────────────────────────
  std::vector<uint8_t> plaintext(static_cast<size_t>(ct_len));

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    OPENSSL_cleanse(dek.data(),static_cast<int>(dek.size()));
    throw std::runtime_error(
        "ExportEncryption: failed to create cipher context");
  }

  try {
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) !=
        1) {
      throw std::runtime_error("ExportEncryption: DecryptInit failed");
    }
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN,
                            static_cast<int>(IV_LEN), nullptr) != 1) {
      throw std::runtime_error("ExportEncryption: set IV length failed");
    }
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, dek.data(), iv.data()) != 1) {
      throw std::runtime_error("ExportEncryption: set key/IV failed");
    }

    // Feed AAD.
    int out_len = 0;
    if (!aad.empty() && EVP_DecryptUpdate(ctx, nullptr, &out_len, aad.data(),
                                          static_cast<int>(aad.size())) != 1) {
      throw std::runtime_error("ExportEncryption: AAD feed failed");
    }

    // Decrypt ciphertext.
    if (ct_len > 0) {
      // Guard against truncation in the OpenSSL int API
      if (ct_len > static_cast<uint64_t>(INT_MAX)) {
        throw std::runtime_error("ExportEncryption: ciphertext exceeds maximum "
                                 "supported size (INT_MAX)");
      }
      if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len, ct_ptr,
                            static_cast<int>(ct_len)) != 1) {
        throw std::runtime_error("ExportEncryption: DecryptUpdate failed");
      }
    }

    // Set expected tag before finalising.
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG,
                            static_cast<int>(TAG_LEN),
                            const_cast<uint8_t *>(tag.data())) != 1) {
      throw std::runtime_error("ExportEncryption: set tag failed");
    }

    // Verify tag and finalise.
    int final_len = 0;
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + out_len, &final_len) <= 0) {
      throw std::runtime_error(
          "ExportEncryption: authentication tag mismatch – "
          "file may be corrupted or tampered with");
    }
  } catch (...) {
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(dek.data(),static_cast<int>(dek.size()));
    throw;
  }
  EVP_CIPHER_CTX_free(ctx);
  OPENSSL_cleanse(dek.data(),static_cast<int>(dek.size()));

  THEMIS_INFO("ExportEncryption: decrypted {} bytes -> {} bytes "
              "(job_id={}, kek_id={}, key_ver={})",
              container.size(),static_cast<int>(plaintext.size()), file_job_id, file_kek_id,
              key_version);

  return plaintext;
}

void ExportEncryption::encryptFile(const std::string &src_path,
                                   const std::string &dst_path) const {
  // FIXED: Protect config_.enabled read with mutex
  bool enabled = {};
  {
    std::lock_guard<std::mutex> lk(key_provider_mutex_);
    enabled = config_.enabled;
  }

  if (!enabled) {
    // Copy the file as-is when encryption is disabled.
    if (src_path != dst_path) {
      std::ifstream src(src_path, std::ios::binary);
      if (!src.is_open()) {
        throw std::runtime_error("ExportEncryption: cannot open source file: " +
                                 src_path);
      }
      std::ofstream dst(dst_path, std::ios::binary | std::ios::trunc);
      if (!dst.is_open()) {
        throw std::runtime_error(
            "ExportEncryption: cannot open destination file: " + dst_path);
      }
      dst << src.rdbuf();
    }
    return;
  }

  // Read plaintext.
  std::ifstream src(src_path, std::ios::binary | std::ios::ate);
  if (!src.is_open()) {
    throw std::runtime_error("ExportEncryption: cannot open source file: " +
                             src_path);
  }
  const auto file_size = static_cast<size_t>(src.tellg());
  src.seekg(0);
  std::vector<uint8_t> plaintext(file_size);
  if (file_size > 0 && !src.read(reinterpret_cast<char *>(plaintext.data()),
                                 static_cast<std::streamsize>(file_size))) {
    throw std::runtime_error("ExportEncryption: failed to read source file: " +
                             src_path);
  }
  // src closes via RAII when it goes out of scope.

  // Encrypt.
  auto container = encrypt(plaintext);

  // Securely zero the plaintext buffer now that encryption is done.
  OPENSSL_cleanse(plaintext.data(),static_cast<int>(plaintext.size()));

  // Write encrypted container.
  std::ofstream dst(dst_path, std::ios::binary | std::ios::trunc);
  if (!dst.is_open()) {
    throw std::runtime_error(
        "ExportEncryption: cannot open destination file: " + dst_path);
  }
  if (!container.empty() &&
      !dst.write(reinterpret_cast<const char *>(container.data()),
                 static_cast<std::streamsize>(container.size()))) {
    throw std::runtime_error(
        "ExportEncryption: failed to write encrypted file: " + dst_path);
  }
}

void ExportEncryption::decryptFile(const std::string &src_path,
                                   const std::string &dst_path) const {
  // FIXED: Protect config_.enabled read with mutex
  bool enabled = {};
  {
    std::lock_guard<std::mutex> lk(key_provider_mutex_);
    enabled = config_.enabled;
  }

  if (!enabled) {
    if (src_path != dst_path) {
      std::ifstream src(src_path, std::ios::binary);
      if (!src.is_open()) {
        throw std::runtime_error("ExportEncryption: cannot open source file: " +
                                 src_path);
      }
      std::ofstream dst(dst_path, std::ios::binary | std::ios::trunc);
      if (!dst.is_open()) {
        throw std::runtime_error(
            "ExportEncryption: cannot open destination file: " + dst_path);
      }
      dst << src.rdbuf();
    }
    return;
  }

  // Read encrypted container.
  std::ifstream src(src_path, std::ios::binary | std::ios::ate);
  if (!src.is_open()) {
    throw std::runtime_error("ExportEncryption: cannot open source file: " +
                             src_path);
  }
  const auto file_size = static_cast<size_t>(src.tellg());
  src.seekg(0);
  std::vector<uint8_t> container(file_size);
  if (file_size > 0 && !src.read(reinterpret_cast<char *>(container.data()),
                                 static_cast<std::streamsize>(file_size))) {
    throw std::runtime_error("ExportEncryption: failed to read source file: " +
                             src_path);
  }
  // src closes via RAII when it goes out of scope.

  // Decrypt.
  auto plaintext = decrypt(container);

  // Write plaintext.
  std::ofstream dst(dst_path, std::ios::binary | std::ios::trunc);
  if (!dst.is_open()) {
    OPENSSL_cleanse(plaintext.data(),static_cast<int>(plaintext.size()));
    throw std::runtime_error(
        "ExportEncryption: cannot open destination file: " + dst_path);
  }
  if (!plaintext.empty() &&
      !dst.write(reinterpret_cast<const char *>(plaintext.data()),
                 static_cast<std::streamsize>(plaintext.size()))) {
    OPENSSL_cleanse(plaintext.data(),static_cast<int>(plaintext.size()));
    throw std::runtime_error(
        "ExportEncryption: failed to write decrypted file: " + dst_path);
  }

  // Securely zero the decrypted buffer after the file has been written.
  OPENSSL_cleanse(plaintext.data(),static_cast<int>(plaintext.size()));
}

// Helper: little-endian binary I/O
// ─────────────────────────────────────────────────────────────────────────────

static void writeU8(std::ostream &out, uint8_t v) {
  out.put(static_cast<char>(v));
}

static void writeU16LE(std::ostream &out, uint16_t v) {
  out.put(static_cast<char>(v & 0xFFU));
  out.put(static_cast<char>((v >> 8) & 0xFFU));
}

static void writeU32LE(std::ostream &out, uint32_t v) {
  out.put(static_cast<char>(v & 0xFFU));
  out.put(static_cast<char>((v >> 8) & 0xFFU));
  out.put(static_cast<char>((v >> 16) & 0xFFU));
  out.put(static_cast<char>((v >> 24) & 0xFFU));
}

static bool readU8(std::istream &in, uint8_t &v) {
  int c = in.get();
  if (c == EOF) {
    return false;
  }
  v = static_cast<uint8_t>(c);
  return true;
}

static bool readU16LE(std::istream &in, uint16_t &v) {
  int lo = in.get(), hi = in.get();
  if (lo == EOF || hi == EOF) {
    return false;
  }
  v = static_cast<uint16_t>(
      static_cast<uint8_t>(lo) |
      (static_cast<uint16_t>(static_cast<uint8_t>(hi)) << 8));
  return true;
}

static bool readU32LE(std::istream &in, uint32_t &v) {
  uint8_t b[4];
  for (int i = 0; i < 4; ++i) {
    int c = in.get();
    if (c == EOF) {
      return false;
    }
    b[i] = static_cast<uint8_t>(c);
  }
  v = static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8) |
      (static_cast<uint32_t>(b[2]) << 16) |
      (static_cast<uint32_t>(b[3]) << 24);
  return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor: construction
// ─────────────────────────────────────────────────────────────────────────────

ExportEncryptor::ExportEncryptor(const ExportEncryptionConfig &config)
    : config_(config) {}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor: private helpers
// ─────────────────────────────────────────────────────────────────────────────

std::vector<uint8_t>
ExportEncryptor::deriveDataKey(const std::vector<uint8_t> &kek,
                               const std::string &job_id) {
  const std::string info = "themis-export-dek:" + job_id;
  return themis::utils::HKDFHelper::derive(kek, {}, info, 32);
}

std::string ExportEncryptor::generateJobId() {
  uint8_t buf[8];
  if (RAND_bytes(buf, static_cast<int>(sizeof(buf))) != 1) {
    throw EncryptionException("Failed to generate random job ID");
  }
  std::ostringstream oss = {};
  oss << std::hex << std::setfill('0');
  for (uint8_t b : buf) {
    oss << std::setw(2) << static_cast<int>(b);
  }
  return oss.str();
}

size_t ExportEncryptor::writeHeader(std::ostream &out,
                                    const std::string &kek_id,
                                    uint32_t kek_version,
                                    const std::string &job_id,
                                    const std::vector<uint8_t> &iv) {
  size_t bytes = 0;

  // magic (4 bytes)
  out.write(kMagic, 4);
  bytes += 4;

  // version (1 byte)
  writeU8(out, kFormatVersion);
  bytes += 1;

  // kek_id: length (2 bytes) + bytes
  const auto kek_id_len = static_cast<uint16_t>(kek_id.size());
  writeU16LE(out, kek_id_len);
  out.write(kek_id.data(), kek_id_len);
  bytes += 2 + kek_id_len;

  // kek_version (4 bytes)
  writeU32LE(out, kek_version);
  bytes += 4;

  // job_id: length (2 bytes) + bytes
  const auto job_id_len = static_cast<uint16_t>(job_id.size());
  writeU16LE(out, job_id_len);
  out.write(job_id.data(), job_id_len);
  bytes += 2 + job_id_len;

  // IV (12 bytes)
  out.write(reinterpret_cast<const char *>(iv.data()), 12);
  bytes += 12;

  return bytes;
}

bool ExportEncryptor::readHeader(std::istream &in, std::string &kek_id,
                                 uint32_t &kek_version, std::string &job_id,
                                 std::vector<uint8_t> &iv) {
  // magic
  char magic[4];
  in.read(magic, 4);
  if (in.gcount() != 4) {
    return false;
  }
  if (std::memcmp(magic, kMagic, 4) != 0) {
    return false;
  }

  // version
  uint8_t version = 0;
  if (!readU8(in, version)) {
    return false;
  }
  if (version != kFormatVersion) {
    return false;
  }

  // kek_id
  uint16_t kek_id_len = 0;
  if (!readU16LE(in, kek_id_len)) {
    return false;
  }
  kek_id.resize(kek_id_len);
  if (kek_id_len > 0) {
    in.read(kek_id.data(), kek_id_len);
    if (in.gcount() != kek_id_len) {
      return false;
    }
  }

  // kek_version
  if (!readU32LE(in, kek_version)) {
    return false;
  }

  // job_id
  uint16_t job_id_len = 0;
  if (!readU16LE(in, job_id_len)) {
    return false;
  }
  job_id.resize(job_id_len);
  if (job_id_len > 0) {
    in.read(job_id.data(), job_id_len);
    if (in.gcount() != job_id_len) {
      return false;
    }
  }

  // IV (12 bytes)
  iv.resize(12);
  in.read(reinterpret_cast<char *>(iv.data()), 12);
  if (in.gcount() != 12)
    return false = {};

  return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor::encryptFile
// ─────────────────────────────────────────────────────────────────────────────

size_t ExportEncryptor::encryptFile(const std::string &input_path,
                                    const std::string &output_path) const {
  if (config_.empty()) {
    throw EncryptionException(
        "Encryption not configured: kek_id or key_provider is missing");
  }

  // ── 1. Retrieve KEK ───────────────────────────────────────────────────
  // Serialise concurrent callers at the KEK-fetch boundary; key material
  // is not copied outside the lock scope.
  std::vector<uint8_t> kek;
  uint32_t kek_version = 0;
  try {
    std::lock_guard<std::mutex> lk(key_provider_mutex_);
    kek = config_.key_provider->getKey(config_.kek_id);
    kek_version = config_.key_provider->getKeyMetadata(config_.kek_id).version;
  } catch (const std::exception &e) {
    throw EncryptionException(std::string("Failed to retrieve KEK '") +
                              config_.kek_id + "': " + e.what());
  }

  // ── 2. Determine job ID ───────────────────────────────────────────────
  const std::string effective_job_id =
      config_.job_id.empty() ? generateJobId() : config_.job_id;

  // ── 3. Derive DEK from KEK using HKDF-SHA256 ─────────────────────────
  std::vector<uint8_t> dek;
  try {
    dek = deriveDataKey(kek, effective_job_id);
  } catch (...) {
    std::fill(kek.begin(), kek.end(), uint8_t{0});
    throw;
  }
  // Clear KEK immediately after DEK derivation — raw key never persists
  std::fill(kek.begin(), kek.end(), uint8_t{0});

  THEMIS_INFO("ExportEncryptor: encrypting '{}' with kek_id='{}' job_id='{}'",
              input_path, config_.kek_id, effective_job_id);

  // ── 4. Generate random IV ─────────────────────────────────────────────
  std::vector<uint8_t> iv(12);
  if (RAND_bytes(iv.data(), 12) != 1) {
    std::fill(dek.begin(), dek.end(), uint8_t{0});
    throw EncryptionException(
        "Failed to generate random IV for export encryption");
  }

  // ── 5. Open I/O streams ───────────────────────────────────────────────
  std::ifstream in_f(input_path, std::ios::binary);
  if (!in_f.is_open()) {
    std::fill(dek.begin(), dek.end(), uint8_t{0});
    throw ExportIOException("Cannot open input file for encryption",
                            input_path);
  }

  std::ofstream out_f(output_path, std::ios::binary | std::ios::trunc);
  if (!out_f.is_open()) {
    std::fill(dek.begin(), dek.end(), uint8_t{0});
    throw ExportIOException("Cannot create output file for encryption",
                            output_path);
  }

  // ── 6. Write header ───────────────────────────────────────────────────
  size_t total_bytes =
      writeHeader(out_f, config_.kek_id, kek_version, effective_job_id, iv);

  // ── 7. Initialise AES-256-GCM context ─────────────────────────────────
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    std::fill(dek.begin(), dek.end(), uint8_t{0});
    throw EncryptionException("Failed to allocate EVP cipher context");
  }

  auto cleanup_ctx = [&]() {
    EVP_CIPHER_CTX_free(ctx);
    std::fill(dek.begin(), dek.end(), uint8_t{0});
  };

  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) !=
      1) {
    cleanup_ctx();
    throw EncryptionException("EVP_EncryptInit_ex (AES-256-GCM) failed");
  }
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
    cleanup_ctx();
    throw EncryptionException("EVP_CTRL_GCM_SET_IVLEN failed");
  }
  if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, dek.data(), iv.data()) != 1) {
    cleanup_ctx();
    throw EncryptionException("EVP_EncryptInit_ex (key/IV) failed");
  }

  // ── 8. Feed job_id as AAD ─────────────────────────────────────────────
  {
    int aad_out = 0;
    const auto *aad_ptr =
        reinterpret_cast<const unsigned char *>(effective_job_id.data());
    if (EVP_EncryptUpdate(ctx, nullptr, &aad_out, aad_ptr,
                          static_cast<int>(effective_job_id.size())) != 1) {
      cleanup_ctx();
      throw EncryptionException("Failed to set GCM AAD (job_id)");
    }
  }

  // ── 9. Stream-encrypt input ───────────────────────────────────────────
  std::vector<unsigned char> plain_buf(kChunkSize);
  std::vector<unsigned char> cipher_buf(kChunkSize + 16);

  while (in_f.good()) {
    in_f.read(reinterpret_cast<char *>(plain_buf.data()),
              static_cast<std::streamsize>(kChunkSize));
    const std::streamsize bytes_read = in_f.gcount();
    if (bytes_read == 0) {
      break;
    }

    int ct_len = 0;
    if (EVP_EncryptUpdate(ctx, cipher_buf.data(), &ct_len, plain_buf.data(),
                          static_cast<int>(bytes_read)) != 1) {
      cleanup_ctx();
      throw EncryptionException("EVP_EncryptUpdate failed during streaming");
    }
    if (ct_len > 0) {
      out_f.write(reinterpret_cast<const char *>(cipher_buf.data()), ct_len);
      total_bytes += static_cast<size_t>(ct_len);
    }
  }

  // ── 10. Finalise and write GCM tag ────────────────────────────────────
  int final_len = 0;
  if (EVP_EncryptFinal_ex(ctx, cipher_buf.data(), &final_len) != 1) {
    cleanup_ctx();
    throw EncryptionException("EVP_EncryptFinal_ex failed");
  }
  if (final_len > 0) {
    out_f.write(reinterpret_cast<const char *>(cipher_buf.data()), final_len);
    total_bytes += static_cast<size_t>(final_len);
  }

  uint8_t tag[16];
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
    cleanup_ctx();
    throw EncryptionException("Failed to retrieve GCM authentication tag");
  }
  out_f.write(reinterpret_cast<const char *>(tag), 16);
  total_bytes += 16;

  EVP_CIPHER_CTX_free(ctx);
  std::fill(dek.begin(), dek.end(), uint8_t{0});

  if (!out_f.good()) {
    throw ExportIOException("Write error while finalising encrypted file",
                            output_path);
  }
  out_f.close();
  in_f.close();

  THEMIS_INFO("ExportEncryptor: encrypted {} bytes -> '{}'", total_bytes,
              output_path);
  return total_bytes;
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportEncryptor::decryptFile
// ─────────────────────────────────────────────────────────────────────────────

size_t ExportEncryptor::decryptFile(const std::string &input_path,
                                    const std::string &output_path) const {
  if (!config_.key_provider) {
    throw DecryptionException(
        "No key_provider configured for export decryption");
  }

  // ── 1. Open encrypted input ───────────────────────────────────────────
  std::ifstream in_f(input_path, std::ios::binary | std::ios::ate);
  if (!in_f.is_open()) {
    throw ExportIOException("Cannot open encrypted file for decryption",
                            input_path);
  }
  const auto file_size = static_cast<size_t>(in_f.tellg());
  in_f.seekg(0, std::ios::beg);

  // ── 2. Parse header ───────────────────────────────────────────────────
  std::string kek_id, job_id;
  uint32_t kek_version = 0;
  std::vector<uint8_t> iv;

  if (!readHeader(in_f, kek_id, kek_version, job_id, iv)) {
    throw DecryptionException("Failed to parse encrypted file header in '" +
                              input_path +
                              "': invalid magic, version, or truncated header");
  }

  const auto header_size = static_cast<size_t>(in_f.tellg());

  // Ciphertext occupies everything between header and 16-byte tag
  if (file_size < header_size + 16) {
    throw DecryptionException("Encrypted file '" + input_path +
                              "' is too small to contain GCM tag");
  }
  const size_t ciphertext_size = file_size - header_size - 16;

  // ── 3. Read GCM tag (last 16 bytes) ──────────────────────────────────
  in_f.seekg(static_cast<std::streamoff>(header_size + ciphertext_size));
  uint8_t expected_tag[16];
  in_f.read(reinterpret_cast<char *>(expected_tag), 16);
  if (in_f.gcount() != 16) {
    throw DecryptionException("Failed to read GCM tag from '" + input_path +
                              "'");
  }

  // Seek back to start of ciphertext
  in_f.seekg(static_cast<std::streamoff>(header_size));

  // ── 4. Retrieve KEK ───────────────────────────────────────────────────
  // Serialise concurrent callers at the KEK-fetch boundary.
  std::vector<uint8_t> kek;
  try {
    std::lock_guard<std::mutex> lk(key_provider_mutex_);
    kek = config_.key_provider->getKey(kek_id, kek_version);
  } catch (const std::exception &e) {
    throw DecryptionException(std::string("Failed to retrieve KEK '") + kek_id +
                              "' v" + std::to_string(kek_version) + ": " +
                              e.what());
  }

  // ── 5. Re-derive DEK ──────────────────────────────────────────────────
  std::vector<uint8_t> dek;
  try {
    dek = deriveDataKey(kek, job_id);
  } catch (...) {
    std::fill(kek.begin(), kek.end(), uint8_t{0});
    throw;
  }
  std::fill(kek.begin(), kek.end(), uint8_t{0});

  THEMIS_INFO("ExportEncryptor: decrypting '{}' kek_id='{}' job_id='{}'",
              input_path, kek_id, job_id);

  // ── 6. Open output ────────────────────────────────────────────────────
  std::ofstream out_f(output_path, std::ios::binary | std::ios::trunc);
  if (!out_f.is_open()) {
    std::fill(dek.begin(), dek.end(), uint8_t{0});
    throw ExportIOException("Cannot create output file for decryption",
                            output_path);
  }

  // ── 7. Initialise AES-256-GCM decrypt context ─────────────────────────
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    std::fill(dek.begin(), dek.end(), uint8_t{0});
    throw DecryptionException("Failed to allocate EVP cipher context");
  }

  auto cleanup_ctx = [&]() {
    EVP_CIPHER_CTX_free(ctx);
    std::fill(dek.begin(), dek.end(), uint8_t{0});
  };

  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) !=
      1) {
    cleanup_ctx();
    throw DecryptionException("EVP_DecryptInit_ex (AES-256-GCM) failed");
  }
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
    cleanup_ctx();
    throw DecryptionException("EVP_CTRL_GCM_SET_IVLEN failed");
  }
  if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, dek.data(), iv.data()) != 1) {
    cleanup_ctx();
    throw DecryptionException("EVP_DecryptInit_ex (key/IV) failed");
  }

  // ── 8. Feed job_id as AAD ─────────────────────────────────────────────
  {
    int aad_out = 0;
    const auto *aad_ptr =
        reinterpret_cast<const unsigned char *>(job_id.data());
    if (EVP_DecryptUpdate(ctx, nullptr, &aad_out, aad_ptr,
                          static_cast<int>(job_id.size())) != 1) {
      cleanup_ctx();
      throw DecryptionException("Failed to set GCM AAD (job_id)");
    }
  }

  // ── 9. Stream-decrypt ciphertext ──────────────────────────────────────
  std::vector<unsigned char> cipher_buf(kChunkSize);
  std::vector<unsigned char> plain_buf(kChunkSize + 16);
  size_t remaining = ciphertext_size;
  size_t total_plain_bytes = 0;

  while (remaining > 0) {
    const size_t to_read = std::min(remaining, kChunkSize);
    in_f.read(reinterpret_cast<char *>(cipher_buf.data()),
              static_cast<std::streamsize>(to_read));
    const size_t bytes_read = static_cast<size_t>(in_f.gcount());
    if (bytes_read == 0) {
      break;
    }
    remaining -= bytes_read;

    int pt_len = 0;
    if (EVP_DecryptUpdate(ctx, plain_buf.data(), &pt_len, cipher_buf.data(),
                          static_cast<int>(bytes_read)) != 1) {
      cleanup_ctx();
      throw DecryptionException("EVP_DecryptUpdate failed during streaming");
    }
    if (pt_len > 0) {
      out_f.write(reinterpret_cast<const char *>(plain_buf.data()), pt_len);
      total_plain_bytes += static_cast<size_t>(pt_len);
    }
  }

  // ── 10. Set expected tag and finalise (verifies GCM authentication) ───
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, expected_tag) != 1) {
    cleanup_ctx();
    throw DecryptionException("Failed to set expected GCM tag");
  }

  int final_len = 0;
  if (EVP_DecryptFinal_ex(ctx, plain_buf.data(), &final_len) != 1) {
    // Authentication tag mismatch — ciphertext has been tampered with
    EVP_CIPHER_CTX_free(ctx);
    std::fill(dek.begin(), dek.end(), uint8_t{0});
    // Remove partially-written output to avoid leaving plaintext on disk
    out_f.close();
    std::error_code ec = {};
    std::filesystem::remove(output_path, ec);
    throw DecryptionException(
        "GCM authentication tag verification failed for '" + input_path +
        "': ciphertext integrity check failed (job_id='" + job_id + "')");
  }
  if (final_len > 0) {
    out_f.write(reinterpret_cast<const char *>(plain_buf.data()), final_len);
    total_plain_bytes += static_cast<size_t>(final_len);
  }

  EVP_CIPHER_CTX_free(ctx);
  std::fill(dek.begin(), dek.end(), uint8_t{0});

  if (!out_f.good()) {
    throw ExportIOException("Write error while finalising decrypted file",
                            output_path);
  }
  out_f.close();
  in_f.close();

  THEMIS_INFO("ExportEncryptor: decrypted {} bytes -> '{}'", total_plain_bytes,
              output_path);
  return total_plain_bytes;
}

} // namespace themis::exporters

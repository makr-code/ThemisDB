/**
 * @file saga_logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/saga_logger.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include "utils/error_contracts.h"

namespace themis {
namespace utils {

// ===== SignedBatch Serialization =====

nlohmann::json SignedBatch::toJson() const {
    nlohmann::json j;
    j["batch_id"] = batch_id;
    j["entry_count"] = entry_count;
    j["start_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                          start_time.time_since_epoch()).count();
    j["end_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                        end_time.time_since_epoch()).count();
    j["lek_id"] = lek_id;
    j["key_version"] = key_version;
    
    // Base64 encode binary data
    auto b64 = [](const std::vector<uint8_t>& v) {
        themis::EncryptedBlob tmp;
        tmp.ciphertext = v;
        auto jblob = tmp.toJson();
        return jblob["ciphertext"];
    };
    
    j["iv_b64"] = b64(iv);
    j["tag_b64"] = b64(tag);
    j["ciphertext_hash_b64"] = b64(ciphertext_hash);
    
    j["signature"] = {
        {"ok", signature.ok},
        {"id", signature.signature_id},
        {"algorithm", signature.algorithm},
        {"sig_b64", signature.signature_b64},
        {"cert_serial", signature.cert_serial}
    };
    
    return j;
}

SignedBatch SignedBatch::fromJson(const nlohmann::json& j) {
    SignedBatch batch;
    auto get_str = [](const nlohmann::json& obj, const char* key) -> std::string {
        if (obj.contains(key) && obj[key].is_string()) return obj[key].get<std::string>();
        return "";
    };
    auto get_i64 = [](const nlohmann::json& obj, const char* key) -> int64_t {
        if (obj.contains(key) && obj[key].is_number_integer()) return obj[key].get<int64_t>();
        if (obj.contains(key) && obj[key].is_number_unsigned()) return static_cast<int64_t>(obj[key].get<uint64_t>());
        return 0;
    };

    batch.batch_id = get_str(j, "batch_id");
    if (j.contains("entry_count") && j["entry_count"].is_number()) batch.entry_count = j["entry_count"].get<size_t>();
    batch.start_time = std::chrono::system_clock::time_point{ std::chrono::milliseconds{ get_i64(j, "start_time") } };
    batch.end_time = std::chrono::system_clock::time_point{ std::chrono::milliseconds{ get_i64(j, "end_time") } };
    batch.lek_id = get_str(j, "lek_id");
    if (j.contains("key_version") && j["key_version"].is_number_unsigned()) {
        batch.key_version = j["key_version"].get<uint32_t>();
    }

    // Decode base64 (reuse EncryptedBlob helper)
    auto from_b64 = [](const std::string& b64) {
        if (b64.empty()) return std::vector<uint8_t>{};
        nlohmann::json tmp = {
            {"key_id", ""},
            {"key_version", 0},
            {"iv", ""},
            {"ciphertext", b64},
            {"tag", ""}
        };
        auto blob = themis::EncryptedBlob::fromJson(tmp);
        return blob.ciphertext;
    };

    batch.iv = from_b64(get_str(j, "iv_b64"));
    batch.tag = from_b64(get_str(j, "tag_b64"));
    batch.ciphertext_hash = from_b64(get_str(j, "ciphertext_hash_b64"));

    if (j.contains("signature") && j["signature"].is_object()) {
        const auto& s = j["signature"];
        batch.signature.ok = s.contains("ok") && s["ok"].is_boolean() ? s["ok"].get<bool>() : false;
        batch.signature.signature_id = get_str(s, "id");
        batch.signature.algorithm = get_str(s, "algorithm");
        batch.signature.signature_b64 = get_str(s, "sig_b64");
        batch.signature.cert_serial = get_str(s, "cert_serial");
    }

    return batch;
}

// ===== SAGALogger Implementation =====

SAGALogger::SAGALogger(std::shared_ptr<FieldEncryption> enc,
                       std::shared_ptr<VCCPKIClient> pki,
                       SAGALoggerConfig cfg)
    : enc_(std::move(enc))
    , pki_(std::move(pki))
    , cfg_(std::move(cfg))
    , batch_start_time_(std::chrono::system_clock::now()) {
    
    // Create log directories
    std::filesystem::create_directories(
        std::filesystem::path(cfg_.log_path).parent_path()
    );
    std::filesystem::create_directories(
        std::filesystem::path(cfg_.signature_path).parent_path()
    );
}

void SAGALogger::logStep(const SAGAStep& step) {
    if (!cfg_.enabled) return;
    
    std::scoped_lock lk(mu_);
    buffer_.push_back(step);
    
    // Check flush conditions
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
        now - batch_start_time_
    );
    
    if (buffer_.size() >= cfg_.batch_size || elapsed >= cfg_.batch_interval) {
        signAndFlushBatch();
    }
}

void SAGALogger::flush() {
    std::scoped_lock lk(mu_);
    if (!buffer_.empty()) {
        signAndFlushBatch();
    }
}

std::string SAGALogger::generateBatchId() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()).count();
    std::ostringstream oss;
    oss << "saga_batch_" << ms;
    return oss.str();
}

std::vector<uint8_t> SAGALogger::sha256(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out(SHA256_DIGEST_LENGTH);
    ::SHA256(data.data(), data.size(), out.data());
    return out;
}

void SAGALogger::appendJsonLine(const std::string& path, const nlohmann::json& j) {
    std::ofstream ofs(path, std::ios::app | std::ios::binary);
    if (!ofs) {
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::AUDIT_PERSISTENCE_FAILED,
            "Failed to open SAGA log file for append – backend unavailable; path=" + path,
            "SAGALogger::appendJsonLine",
            themis::utils::ErrorSeverity::Critical,
            false);
        themis::utils::logErrorWithContext(ctx);
        throw std::runtime_error(
            "SAGALogger: cannot open log file '" + path + "' – backend unavailable");
    }
    ofs << j.dump() << "\n";
    if (!ofs) {
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::AUDIT_WRITE_FAILED,
            "Write to SAGA log file failed; path=" + path,
            "SAGALogger::appendJsonLine",
            themis::utils::ErrorSeverity::Critical,
            false);
        themis::utils::logErrorWithContext(ctx);
        throw std::runtime_error(
            "SAGALogger: write failure on log file '" + path + "'");
    }
}

void SAGALogger::signAndFlushBatch() {
    if (buffer_.empty()) return;
    
    auto batch_id = generateBatchId();
    auto start_time = batch_start_time_;
    auto end_time = std::chrono::system_clock::now();
    
    // 1. Serialize batch to canonical JSON
    nlohmann::json batch_array = nlohmann::json::array();
    for (const auto& step : buffer_) {
        nlohmann::json entry;
        entry["saga_id"] = step.saga_id;
        entry["step_name"] = step.step_name;
        entry["action"] = step.action;
        entry["entity_id"] = step.entity_id;
        entry["payload"] = step.payload;
        entry["status"] = step.status;
        entry["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 step.timestamp.time_since_epoch()).count();
        batch_array.push_back(entry);
    }
    
    std::string plaintext = batch_array.dump();
    
    SignedBatch signed_batch;
    signed_batch.batch_id = batch_id;
    signed_batch.entry_count = buffer_.size();
    signed_batch.start_time = start_time;
    signed_batch.end_time = end_time;
    signed_batch.lek_id = cfg_.key_id;
    
    if (cfg_.encrypt_then_sign && enc_) {
        // 2. Encrypt with LEK (fail-closed: encryption failure propagates)
        EncryptedBlob blob;
        try {
            blob = enc_->encrypt(plaintext, cfg_.key_id);
        } catch (const std::exception& e) {
            auto ctx = themis::utils::makeErrorContext(
                themis::utils::ErrorCode::AUDIT_ENCRYPTION_FAILED,
                "SAGA batch encryption failed – key unavailable or LEK error; batch_id=" +
                    batch_id + "; error=" + e.what(),
                "SAGALogger::signAndFlushBatch",
                themis::utils::ErrorSeverity::Critical,
                false);
            themis::utils::logErrorWithContext(ctx);
            throw;  // fail-closed: do not persist plaintext on encryption failure
        }
        signed_batch.iv = blob.iv;
        signed_batch.tag = blob.tag;
    signed_batch.key_version = blob.key_version;
        
        // 3. Build ciphertext for hashing: iv || ciphertext || tag
        std::vector<uint8_t> to_hash;
        to_hash.reserve(blob.iv.size() + blob.ciphertext.size() + blob.tag.size());
        to_hash.insert(to_hash.end(), blob.iv.begin(), blob.iv.end());
        to_hash.insert(to_hash.end(), blob.ciphertext.begin(), blob.ciphertext.end());
        to_hash.insert(to_hash.end(), blob.tag.begin(), blob.tag.end());
        
        // 4. Hash ciphertext
        signed_batch.ciphertext_hash = sha256(to_hash);
        
        // 5. Sign hash with PKI
        if (pki_) {
            signed_batch.signature = pki_->signHash(signed_batch.ciphertext_hash);
        }
        
        // 6. Persist ciphertext to log file
        nlohmann::json log_entry;
        log_entry["batch_id"] = batch_id;
    log_entry["ciphertext_b64"] = themis::EncryptedBlob{blob}.toJson()["ciphertext"];
        appendJsonLine(cfg_.log_path, log_entry);
        
    } else {
        // Plaintext mode (not recommended)
        appendJsonLine(cfg_.log_path, batch_array);
    }
    
    // 7. Persist signature metadata
    appendJsonLine(cfg_.signature_path, signed_batch.toJson());
    
    // Reset buffer
    buffer_.clear();
    batch_start_time_ = std::chrono::system_clock::now();
}

bool SAGALogger::verifyBatch(const std::string& batch_id) {
    // 1. Load signature metadata
    std::ifstream sig_file(cfg_.signature_path);
    if (!sig_file) return false;
    
    std::optional<SignedBatch> batch_meta;
    std::string line;
    while (std::getline(sig_file, line)) {
        auto j = nlohmann::json::parse(line, nullptr, false);
        if (j.is_discarded() || !j.is_object() || !j.contains("batch_id") || !j["batch_id"].is_string() || j["batch_id"].get<std::string>() != batch_id) {
            continue;
        }
        batch_meta = SignedBatch::fromJson(j);
        break;
    }
    
    if (!batch_meta) return false;
    
    // 2. Load ciphertext
    std::ifstream log_file(cfg_.log_path);
    if (!log_file) return false;
    
    std::optional<std::vector<uint8_t>> ciphertext;
    while (std::getline(log_file, line)) {
        auto j = nlohmann::json::parse(line, nullptr, false);
        if (j.is_discarded() || !j.is_object() || !j.contains("batch_id") || !j["batch_id"].is_string() || j["batch_id"].get<std::string>() != batch_id) {
            continue;
        }
        
        // Decode ciphertext
        if (!j.contains("ciphertext_b64") || !j["ciphertext_b64"].is_string()) {
            continue;
        }
        nlohmann::json tmp = {
            {"key_id", ""},
            {"key_version", 0},
            {"iv", ""},
            {"ciphertext", j["ciphertext_b64"].get<std::string>()},
            {"tag", ""}
        };
        auto blob = themis::EncryptedBlob::fromJson(tmp);
        ciphertext = blob.ciphertext;
        break;
    }
    
    if (!ciphertext) return false;
    
    // 3. Rebuild hash: iv || ciphertext || tag
    std::vector<uint8_t> to_hash;
    to_hash.reserve(batch_meta->iv.size() + ciphertext->size() + batch_meta->tag.size());
    to_hash.insert(to_hash.end(), batch_meta->iv.begin(), batch_meta->iv.end());
    to_hash.insert(to_hash.end(), ciphertext->begin(), ciphertext->end());
    to_hash.insert(to_hash.end(), batch_meta->tag.begin(), batch_meta->tag.end());
    
    auto computed_hash = sha256(to_hash);
    
    // 4. Compare with stored hash
    if (computed_hash != batch_meta->ciphertext_hash) {
        return false; // Hash mismatch → tampered
    }
    
    // 5. Verify PKI signature (verifyHash instead of verifySignature)
    if (pki_ && !pki_->verifyHash(computed_hash, batch_meta->signature)) {
        return false; // Signature invalid
    }
    
    return true;
}

std::vector<SAGAStep> SAGALogger::loadBatch(const std::string& batch_id) {
    if (!verifyBatch(batch_id)) {
        return {}; // Verification failed
    }
    
    // Load and decrypt batch
    std::ifstream log_file(cfg_.log_path);
    std::ifstream sig_file(cfg_.signature_path);
    if (!log_file || !sig_file) return {};
    
    // Find batch metadata
    std::optional<SignedBatch> batch_meta;
    std::string line;
    while (std::getline(sig_file, line)) {
        auto j = nlohmann::json::parse(line, nullptr, false);
        if (j.is_discarded() || !j.is_object() || !j.contains("batch_id") || !j["batch_id"].is_string() || j["batch_id"].get<std::string>() != batch_id) {
            continue;
        }
        batch_meta = SignedBatch::fromJson(j);
        break;
    }
    
    if (!batch_meta) return {};
    
    // Find ciphertext
    while (std::getline(log_file, line)) {
        auto j = nlohmann::json::parse(line, nullptr, false);
        if (j.is_discarded() || !j.is_object() || !j.contains("batch_id") || !j["batch_id"].is_string() || j["batch_id"].get<std::string>() != batch_id) continue;
        
        // Decrypt
    themis::EncryptedBlob blob;
    blob.key_id = batch_meta->lek_id;
    blob.key_version = batch_meta->key_version;
        blob.iv = batch_meta->iv;
        blob.tag = batch_meta->tag;
        
        if (!j.contains("ciphertext_b64") || !j["ciphertext_b64"].is_string()) continue;
        nlohmann::json tmp = {
            {"key_id", ""},
            {"key_version", 0},
            {"iv", ""},
            {"ciphertext", j["ciphertext_b64"].get<std::string>()},
            {"tag", ""}
        };
        blob.ciphertext = themis::EncryptedBlob::fromJson(tmp).ciphertext;
        
        if (!enc_) return {};
        
        auto plaintext = enc_->decrypt(blob);
        auto batch_array = nlohmann::json::parse(plaintext);
        
        // Parse steps
        std::vector<SAGAStep> steps;
        for (const auto& entry : batch_array) {
            SAGAStep step;
            step.saga_id = entry["saga_id"].get<std::string>();
            step.step_name = entry["step_name"].get<std::string>();
            step.action = entry["action"].get<std::string>();
            step.entity_id = entry["entity_id"].get<std::string>();
            step.payload = entry["payload"];
            step.status = entry["status"].get<std::string>();
            step.timestamp = std::chrono::system_clock::time_point{
                std::chrono::milliseconds{entry["timestamp"].get<int64_t>()}
            };
            steps.push_back(step);
        }
        
        return steps;
    }
    
    return {};
}

std::vector<std::string> SAGALogger::listBatches() const {
    std::vector<std::string> batch_ids;
    std::ifstream sig_file(cfg_.signature_path);
    if (!sig_file) return batch_ids;
    
    std::string line;
    while (std::getline(sig_file, line)) {
        auto j = nlohmann::json::parse(line, nullptr, false);
        if (j.is_discarded() || !j.is_object() || !j.contains("batch_id") || !j["batch_id"].is_string()) {
            continue;
        }
        batch_ids.push_back(j["batch_id"].get<std::string>());
    }
    
    return batch_ids;
}

// ===========================================================================
// SAGALogCompactor
// ===========================================================================

SAGALogCompactor::SAGALogCompactor(const SAGALoggerConfig& cfg)
    : cfg_(cfg)
    , archive_path_(cfg.log_path + ".archive.jsonl")
{}

std::string SAGALogCompactor::archivePath() const {
    return archive_path_;
}

size_t SAGALogCompactor::compact(const std::string& before_txn_id) {
    // Read all lines from the WAL and separate into two buckets:
    //   completed  — saga_id < before_txn_id AND status == "success"
    //   retained   — everything else
    //
    // "Completed" lines are appended to the archive file; the remaining
    // lines atomically replace the original WAL.

    std::vector<std::string> retained_lines;
    std::vector<std::string> archive_lines;
    size_t archived_count = 0;

    {
        std::ifstream ifs(cfg_.log_path);
        if (!ifs) return 0; // WAL does not exist yet

        std::string line;
        while (std::getline(ifs, line)) {
            if (line.empty()) continue;

            auto j = nlohmann::json::parse(line, nullptr, false);
            if (j.is_discarded() || !j.is_object()) {
                retained_lines.push_back(line);
                continue;
            }

            // Decode plaintext (non-encrypted) WAL entries expected to have
            // saga_id and status at the top level.
            bool is_completed = false;
            if (j.contains("saga_id") && j["saga_id"].is_string()
                && j.contains("status") && j["status"].is_string())
            {
                const std::string& saga_id = j["saga_id"].get_ref<const std::string&>();
                const std::string& status  = j["status"].get_ref<const std::string&>();
                if (status == "success" && saga_id < before_txn_id) {
                    is_completed = true;
                }
            }

            if (is_completed) {
                archive_lines.push_back(line);
                ++archived_count;
            } else {
                retained_lines.push_back(line);
            }
        }
    }

    if (archived_count == 0) return 0;

    // Write retained lines to a temp file, then atomically rename.
    const std::string tmp_path = cfg_.log_path + ".compact.tmp";
    {
        std::ofstream ofs(tmp_path, std::ios::trunc | std::ios::binary);
        for (const auto& l : retained_lines) ofs << l << '\n';
    }
    std::filesystem::rename(tmp_path, cfg_.log_path);

    // Append archived lines to the archive file.
    {
        std::ofstream arc(archive_path_, std::ios::app | std::ios::binary);
        for (const auto& l : archive_lines) arc << l << '\n';
    }

    return archived_count;
}

// ===========================================================================
// SAGALogReplayer
// ===========================================================================

SAGALogReplayer::SAGALogReplayer(const SAGALoggerConfig& cfg)
    : cfg_(cfg)
{}

size_t SAGALogReplayer::replay_incomplete(RecoveryHandler handler) {
    // Scan the WAL for steps where action == "compensate" and
    // status == "pending".  These represent compensation steps that were
    // recorded but not yet confirmed as complete after a crash.

    size_t replayed = 0;

    std::ifstream ifs(cfg_.log_path);
    if (!ifs) return 0;

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        auto j = nlohmann::json::parse(line, nullptr, false);
        if (j.is_discarded() || !j.is_object()) continue;
        if (!j.contains("action") || !j["action"].is_string()) continue;
        if (!j.contains("status") || !j["status"].is_string()) continue;

        const std::string& action = j["action"].get_ref<const std::string&>();
        const std::string& status = j["status"].get_ref<const std::string&>();

        if (action != "compensate" || status != "pending") continue;

        // Reconstruct a SAGAStep from the JSON record.
        SAGAStep step;
        step.action = action;
        step.status = status;
        if (j.contains("saga_id")   && j["saga_id"].is_string())
            step.saga_id   = j["saga_id"].get<std::string>();
        if (j.contains("step_name") && j["step_name"].is_string())
            step.step_name = j["step_name"].get<std::string>();
        if (j.contains("entity_id") && j["entity_id"].is_string())
            step.entity_id = j["entity_id"].get<std::string>();
        if (j.contains("payload"))
            step.payload   = j["payload"];
        if (j.contains("timestamp") && j["timestamp"].is_number_integer()) {
            step.timestamp = std::chrono::system_clock::time_point{
                std::chrono::milliseconds{j["timestamp"].get<int64_t>()}
            };
        }

        handler(step);
        ++replayed;
    }

    return replayed;
}

} // namespace utils
} // namespace themis

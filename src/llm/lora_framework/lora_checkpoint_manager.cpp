/**
 * @file lora_checkpoint_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lora_checkpoint_manager.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace fs = std::filesystem;
using json   = nlohmann::json;

namespace themis {
namespace llm {
namespace lora {

// ─────────────────────────────────────────────────────────────────────────────
// CheckpointMeta serialisation
// ─────────────────────────────────────────────────────────────────────────────

json CheckpointMeta::toJSON() const {
    return json{
        {"adapter_id",    adapter_id},
        {"step",          step},
        {"epoch",         epoch},
        {"train_loss",    train_loss},
        {"val_loss",      val_loss},
        {"elapsed_s",     elapsed_s},
        {"created_at",    created_at},
        {"weights_sha256",weights_sha256}
    };
}

CheckpointMeta CheckpointMeta::fromJSON(const json& j) {
    CheckpointMeta m;
    m.adapter_id     = j.value("adapter_id",     std::string{});
    m.step           = j.value("step",            uint64_t{0});
    m.epoch          = j.value("epoch",           uint32_t{0});
    m.train_loss     = j.value("train_loss",      0.0f);
    m.val_loss       = j.value("val_loss",        std::numeric_limits<float>::infinity());
    m.elapsed_s      = j.value("elapsed_s",       0.0);
    m.created_at     = j.value("created_at",      std::string{});
    m.weights_sha256 = j.value("weights_sha256",  std::string{});
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/** Compute SHA-256 hex digest of a byte buffer. */
std::string sha256Hex(const uint8_t* data, size_t len) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss = {};
    for (unsigned int i = 0; i < digest_len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(digest[i]);
    return oss.str();
}

/** Current UTC time as ISO-8601 string. */
std::string utcNow() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss = {};
    oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

/** Write bytes atomically: write to tmp file, then rename. */
void atomicWrite(const fs::path& dest,
                 const uint8_t* data,
                 size_t         len)
{
    fs::path tmp = dest;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) {
          throw std::runtime_error("atomicWrite: cannot open " + tmp.string());
        }
        f.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(len));
    }
    fs::rename(tmp, dest);
}

/** Read all bytes from a file. */
std::vector<uint8_t> readFile(const fs::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) {
      throw std::runtime_error("readFile: cannot open " + p.string());
    }
    auto size = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
    return buf;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

LoRACheckpointManager::LoRACheckpointManager()
    : LoRACheckpointManager(Config{}) {}

LoRACheckpointManager::LoRACheckpointManager(Config config)
    : config_(std::move(config)) {}

LoRACheckpointManager::~LoRACheckpointManager() = default;

// ─────────────────────────────────────────────────────────────────────────────
// Path helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string LoRACheckpointManager::adapterDir(const std::string& adapter_id) const {
    return (fs::path(config_.root_dir) / adapter_id).string();
}

std::string LoRACheckpointManager::weightPath(const std::string& adapter_id,
                                               uint64_t step) const {
    std::ostringstream oss = {};
    oss << "checkpoint-" << step << ".bin";
    return (fs::path(adapterDir(adapter_id)) / oss.str()).string();
}

std::string LoRACheckpointManager::metaPath(const std::string& adapter_id,
                                             uint64_t step) const {
    std::ostringstream oss = {};
    oss << "checkpoint-" << step << ".meta.json";
    return (fs::path(adapterDir(adapter_id)) / oss.str()).string();
}

// ─────────────────────────────────────────────────────────────────────────────
// Metadata I/O
// ─────────────────────────────────────────────────────────────────────────────

void LoRACheckpointManager::writeMeta(const std::string& path,
                                       const CheckpointMeta& meta) const {
    std::string s = meta.toJSON().dump(2);
    atomicWrite(fs::path(path),
                reinterpret_cast<const uint8_t*>(s.data()),
                s.size());
}

CheckpointMeta LoRACheckpointManager::readMeta(const std::string& path) const {
    auto buf = readFile(fs::path(path));
    return CheckpointMeta::fromJSON(
        json::parse(buf.begin(), buf.end()));
}

void LoRACheckpointManager::updateBestRecord(const std::string& adapter_id,
                                              const CheckpointMeta& meta) const {
    fs::path best_path = fs::path(adapterDir(adapter_id)) / "best.json";
    bool update = true;
    if (fs::exists(best_path)) {
        try {
            auto cur = readMeta(best_path.string());
            update = meta.val_loss < cur.val_loss;
        } catch (...) {}
    }
    if (update) {
        writeMeta(best_path.string(), meta);
    }
}

std::optional<CheckpointMeta>
LoRACheckpointManager::readBestMeta(const std::string& adapter_id) const {
    fs::path p = fs::path(adapterDir(adapter_id)) / "best.json";
    if (!fs::exists(p)) {
      return std::nullopt;
    }
    try {
        return readMeta(p.string());
    } catch (...) {
        return std::nullopt;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// save
// ─────────────────────────────────────────────────────────────────────────────

std::string LoRACheckpointManager::save(const std::string&         adapter_id,
                                         const std::vector<uint8_t>& weights,
                                         CheckpointMeta              meta)
{
    std::lock_guard<std::mutex> lock(mutex_);

    fs::path dir = fs::path(adapterDir(adapter_id));
    fs::create_directories(dir);

    meta.adapter_id     = adapter_id;
    meta.created_at     = utcNow();
    meta.weights_sha256 = sha256Hex(weights.data(),static_cast<int>(weights.size()));

    const std::string wpath = weightPath(adapter_id, meta.step);
    const std::string mpath = metaPath(adapter_id, meta.step);

    // Atomic write of weight blob
    atomicWrite(fs::path(wpath), weights.data(),static_cast<int>(weights.size()));
    writeMeta(mpath, meta);

    // Update best-checkpoint record
    if (config_.keep_best) {
        updateBestRecord(adapter_id, meta);
    }

    THEMIS_INFO("LoRACheckpointManager: saved checkpoint step={} adapter={} size={}B val_loss={:.4f}",
                meta.step, adapter_id,static_cast<int>(weights.size()), meta.val_loss);

    // Prune old checkpoints
    prune(adapter_id);

    return wpath;
}

// ─────────────────────────────────────────────────────────────────────────────
// Listing helpers
// ─────────────────────────────────────────────────────────────────────────────

std::vector<CheckpointRef>
LoRACheckpointManager::listCheckpoints(const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    fs::path dir = fs::path(adapterDir(adapter_id));
    if (!fs::exists(dir)) return {};

    auto best_meta = readBestMeta(adapter_id);

    std::vector<CheckpointRef> refs = {};

    for (const auto& entry : fs::directory_iterator(dir)) {
        const std::string fname = entry.path().filename().string();
        if (fname.rfind("checkpoint-", 0) != 0) {
          continue;
        }
        if (entry.path().extension() != ".bin") {
          continue;
        }

        // Extract step from filename: "checkpoint-<step>.bin"
        uint64_t step = 0;
        try {
            std::string base = entry.path().stem().string(); // "checkpoint-N"
            step = std::stoull(base.substr(base.rfind('-') + 1));
        } catch (...) { continue; }

        std::string mpath = metaPath(adapter_id, step);
        if (!fs::exists(mpath)) {
          continue;
        }

        try {
            CheckpointRef ref;
            ref.path    = entry.path().string();
            ref.meta    = readMeta(mpath);
            ref.is_best = best_meta.has_value() &&
                          best_meta->step == step;
            refs.push_back(std::move(ref));
        } catch (...) {}
    }

    // Sort newest first (largest step first)
    std::sort(refs.begin(), refs.end(),
              [](const CheckpointRef& a, const CheckpointRef& b){
                  return a.meta.step > b.meta.step;
              });
    return refs;
}

// ─────────────────────────────────────────────────────────────────────────────
// loadLatest / loadBest / loadByStep
// ─────────────────────────────────────────────────────────────────────────────

std::optional<CheckpointRef>
LoRACheckpointManager::loadLatest(const std::string& adapter_id) const {
    auto refs = listCheckpoints(adapter_id);
    if (refs.empty()) {
      return std::nullopt;
    }
    return refs.front(); // Already sorted newest-first
}

std::optional<CheckpointRef>
LoRACheckpointManager::loadBest(const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto best_meta = readBestMeta(adapter_id);
    if (!best_meta) {
      return std::nullopt;
    }

    CheckpointRef ref;
    ref.path    = weightPath(adapter_id, best_meta->step);
    ref.meta    = *best_meta;
    ref.is_best = true;
    if (!fs::exists(ref.path)) {
      return std::nullopt;
    }
    return ref;
}

std::optional<CheckpointRef>
LoRACheckpointManager::loadByStep(const std::string& adapter_id,
                                   uint64_t           step) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string wpath = weightPath(adapter_id, step);
    std::string mpath = metaPath(adapter_id, step);
    if (!fs::exists(wpath) || !fs::exists(mpath)) {
      return std::nullopt;
    }

    CheckpointRef ref;
    ref.path = wpath;
    try {
        ref.meta = readMeta(mpath);
    } catch (...) {
        return std::nullopt;
    }
    auto best_meta = readBestMeta(adapter_id);
    ref.is_best = best_meta.has_value() && best_meta->step == step;
    return ref;
}

// ─────────────────────────────────────────────────────────────────────────────
// readWeights
// ─────────────────────────────────────────────────────────────────────────────

std::vector<uint8_t>
LoRACheckpointManager::readWeights(const CheckpointRef& ref) const {
    auto buf = readFile(fs::path(ref.path));

    if (config_.verify_hash && !ref.meta.weights_sha256.empty()) {
        std::string actual = sha256Hex(buf.data(),static_cast<int>(buf.size()));
        if (actual != ref.meta.weights_sha256) {
            throw std::runtime_error(
                "LoRACheckpointManager::readWeights: SHA-256 mismatch for " +
                ref.path + " (expected " + ref.meta.weights_sha256 +
                ", got " + actual + ")");
        }
    }
    return buf;
}

// ─────────────────────────────────────────────────────────────────────────────
// deleteCheckpoint / deleteAll / prune
// ─────────────────────────────────────────────────────────────────────────────

bool LoRACheckpointManager::deleteCheckpoint(const std::string& adapter_id,
                                              uint64_t           step) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool removed = false;
    auto wp = fs::path(weightPath(adapter_id, step));
    auto mp = fs::path(metaPath(adapter_id, step));
    if (fs::exists(wp)) { fs::remove(wp); removed = true; }
    if (fs::exists(mp)) { fs::remove(mp); }
    return removed;
}

void LoRACheckpointManager::deleteAll(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    fs::path dir = fs::path(adapterDir(adapter_id));
    if (fs::exists(dir)) {
      fs::remove_all(dir);
    }
}

void LoRACheckpointManager::prune(const std::string& adapter_id) {
    // Caller must already hold mutex_
    if (config_.keep_last == 0) {
      return;
    }

    auto best_meta = readBestMeta(adapter_id);

    // Re-list without locking again (mutex_ already held by caller)
    fs::path dir = fs::path(adapterDir(adapter_id));
    if (!fs::exists(dir)) {
      return;
    }

    std::vector<std::pair<uint64_t, std::string>> checkpoints; // (step, weight_path)
    for (const auto& entry : fs::directory_iterator(dir)) {
        const std::string fname = entry.path().filename().string();
        if (fname.rfind("checkpoint-", 0) != 0) {
          continue;
        }
        if (entry.path().extension() != ".bin") {
          continue;
        }
        try {
            std::string base = entry.path().stem().string();
            uint64_t step = std::stoull(base.substr(base.rfind('-') + 1));
            checkpoints.emplace_back(step, entry.path().string());
        } catch (...) {}
    }

    // Sort oldest first
    std::sort(checkpoints.begin(), checkpoints.end(),
              [](const auto& a, const auto& b){ return a.first < b.first; });

    // Determine how many to keep
    size_t to_keep = config_.keep_last;
    int64_t delete_count = static_cast<int64_t>(checkpoints.size()) -
                           static_cast<int64_t>(to_keep);

    for (int64_t i = 0; i < delete_count; ++i) {
        uint64_t step = checkpoints[static_cast<size_t>(i)].first;
        // Never delete the best checkpoint
        if (config_.keep_best && best_meta.has_value() && best_meta->step == step) {
          continue;
        }
        fs::remove(fs::path(weightPath(adapter_id, step)));
        fs::remove(fs::path(metaPath(adapter_id, step)));
        THEMIS_INFO("LoRACheckpointManager: pruned checkpoint step={} adapter={}",
                    step, adapter_id);
    }
}

} // namespace lora
} // namespace llm
} // namespace themis



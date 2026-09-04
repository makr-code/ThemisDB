/**
 * @file ht_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "tensor/ht_index.h"
#include "tensor/ht_train.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace themis {
namespace tensor {

// ============================================================================
// FlatHTIndex
// ============================================================================

void FlatHTIndex::add(const std::string& id, HTTrain train) {
    std::lock_guard<std::mutex> lk(mutex_);
    // Replace if id already exists
    for (auto& e : entries_) {
        if (e.id == id) {
            e.train = std::move(train);
            return;
        }
    }
    entries_.push_back({id, std::move(train)});
}

bool FlatHTIndex::remove(const std::string& id) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&]([[maybe_unused]] const Entry& e) { return e.id == id; });
    if (it == entries_.end()) {
      return false;
    }
    entries_.erase(it);
    return true;
}

std::vector<HTSearchResult>
FlatHTIndex::search(const HTTrain& query, std::size_t k) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<HTSearchResult> results;
    results.reserve(entries_.size());

    for (const auto& e : entries_) {
        double sim = HTContractionEngine::cosineSimilarity(query, e.train);
        results.push_back({e.id, sim});
    }

    // Sort descending by similarity
    std::sort(results.begin(), results.end(),
              [](const HTSearchResult& a, const HTSearchResult& b) {
                  return a.similarity > b.similarity;
              });

    if (results.size() > k) {
      results.resize(k);
    }
    return results;
}

std::size_t FlatHTIndex::size() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return entries_.size();
}

std::optional<const HTTrain*> FlatHTIndex::get(const std::string& id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& e : entries_)
        if (e.id == id) {
          return &e.train;
        }
    return std::nullopt;
}

// ============================================================================
// Serialization
// ============================================================================

namespace {

constexpr uint64_t kFlatHTMagic   = 0x464C41544854494DULL;  // "FLATHTI" (8 bytes)
constexpr uint8_t  kFlatHTVersion = 1;

void appendU64(std::vector<uint8_t>& buf, uint64_t v) {
    uint8_t tmp[8]; std::memcpy(tmp, &v, 8);
    buf.insert(buf.end(), tmp, tmp + 8);
}
void appendU8(std::vector<uint8_t>& buf, uint8_t v) { buf.push_back(v); }

struct BufReader {
    const uint8_t* p;
    std::size_t    left;
    bool           ok = true;

    bool u64([[maybe_unused]] uint64_t& v) {
        if (left < 8) { ok = false; return false; }
        std::memcpy(&v, p, 8); p += 8; left -= 8; return true;
    }
    bool u8([[maybe_unused]] uint8_t& v) {
        if (left < 1) { ok = false; return false; }
        v = *p++; left--; return true;
    }
    bool bytes(std::vector<uint8_t>& v, std::size_t n) {
        if (left < n) { ok = false; return false; }
        v.assign(p, p + n); p += n; left -= n; return true;
    }
};

} // anonymous namespace

std::vector<uint8_t> FlatHTIndex::serialize() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<uint8_t> buf;
    appendU64(buf, kFlatHTMagic);
    appendU8(buf, kFlatHTVersion);
    appendU64(buf, static_cast<uint64_t>(entries_.size()));

    for (const auto& e : entries_) {
        // id: 4-byte length + bytes
        uint32_t id_len = static_cast<uint32_t>(e.id.size());
        uint8_t tmp[4]; std::memcpy(tmp, &id_len, 4);
        buf.insert(buf.end(), tmp, tmp + 4);
        buf.insert(buf.end(), e.id.begin(), e.id.end());

        // serialized HTTrain
        auto train_bytes = e.train.serialize();
        appendU64(buf, static_cast<uint64_t>(train_bytes.size()));
        buf.insert(buf.end(), train_bytes.begin(), train_bytes.end());
    }
    return buf;
}

bool FlatHTIndex::deserialize(const std::vector<uint8_t>& bytes) {
    BufReader r{bytes.data(), bytes.size(), true};

    uint64_t magic = 0; uint8_t ver = 0;
    if (!r.u64(magic) || magic != kFlatHTMagic) {
      return false;
    }
    if (!r.u8(ver)    || ver   != kFlatHTVersion) {
      return false;
    }

    uint64_t n_entries = 0;
    if (!r.u64(n_entries)) {
      return false;
    }

    std::lock_guard<std::mutex> lk(mutex_);
    entries_.clear();
    entries_.reserve(static_cast<std::size_t>(n_entries));

    for (uint64_t i = 0; i < n_entries; ++i) {
        if (r.left < 4) {
          return false;
        }
        uint32_t id_len = 0;
        std::memcpy(&id_len, r.p, 4); r.p += 4; r.left -= 4;
        if (r.left < id_len) {
          return false;
        }
        std::string id(reinterpret_cast<const char*>(r.p), id_len);
        r.p += id_len; r.left -= id_len;

        uint64_t train_sz = 0;
        if (!r.u64(train_sz)) {
          return false;
        }
        std::vector<uint8_t> train_bytes;
        if (!r.bytes(train_bytes, static_cast<std::size_t>(train_sz))) {
          return false;
        }

        auto ht = HTTrain::deserialize(train_bytes);
        if (!ht) {
          return false;
        }
        entries_.push_back({std::move(id), std::move(*ht)});
    }
    return r.ok;
}

} // namespace tensor
} // namespace themis

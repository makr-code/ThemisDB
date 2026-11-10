/**
 * @file training_snapshot.cpp
 * @brief Implementation of MVCC training-snapshot capture, pinning and
 *        Merkle fingerprinting (see training_snapshot.h).
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "llm/lora_framework/training_snapshot.h"

#include "llm/lora_framework/lora_dataset_loader.h"
#include "storage/mvcc_snapshot_pins.h"
#include "storage/mvcc_store.h"

#include <openssl/evp.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <system_error>

namespace themis {
namespace llm {
namespace lora {

namespace {

struct EvpCtxDeleter {
    void operator()(EVP_MD_CTX* c) const noexcept { EVP_MD_CTX_free(c); }
};

class Sha256Stream {
public:
    Sha256Stream() : ctx_(EVP_MD_CTX_new()) {
        if (!ctx_ || EVP_DigestInit_ex(ctx_.get(), EVP_sha256(), nullptr) != 1) {
            throw std::runtime_error("SHA-256 init failed");
        }
    }
    void update(const void* data, size_t n) {
        if (n > 0 && EVP_DigestUpdate(ctx_.get(), data, n) != 1) {
            throw std::runtime_error("SHA-256 update failed");
        }
    }
    void update(std::string_view s) { update(s.data(), s.size()); }
    void updateU32(uint32_t v) {
        const uint8_t b[4] = {static_cast<uint8_t>(v >> 24), static_cast<uint8_t>(v >> 16),
                              static_cast<uint8_t>(v >> 8), static_cast<uint8_t>(v)};
        update(b, sizeof(b));
    }
    void updateU64(uint64_t v) {
        uint8_t b[8];
        for (int i = 0; i < 8; ++i) b[i] = static_cast<uint8_t>(v >> (56 - 8 * i));
        update(b, sizeof(b));
    }
    Sha256Digest finish() {
        Sha256Digest out{};
        unsigned int len = 0;
        if (EVP_DigestFinal_ex(ctx_.get(), out.data(), &len) != 1 || len != out.size()) {
            throw std::runtime_error("SHA-256 final failed");
        }
        return out;
    }

private:
    std::unique_ptr<EVP_MD_CTX, EvpCtxDeleter> ctx_;
};

constexpr const char* kSnapshotKeyPrefix = "lora_ds:";

std::string formatDouble(double v) {
    char buf[64];
    auto res = std::to_chars(buf, buf + sizeof(buf), v);
    return std::string(buf, res.ptr);
}

bool parseU64(const std::string& s, uint64_t& out) {
    if (s.empty()) return false;
    auto res = std::from_chars(s.data(), s.data() + s.size(), out);
    return res.ec == std::errc() && res.ptr == s.data() + s.size();
}

bool parseDouble(const std::string& s, double& out) {
    if (s.empty()) return false;
    auto res = std::from_chars(s.data(), s.data() + s.size(), out);
    return res.ec == std::errc() && res.ptr == s.data() + s.size() && std::isfinite(out);
}

bool isHex64(const std::string& s) {
    return s.size() == 64 && std::all_of(s.begin(), s.end(), [](char c) {
               return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
           });
}

TrainingSnapshotRead fail(SnapshotErrorCode code, std::string msg) {
    TrainingSnapshotRead r;
    r.ok = false;
    r.code = code;
    r.error = std::move(msg);
    return r;
}

}  // namespace

// ─── Hash helpers ───────────────────────────────────────────────────────────

Sha256Digest sha256Digest(std::string_view bytes) {
    Sha256Stream s;
    s.update(bytes);
    return s.finish();
}

std::string digestToHex(const Sha256Digest& d) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (uint8_t b : d) {
        out.push_back(kHex[b >> 4]);
        out.push_back(kHex[b & 0x0F]);
    }
    return out;
}

std::string sha256Hex(std::string_view bytes) { return digestToHex(sha256Digest(bytes)); }

// ─── Merkle tree ────────────────────────────────────────────────────────────

Sha256Digest SnapshotMerkleTree::leafHash(const SnapshotRecordRef& rec) {
    Sha256Stream s;
    const uint8_t tag = 0x00;
    s.update(&tag, 1);
    s.updateU32(static_cast<uint32_t>(rec.collection.size()));
    s.update(rec.collection);
    s.updateU32(static_cast<uint32_t>(rec.key.size()));
    s.update(rec.key);
    s.updateU64(rec.version_ts);
    s.update(rec.value_sha256.data(), rec.value_sha256.size());
    return s.finish();
}

std::optional<std::string> SnapshotMerkleTree::root(std::vector<SnapshotRecordRef> records,
                                                    std::string* error) {
    if (records.empty()) {
        return sha256Hex("");
    }
    std::sort(records.begin(), records.end(),
              [](const SnapshotRecordRef& a, const SnapshotRecordRef& b) { return a.key < b.key; });
    for (size_t i = 1; i < records.size(); ++i) {
        if (records[i].key == records[i - 1].key) {
            if (error) *error = "duplicate record key in snapshot: " + records[i].key;
            return std::nullopt;
        }
    }
    std::vector<Sha256Digest> level;
    level.reserve(records.size());
    for (const auto& r : records) level.push_back(leafHash(r));

    while (level.size() > 1) {
        std::vector<Sha256Digest> next;
        next.reserve((level.size() + 1) / 2);
        for (size_t i = 0; i + 1 < level.size(); i += 2) {
            Sha256Stream s;
            const uint8_t tag = 0x01;
            s.update(&tag, 1);
            s.update(level[i].data(), level[i].size());
            s.update(level[i + 1].data(), level[i + 1].size());
            next.push_back(s.finish());
        }
        if (level.size() % 2 == 1) {
            next.push_back(level.back());  // promote odd node unchanged
        }
        level.swap(next);
    }
    return digestToHex(level.front());
}

// ─── Selector ───────────────────────────────────────────────────────────────

std::string SnapshotSelector::keyPrefixFor(std::string_view collection) {
    std::string p(kSnapshotKeyPrefix);
    p.append(collection);
    p.push_back(':');
    return p;
}

std::string SnapshotSelector::keyPrefix() const { return keyPrefixFor(collection); }

bool SnapshotSelector::validate(std::string* error) const {
    if (!LoRADatasetLoader::isValidCollectionName(collection)) {
        if (error) *error = "invalid snapshot collection name";
        return false;
    }
    if (limit == 0 || limit > LoRADatasetLoader::kMaxSamples) {
        if (error) {
            *error = "snapshot limit must be in [1, " +
                     std::to_string(LoRADatasetLoader::kMaxSamples) + "]";
        }
        return false;
    }
    return true;
}

std::string SnapshotSelector::canonicalJson() const {
    nlohmann::json j = {
        {"collection", collection},
        {"key_prefix", keyPrefix()},
        {"limit", static_cast<uint64_t>(limit)},
        {"order", "key_asc"},
        {"source", "mvcc"},
        {"v", 1},
    };
    return j.dump();  // nlohmann::json object keys are sorted; no whitespace
}

std::string SnapshotSelector::sha256() const { return sha256Hex(canonicalJson()); }

std::optional<SnapshotSelector> SnapshotSelector::fromCanonicalJson(std::string_view json,
                                                                   std::string* error) {
    nlohmann::json j = nlohmann::json::parse(json, nullptr, /*allow_exceptions=*/false);
    if (!j.is_object() || !j.contains("collection") || !j["collection"].is_string() ||
        !j.contains("limit") || !j["limit"].is_number_unsigned()) {
        if (error) *error = "malformed snapshot selector JSON";
        return std::nullopt;
    }
    SnapshotSelector sel;
    sel.collection = j["collection"].get<std::string>();
    sel.limit = j["limit"].get<std::size_t>();
    if (!sel.validate(error)) return std::nullopt;
    if (sel.canonicalJson() != json) {
        if (error) *error = "snapshot selector JSON is not canonical or has an unsupported version";
        return std::nullopt;
    }
    return sel;
}

// ─── Fingerprint ────────────────────────────────────────────────────────────

std::map<std::string, std::string> TrainingSnapshotFingerprint::toMetadata() const {
    namespace k = snapshot_keys;
    return {
        {k::kCollection, collection},
        {k::kHlcTs, std::to_string(hlc_ts)},
        {k::kQuery, query},
        {k::kQuerySha256, query_sha256},
        {k::kRecordCount, std::to_string(record_count)},
        {k::kMerkleRoot, merkle_root},
        {k::kSplitSeed, std::to_string(split_seed)},
        {k::kHoldoutFraction, formatDouble(holdout_fraction)},
        {k::kPinned, pinned ? "true" : "false"},
    };
}

std::optional<TrainingSnapshotFingerprint> TrainingSnapshotFingerprint::fromMetadata(
    const std::map<std::string, std::string>& md, std::string* error) {
    namespace k = snapshot_keys;
    auto get = [&](const char* key) -> const std::string* {
        auto it = md.find(key);
        return it == md.end() ? nullptr : &it->second;
    };
    auto bad = [&](const std::string& msg) -> std::optional<TrainingSnapshotFingerprint> {
        if (error) *error = msg;
        return std::nullopt;
    };
    for (const char* key : {k::kCollection, k::kHlcTs, k::kQuery, k::kQuerySha256, k::kRecordCount,
                            k::kMerkleRoot, k::kSplitSeed, k::kHoldoutFraction, k::kPinned}) {
        if (!get(key)) return bad(std::string("missing snapshot metadata key: ") + key);
    }
    TrainingSnapshotFingerprint fp;
    fp.collection = *get(k::kCollection);
    fp.query = *get(k::kQuery);
    fp.query_sha256 = *get(k::kQuerySha256);
    fp.merkle_root = *get(k::kMerkleRoot);
    if (!parseU64(*get(k::kHlcTs), fp.hlc_ts) || fp.hlc_ts == 0) return bad("malformed snapshot hlc_ts");
    if (!parseU64(*get(k::kRecordCount), fp.record_count)) return bad("malformed snapshot record_count");
    if (!parseU64(*get(k::kSplitSeed), fp.split_seed)) return bad("malformed snapshot split_seed");
    if (!parseDouble(*get(k::kHoldoutFraction), fp.holdout_fraction) || fp.holdout_fraction < 0.0 ||
        fp.holdout_fraction >= 1.0) {
        return bad("malformed snapshot holdout_fraction");
    }
    const std::string& pinned = *get(k::kPinned);
    if (pinned != "true" && pinned != "false") return bad("malformed snapshot pinned flag");
    fp.pinned = pinned == "true";
    if (!isHex64(fp.merkle_root)) return bad("malformed snapshot merkle_root");
    if (!isHex64(fp.query_sha256) || sha256Hex(fp.query) != fp.query_sha256) {
        return bad("snapshot query_sha256 does not match the selector specification");
    }
    std::string sel_err;
    auto sel = SnapshotSelector::fromCanonicalJson(fp.query, &sel_err);
    if (!sel) return bad(sel_err);
    if (sel->collection != fp.collection) return bad("snapshot selector collection mismatch");
    return fp;
}

bool TrainingSnapshotFingerprint::sameData(const TrainingSnapshotFingerprint& o) const {
    return collection == o.collection && hlc_ts == o.hlc_ts && query_sha256 == o.query_sha256 &&
           record_count == o.record_count && merkle_root == o.merkle_root;
}

// ─── Reader ─────────────────────────────────────────────────────────────────

TrainingSnapshotReader::TrainingSnapshotReader(std::shared_ptr<MVCCStore> store)
    : store_(std::move(store)) {}

TrainingSnapshotRead TrainingSnapshotReader::readAt(const SnapshotSelector& selector,
                                                    HLCTimestamp ts,
                                                    uint64_t split_seed,
                                                    double holdout_fraction,
                                                    bool pinned) const {
    if (!store_) return fail(SnapshotErrorCode::NoStore, "no MVCC store configured for training snapshots");
    std::string err;
    if (!selector.validate(&err)) return fail(SnapshotErrorCode::InvalidSelector, err);
    if (!(holdout_fraction >= 0.0 && holdout_fraction < 1.0)) {
        return fail(SnapshotErrorCode::InvalidSelector, "holdout_fraction must be in [0, 1)");
    }

    const std::string prefix = selector.keyPrefix();
    std::vector<std::string> base_keys;
    store_->scanBaseKeysWithPrefix(prefix, [&](std::string_view bk) -> bool {
        base_keys.emplace_back(bk);
        return true;
    });

    std::vector<SnapshotRecordRef> refs;
    nlohmann::json rows = nlohmann::json::array();
    for (const auto& bk : base_keys) {  // ascending bytewise order
        if (refs.size() >= selector.limit) break;
        auto v = store_->getVersionAtTimestamp(bk, ts);
        if (!v || v->value.empty()) continue;  // absent or tombstone at T
        std::string_view raw(reinterpret_cast<const char*>(v->value.data()), v->value.size());
        auto doc = nlohmann::json::parse(raw, nullptr, /*allow_exceptions=*/false);
        if (doc.is_discarded()) {
            return fail(SnapshotErrorCode::InvalidRecord,
                        "snapshot record is not valid JSON: " + bk.substr(prefix.size()));
        }
        SnapshotRecordRef ref;
        ref.collection = selector.collection;
        ref.key = bk.substr(prefix.size());
        ref.version_ts = v->timestamp.value;
        ref.value_sha256 = sha256Digest(raw);
        refs.push_back(std::move(ref));
        rows.push_back(std::move(doc));
    }

    TrainingSnapshotRead out;
    out.fingerprint.collection = selector.collection;
    out.fingerprint.hlc_ts = ts.value;
    out.fingerprint.query = selector.canonicalJson();
    out.fingerprint.query_sha256 = sha256Hex(out.fingerprint.query);
    out.fingerprint.record_count = refs.size();
    out.fingerprint.split_seed = split_seed;
    out.fingerprint.holdout_fraction = holdout_fraction;
    out.fingerprint.pinned = pinned;
    auto root = SnapshotMerkleTree::root(std::move(refs), &err);
    if (!root) return fail(SnapshotErrorCode::Internal, err);
    out.fingerprint.merkle_root = *root;
    out.rows = std::move(rows);
    out.ok = true;
    return out;
}

TrainingSnapshotRead TrainingSnapshotReader::capture(const SnapshotSelector& selector,
                                                     uint64_t split_seed,
                                                     double holdout_fraction,
                                                     std::string_view owner) {
    if (!store_) return fail(SnapshotErrorCode::NoStore, "no MVCC store configured for training snapshots");
    std::string err;
    if (!selector.validate(&err)) return fail(SnapshotErrorCode::InvalidSelector, err);

    // Fresh tick: strictly greater than every timestamp issued so far, so all
    // writes started after this point are invisible at T.
    const HLCTimestamp t = store_->updateClock(store_->currentTimestamp());

    std::optional<std::string> lease;
    if (auto pins = store_->snapshotPinRegistry()) {
        auto pin = pins->acquire(t, owner);
        if (!pin) return fail(SnapshotErrorCode::PinFailed, "failed to persist snapshot retention pin");
        lease = pin->lease_id;
    }
    auto r = readAt(selector, t, split_seed, holdout_fraction, lease.has_value());
    if (!r.ok) {
        if (lease) releasePin(*lease);
        return r;
    }
    r.lease_id = lease;
    return r;
}

TrainingSnapshotRead TrainingSnapshotReader::verify(const TrainingSnapshotFingerprint& expected,
                                                    bool acquire_pin,
                                                    std::string_view owner) {
    if (!store_) return fail(SnapshotErrorCode::NoStore, "no MVCC store configured for training snapshots");
    std::string err;
    auto sel = SnapshotSelector::fromCanonicalJson(expected.query, &err);
    if (!sel) return fail(SnapshotErrorCode::InvalidSelector, err);
    if (sha256Hex(expected.query) != expected.query_sha256 || sel->collection != expected.collection) {
        return fail(SnapshotErrorCode::InvalidSelector, "snapshot selector does not match its fingerprint");
    }

    std::optional<std::string> lease;
    if (acquire_pin) {
        if (auto pins = store_->snapshotPinRegistry()) {
            auto pin = pins->acquire(HLCTimestamp(expected.hlc_ts), owner);
            if (!pin) return fail(SnapshotErrorCode::PinFailed, "failed to persist snapshot retention pin");
            lease = pin->lease_id;
        }
    }
    auto r = readAt(*sel, HLCTimestamp(expected.hlc_ts), expected.split_seed, expected.holdout_fraction,
                    expected.pinned);
    auto release = [&] {
        if (lease) releasePin(*lease);
    };
    if (!r.ok) {
        release();
        return r;
    }
    if (!r.fingerprint.sameData(expected)) {
        release();
        if (r.fingerprint.record_count < expected.record_count) {
            return fail(SnapshotErrorCode::SnapshotUnavailable,
                        "training snapshot at hlc_ts=" + std::to_string(expected.hlc_ts) +
                            " is no longer available: " + std::to_string(r.fingerprint.record_count) +
                            " of " + std::to_string(expected.record_count) +
                            " records readable (versions pruned or deleted)");
        }
        return fail(SnapshotErrorCode::FingerprintMismatch,
                    "training snapshot at hlc_ts=" + std::to_string(expected.hlc_ts) +
                        " does not reproduce: merkle_root " + r.fingerprint.merkle_root +
                        " != recorded " + expected.merkle_root);
    }
    r.lease_id = lease;
    return r;
}

bool TrainingSnapshotReader::releasePin(std::string_view lease_id) {
    if (!store_) return false;
    auto pins = store_->snapshotPinRegistry();
    return pins && pins->release(lease_id);
}

}  // namespace lora
}  // namespace llm
}  // namespace themis

/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_lora_training_snapshot_provenance.cpp
  Description:     WP H – MVCC training-data snapshot provenance:
                   Merkle fingerprint determinism, GC retention pins,
                   snapshot re-verification and adapter metadata signing.
╚═════════════════════════════════════════════════════════════════════╝
*/

#include <gtest/gtest.h>

#include "llm/lora_framework/adapter_provenance.h"
#include "llm/lora_framework/lora_safetensors_package.h"
#include "llm/lora_framework/training_snapshot.h"
#include "security/signing.h"
#include "storage/mvcc_snapshot_pins.h"
#include "storage/mvcc_store.h"
#include "storage/rocksdb_wrapper.h"

#include <nlohmann/json.hpp>
#include <openssl/evp.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using nlohmann::json;
using themis::HLCTimestamp;
using themis::MVCCSnapshotPinRegistry;
using themis::MVCCStore;
using themis::RocksDBWrapper;
using namespace themis::llm::lora;

namespace {

// ─── Real Ed25519 SigningService (OpenSSL EVP) ──────────────────────────────

class Ed25519SigningService final : public themis::SigningService {
public:
    Ed25519SigningService() {
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
        if (ctx && EVP_PKEY_keygen_init(ctx) == 1) EVP_PKEY_keygen(ctx, &key_);
        EVP_PKEY_CTX_free(ctx);
    }
    ~Ed25519SigningService() override { EVP_PKEY_free(key_); }
    Ed25519SigningService(const Ed25519SigningService&) = delete;
    Ed25519SigningService& operator=(const Ed25519SigningService&) = delete;

    themis::SigningResult sign(const std::vector<uint8_t>& data, const std::string& key_id) override {
        themis::SigningResult r;
        if (!key_ || key_id != kKeyId) { r.error = "unknown key"; return r; }
        EVP_MD_CTX* md = EVP_MD_CTX_new();
        size_t len = 0;
        if (md && EVP_DigestSignInit(md, nullptr, nullptr, nullptr, key_) == 1 &&
            EVP_DigestSign(md, nullptr, &len, data.data(), data.size()) == 1) {
            r.signature.resize(len);
            if (EVP_DigestSign(md, r.signature.data(), &len, data.data(), data.size()) == 1) {
                r.signature.resize(len);
                r.algorithm = "Ed25519";
            } else {
                r.signature.clear();
                r.error = "sign failed";
            }
        } else {
            r.error = "sign init failed";
        }
        EVP_MD_CTX_free(md);
        return r;
    }

    bool verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& sig,
                const std::string& key_id) override {
        if (!key_ || key_id != kKeyId) return false;
        EVP_MD_CTX* md = EVP_MD_CTX_new();
        bool ok = md && EVP_DigestVerifyInit(md, nullptr, nullptr, nullptr, key_) == 1 &&
                  EVP_DigestVerify(md, sig.data(), sig.size(), data.data(), data.size()) == 1;
        EVP_MD_CTX_free(md);
        return ok;
    }

    static constexpr const char* kKeyId = "lora-test-ed25519";

private:
    EVP_PKEY* key_ = nullptr;
};

std::vector<uint8_t> bytes(const std::string& s) { return {s.begin(), s.end()}; }

fs::path uniqueTempDir(const std::string& tag) {
    static std::atomic<int> counter{0};
    auto p = fs::temp_directory_path() /
             ("themis_wph_" + tag + "_" +
              std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "_" +
              std::to_string(counter++));
    fs::create_directories(p);
    return p;
}

std::shared_ptr<RocksDBWrapper> openDb(const fs::path& dir) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = dir.string();
    cfg.enable_wal = false;
    auto db = std::make_shared<RocksDBWrapper>(cfg);
    if (!db->open()) return nullptr;
    return db;
}

SnapshotRecordRef ref(const std::string& key, uint64_t ts, const std::string& value) {
    return SnapshotRecordRef{"docs", key, ts, sha256Digest(value)};
}

std::string sample(const std::string& in, const std::string& out) {
    return json{{"input", in}, {"output", out}}.dump();
}

// Minimal valid safetensors file: one F32 tensor of 2 elements.
void writeSafetensors(const fs::path& path, const std::map<std::string, std::string>& md,
                      const std::string& data) {
    json h;
    h["lora_A.weight"] = {{"dtype", "F32"}, {"shape", {2}}, {"data_offsets", {0, data.size()}}};
    json m = json::object();
    for (const auto& [k, v] : md) m[k] = v;
    h["__metadata__"] = m;
    std::string header = h.dump();
    while (header.size() % 8 != 0) header.push_back(' ');
    std::ofstream out(path, std::ios::binary);
    uint64_t n = header.size();
    for (int i = 0; i < 8; ++i) out.put(static_cast<char>((n >> (8 * i)) & 0xff));
    out.write(header.data(), static_cast<std::streamsize>(header.size()));
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

std::map<std::string, std::string> baseAdapterMetadata(const std::string& tensor_sha) {
    TrainingSnapshotFingerprint fp;
    fp.collection = "docs";
    fp.hlc_ts = 123456789;
    SnapshotSelector sel{"docs", 10};
    fp.query = sel.canonicalJson();
    fp.query_sha256 = sel.sha256();
    fp.record_count = 3;
    fp.merkle_root = std::string(64, 'a');
    fp.split_seed = 42;
    fp.holdout_fraction = 0.1;
    fp.pinned = true;
    auto md = fp.toMetadata();
    md["themis.adapter.id"] = "support-de";
    md["themis.adapter.method"] = "adalora";
    md["themis.adapter.base_model"] = "llama-3-8b";
    md["themis.adapter.rank"] = "16";
    md[provenance_keys::kTensorSha256] = tensor_sha;
    md["format"] = "pt";  // non-themis key: not signed
    return md;
}

}  // namespace

// ─── Merkle root ────────────────────────────────────────────────────────────

TEST(LoraSnapshotMerkle, DeterministicAndOrderIndependent) {
    std::vector<SnapshotRecordRef> rows = {
        ref("a", 10, "va"), ref("b", 11, "vb"), ref("c", 12, "vc"), ref("d", 13, "vd"), ref("e", 14, "ve")};
    auto r1 = SnapshotMerkleTree::root(rows);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(r1->size(), 64u);
    EXPECT_EQ(*r1, *SnapshotMerkleTree::root(rows));

    std::mt19937 rng(7);
    for (int i = 0; i < 10; ++i) {
        auto shuffled = rows;
        std::shuffle(shuffled.begin(), shuffled.end(), rng);
        EXPECT_EQ(*SnapshotMerkleTree::root(shuffled), *r1);
    }
}

TEST(LoraSnapshotMerkle, SensitiveToEveryLeafField) {
    std::vector<SnapshotRecordRef> rows = {ref("a", 10, "va"), ref("b", 11, "vb"), ref("c", 12, "vc")};
    const auto base = *SnapshotMerkleTree::root(rows);

    auto v = rows; v[1].value_sha256 = sha256Digest("vB");
    EXPECT_NE(*SnapshotMerkleTree::root(v), base);
    auto t = rows; t[1].version_ts = 99;
    EXPECT_NE(*SnapshotMerkleTree::root(t), base);
    auto k = rows; k[1].key = "bb";
    EXPECT_NE(*SnapshotMerkleTree::root(k), base);
    auto c = rows; for (auto& r : c) r.collection = "other";
    EXPECT_NE(*SnapshotMerkleTree::root(c), base);
    auto dropped = rows; dropped.pop_back();
    EXPECT_NE(*SnapshotMerkleTree::root(dropped), base);
}

TEST(LoraSnapshotMerkle, RejectsDuplicateKeysAndHandlesEmpty) {
    std::string err;
    EXPECT_FALSE(SnapshotMerkleTree::root({ref("a", 1, "x"), ref("a", 2, "y")}, &err).has_value());
    EXPECT_FALSE(err.empty());
    auto empty = SnapshotMerkleTree::root({});
    ASSERT_TRUE(empty.has_value());
    EXPECT_EQ(*empty, sha256Hex(""));
}

TEST(LoraSnapshotFingerprint, MetadataRoundTripAndQueryHashBinding) {
    auto md = baseAdapterMetadata(std::string(64, 'b'));
    std::string err;
    auto fp = TrainingSnapshotFingerprint::fromMetadata(md, &err);
    ASSERT_TRUE(fp.has_value()) << err;
    EXPECT_EQ(fp->toMetadata().at(snapshot_keys::kMerkleRoot), md.at(snapshot_keys::kMerkleRoot));
    EXPECT_DOUBLE_EQ(fp->holdout_fraction, 0.1);

    auto bad = md;
    bad[snapshot_keys::kQuerySha256] = std::string(64, '0');
    EXPECT_FALSE(TrainingSnapshotFingerprint::fromMetadata(bad, &err).has_value());
}

// ─── GC victim selection with pins ─────────────────────────────────────────

TEST(MvccSnapshotPins, SelectGcVictimsHonoursPins) {
    std::vector<HLCTimestamp> ts = {HLCTimestamp(10), HLCTimestamp(20), HLCTimestamp(30), HLCTimestamp(40)};
    auto none = MVCCStore::selectGcVictims(ts, HLCTimestamp(35), 1, {});
    EXPECT_EQ(none, (std::vector<size_t>{0, 1, 2}));

    // Pin at 25 protects the version visible at 25, i.e. ts=20.
    auto pinned = MVCCStore::selectGcVictims(ts, HLCTimestamp(35), 1, {HLCTimestamp(25)});
    EXPECT_EQ(pinned, (std::vector<size_t>{0, 2}));

    // Pin exactly on a version protects it; pin before all versions protects nothing.
    auto exact = MVCCStore::selectGcVictims(ts, HLCTimestamp(35), 1, {HLCTimestamp(10), HLCTimestamp(5)});
    EXPECT_EQ(exact, (std::vector<size_t>{1, 2}));
}

class MvccSnapshotStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = uniqueTempDir("store");
        db_ = openDb(dir_ / "db");
        ASSERT_TRUE(db_);
        store_ = std::make_shared<MVCCStore>(db_);
        pins_ = std::make_shared<MVCCSnapshotPinRegistry>(db_);
        store_->setSnapshotPinRegistry(pins_);
    }
    void TearDown() override {
        store_.reset();
        pins_.reset();
        db_.reset();
        std::error_code ec;
        fs::remove_all(dir_, ec);
    }

    std::size_t versionCount(const std::string& key) {
        std::size_t n = 0;
        store_->scanVersions(key, [&](const MVCCStore::VersionEntry&) { ++n; return true; });
        return n;
    }

    fs::path dir_;
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<MVCCStore> store_;
    std::shared_ptr<MVCCSnapshotPinRegistry> pins_;
};

TEST_F(MvccSnapshotStoreTest, PinnedSnapshotBlocksGcOfNeededVersions) {
    const std::string key = SnapshotSelector::keyPrefixFor("docs") + "r1";
    store_->putWithTimestamp(key, bytes(sample("q", "v1")), HLCTimestamp(1000));
    store_->putWithTimestamp(key, bytes(sample("q", "v2")), HLCTimestamp(2000));
    store_->putWithTimestamp(key, bytes(sample("q", "v3")), HLCTimestamp(3000));

    auto pin = pins_->acquire(HLCTimestamp(1500), "test");
    ASSERT_TRUE(pin.has_value());
    EXPECT_TRUE(pins_->isPinned(HLCTimestamp(1500)));

    EXPECT_EQ(store_->gcVersionsBefore(key, HLCTimestamp(5000)), 1u);  // only v2 removable
    auto at = store_->getVersionAtTimestamp(key, HLCTimestamp(1500));
    ASSERT_TRUE(at.has_value());
    EXPECT_EQ(at->timestamp.value, 1000u);

    // deleteVersions (pruner path) re-checks pins.
    EXPECT_EQ(store_->deleteVersions(key, {HLCTimestamp(1000)}), 0u);
    EXPECT_EQ(versionCount(key), 2u);

    ASSERT_TRUE(pins_->release(pin->lease_id));
    EXPECT_EQ(store_->gcVersionsBefore(key, HLCTimestamp(5000)), 1u);
    EXPECT_EQ(versionCount(key), 1u);
}

TEST_F(MvccSnapshotStoreTest, PinsArePersistedAcrossRegistryInstances) {
    auto pin = pins_->acquire(HLCTimestamp(4242), "lora:adapter-x");
    ASSERT_TRUE(pin.has_value());
    MVCCSnapshotPinRegistry reloaded(db_);
    auto found = reloaded.find(pin->lease_id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->ts.value, 4242u);
    EXPECT_EQ(found->owner, "lora:adapter-x");
    ASSERT_TRUE(reloaded.release(pin->lease_id));
    MVCCSnapshotPinRegistry again(db_);
    EXPECT_FALSE(again.find(pin->lease_id).has_value());
}

// ─── Snapshot capture / regeneration verification ──────────────────────────

TEST_F(MvccSnapshotStoreTest, CaptureVerifyAndAbortOnModifiedRecord) {
    const std::string prefix = SnapshotSelector::keyPrefixFor("docs");
    store_->put(prefix + "a", bytes(sample("qa", "aa")));
    store_->put(prefix + "b", bytes(sample("qb", "bb")));
    store_->put(prefix + "c", bytes(sample("qc", "cc")));

    TrainingSnapshotReader reader(store_);
    auto cap = reader.capture(SnapshotSelector{"docs", 100}, 42, 0.2, "lora:test");
    ASSERT_TRUE(cap.ok) << cap.error;
    ASSERT_TRUE(cap.lease_id.has_value());
    EXPECT_TRUE(cap.fingerprint.pinned);
    EXPECT_EQ(cap.fingerprint.record_count, 3u);
    EXPECT_EQ(cap.rows.size(), 3u);

    // Later writes do not change the snapshot at T.
    store_->put(prefix + "b", bytes(sample("qb", "CHANGED-after-T")));
    store_->put(prefix + "d", bytes(sample("qd", "dd")));
    auto again = reader.verify(cap.fingerprint, /*acquire_pin=*/false, "lora:regen");
    ASSERT_TRUE(again.ok) << again.error;
    EXPECT_TRUE(again.fingerprint.sameData(cap.fingerprint));

    // A record version that is visible at T is modified (simulated tamper:
    // rewrite the exact version key at its original timestamp).
    auto vb = store_->getVersionAtTimestamp(prefix + "b", HLCTimestamp(cap.fingerprint.hlc_ts));
    ASSERT_TRUE(vb.has_value());
    ASSERT_TRUE(db_->put(MVCCStore::encodeVersionedKey(prefix + "b", vb->timestamp),
                         bytes(sample("qb", "tampered"))));
    auto mismatch = reader.verify(cap.fingerprint, /*acquire_pin=*/true, "lora:regen");
    EXPECT_FALSE(mismatch.ok);
    EXPECT_EQ(mismatch.code, SnapshotErrorCode::FingerprintMismatch);
    EXPECT_FALSE(mismatch.lease_id.has_value());  // no lease leaked on failure

    EXPECT_TRUE(reader.releasePin(*cap.lease_id));
}

TEST_F(MvccSnapshotStoreTest, VerifyReportsUnavailableWhenSnapshotWasPruned) {
    const std::string prefix = SnapshotSelector::keyPrefixFor("docs");
    store_->putWithTimestamp(prefix + "a", bytes(sample("qa", "aa")), HLCTimestamp(1000));
    store_->putWithTimestamp(prefix + "b", bytes(sample("qb", "bb")), HLCTimestamp(1100));

    TrainingSnapshotReader reader(store_);
    auto at = reader.readAt(SnapshotSelector{"docs", 10}, HLCTimestamp(1200), 1, 0.1, false);
    ASSERT_TRUE(at.ok) << at.error;
    EXPECT_EQ(at.fingerprint.record_count, 2u);

    // Without a pin, newer versions + GC prune the version visible at 1200.
    store_->putWithTimestamp(prefix + "a", bytes(sample("qa", "a2")), HLCTimestamp(2000));
    EXPECT_EQ(store_->gcVersionsBefore(prefix + "a", HLCTimestamp(3000)), 1u);

    auto v = reader.verify(at.fingerprint, /*acquire_pin=*/true, "lora:regen");
    EXPECT_FALSE(v.ok);
    EXPECT_EQ(v.code, SnapshotErrorCode::SnapshotUnavailable);
    EXPECT_TRUE(pins_->list().empty());
}

// ─── Adapter provenance signing / verification ─────────────────────────────

class AdapterProvenanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        svc_ = std::make_shared<Ed25519SigningService>();
        signer_ = std::make_unique<AdapterProvenanceSigner>(svc_, Ed25519SigningService::kKeyId);
        verifier_ = std::make_unique<AdapterProvenanceVerifier>(
            svc_, std::set<std::string>{Ed25519SigningService::kKeyId});
        md_ = baseAdapterMetadata(sha256Hex(tensorData()));
        std::string err;
        ASSERT_TRUE(signer_->sign(md_, &err)) << err;
    }

    /// GGUF KV as the converter writes it (typed values, see THEMIS_KV_TYPES).
    json ggufKv() const {
        json kv = json::object();
        for (const auto& [k, v] : md_) {
            if (k.rfind("themis.", 0) != 0) continue;
            if (k == snapshot_keys::kRecordCount || k == "themis.adapter.rank") {
                kv[k] = static_cast<uint64_t>(std::stoull(v));
            } else if (k == snapshot_keys::kHoldoutFraction) {
                kv[k] = static_cast<double>(static_cast<float>(std::stod(v)));
            } else if (k == snapshot_keys::kPinned) {
                kv[k] = (v == "true");
            } else {
                kv[k] = v;
            }
        }
        kv["general.architecture"] = "llama";
        return kv;
    }

    /// Two little-endian F32 values (1.0f, 2.0f); contains NUL bytes.
    static std::string tensorData() { return std::string("\x00\x00\x80\x3f\x00\x00\x00\x40", 8); }
    std::shared_ptr<Ed25519SigningService> svc_;
    std::unique_ptr<AdapterProvenanceSigner> signer_;
    std::unique_ptr<AdapterProvenanceVerifier> verifier_;
    std::map<std::string, std::string> md_;
};

TEST_F(AdapterProvenanceTest, SignedMetadataVerifies) {
    auto v = verifier_->verifyMetadata(md_);
    EXPECT_EQ(v.status, ProvenanceVerification::Status::Verified) << v.error;
    EXPECT_EQ(v.key_id, Ed25519SigningService::kKeyId);
    EXPECT_EQ(md_.at(provenance_keys::kCanonicalSha256),
              sha256Hex(md_.at(provenance_keys::kCanonicalJson)));
    EXPECT_EQ(v.signed_fields.count("format"), 0u);  // non-themis keys are not signed
}

TEST_F(AdapterProvenanceTest, TamperingAnySignedKeyFailsVerification) {
    for (const auto& [k, v] : md_) {
        if (k.rfind("themis.", 0) != 0 || k.rfind(provenance_keys::kPrefix, 0) == 0) continue;
        auto tampered = md_;
        tampered[k] = v + "x";
        EXPECT_EQ(verifier_->verifyMetadata(tampered).status, ProvenanceVerification::Status::Invalid)
            << "tampered key " << k;
    }
    auto added = md_;
    added["themis.adapter.extra"] = "injected";
    EXPECT_EQ(verifier_->verifyMetadata(added).status, ProvenanceVerification::Status::Invalid);

    auto removed = md_;
    removed.erase(snapshot_keys::kMerkleRoot);
    EXPECT_EQ(verifier_->verifyMetadata(removed).status, ProvenanceVerification::Status::Invalid);

    auto sig = md_;
    sig[provenance_keys::kSignature][0] = sig[provenance_keys::kSignature][0] == '0' ? '1' : '0';
    EXPECT_EQ(verifier_->verifyMetadata(sig).status, ProvenanceVerification::Status::Invalid);
}

TEST_F(AdapterProvenanceTest, UnsignedAndUntrustedKeyHandling) {
    auto unsigned_md = baseAdapterMetadata(sha256Hex(tensorData()));
    auto u = verifier_->verifyMetadata(unsigned_md);
    EXPECT_EQ(u.status, ProvenanceVerification::Status::Unsigned);
    EXPECT_FALSE(u.acceptable(/*require_signed=*/true));
    EXPECT_TRUE(u.acceptable(/*require_signed=*/false));

    AdapterProvenanceVerifier strict(svc_, {"some-other-key"});
    EXPECT_EQ(strict.verifyMetadata(md_).status, ProvenanceVerification::Status::Invalid);

    AdapterProvenanceVerifier other_key(std::make_shared<Ed25519SigningService>());
    EXPECT_EQ(other_key.verifyMetadata(md_).status, ProvenanceVerification::Status::Invalid);
}

TEST_F(AdapterProvenanceTest, GgufKvVerificationIsBoundToSignedPayload) {
    auto kv = ggufKv();
    auto ok = verifier_->verifyGgufKv(kv);
    EXPECT_EQ(ok.status, ProvenanceVerification::Status::Verified) << ok.error;

    for (auto it = kv.begin(); it != kv.end(); ++it) {
        const std::string& k = it.key();
        if (k.rfind("themis.", 0) != 0 || k.rfind(provenance_keys::kPrefix, 0) == 0) continue;
        json t = kv;
        if (t[k].is_string()) t[k] = t[k].get<std::string>() + "x";
        else if (t[k].is_boolean()) t[k] = !t[k].get<bool>();
        else if (t[k].is_number_unsigned()) t[k] = t[k].get<uint64_t>() + 1;
        else t[k] = t[k].get<double>() + 0.5;
        EXPECT_EQ(verifier_->verifyGgufKv(t).status, ProvenanceVerification::Status::Invalid)
            << "tampered GGUF key " << k;
    }
    json extra = kv;
    extra["themis.adapter.extra"] = "x";
    EXPECT_EQ(verifier_->verifyGgufKv(extra).status, ProvenanceVerification::Status::Invalid);
    json missing = kv;
    missing.erase(snapshot_keys::kMerkleRoot);
    EXPECT_EQ(verifier_->verifyGgufKv(missing).status, ProvenanceVerification::Status::Invalid);
    json payload = kv;
    payload[provenance_keys::kCanonicalJson] = kv[provenance_keys::kCanonicalJson].get<std::string>() + " ";
    EXPECT_EQ(verifier_->verifyGgufKv(payload).status, ProvenanceVerification::Status::Invalid);
}

TEST_F(AdapterProvenanceTest, SafetensorsRewriteRoundTripAndTensorTamper) {
    const auto dir = uniqueTempDir("st");
    const auto file = dir / "adapter_model.safetensors";
    writeSafetensors(file, {{"format", "pt"}}, tensorData());
    ASSERT_TRUE(SafetensorsPackageValidator::inspectFile(file).valid);

    std::string err;
    ASSERT_TRUE(rewriteSafetensorsMetadata(file, md_, &err)) << err;
    auto insp = SafetensorsPackageValidator::inspectFile(file);
    ASSERT_TRUE(insp.valid) << insp.error;
    EXPECT_EQ(insp.metadata, md_);
    auto ok = verifier_->verifySafetensorsFile(file);
    EXPECT_EQ(ok.status, ProvenanceVerification::Status::Verified) << ok.error;

    // Flip one tensor byte (last byte of the file).
    {
        std::fstream f(file, std::ios::in | std::ios::out | std::ios::binary);
        f.seekp(-1, std::ios::end);
        f.put('\x41');
    }
    EXPECT_EQ(verifier_->verifySafetensorsFile(file).status, ProvenanceVerification::Status::Invalid);

    std::error_code ec;
    fs::remove_all(dir, ec);
}

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file lora_package_provenance.cpp
 * @brief Phase 4 HashChain & Provenance Layer — implementation.
 *
 * Implements ProvenanceHashLedger, LoRAPackage, AdapterProduct,
 * DistributionReceipt, ReceiptManifest, ShardLedgerEntry, and ReceiptChain
 * as specified in issue #5417.
 */

#include "llm/lora_framework/lora_package_provenance.h"

#include <openssl/sha.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// ISO 8601 UTC timestamp for the current moment (package-specific helper).
static std::string nowISO8601_pkg() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc{};
#ifdef _WIN32
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    std::ostringstream ss = {};
    ss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

/// Generate a UUID-like identifier from two random 64-bit values (package-specific helper).
static std::string generateId_pkg() {
    static std::mutex id_mu;
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    std::lock_guard<std::mutex> lock(id_mu);
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    oss << std::setw(16) << dis(gen);
    oss << std::setw(16) << dis(gen);
    return oss.str();
}

/// Compute SHA-256 of a string; return lowercase hex.
static std::string sha256Hex(const std::string& data) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()),
           data.size(), digest);
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (auto byte : digest) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

/// Compute SHA-256 of the concatenation of two hex-string hash values.
static std::string hashPair(const std::string& left, const std::string& right) {
    return sha256Hex(left + right);
}

/// Compute a binary Merkle root from a list of leaf hashes.
/// Returns sha256Hex("") when leaves is empty.
static std::string computeMerkleRoot(std::vector<std::string> leaves) {
    if (leaves.empty()) {
        return sha256Hex("");
    }
    // Pad to even number of leaves by duplicating the last one.
    while (static_cast<int>(leaves.size()) > 1 && (leaves.size() % 2 != 0)) {
        leaves.push_back(leaves.back());
    }
    while (static_cast<int>(leaves.size()) > 1) {
        std::vector<std::string> next = {};

        next.reserve(leaves.size() / 2);
        for (std::size_t i = 0; i + 1 <static_cast<int>(leaves.size()); i += 2) {
            next.push_back(hashPair(leaves[i], leaves[i + 1]));
        }
        leaves = std::move(next);
        // Pad again if needed.
        if (static_cast<int>(leaves.size()) > 1 && (leaves.size() % 2 != 0)) {
            leaves.push_back(leaves.back());
        }
    }
    return leaves.front();
}

/// Safe JSON string getter.
static void getStr(const json& j, const char* key, std::string& dest) {
    if (j.contains(key) && j[key].is_string()) {
        dest = j[key].get<std::string>();
    }
}

} // anonymous namespace

// ============================================================================
// ProvenanceHashLedger::sha256Hex (public utility)
// ============================================================================

/*static*/ std::string ProvenanceHashLedger::sha256Hex(const std::string& data) {
    return ::themis::llm::lora::sha256Hex(data);
}

// ============================================================================
// LoRAPackage — serialisation and hash
// ============================================================================

json LoRAPackage::toJSON() const {
    return json{
        {"package_id",    package_id},
        {"adapter_id",    adapter_id},
        {"version",       version},
        {"format",        format},
        {"weights_hash",  weights_hash},
        {"package_hash",  package_hash},
        {"parent_hash",   parent_hash},
        {"created_at",    created_at},
        {"metadata",      metadata}
    };
}

/*static*/ LoRAPackage LoRAPackage::fromJSON(const json& j) {
    LoRAPackage p;
    getStr(j, "package_id",   p.package_id);
    getStr(j, "adapter_id",   p.adapter_id);
    getStr(j, "version",      p.version);
    getStr(j, "format",       p.format);
    getStr(j, "weights_hash", p.weights_hash);
    getStr(j, "package_hash", p.package_hash);
    getStr(j, "parent_hash",  p.parent_hash);
    getStr(j, "created_at",   p.created_at);
    if (j.contains("metadata") && j["metadata"].is_object()) {
        p.metadata = j["metadata"];
    }
    return p;
}

std::string LoRAPackage::computeContentHash() const {
    // Canonical input: stable JSON serialisation of content fields.
    json canonical{
        {"adapter_id",   adapter_id},
        {"version",      version},
        {"format",       format},
        {"weights_hash", weights_hash},
        {"parent_hash",  parent_hash},
        {"created_at",   created_at},
        {"metadata",     metadata}
    };
    return sha256Hex(canonical.dump());
}

// ============================================================================
// AdapterProduct — serialisation and hash
// ============================================================================

json AdapterProduct::toJSON() const {
    return json{
        {"product_id",           product_id},
        {"package_id",           package_id},
        {"base_model_id",        base_model_id},
        {"base_model_hash",      base_model_hash},
        {"product_hash",         product_hash},
        {"parent_hash",          parent_hash},
        {"compatibility_hash",   compatibility_hash},
        {"created_at",           created_at},
        {"deployment_metadata",  deployment_metadata}
    };
}

/*static*/ AdapterProduct AdapterProduct::fromJSON(const json& j) {
    AdapterProduct p;
    getStr(j, "product_id",          p.product_id);
    getStr(j, "package_id",          p.package_id);
    getStr(j, "base_model_id",       p.base_model_id);
    getStr(j, "base_model_hash",     p.base_model_hash);
    getStr(j, "product_hash",        p.product_hash);
    getStr(j, "parent_hash",         p.parent_hash);
    getStr(j, "compatibility_hash",  p.compatibility_hash);
    getStr(j, "created_at",          p.created_at);
    if (j.contains("deployment_metadata") && j["deployment_metadata"].is_object()) {
        p.deployment_metadata = j["deployment_metadata"];
    }
    return p;
}

std::string AdapterProduct::computeContentHash() const {
    json canonical{
        {"package_id",          package_id},
        {"base_model_id",       base_model_id},
        {"base_model_hash",     base_model_hash},
        {"parent_hash",         parent_hash},
        {"compatibility_hash",  compatibility_hash},
        {"created_at",          created_at},
        {"deployment_metadata", deployment_metadata}
    };
    return sha256Hex(canonical.dump());
}

// ============================================================================
// DistributionReceipt — serialisation and hash
// ============================================================================

json DistributionReceipt::toJSON() const {
    return json{
        {"receipt_id",              receipt_id},
        {"event_type",              event_type},
        {"artifact_id",             artifact_id},
        {"artifact_hash",           artifact_hash},
        {"recipient_id",            recipient_id},
        {"distribution_timestamp",  distribution_timestamp},
        {"operator_signature",      operator_signature},
        {"parent_receipt_hash",     parent_receipt_hash},
        {"receipt_hash",            receipt_hash},
        {"extra_metadata",          extra_metadata}
    };
}

/*static*/ DistributionReceipt DistributionReceipt::fromJSON(const json& j) {
    DistributionReceipt r;
    getStr(j, "receipt_id",             r.receipt_id);
    getStr(j, "event_type",             r.event_type);
    getStr(j, "artifact_id",            r.artifact_id);
    getStr(j, "artifact_hash",          r.artifact_hash);
    getStr(j, "recipient_id",           r.recipient_id);
    getStr(j, "distribution_timestamp", r.distribution_timestamp);
    getStr(j, "operator_signature",     r.operator_signature);
    getStr(j, "parent_receipt_hash",    r.parent_receipt_hash);
    getStr(j, "receipt_hash",           r.receipt_hash);
    if (j.contains("extra_metadata") && j["extra_metadata"].is_object()) {
        r.extra_metadata = j["extra_metadata"];
    }
    return r;
}

std::string DistributionReceipt::computeContentHash() const {
    json canonical{
        {"event_type",             event_type},
        {"artifact_id",            artifact_id},
        {"artifact_hash",          artifact_hash},
        {"recipient_id",           recipient_id},
        {"distribution_timestamp", distribution_timestamp},
        {"operator_signature",     operator_signature},
        {"parent_receipt_hash",    parent_receipt_hash},
        {"extra_metadata",         extra_metadata}
    };
    return sha256Hex(canonical.dump());
}

// ============================================================================
// ReceiptManifest — serialisation, Merkle root, and hash
// ============================================================================

json ReceiptManifest::toJSON() const {
    json receipts_json = json::array();
    for (const auto& r : receipts) {
        receipts_json.push_back(r.toJSON());
    }
    return json{
        {"manifest_id",   manifest_id},
        {"event_type",    event_type},
        {"artifact_id",   artifact_id},
        {"created_at",    created_at},
        {"receipts",      receipts_json},
        {"merkle_root",   merkle_root},
        {"manifest_hash", manifest_hash}
    };
}

/*static*/ ReceiptManifest ReceiptManifest::fromJSON(const json& j) {
    ReceiptManifest m;
    getStr(j, "manifest_id",   m.manifest_id);
    getStr(j, "event_type",    m.event_type);
    getStr(j, "artifact_id",   m.artifact_id);
    getStr(j, "created_at",    m.created_at);
    getStr(j, "merkle_root",   m.merkle_root);
    getStr(j, "manifest_hash", m.manifest_hash);
    if (j.contains("receipts") && j["receipts"].is_array()) {
        for (const auto& rj : j["receipts"]) {
            m.receipts.push_back(DistributionReceipt::fromJSON(rj));
        }
    }
    return m;
}

std::string ReceiptManifest::computeMerkleRoot() const {
    std::vector<std::string> leaves = {};

    leaves.reserve(receipts.size());
    for (const auto& r : receipts) {
        leaves.push_back(r.receipt_hash.empty() ? r.computeContentHash()
                                                 : r.receipt_hash);
    }
    return ::themis::llm::lora::computeMerkleRoot(std::move(leaves));
}

std::string ReceiptManifest::computeContentHash() const {
    // Build a stable JSON with all receipt hashes in order.
    json receipt_hashes_json = json::array();
    for (const auto& r : receipts) {
        receipt_hashes_json.push_back(
            r.receipt_hash.empty() ? r.computeContentHash() : r.receipt_hash);
    }
    json canonical{
        {"manifest_id",   manifest_id},
        {"event_type",    event_type},
        {"artifact_id",   artifact_id},
        {"created_at",    created_at},
        {"receipt_hashes", receipt_hashes_json},
        {"merkle_root",   merkle_root}
    };
    return sha256Hex(canonical.dump());
}

// ============================================================================
// ShardLedgerEntry — serialisation and hash
// ============================================================================

json ShardLedgerEntry::toJSON() const {
    json proof_json = json::array();
    for (const auto& h : merkle_proof) {
        proof_json.push_back(h);
    }
    return json{
        {"entry_id",             entry_id},
        {"artifact_id",          artifact_id},
        {"shard_id",             shard_id},
        {"shard_index",          shard_index},
        {"total_shards",         total_shards},
        {"shard_hash",           shard_hash},
        {"merkle_proof",         proof_json},
        {"placement_timestamp",  placement_timestamp},
        {"prev_entry_hash",      prev_entry_hash},
        {"entry_hash",           entry_hash}
    };
}

/*static*/ ShardLedgerEntry ShardLedgerEntry::fromJSON(const json& j) {
    ShardLedgerEntry e;
    getStr(j, "entry_id",            e.entry_id);
    getStr(j, "artifact_id",         e.artifact_id);
    getStr(j, "shard_id",            e.shard_id);
    getStr(j, "shard_hash",          e.shard_hash);
    getStr(j, "placement_timestamp", e.placement_timestamp);
    getStr(j, "prev_entry_hash",     e.prev_entry_hash);
    getStr(j, "entry_hash",          e.entry_hash);
    if (j.contains("shard_index") && j["shard_index"].is_number_unsigned()) {
        e.shard_index = j["shard_index"].get<uint32_t>();
    }
    if (j.contains("total_shards") && j["total_shards"].is_number_unsigned()) {
        e.total_shards = j["total_shards"].get<uint32_t>();
    }
    if (j.contains("merkle_proof") && j["merkle_proof"].is_array()) {
        for (const auto& h : j["merkle_proof"]) {
            if (h.is_string()) {
                e.merkle_proof.push_back(h.get<std::string>());
            }
        }
    }
    return e;
}

std::string ShardLedgerEntry::computeContentHash() const {
    json proof_json = json::array();
    for (const auto& h : merkle_proof) {
        proof_json.push_back(h);
    }
    json canonical{
        {"artifact_id",         artifact_id},
        {"shard_id",            shard_id},
        {"shard_index",         shard_index},
        {"total_shards",        total_shards},
        {"shard_hash",          shard_hash},
        {"merkle_proof",        proof_json},
        {"placement_timestamp", placement_timestamp},
        {"prev_entry_hash",     prev_entry_hash}
    };
    return sha256Hex(canonical.dump());
}

// ============================================================================
// ReceiptChain — serialisation
// ============================================================================

json ReceiptChain::toJSON() const {
    json entries_json = json::array();
    for (const auto& e : entries) {
        entries_json.push_back(e.toJSON());
    }
    return json{
        {"chain_id",     chain_id},
        {"artifact_id",  artifact_id},
        {"genesis_hash", genesis_hash},
        {"entries",      entries_json},
        {"head_hash",    head_hash}
    };
}

/*static*/ ReceiptChain ReceiptChain::fromJSON(const json& j) {
    ReceiptChain c;
    getStr(j, "chain_id",     c.chain_id);
    getStr(j, "artifact_id",  c.artifact_id);
    getStr(j, "genesis_hash", c.genesis_hash);
    getStr(j, "head_hash",    c.head_hash);
    if (j.contains("entries") && j["entries"].is_array()) {
        for (const auto& ej : j["entries"]) {
            c.entries.push_back(DistributionReceipt::fromJSON(ej));
        }
    }
    return c;
}

// ============================================================================
// ProvenanceHashLedger — internal state
// ============================================================================

struct ProvenanceHashLedger::Impl {
    mutable std::mutex mu;

    // package_id -> LoRAPackage (per-package)
    std::unordered_map<std::string, LoRAPackage> packages_by_id;
    // adapter_id -> ordered chain of LoRAPackages
    std::unordered_map<std::string, std::vector<LoRAPackage>> package_chains;

    // product_id -> AdapterProduct
    std::unordered_map<std::string, AdapterProduct> products_by_id;
    // package_id -> ordered chain of AdapterProducts
    std::unordered_map<std::string, std::vector<AdapterProduct>> product_chains;

    // artifact_id -> ReceiptChain
    std::unordered_map<std::string, ReceiptChain> receipt_chains;

    // manifest_id -> ReceiptManifest
    std::unordered_map<std::string, ReceiptManifest> manifests;

    // artifact_id -> ordered ShardLedgerEntry chain
    std::unordered_map<std::string, std::vector<ShardLedgerEntry>> shard_ledgers;
};

// ============================================================================
// ProvenanceHashLedger — construction / destruction
// ============================================================================

ProvenanceHashLedger::ProvenanceHashLedger()
    : impl_(std::make_unique<Impl>()) {}

ProvenanceHashLedger::~ProvenanceHashLedger() = default;

// ============================================================================
// ProvenanceHashLedger — LoRAPackage lifecycle
// ============================================================================

LoRAPackage ProvenanceHashLedger::appendPackage(LoRAPackage pkg) {
    std::lock_guard<std::mutex> lock(impl_->mu);

    if (pkg.package_id.empty()) {
        pkg.package_id = generateId_pkg();
    }
    if (pkg.created_at.empty()) {
        pkg.created_at = nowISO8601_pkg();
    }

    // Set parent_hash from the current head of this adapter's chain.
    auto& chain = impl_->package_chains[pkg.adapter_id];
    if (!chain.empty()) {
        pkg.parent_hash = chain.back().package_hash;
    } else {
        // Genesis: parent_hash is empty for the first package in this adapter's chain.
        pkg.parent_hash = "";
    }

    pkg.package_hash = pkg.computeContentHash();

    chain.push_back(pkg);
    impl_->packages_by_id[pkg.package_id] = pkg;

    spdlog::debug("ProvenanceHashLedger: appended package '{}' for adapter '{}'",
                  pkg.package_id, pkg.adapter_id);
    return pkg;
}

std::vector<LoRAPackage> ProvenanceHashLedger::getPackageChain(
    const std::string& adapter_id) const
{
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->package_chains.find(adapter_id);
    if (it == impl_->package_chains.end()) {
        return {};
    }
    return it->second;
}

std::optional<LoRAPackage> ProvenanceHashLedger::getPackage(
    const std::string& package_id) const
{
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->packages_by_id.find(package_id);
    if (it == impl_->packages_by_id.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ============================================================================
// ProvenanceHashLedger — AdapterProduct lifecycle
// ============================================================================

AdapterProduct ProvenanceHashLedger::appendProduct(AdapterProduct product) {
    std::lock_guard<std::mutex> lock(impl_->mu);

    if (product.product_id.empty()) {
        product.product_id = generateId_pkg();
    }
    if (product.created_at.empty()) {
        product.created_at = nowISO8601_pkg();
    }

    auto& chain = impl_->product_chains[product.package_id];
    if (!chain.empty()) {
        product.parent_hash = chain.back().product_hash;
    } else {
        product.parent_hash = "";
    }

    product.product_hash = product.computeContentHash();

    chain.push_back(product);
    impl_->products_by_id[product.product_id] = product;

    spdlog::debug("ProvenanceHashLedger: appended product '{}' for package '{}'",
                  product.product_id, product.package_id);
    return product;
}

std::vector<AdapterProduct> ProvenanceHashLedger::getProductChain(
    const std::string& package_id) const
{
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->product_chains.find(package_id);
    if (it == impl_->product_chains.end()) {
        return {};
    }
    return it->second;
}

std::optional<AdapterProduct> ProvenanceHashLedger::getProduct(
    const std::string& product_id) const
{
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->products_by_id.find(product_id);
    if (it == impl_->products_by_id.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ============================================================================
// ProvenanceHashLedger — DistributionReceipt and ReceiptChain
// ============================================================================

DistributionReceipt ProvenanceHashLedger::appendReceipt(DistributionReceipt receipt) {
    if (receipt.artifact_id.empty()) {
        throw std::invalid_argument(
            "ProvenanceHashLedger::appendReceipt: artifact_id must be non-empty");
    }

    std::lock_guard<std::mutex> lock(impl_->mu);

    if (receipt.receipt_id.empty()) {
        receipt.receipt_id = generateId_pkg();
    }
    if (receipt.distribution_timestamp.empty()) {
        receipt.distribution_timestamp = nowISO8601_pkg();
    }

    // Initialise or update the receipt chain for this artifact.
    auto& chain = impl_->receipt_chains[receipt.artifact_id];
    if (chain.chain_id.empty()) {
        // Genesis chain.
        chain.chain_id    = receipt.artifact_id;
        chain.artifact_id = receipt.artifact_id;
        chain.genesis_hash = sha256Hex(receipt.artifact_id);
        chain.head_hash   = chain.genesis_hash;
    }

    receipt.parent_receipt_hash = chain.head_hash;
    receipt.receipt_hash        = receipt.computeContentHash();

    chain.entries.push_back(receipt);
    chain.head_hash = receipt.receipt_hash;

    spdlog::debug(
        "ProvenanceHashLedger: appended receipt '{}' to chain for artifact '{}'",
        receipt.receipt_id, receipt.artifact_id);
    return receipt;
}

ReceiptChain ProvenanceHashLedger::getReceiptChain(
    const std::string& artifact_id) const
{
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->receipt_chains.find(artifact_id);
    if (it == impl_->receipt_chains.end()) {
        ReceiptChain empty;
        empty.chain_id    = artifact_id;
        empty.artifact_id = artifact_id;
        return empty;
    }
    return it->second;
}

bool ProvenanceHashLedger::verifyReceiptChain(
    const std::string& artifact_id) const
{
    std::lock_guard<std::mutex> lock(impl_->mu);

    auto it = impl_->receipt_chains.find(artifact_id);
    if (it == impl_->receipt_chains.end()) {
        // No chain registered — nothing to verify; treat as valid.
        return true;
    }
    const auto& chain = it->second;

    std::string expected_parent = chain.genesis_hash;
    for (const auto& receipt : chain.entries) {
        // Verify parent linkage.
        if (receipt.parent_receipt_hash != expected_parent) {
            spdlog::warn(
                "ProvenanceHashLedger::verifyReceiptChain: parent hash mismatch "
                "for receipt '{}' in chain '{}'",
                receipt.receipt_id, artifact_id);
            return false;
        }
        // Recompute and verify the receipt's own hash.
        const std::string recomputed = receipt.computeContentHash();
        if (recomputed != receipt.receipt_hash) {
            spdlog::warn(
                "ProvenanceHashLedger::verifyReceiptChain: receipt hash mismatch "
                "for receipt '{}' in chain '{}'",
                receipt.receipt_id, artifact_id);
            return false;
        }
        expected_parent = receipt.receipt_hash;
    }
    return true;
}

// ============================================================================
// ProvenanceHashLedger — ReceiptManifest
// ============================================================================

ReceiptManifest ProvenanceHashLedger::createManifest(
    const std::string&               event_type,
    const std::string&               artifact_id,
    std::vector<DistributionReceipt> receipts)
{
    if (event_type.empty()) {
        throw std::invalid_argument(
            "ProvenanceHashLedger::createManifest: event_type must be non-empty");
    }
    if (artifact_id.empty()) {
        throw std::invalid_argument(
            "ProvenanceHashLedger::createManifest: artifact_id must be non-empty");
    }
    ReceiptManifest manifest;
    manifest.manifest_id = generateId_pkg();
    manifest.event_type  = event_type;
    manifest.artifact_id = artifact_id;
    manifest.created_at  = nowISO8601_pkg();

    // Append each receipt to the artifact's chain so they are durably linked.
    // We do this without the outer mutex held to avoid re-entrancy; appendReceipt
    // takes the lock internally.  We accumulate the populated receipts.
    std::vector<DistributionReceipt> populated = {};

    populated.reserve(receipts.size());
    for (auto& r : receipts) {
        if (r.artifact_id.empty()) {
            r.artifact_id = artifact_id;
        }
        if (r.event_type.empty()) {
            r.event_type = event_type;
        }
        populated.push_back(appendReceipt(std::move(r)));
    }
    manifest.receipts = std::move(populated);

    // Compute Merkle root and manifest hash.
    manifest.merkle_root   = manifest.computeMerkleRoot();
    manifest.manifest_hash = manifest.computeContentHash();

    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        impl_->manifests[manifest.manifest_id] = manifest;
    }

    spdlog::debug(
        "ProvenanceHashLedger: created manifest '{}' for artifact '{}'",
        manifest.manifest_id, artifact_id);
    return manifest;
}

std::optional<ReceiptManifest> ProvenanceHashLedger::getManifest(
    const std::string& manifest_id) const
{
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->manifests.find(manifest_id);
    if (it == impl_->manifests.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool ProvenanceHashLedger::validateManifest(
    const ReceiptManifest& manifest) const
{
    // Verify Merkle root.
    const std::string expected_merkle = manifest.computeMerkleRoot();
    if (expected_merkle != manifest.merkle_root) {
        spdlog::warn(
            "ProvenanceHashLedger::validateManifest: Merkle root mismatch for '{}'",
            manifest.manifest_id);
        return false;
    }
    // Verify manifest hash.
    const std::string expected_hash = manifest.computeContentHash();
    if (expected_hash != manifest.manifest_hash) {
        spdlog::warn(
            "ProvenanceHashLedger::validateManifest: manifest hash mismatch for '{}'",
            manifest.manifest_id);
        return false;
    }
    return true;
}

// ============================================================================
// ProvenanceHashLedger — ShardLedger
// ============================================================================

ShardLedgerEntry ProvenanceHashLedger::appendShardEntry(ShardLedgerEntry entry) {
    if (entry.artifact_id.empty()) {
        throw std::invalid_argument(
            "ProvenanceHashLedger::appendShardEntry: artifact_id must be non-empty");
    }

    std::lock_guard<std::mutex> lock(impl_->mu);

    if (entry.entry_id.empty()) {
        entry.entry_id = generateId_pkg();
    }
    if (entry.placement_timestamp.empty()) {
        entry.placement_timestamp = nowISO8601_pkg();
    }

    auto& ledger = impl_->shard_ledgers[entry.artifact_id];
    if (!ledger.empty()) {
        entry.prev_entry_hash = ledger.back().entry_hash;
    } else {
        // Genesis: seed from artifact_id.
        entry.prev_entry_hash = sha256Hex(entry.artifact_id + "_shard_genesis");
    }

    entry.entry_hash = entry.computeContentHash();
    ledger.push_back(entry);

    spdlog::debug(
        "ProvenanceHashLedger: appended shard entry '{}' (shard {}/{}) for artifact '{}'",
        entry.entry_id, entry.shard_index, entry.total_shards, entry.artifact_id);
    return entry;
}

std::vector<ShardLedgerEntry> ProvenanceHashLedger::getShardLedger(
    const std::string& artifact_id) const
{
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->shard_ledgers.find(artifact_id);
    if (it == impl_->shard_ledgers.end()) {
        return {};
    }
    return it->second;
}

bool ProvenanceHashLedger::verifyShardLedger(
    const std::string& artifact_id) const
{
    std::lock_guard<std::mutex> lock(impl_->mu);

    auto it = impl_->shard_ledgers.find(artifact_id);
    if (it == impl_->shard_ledgers.end()) {
        return true;  // Nothing to verify.
    }
    const auto& ledger = it->second;

    // Reconstruct expected genesis prev_entry_hash.
    std::string expected_prev = sha256Hex(artifact_id + "_shard_genesis");

    for (const auto& entry : ledger) {
        if (entry.prev_entry_hash != expected_prev) {
            spdlog::warn(
                "ProvenanceHashLedger::verifyShardLedger: prev_entry_hash mismatch "
                "for entry '{}' in ledger '{}'",
                entry.entry_id, artifact_id);
            return false;
        }
        const std::string recomputed = entry.computeContentHash();
        if (recomputed != entry.entry_hash) {
            spdlog::warn(
                "ProvenanceHashLedger::verifyShardLedger: entry hash mismatch "
                "for entry '{}' in ledger '{}'",
                entry.entry_id, artifact_id);
            return false;
        }
        expected_prev = entry.entry_hash;
    }
    return true;
}

// ============================================================================
// ProvenanceHashLedger — audit path export
// ============================================================================

json ProvenanceHashLedger::exportAuditPath(
    const std::string& artifact_id) const
{
    std::lock_guard<std::mutex> lock(impl_->mu);

    // Resolve the supplied artifact_id to its canonical adapter_id and, when
    // the caller has given a package_id or product_id, to the specific
    // package_id whose products we want.
    //
    // Keying overview:
    //   package_chains : adapter_id  → [LoRAPackage …]
    //   product_chains : package_id  → [AdapterProduct …]
    //   packages_by_id : package_id  → LoRAPackage
    //   products_by_id : product_id  → AdapterProduct
    std::string resolved_adapter_id = {};
    std::string resolved_package_id; // non-empty → filter products to one package

    if (impl_->package_chains.count(artifact_id)) {
        // artifact_id is an adapter_id.
        resolved_adapter_id = artifact_id;
    } else if (impl_->packages_by_id.count(artifact_id)) {
        // artifact_id is a package_id — resolve back to adapter_id.
        resolved_adapter_id = impl_->packages_by_id.at(artifact_id).adapter_id;
        resolved_package_id = artifact_id;
    } else if (impl_->products_by_id.count(artifact_id)) {
        // artifact_id is a product_id — resolve via package → adapter.
        const auto& product = impl_->products_by_id.at(artifact_id);
        resolved_package_id = product.package_id;
        auto pkg_it = impl_->packages_by_id.find(resolved_package_id);
        if (pkg_it != impl_->packages_by_id.end()) {
            resolved_adapter_id = pkg_it->second.adapter_id;
        }
    }

    // Collect package chain for the resolved adapter.
    json packages_json = json::array();
    if (!resolved_adapter_id.empty()) {
        auto pit = impl_->package_chains.find(resolved_adapter_id);
        if (pit != impl_->package_chains.end()) {
            for (const auto& pkg : pit->second) {
                packages_json.push_back(pkg.toJSON());
            }
        }
    }

    // Collect product chain(s).
    // When given an adapter_id, include products for every package in the
    // adapter's chain.  When given a package_id or product_id, scope to that
    // specific package only.
    json products_json = json::array();
    if (!resolved_package_id.empty()) {
        auto pit = impl_->product_chains.find(resolved_package_id);
        if (pit != impl_->product_chains.end()) {
            for (const auto& prod : pit->second) {
                products_json.push_back(prod.toJSON());
            }
        }
    } else if (!resolved_adapter_id.empty()) {
        auto chain_it = impl_->package_chains.find(resolved_adapter_id);
        if (chain_it != impl_->package_chains.end()) {
            for (const auto& pkg : chain_it->second) {
                auto pit = impl_->product_chains.find(pkg.package_id);
                if (pit != impl_->product_chains.end()) {
                    for (const auto& prod : pit->second) {
                        products_json.push_back(prod.toJSON());
                    }
                }
            }
        }
    }

    // Receipt chain (looked up directly by the supplied artifact_id).
    json receipt_chain_json = json::object();
    {
        auto rit = impl_->receipt_chains.find(artifact_id);
        if (rit != impl_->receipt_chains.end()) {
            receipt_chain_json = rit->second.toJSON();
        }
    }

    // Manifests for this artifact.
    json manifests_json = json::array();
    for (const auto& [mid, manifest] : impl_->manifests) {
        if (manifest.artifact_id == artifact_id) {
            manifests_json.push_back(manifest.toJSON());
        }
    }

    // Shard ledger (looked up directly by the supplied artifact_id).
    json shard_ledger_json = json::array();
    {
        auto sit = impl_->shard_ledgers.find(artifact_id);
        if (sit != impl_->shard_ledgers.end()) {
            for (const auto& entry : sit->second) {
                shard_ledger_json.push_back(entry.toJSON());
            }
        }
    }

    return json{
        {"artifact_id",    artifact_id},
        {"packages",       packages_json},
        {"products",       products_json},
        {"receipt_chain",  receipt_chain_json},
        {"manifests",      manifests_json},
        {"shard_ledger",   shard_ledger_json}
    };
}

} // namespace lora
} // namespace llm
} // namespace themis

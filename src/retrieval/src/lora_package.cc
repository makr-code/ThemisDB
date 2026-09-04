/**
 * @file lora_package.cc
 * @brief Implementation of LoRAPackage, PortableAdapterProduct, and
 *        LoRAManifestStore artifact classes (Phase 3 manifest & serialization).
 */

#include "lora_package.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <iomanip>

namespace themis {
namespace retrieval {

// ============================================================================
// Internal: portable SHA-256 (FIPS 180-4)
// No third-party crypto dependency; suitable for manifest hashing.
// ============================================================================

namespace {

// --- SHA-256 constants -------------------------------------------------------
static constexpr std::array<uint32_t, 64> kK = {{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
}};

inline uint32_t rotr32(uint32_t x, unsigned n) noexcept {
    return (x >> n) | (x << (32u - n));
}

struct Sha256State {
    uint32_t h[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    };
    uint64_t bitlen = 0;
    uint8_t  buf[64]{};
    uint32_t buflen = 0;

    void processBlock(const uint8_t block[64]) noexcept {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(block[i * 4])     << 24u) |
                   (static_cast<uint32_t>(block[i * 4 + 1]) << 16u) |
                   (static_cast<uint32_t>(block[i * 4 + 2]) <<  8u) |
                    static_cast<uint32_t>(block[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15], 18) ^ (w[i-15] >> 3u);
            uint32_t s1 = rotr32(w[i-2],  17) ^ rotr32(w[i-2],  19) ^ (w[i-2] >> 10u);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t S1  = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            uint32_t ch  = (e & f) ^ (~e & g);
            uint32_t tmp1 = hh + S1 + ch + kK[static_cast<size_t>(i)] + w[i];
            uint32_t S0  = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t tmp2 = S0 + maj;
            hh = g; g = f; f = e;
            e  = d + tmp1;
            d  = c; c = b; b = a;
            a  = tmp1 + tmp2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    void update(const uint8_t* data, size_t len) noexcept {
        bitlen += static_cast<uint64_t>(len) * 8u;
        size_t off = 0;
        if (buflen > 0) {
            size_t fill = std::min(static_cast<size_t>(64u - buflen), len);
            std::memcpy(buf + buflen, data, fill);
            buflen += static_cast<uint32_t>(fill);
            off    += fill;
            if (buflen == 64u) { processBlock(buf); buflen = 0; }
        }
        while (off + 64 <= len) {
            processBlock(data + off);
            off += 64;
        }
        if (off < len) {
            std::memcpy(buf, data + off, len - off);
            buflen = static_cast<uint32_t>(len - off);
        }
    }

    std::array<uint8_t, 32> finalise() noexcept {
        buf[buflen++] = 0x80u;
        if (buflen > 56) {
            while (buflen < 64) {
              buf[buflen++] = 0;
            }
            processBlock(buf);
            buflen = 0;
        }
        while (buflen < 56) {
          buf[buflen++] = 0;
        }
        for (int i = 7; i >= 0; --i) {
            buf[buflen++] = static_cast<uint8_t>((bitlen >> (static_cast<unsigned>(i) * 8u)) & 0xffu);
        }
        processBlock(buf);
        std::array<uint8_t, 32> digest{};
        for (int i = 0; i < 8; ++i) {
            digest[static_cast<size_t>(i) * 4 + 0] = static_cast<uint8_t>(h[i] >> 24u);
            digest[static_cast<size_t>(i) * 4 + 1] = static_cast<uint8_t>(h[i] >> 16u);
            digest[static_cast<size_t>(i) * 4 + 2] = static_cast<uint8_t>(h[i] >>  8u);
            digest[static_cast<size_t>(i) * 4 + 3] = static_cast<uint8_t>(h[i]        );
        }
        return digest;
    }
};

std::string bytesToHex(const std::array<uint8_t, 32>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : bytes) {
        oss << std::setw(2) << static_cast<unsigned>(b);
    }
    return oss.str();
}

} // anonymous namespace

// ============================================================================
// IntegrityHelper
// ============================================================================

std::string IntegrityHelper::sha256Hex(const uint8_t* data, size_t size) {
    Sha256State s;
    s.update(data, size);
    return bytesToHex(s.finalise());
}

std::string IntegrityHelper::sha256Hex(const std::string& input) {
    return sha256Hex(reinterpret_cast<const uint8_t*>(input.data()), input.size());
}

bool IntegrityHelper::verifyHash(const uint8_t* data, size_t size,
                                  const std::string& expected_hex) {
    return sha256Hex(data, size) == expected_hex;
}

bool IntegrityHelper::verifyHash(const std::string& input,
                                  const std::string& expected_hex) {
    return sha256Hex(input) == expected_hex;
}

// ============================================================================
// AdapterUsagePolicy
// ============================================================================

json AdapterUsagePolicy::to_json() const {
    json j;
    j["license"]                    = license;
    j["restrictions"]               = restrictions;
    j["allowed_base_models"]        = allowed_base_models;
    j["max_concurrent_deployments"] = max_concurrent_deployments;
    j["expiry_date"]                = expiry_date;
    return j;
}

AdapterUsagePolicy AdapterUsagePolicy::from_json(const json& j) {
    AdapterUsagePolicy p;
    if (j.contains("license")) {
      p.license = j.at("license").get<std::string>();
    }
    if (j.contains("restrictions")) {
      p.restrictions = j.at("restrictions").get<std::string>();
    }
    if (j.contains("allowed_base_models")) {
      p.allowed_base_models = j.at("allowed_base_models").get<std::vector<std::string>>();
    }
    if (j.contains("max_concurrent_deployments")) {
      p.max_concurrent_deployments = j.at("max_concurrent_deployments").get<int>();
    }
    if (j.contains("expiry_date")) {
      p.expiry_date = j.at("expiry_date").get<std::string>();
    }
    return p;
}

// ============================================================================
// LoRAPackageProvenance
// ============================================================================

json LoRAPackageProvenance::to_json() const {
    json j;
    j["trainer_id"]           = trainer_id;
    j["training_framework"]   = training_framework;
    j["dataset_id"]           = dataset_id;
    j["dataset_hash"]         = dataset_hash;
    j["base_model_id"]        = base_model_id;
    j["base_model_hash"]      = base_model_hash;
    j["hyperparameter_hash"]  = hyperparameter_hash;
    j["training_duration_secs"] = training_duration_secs;
    j["created_at"]           = created_at;
    j["hardware_info"]        = hardware_info.is_null() ? json::object() : hardware_info;
    j["custom_metadata"]      = custom_metadata.is_null() ? json::object() : custom_metadata;
    return j;
}

LoRAPackageProvenance LoRAPackageProvenance::from_json(const json& j) {
    LoRAPackageProvenance p;
    if (j.contains("trainer_id")) {
      p.trainer_id           = j.at("trainer_id").get<std::string>();
    }
    if (j.contains("training_framework")) {
      p.training_framework   = j.at("training_framework").get<std::string>();
    }
    if (j.contains("dataset_id")) {
      p.dataset_id           = j.at("dataset_id").get<std::string>();
    }
    if (j.contains("dataset_hash")) {
      p.dataset_hash         = j.at("dataset_hash").get<std::string>();
    }
    if (j.contains("base_model_id")) {
      p.base_model_id        = j.at("base_model_id").get<std::string>();
    }
    if (j.contains("base_model_hash")) {
      p.base_model_hash      = j.at("base_model_hash").get<std::string>();
    }
    if (j.contains("hyperparameter_hash")) {
      p.hyperparameter_hash  = j.at("hyperparameter_hash").get<std::string>();
    }
    if (j.contains("training_duration_secs")) {
      p.training_duration_secs = j.at("training_duration_secs").get<double>();
    }
    if (j.contains("created_at")) {
      p.created_at           = j.at("created_at").get<std::string>();
    }
    if (j.contains("hardware_info")) {
      p.hardware_info        = j.at("hardware_info");
    }
    if (j.contains("custom_metadata")) {
      p.custom_metadata      = j.at("custom_metadata");
    }
    return p;
}

// ============================================================================
// ArtifactIntegrity
// ============================================================================

json ArtifactIntegrity::to_json() const {
    json j;
    j["weights_hash"]         = weights_hash;
    j["manifest_hash"]        = manifest_hash;
    j["signature"]            = signature;
    j["signature_algorithm"]  = signature_algorithm;
    j["signer_id"]            = signer_id;
    j["signed_at"]            = signed_at;
    // Note: signature_verified is intentionally omitted — runtime-only field
    return j;
}

ArtifactIntegrity ArtifactIntegrity::from_json(const json& j) {
    ArtifactIntegrity i;
    if (j.contains("weights_hash")) {
      i.weights_hash        = j.at("weights_hash").get<std::string>();
    }
    if (j.contains("manifest_hash")) {
      i.manifest_hash       = j.at("manifest_hash").get<std::string>();
    }
    if (j.contains("signature")) {
      i.signature           = j.at("signature").get<std::string>();
    }
    if (j.contains("signature_algorithm")) {
      i.signature_algorithm = j.at("signature_algorithm").get<std::string>();
    }
    if (j.contains("signer_id")) {
      i.signer_id           = j.at("signer_id").get<std::string>();
    }
    if (j.contains("signed_at")) {
      i.signed_at           = j.at("signed_at").get<std::string>();
    }
    return i;
}

// ============================================================================
// LoRAPackage
// ============================================================================

json LoRAPackage::to_json() const {
    json j;
    // Identity — fixed order for deterministic hashing
    j["package_id"]               = package_id;
    j["name"]                     = name;
    j["version"]                  = version;
    j["description"]              = description;
    j["supported_architectures"]  = supported_architectures;
    j["lora_rank"]                = lora_rank;
    j["lora_alpha"]               = lora_alpha;
    j["target_modules"]           = target_modules;
    j["parent_package_id"]        = parent_package_id;
    j["provenance"]               = provenance.to_json();
    j["policy"]                   = policy.to_json();
    j["weights_path"]             = weights_path;
    j["integrity"]                = integrity.to_json();
    j["status"]                   = statusToString();
    j["created_at"]               = created_at;
    j["updated_at"]               = updated_at;
    return j;
}

LoRAPackage LoRAPackage::from_json(const json& j) {
    // Validate required fields
    if (!j.contains("package_id") || !j.contains("name") || !j.contains("version")) {
        throw std::invalid_argument(
            "LoRAPackage::from_json: required fields missing "
            "(package_id, name, version)");
    }
    LoRAPackage p;
    p.package_id              = j.at("package_id").get<std::string>();
    p.name                    = j.at("name").get<std::string>();
    p.version                 = j.at("version").get<std::string>();
    if (j.contains("description")) {
      p.description           = j.at("description").get<std::string>();
    }
    if (j.contains("supported_architectures")) {
      p.supported_architectures = j.at("supported_architectures").get<std::vector<std::string>>();
    }
    if (j.contains("lora_rank")) {
      p.lora_rank             = j.at("lora_rank").get<int>();
    }
    if (j.contains("lora_alpha")) {
      p.lora_alpha            = j.at("lora_alpha").get<float>();
    }
    if (j.contains("target_modules")) {
      p.target_modules        = j.at("target_modules").get<std::vector<std::string>>();
    }
    if (j.contains("parent_package_id")) {
      p.parent_package_id     = j.at("parent_package_id").get<std::string>();
    }
    if (j.contains("provenance")) {
      p.provenance            = LoRAPackageProvenance::from_json(j.at("provenance"));
    }
    if (j.contains("policy")) {
      p.policy                = AdapterUsagePolicy::from_json(j.at("policy"));
    }
    if (j.contains("weights_path")) {
      p.weights_path          = j.at("weights_path").get<std::string>();
    }
    if (j.contains("integrity")) {
      p.integrity             = ArtifactIntegrity::from_json(j.at("integrity"));
    }
    if (j.contains("status")) {
      p.status                = statusFromString(j.at("status").get<std::string>());
    }
    if (j.contains("created_at")) {
      p.created_at            = j.at("created_at").get<std::string>();
    }
    if (j.contains("updated_at")) {
      p.updated_at            = j.at("updated_at").get<std::string>();
    }
    return p;
}

void LoRAPackage::computeManifestHash() {
    // Temporarily clear signature fields AND manifest_hash to obtain the
    // canonical content bytes that the hash covers.  Only non-signature
    // integrity fields (weights_hash) are included in the canonical form.
    const ArtifactIntegrity saved = integrity;
    integrity.manifest_hash       = "";
    integrity.signature           = "";
    integrity.signature_algorithm = "";
    integrity.signer_id           = "";
    integrity.signed_at           = "";
    const std::string canonical   = to_json().dump();
    // Restore signature fields; update hash only.
    integrity                     = saved;
    integrity.manifest_hash       = IntegrityHelper::sha256Hex(canonical);
}

bool LoRAPackage::supportsArchitecture(const std::string& arch) const {
    if (supported_architectures.empty()) {
      return true;
    }
    const std::string arch_lower = [&] {
        std::string s = arch;
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }();
    return std::any_of(supported_architectures.begin(), supported_architectures.end(),
                       [&arch_lower](const std::string& a) {
                           std::string al = a;
                           std::transform(al.begin(), al.end(), al.begin(),
                                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                           return al == arch_lower;
                       });
}

std::string LoRAPackage::statusToString() const {
    switch (status) {
        case LoRAPackageStatus::DRAFT:       return "DRAFT";
        case LoRAPackageStatus::VALIDATED:   return "VALIDATED";
        case LoRAPackageStatus::DEPRECATED:  return "DEPRECATED";
        case LoRAPackageStatus::REVOKED:     return "REVOKED";
    }
    return "DRAFT";
}

LoRAPackageStatus LoRAPackage::statusFromString(const std::string& s) {
    if (s == "DRAFT") {
      return LoRAPackageStatus::DRAFT;
    }
    if (s == "VALIDATED") {
      return LoRAPackageStatus::VALIDATED;
    }
    if (s == "DEPRECATED") {
      return LoRAPackageStatus::DEPRECATED;
    }
    if (s == "REVOKED") {
      return LoRAPackageStatus::REVOKED;
    }
    throw std::invalid_argument("LoRAPackage::statusFromString: unknown status '" + s + "'");
}

// ============================================================================
// PortableAdapterProduct
// ============================================================================

json PortableAdapterProduct::to_json() const {
    json j;
    j["product_id"]                  = product_id;
    j["name"]                        = name;
    j["version"]                     = version;
    j["source_package_id"]           = source_package_id;
    j["target_base_model_id"]        = target_base_model_id;
    j["target_model_architecture"]   = target_model_architecture;
    j["quantization"]                = quantization;
    j["format"]                      = format;
    j["file_path"]                   = file_path;
    j["file_size_bytes"]             = file_size_bytes;
    j["max_context_length"]          = max_context_length;
    j["memory_requirement_mb"]       = memory_requirement_mb;
    j["compatible_model_versions"]   = compatible_model_versions;
    j["integrity"]                   = integrity.to_json();
    j["status"]                      = statusToString();
    j["created_at"]                  = created_at;
    j["updated_at"]                  = updated_at;
    j["deployed_at"]                 = deployed_at;
    return j;
}

PortableAdapterProduct PortableAdapterProduct::from_json(const json& j) {
    if (!j.contains("product_id") || !j.contains("source_package_id") ||
        !j.contains("target_base_model_id")) {
        throw std::invalid_argument(
            "PortableAdapterProduct::from_json: required fields missing "
            "(product_id, source_package_id, target_base_model_id)");
    }
    PortableAdapterProduct p;
    p.product_id                = j.at("product_id").get<std::string>();
    p.source_package_id         = j.at("source_package_id").get<std::string>();
    p.target_base_model_id      = j.at("target_base_model_id").get<std::string>();
    if (j.contains("name")) {
      p.name = j.at("name").get<std::string>();
    }
    if (j.contains("version")) {
      p.version = j.at("version").get<std::string>();
    }
    if (j.contains("target_model_architecture")) {
      p.target_model_architecture = j.at("target_model_architecture").get<std::string>();
    }
    if (j.contains("quantization")) {
      p.quantization = j.at("quantization").get<std::string>();
    }
    if (j.contains("format")) {
      p.format = j.at("format").get<std::string>();
    }
    if (j.contains("file_path")) {
      p.file_path = j.at("file_path").get<std::string>();
    }
    if (j.contains("file_size_bytes")) {
      p.file_size_bytes = j.at("file_size_bytes").get<size_t>();
    }
    if (j.contains("max_context_length")) {
      p.max_context_length = j.at("max_context_length").get<int>();
    }
    if (j.contains("memory_requirement_mb")) {
      p.memory_requirement_mb = j.at("memory_requirement_mb").get<size_t>();
    }
    if (j.contains("compatible_model_versions")) {
      p.compatible_model_versions = j.at("compatible_model_versions").get<std::vector<std::string>>();
    }
    if (j.contains("integrity")) {
      p.integrity = ArtifactIntegrity::from_json(j.at("integrity"));
    }
    if (j.contains("status")) {
      p.status = statusFromString(j.at("status").get<std::string>());
    }
    if (j.contains("created_at")) {
      p.created_at = j.at("created_at").get<std::string>();
    }
    if (j.contains("updated_at")) {
      p.updated_at = j.at("updated_at").get<std::string>();
    }
    if (j.contains("deployed_at")) {
      p.deployed_at = j.at("deployed_at").get<std::string>();
    }
    return p;
}

void PortableAdapterProduct::computeManifestHash() {
    const ArtifactIntegrity saved = integrity;
    integrity.manifest_hash       = "";
    integrity.signature           = "";
    integrity.signature_algorithm = "";
    integrity.signer_id           = "";
    integrity.signed_at           = "";
    const std::string canonical   = to_json().dump();
    integrity                     = saved;
    integrity.manifest_hash       = IntegrityHelper::sha256Hex(canonical);
}

std::string PortableAdapterProduct::statusToString() const {
    switch (status) {
        case AdapterProductStatus::BUILDING:  return "BUILDING";
        case AdapterProductStatus::READY:     return "READY";
        case AdapterProductStatus::DEPLOYED:  return "DEPLOYED";
        case AdapterProductStatus::RETIRED:   return "RETIRED";
        case AdapterProductStatus::FAILED:    return "FAILED";
    }
    return "BUILDING";
}

AdapterProductStatus PortableAdapterProduct::statusFromString(const std::string& s) {
    if (s == "BUILDING") {
      return AdapterProductStatus::BUILDING;
    }
    if (s == "READY") {
      return AdapterProductStatus::READY;
    }
    if (s == "DEPLOYED") {
      return AdapterProductStatus::DEPLOYED;
    }
    if (s == "RETIRED") {
      return AdapterProductStatus::RETIRED;
    }
    if (s == "FAILED") {
      return AdapterProductStatus::FAILED;
    }
    throw std::invalid_argument(
        "PortableAdapterProduct::statusFromString: unknown status '" + s + "'");
}

// ============================================================================
// LoRAManifestStore
// ============================================================================

void LoRAManifestStore::setSignatureVerifier(SignatureVerifier verifier) {
    std::lock_guard<std::mutex> lk(mutex_);
    signature_verifier_ = std::move(verifier);
}

// ── LoRAPackage CRUD ──────────────────────────────────────────────────────────

bool LoRAManifestStore::storePackage(const LoRAPackage& pkg) {
    if (pkg.package_id.empty()) {
      return false;
    }
    std::lock_guard<std::mutex> lk(mutex_);
    packages_[pkg.package_id] = pkg;
    return true;
}

std::optional<LoRAPackage> LoRAManifestStore::loadPackage(
    const std::string& package_id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = packages_.find(package_id);
    if (it == packages_.end()) {
      return std::nullopt;
    }
    return it->second;
}

bool LoRAManifestStore::deletePackage(const std::string& package_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    return packages_.erase(package_id) > 0;
}

std::vector<std::string> LoRAManifestStore::listPackageIds() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<std::string> ids = {};

    ids.reserve(packages_.size());
    for (const auto& kv : packages_) {
      ids.push_back(kv.first);
    }
    return ids;
}

std::vector<LoRAPackage> LoRAManifestStore::listPackagesByStatus(
    LoRAPackageStatus status) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<LoRAPackage> out = {};

    for (const auto& kv : packages_) {
        if (kv.second.status == status) {
          out.push_back(kv.second);
        }
    }
    return out;
}

// ── PortableAdapterProduct CRUD ───────────────────────────────────────────────

bool LoRAManifestStore::storeProduct(const PortableAdapterProduct& product) {
    if (product.product_id.empty() || product.source_package_id.empty()) {
      return false;
    }
    std::lock_guard<std::mutex> lk(mutex_);
    products_[product.product_id] = product;
    return true;
}

std::optional<PortableAdapterProduct> LoRAManifestStore::loadProduct(
    const std::string& product_id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = products_.find(product_id);
    if (it == products_.end()) {
      return std::nullopt;
    }
    return it->second;
}

bool LoRAManifestStore::deleteProduct(const std::string& product_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    return products_.erase(product_id) > 0;
}

std::vector<std::string> LoRAManifestStore::listProductIds() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<std::string> ids = {};

    ids.reserve(products_.size());
    for (const auto& kv : products_) {
      ids.push_back(kv.first);
    }
    return ids;
}

std::vector<PortableAdapterProduct> LoRAManifestStore::listProductsByPackage(
    const std::string& package_id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<PortableAdapterProduct> out = {};

    for (const auto& kv : products_) {
        if (kv.second.source_package_id == package_id) {
          out.push_back(kv.second);
        }
    }
    return out;
}

std::vector<PortableAdapterProduct> LoRAManifestStore::listProductsByStatus(
    AdapterProductStatus status) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<PortableAdapterProduct> out = {};

    for (const auto& kv : products_) {
        if (kv.second.status == status) {
          out.push_back(kv.second);
        }
    }
    return out;
}

// ── Integrity verification ────────────────────────────────────────────────────

bool LoRAManifestStore::verifyPackageIntegrity(
    const std::string& package_id) const {
    // Take a copy under lock; verification itself is outside the lock.
    std::optional<LoRAPackage> maybe;
    SignatureVerifier verifier;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = packages_.find(package_id);
        if (it == packages_.end()) {
          return false;
        }
        maybe    = it->second;
        verifier = signature_verifier_;
    }
    LoRAPackage pkg = *maybe;

    const std::string stored_hash = pkg.integrity.manifest_hash;
    if (stored_hash.empty()) return false; // Unsigned / unhashed package

    // Reproduce the canonical bytes: clear manifest_hash AND all signature fields.
    pkg.integrity.manifest_hash       = "";
    pkg.integrity.signature           = "";
    pkg.integrity.signature_algorithm = "";
    pkg.integrity.signer_id           = "";
    pkg.integrity.signed_at           = "";
    const std::string canonical = pkg.to_json().dump();
    const std::string computed  = IntegrityHelper::sha256Hex(canonical);
    if (computed != stored_hash) {
      return false;
    }

    // Signature check: use the original (non-cleared) signature from the copy.
    const std::string sig        = maybe->integrity.signature;
    const std::string signer_id  = maybe->integrity.signer_id;
    if (verifier && !sig.empty()) {
        return verifier(stored_hash, sig, signer_id);
    }
    return true;
}

bool LoRAManifestStore::verifyProductIntegrity(
    const std::string& product_id) const {
    std::optional<PortableAdapterProduct> maybe;
    SignatureVerifier verifier;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = products_.find(product_id);
        if (it == products_.end()) {
          return false;
        }
        maybe    = it->second;
        verifier = signature_verifier_;
    }
    PortableAdapterProduct prod = *maybe;

    const std::string stored_hash = prod.integrity.manifest_hash;
    if (stored_hash.empty()) {
      return false;
    }

    // Reproduce canonical bytes — clear manifest_hash and all signature fields.
    prod.integrity.manifest_hash       = "";
    prod.integrity.signature           = "";
    prod.integrity.signature_algorithm = "";
    prod.integrity.signer_id           = "";
    prod.integrity.signed_at           = "";
    const std::string canonical = prod.to_json().dump();
    const std::string computed  = IntegrityHelper::sha256Hex(canonical);
    if (computed != stored_hash) {
      return false;
    }

    const std::string sig       = maybe->integrity.signature;
    const std::string signer_id = maybe->integrity.signer_id;
    if (verifier && !sig.empty()) {
        return verifier(stored_hash, sig, signer_id);
    }
    return true;
}

// ── Bulk export / import ──────────────────────────────────────────────────────

json LoRAManifestStore::exportPackages() const {
    std::lock_guard<std::mutex> lk(mutex_);
    // Collect keys and sort for stable, deterministic output order.
    std::vector<std::string> keys = {};

    keys.reserve(packages_.size());
    for (const auto& kv : packages_) {
      keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());
    json arr = json::array();
    for (const auto& k : keys) {
      arr.push_back(packages_.at(k).to_json());
    }
    return arr;
}

size_t LoRAManifestStore::importPackages(const json& j) {
    if (!j.is_array()) {
      return 0;
    }
    size_t count = 0;
    for (const auto& item : j) {
        try {
            LoRAPackage pkg = LoRAPackage::from_json(item);
            std::lock_guard<std::mutex> lk(mutex_);
            packages_[pkg.package_id] = std::move(pkg);
            ++count;
        } catch (const std::exception&) {
            // Skip malformed entries; caller may inspect the array for details.
        }
    }
    return count;
}

json LoRAManifestStore::exportProducts() const {
    std::lock_guard<std::mutex> lk(mutex_);
    // Collect keys and sort for stable, deterministic output order.
    std::vector<std::string> keys = {};

    keys.reserve(products_.size());
    for (const auto& kv : products_) {
      keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());
    json arr = json::array();
    for (const auto& k : keys) {
      arr.push_back(products_.at(k).to_json());
    }
    return arr;
}

size_t LoRAManifestStore::importProducts(const json& j) {
    if (!j.is_array()) {
      return 0;
    }
    size_t count = 0;
    for (const auto& item : j) {
        try {
            PortableAdapterProduct prod = PortableAdapterProduct::from_json(item);
            std::lock_guard<std::mutex> lk(mutex_);
            products_[prod.product_id] = std::move(prod);
            ++count;
        } catch (const std::exception&) {
        }
    }
    return count;
}

// ── Statistics ────────────────────────────────────────────────────────────────

size_t LoRAManifestStore::packageCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return packages_.size();
}

size_t LoRAManifestStore::productCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return products_.size();
}

} // namespace retrieval
} // namespace themis

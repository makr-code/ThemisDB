/**
 * @file adapter_provenance.cpp
 * @brief Implementation of adapter metadata canonicalisation, signing,
 *        verification and safetensors header rewriting (see adapter_provenance.h).
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "llm/lora_framework/adapter_provenance.h"

#include "llm/lora_framework/lora_safetensors_package.h"
#include "llm/lora_framework/training_snapshot.h"
#include "security/signing.h"

#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <system_error>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

namespace fs = std::filesystem;
using nlohmann::json;

namespace {

bool startsWith(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }

std::string toHex(const std::vector<uint8_t>& bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (uint8_t b : bytes) {
        out.push_back(kHex[b >> 4]);
        out.push_back(kHex[b & 0x0F]);
    }
    return out;
}

std::optional<std::vector<uint8_t>> fromHex(const std::string& hex) {
    if (hex.empty() || hex.size() % 2 != 0) return std::nullopt;
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::vector<uint8_t> out(hex.size() / 2);
    for (size_t i = 0; i < out.size(); ++i) {
        const int hi = nib(hex[2 * i]);
        const int lo = nib(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) return std::nullopt;
        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return out;
}

ProvenanceVerification invalid(std::string msg) {
    ProvenanceVerification v;
    v.status = ProvenanceVerification::Status::Invalid;
    v.error = std::move(msg);
    return v;
}

ProvenanceVerification unsignedResult(std::string msg) {
    ProvenanceVerification v;
    v.status = ProvenanceVerification::Status::Unsigned;
    v.error = std::move(msg);
    return v;
}

/// Is a typed GGUF value consistent with the signed decimal/string value?
bool ggufValueMatches(const json& v, const std::string& signed_value) {
    if (v.is_string()) return v.get<std::string>() == signed_value;
    if (v.is_boolean()) return signed_value == (v.get<bool>() ? "true" : "false");
    if (v.is_number_unsigned() || v.is_number_integer()) {
        const char* b = signed_value.data();
        const char* e = b + signed_value.size();
        if (v.is_number_unsigned()) {
            uint64_t parsed = 0;
            auto r = std::from_chars(b, e, parsed);
            return r.ec == std::errc() && r.ptr == e && parsed == v.get<uint64_t>();
        }
        int64_t parsed = 0;
        auto r = std::from_chars(b, e, parsed);
        return r.ec == std::errc() && r.ptr == e && parsed == v.get<int64_t>();
    }
    if (v.is_number_float()) {
        double parsed = 0.0;
        const char* b = signed_value.data();
        const char* e = b + signed_value.size();
        auto r = std::from_chars(b, e, parsed);
        if (r.ec != std::errc() || r.ptr != e || !std::isfinite(parsed)) return false;
        // GGUF f32: compare after the same double→float rounding the writer applied.
        return static_cast<float>(parsed) == static_cast<float>(v.get<double>());
    }
    return false;
}

}  // namespace

// ─── Canonicalisation ───────────────────────────────────────────────────────

std::optional<std::map<std::string, std::string>> AdapterProvenanceCanonical::signedFields(
    const std::map<std::string, std::string>& metadata, std::string* error) {
    std::map<std::string, std::string> out;
    for (const auto& [k, v] : metadata) {
        if (startsWith(k, "themis.") && !startsWith(k, provenance_keys::kPrefix)) {
            out.emplace(k, v);
        }
    }
    auto it = out.find(provenance_keys::kTensorSha256);
    if (it == out.end() || it->second.empty()) {
        if (error) *error = std::string("adapter metadata lacks ") + provenance_keys::kTensorSha256;
        return std::nullopt;
    }
    return out;
}

std::optional<std::string> AdapterProvenanceCanonical::canonicalJson(
    const std::map<std::string, std::string>& metadata, std::string* error) {
    auto fields = signedFields(metadata, error);
    if (!fields) return std::nullopt;
    json obj = json::object();
    for (const auto& [k, v] : *fields) obj[k] = v;
    try {
        // Object keys are sorted (std::map); no whitespace; UTF-8 kept raw;
        // invalid UTF-8 throws (strict error handler).
        return obj.dump(-1, ' ', /*ensure_ascii=*/false, json::error_handler_t::strict);
    } catch (const json::exception& e) {
        if (error) *error = std::string("adapter metadata is not valid UTF-8: ") + e.what();
        return std::nullopt;
    }
}

// ─── Signer ─────────────────────────────────────────────────────────────────

AdapterProvenanceSigner::AdapterProvenanceSigner(std::shared_ptr<SigningService> signer,
                                                 std::string key_id)
    : signer_(std::move(signer)), key_id_(std::move(key_id)) {}

bool AdapterProvenanceSigner::sign(std::map<std::string, std::string>& metadata,
                                   std::string* error) const {
    if (!signer_) {
        if (error) *error = "no signing service configured";
        return false;
    }
    if (key_id_.empty()) {
        if (error) *error = "no signing key id configured";
        return false;
    }
    auto canonical = AdapterProvenanceCanonical::canonicalJson(metadata, error);
    if (!canonical) return false;
    const std::vector<uint8_t> bytes(canonical->begin(), canonical->end());
    SigningResult res = signer_->sign(bytes, key_id_);
    if (!res.error.empty() || res.signature.empty()) {
        if (error) *error = "adapter signing failed: " + (res.error.empty() ? std::string("empty signature") : res.error);
        return false;
    }
    for (auto it = metadata.begin(); it != metadata.end();) {
        it = startsWith(it->first, provenance_keys::kPrefix) ? metadata.erase(it) : std::next(it);
    }
    metadata[provenance_keys::kCanonicalJson] = *canonical;
    metadata[provenance_keys::kCanonicalSha256] = sha256Hex(*canonical);
    metadata[provenance_keys::kSignature] = toHex(res.signature);
    metadata[provenance_keys::kKeyId] = key_id_;
    metadata[provenance_keys::kAlg] = res.algorithm.empty() ? std::string("unspecified") : res.algorithm;
    return true;
}

// ─── Verifier ───────────────────────────────────────────────────────────────

AdapterProvenanceVerifier::AdapterProvenanceVerifier(std::shared_ptr<SigningService> verifier,
                                                     std::set<std::string> trusted_key_ids)
    : verifier_(std::move(verifier)), trusted_key_ids_(std::move(trusted_key_ids)) {}

ProvenanceVerification AdapterProvenanceVerifier::verifyPayload(
    const std::string& canonical, const std::map<std::string, std::string>& prov) const {
    auto get = [&](const char* k) -> std::string {
        auto it = prov.find(k);
        return it == prov.end() ? std::string() : it->second;
    };
    const std::string sig_hex = get(provenance_keys::kSignature);
    if (sig_hex.empty()) return unsignedResult("adapter metadata carries no provenance signature");
    const std::string key_id = get(provenance_keys::kKeyId);
    if (key_id.empty()) return invalid("provenance key_id missing");
    if (get(provenance_keys::kAlg).empty()) return invalid("provenance alg missing");
    if (!trusted_key_ids_.empty() && trusted_key_ids_.count(key_id) == 0) {
        return invalid("provenance key_id '" + key_id + "' is not trusted");
    }
    if (get(provenance_keys::kCanonicalJson) != canonical) {
        return invalid("provenance canonical payload does not match the adapter metadata");
    }
    if (get(provenance_keys::kCanonicalSha256) != sha256Hex(canonical)) {
        return invalid("provenance canonical_sha256 mismatch");
    }
    auto sig = fromHex(sig_hex);
    if (!sig) return invalid("provenance signature is not hex");
    if (!verifier_) return invalid("no signature verification service configured");
    const std::vector<uint8_t> bytes(canonical.begin(), canonical.end());
    if (!verifier_->verify(bytes, *sig, key_id)) {
        return invalid("provenance signature verification failed");
    }
    ProvenanceVerification ok;
    ok.status = ProvenanceVerification::Status::Verified;
    ok.key_id = key_id;
    return ok;
}

ProvenanceVerification AdapterProvenanceVerifier::verifyMetadata(
    const std::map<std::string, std::string>& metadata) const {
    std::map<std::string, std::string> prov;
    for (const auto& [k, v] : metadata) {
        if (startsWith(k, provenance_keys::kPrefix)) prov.emplace(k, v);
    }
    if (prov.empty()) return unsignedResult("adapter metadata carries no provenance signature");
    std::string err;
    auto canonical = AdapterProvenanceCanonical::canonicalJson(metadata, &err);
    if (!canonical) return invalid(err);
    auto v = verifyPayload(*canonical, prov);
    if (v.status == ProvenanceVerification::Status::Verified) {
        v.signed_fields = *AdapterProvenanceCanonical::signedFields(metadata);
    }
    return v;
}

ProvenanceVerification AdapterProvenanceVerifier::verifySafetensorsFile(const fs::path& path) const {
    const SafetensorsInspection insp = SafetensorsPackageValidator::inspectFile(path);
    if (!insp.valid) return invalid("invalid safetensors file: " + insp.error);
    auto v = verifyMetadata(insp.metadata);
    if (v.status != ProvenanceVerification::Status::Verified) return v;
    const std::string actual = SafetensorsPackageValidator::sha256OfTensorData(path, insp);
    if (actual.empty()) return invalid("failed to hash safetensors tensor data");
    if (actual != v.signed_fields.at(provenance_keys::kTensorSha256)) {
        return invalid("tensor data does not match signed themis.adapter.sha256_tensors");
    }
    return v;
}

ProvenanceVerification AdapterProvenanceVerifier::verifyGgufKv(const json& kv) const {
    if (!kv.is_object()) return invalid("GGUF metadata is not an object");
    std::map<std::string, std::string> prov;
    for (auto it = kv.begin(); it != kv.end(); ++it) {
        if (startsWith(it.key(), provenance_keys::kPrefix)) {
            if (!it.value().is_string()) return invalid("GGUF provenance key '" + it.key() + "' is not a string");
            prov.emplace(it.key(), it.value().get<std::string>());
        }
    }
    if (prov.empty()) return unsignedResult("GGUF adapter carries no provenance signature");
    auto cj = prov.find(provenance_keys::kCanonicalJson);
    if (cj == prov.end()) return invalid("GGUF adapter lacks themis.provenance.canonical_json");
    const std::string& canonical = cj->second;

    auto v = verifyPayload(canonical, prov);
    if (v.status != ProvenanceVerification::Status::Verified) return v;

    // Signature is valid for the embedded payload; now bind the GGUF KV to it.
    json payload = json::parse(canonical, nullptr, /*allow_exceptions=*/false);
    if (!payload.is_object()) return invalid("provenance canonical payload is not a JSON object");
    std::map<std::string, std::string> fields;
    for (auto it = payload.begin(); it != payload.end(); ++it) {
        if (!it.value().is_string()) return invalid("provenance payload value is not a string: " + it.key());
        fields.emplace(it.key(), it.value().get<std::string>());
    }
    std::string err;
    auto recanon = AdapterProvenanceCanonical::canonicalJson(fields, &err);
    if (!recanon || *recanon != canonical) {
        return invalid("provenance canonical payload is not in canonical form");
    }
    size_t gguf_signed_keys = 0;
    for (auto it = kv.begin(); it != kv.end(); ++it) {
        const std::string& k = it.key();
        if (!startsWith(k, "themis.") || startsWith(k, provenance_keys::kPrefix)) continue;
        ++gguf_signed_keys;
        auto f = fields.find(k);
        if (f == fields.end()) return invalid("GGUF key '" + k + "' is not covered by the signature");
        if (!ggufValueMatches(it.value(), f->second)) {
            return invalid("GGUF key '" + k + "' does not match the signed value");
        }
    }
    if (gguf_signed_keys != fields.size()) {
        return invalid("GGUF adapter lacks signed themis.* keys");
    }
    v.signed_fields = std::move(fields);
    return v;
}

// ─── Safetensors header rewrite ─────────────────────────────────────────────

bool rewriteSafetensorsMetadata(const fs::path& path,
                                const std::map<std::string, std::string>& metadata,
                                std::string* error) {
    auto fail = [&](const std::string& msg) {
        if (error) *error = msg;
        return false;
    };
    const SafetensorsInspection insp = SafetensorsPackageValidator::inspectFile(path);
    if (!insp.valid) return fail("invalid safetensors file: " + insp.error);

    std::ifstream in(path, std::ios::binary);
    if (!in) return fail("cannot open " + path.string());
    in.seekg(8);
    std::string header(static_cast<size_t>(insp.header_length), '\0');
    if (!in.read(header.data(), static_cast<std::streamsize>(header.size()))) {
        return fail("cannot read safetensors header");
    }
    json h = json::parse(header, nullptr, /*allow_exceptions=*/false);
    if (!h.is_object()) return fail("safetensors header is not a JSON object");
    json md = json::object();
    for (const auto& [k, v] : metadata) md[k] = v;
    h["__metadata__"] = std::move(md);
    std::string new_header;
    try {
        new_header = h.dump(-1, ' ', /*ensure_ascii=*/false, json::error_handler_t::strict);
    } catch (const json::exception& e) {
        return fail(std::string("cannot serialise safetensors header: ") + e.what());
    }
    while (new_header.size() % 8 != 0) new_header.push_back(' ');
    if (new_header.size() > SafetensorsPackageValidator::kMaxHeaderBytes) {
        return fail("safetensors header exceeds size limit");
    }

    fs::path tmp = path;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return fail("cannot create " + tmp.string());
        uint8_t len_le[8];
        uint64_t n = new_header.size();
        for (int i = 0; i < 8; ++i) len_le[i] = static_cast<uint8_t>(n >> (8 * i));
        out.write(reinterpret_cast<const char*>(len_le), 8);
        out.write(new_header.data(), static_cast<std::streamsize>(new_header.size()));
        in.seekg(static_cast<std::streamoff>(insp.data_offset));
        std::array<char, 1 << 16> buf{};
        while (in) {
            in.read(buf.data(), buf.size());
            const std::streamsize got = in.gcount();
            if (got > 0) out.write(buf.data(), got);
        }
        out.flush();
        if (!out) {
            std::error_code ec;
            fs::remove(tmp, ec);
            return fail("failed writing " + tmp.string());
        }
    }
    const SafetensorsInspection check = SafetensorsPackageValidator::inspectFile(tmp);
    if (!check.valid || check.metadata != metadata) {
        std::error_code ec;
        fs::remove(tmp, ec);
        return fail("rewritten safetensors header failed validation: " + check.error);
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    if (ec) {
        fs::remove(tmp, ec);
        return fail("cannot replace " + path.string());
    }
    return true;
}

}  // namespace lora
}  // namespace llm
}  // namespace themis

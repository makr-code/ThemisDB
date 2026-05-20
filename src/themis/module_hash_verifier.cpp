// THEMIS_GAP_STATS: gaps=1 unimpl=1 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            module_hash_verifier.cpp                           ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:51:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     249                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SHA-256 hash verification for loaded ThemisDB modules.
// Uses OpenSSL EVP_Digest API to compute hashes (same approach as the rest
// of the ThemisDB codebase, e.g. plugin_security.cpp and lora_provenance.cpp).
//
// Roadmap item: Phase 3 – SHA-256 hash verification for loaded modules
// Issue: #2471

#include "themis/module_hash_verifier.h"

#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <iomanip>
#include <sstream>

namespace themis {
namespace modules {

// ============================================================================
// Static helpers
// ============================================================================

std::string ModuleHashVerifier::computeSHA256(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        spdlog::warn("ModuleHashVerifier::computeSHA256: cannot open '{}'", filePath);
        return "";
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        spdlog::error("ModuleHashVerifier::computeSHA256: EVP_MD_CTX_new failed");
        return "";
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        spdlog::error("ModuleHashVerifier::computeSHA256: EVP_DigestInit_ex failed");
        return "";
    }

    constexpr std::size_t kBufSize = 32768;
    char buf[kBufSize];
    while (file.read(buf, kBufSize) || file.gcount() > 0) {
        if (EVP_DigestUpdate(ctx, buf, static_cast<std::size_t>(file.gcount())) != 1) {
            EVP_MD_CTX_free(ctx);
            spdlog::error("ModuleHashVerifier::computeSHA256: EVP_DigestUpdate failed");
            return "";
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hashLen = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        spdlog::error("ModuleHashVerifier::computeSHA256: EVP_DigestFinal_ex failed");
        return "";
    }
    EVP_MD_CTX_free(ctx);

    std::ostringstream ss;
    for (unsigned int i = 0; i < hashLen; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool ModuleHashVerifier::verifyHash(const std::string& modulePath,
                                    const std::string& expectedHash) {
    const std::string computed = computeSHA256(modulePath);
    if (computed.empty()) {
        return false;
    }
    const bool match = (computed == expectedHash);
    if (!match) {
        spdlog::error(
            "ModuleHashVerifier::verifyHash: hash mismatch for '{}': "
            "expected={} got={}",
            modulePath, expectedHash, computed);
    }
    return match;
}

// ============================================================================
// Manifest-driven API
// ============================================================================

bool ModuleHashVerifier::loadManifest(const std::string& manifestPath) {
    std::ifstream file(manifestPath);
    if (!file) {
        spdlog::error("ModuleHashVerifier::loadManifest: cannot open '{}'",
                      manifestPath);
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (!j.is_object()) {
            spdlog::error(
                "ModuleHashVerifier::loadManifest: '{}' is not a JSON object",
                manifestPath);
            return false;
        }

        manifest_.clear();
        for (auto& [name, value] : j.items()) {
            if (!value.is_string()) {
                spdlog::warn(
                    "ModuleHashVerifier::loadManifest: skipping non-string "
                    "value for key '{}'",
                    name);
                continue;
            }
            manifest_[name] = value.get<std::string>();
        }

        spdlog::info(
            "ModuleHashVerifier::loadManifest: loaded {} entries from '{}'",
            manifest_.size(), manifestPath);
        return true;
    } catch (const std::exception& e) {
        spdlog::error(
            "ModuleHashVerifier::loadManifest: failed to parse '{}': {}",
            manifestPath, e.what());
        return false;
    }
}

bool ModuleHashVerifier::saveManifest(const std::string& outputPath) const {
    try {
        nlohmann::json j = nlohmann::json::object();
        for (const auto& [name, hash] : manifest_) {
            j[name] = hash;
        }

        std::ofstream file(outputPath);
        if (!file) {
            spdlog::error(
                "ModuleHashVerifier::saveManifest: cannot write to '{}'",
                outputPath);
            return false;
        }
        file << j.dump(2);
        spdlog::info(
            "ModuleHashVerifier::saveManifest: wrote {} entries to '{}'",
            manifest_.size(), outputPath);
        return true;
    } catch (const std::exception& e) {
        spdlog::error(
            "ModuleHashVerifier::saveManifest: failed to write '{}': {}",
            outputPath, e.what());
        return false;
    }
}

void ModuleHashVerifier::addExpectedHash(const std::string& moduleName,
                                         const std::string& expectedHash) {
    manifest_[moduleName] = expectedHash;
    spdlog::debug(
        "ModuleHashVerifier::addExpectedHash: registered '{}' -> {}",
        moduleName, expectedHash);
}

ModuleHashVerificationResult ModuleHashVerifier::verifyModule(
    const std::string& moduleName,
    const std::string& modulePath) const {

    ModuleHashVerificationResult result;

    const auto it = manifest_.find(moduleName);
    if (it == manifest_.end()) {
        result.errorMessage =
            "Module '" + moduleName + "' not found in hash manifest";
        spdlog::warn("ModuleHashVerifier::verifyModule: {}", result.errorMessage);
        return result;
    }
    result.expectedHash = it->second;

    result.computedHash = computeSHA256(modulePath);
    if (result.computedHash.empty()) {
        result.errorMessage =
            "Failed to compute SHA-256 for '" + modulePath + "'";
        spdlog::error("ModuleHashVerifier::verifyModule: {}", result.errorMessage);
        return result;
    }

    if (result.computedHash != result.expectedHash) {
        result.errorMessage =
            "Integrity violation for module '" + moduleName +
            "': expected=" + result.expectedHash +
            " got=" + result.computedHash;
        spdlog::critical("ModuleHashVerifier::verifyModule: {}",
                         result.errorMessage);
        return result;
    }

    result.success = true;
    spdlog::info(
        "ModuleHashVerifier::verifyModule: integrity OK for '{}' ({})",
        moduleName, result.computedHash);
    return result;
}

size_t ModuleHashVerifier::manifestSize() const {
    return manifest_.size();
}

std::optional<std::string> ModuleHashVerifier::getExpectedHash(
    const std::string& moduleName) const {
    const auto it = manifest_.find(moduleName);
    if (it == manifest_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void ModuleHashVerifier::clearManifest() {
    manifest_.clear();
}

} // namespace modules
} // namespace themis


/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            oci_manifest_signing.h                             ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 Interface Header (Target: Q3 2026)                       ║
╚═════════════════════════════════════════════════════════════════════╝
 */
#pragma once
// Signed OCI manifests with Sigstore/Cosign support
#include <string>
#include <vector>

namespace themis { namespace plugins {

struct OciImageRef {
    std::string registry;
    std::string repository;
    std::string tag;
    std::string digest;
    std::string fullRef() const;
};

struct CosignVerifyConfig {
    std::string certificate_identity;
    std::string certificate_oidc_issuer;
    bool require_rekor_bundle = true;
    std::string rekor_url = "https://rekor.sigstore.dev";
    std::string cosign_public_key_path;
};

enum class SignatureVerifyResult {
    VALID, INVALID_SIGNATURE, NO_SIGNATURE, CERT_EXPIRED,
    REKOR_NOT_FOUND, IDENTITY_MISMATCH, POLICY_VIOLATION,
};

struct CosignVerifyReport {
    SignatureVerifyResult result;
    std::string signer_identity;
    std::string signing_time;
    std::string rekor_log_index;
    std::string transparency_log_entry;
    std::vector<std::string> warnings;
};

class IOciManifestVerifier {
public:
    virtual ~IOciManifestVerifier() = default;
    virtual CosignVerifyReport verify(const OciImageRef& ref,
                                      const CosignVerifyConfig& config) = 0;
    virtual bool requireSignedPlugins() const = 0;
    virtual void setEnforceSignatures(bool enforce) = 0;
};

}} // namespace themis::plugins

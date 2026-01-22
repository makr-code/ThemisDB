---
name: "🔐 Plugin Embedded Manufacturer Signature"
about: Implement hardened embedded signature system for plugin DLLs
title: "[Security] Implement Embedded ThemisDB.org Manufacturer Signature in Plugin DLLs"
labels: 
  - type:security
  - area:plugins
  - priority:P1
  - effort:x-large
  - breaking-change
assignees: ''

---

## 📋 Problem / Motivation

**Neue Sicherheitsanforderung:**

Das aktuelle Signierungssystem für Plugins muss gehärtet werden. In jede Plugin-DLL soll eine **eingebettete Herstellersignierung von ThemisDB.org** integriert werden, um sicherzustellen, dass die DLL vom offiziellen Hersteller kommt.

**Aktueller Stand:**
- ✅ Externe Signatur in `.json` Metadata-Datei
- ✅ SHA-256 Hash-Verifikation
- ✅ X.509 Zertifikat-Prüfung
- ❌ **Keine eingebettete Signatur in der DLL selbst**
- ❌ **Metadata-Datei kann manipuliert werden**
- ❌ **Keine Code-Signing auf PE/ELF-Ebene**

**Sicherheitsrisiken:**
1. **Metadata-Manipulation**: `.json` Datei kann durch Angreifer ersetzt werden
2. **Fehlende Authentizität**: Keine direkte Verifikation dass DLL von ThemisDB kommt
3. **Supply-Chain-Angriffe**: Gefälschte Plugins können als offiziell getarnt werden

## 🎯 Proposed Solution

Implementierung eines **mehrschichtigen Embedded-Signature-Systems**:

### 1. Platform-Native Code Signing

#### Windows: Authenticode Signierung
- Signierung mit Microsoft Authenticode
- Verwendung von ThemisDB.org Code-Signing-Zertifikat
- Einbettung der Signatur in PE-Header
- Timestamp-Signatur für Langzeitgültigkeit

#### Linux/Unix: ELF Binary Signierung
- Signierung mit `.note.gnu.build-id` oder Custom-Section
- GPG/PGP-Signatur eingebettet in ELF
- Alternat

iv: Separate Signatur-Datei mit Hash

#### macOS: Code Signing
- Verwendung von Apple `codesign` Tool
- Developer-ID-Zertifikat von Apple
- Notarisierung für Gatekeeper-Kompatibilität

### 2. Embedded Manufacturer Certificate

Einbettung eines ThemisDB.org-Zertifikats direkt in die DLL:

```cpp
// Embedded in DLL data section
namespace themis {
namespace plugins {

// ThemisDB.org Official Plugin Certificate (X.509 DER format)
// Expires: 2027-01-20
// Issuer: CN=ThemisDB Official Plugins CA, O=ThemisDB.org, C=DE
// Subject: CN=ThemisDB Plugin Signer, O=ThemisDB.org, C=DE
extern const unsigned char THEMISDB_PLUGIN_CERT[] = {
    0x30, 0x82, 0x03, 0x52, 0x30, 0x82, 0x02, 0x3a, // ... DER-encoded cert
    // ...
};
extern const size_t THEMISDB_PLUGIN_CERT_LEN;

// Digital signature of this DLL (RSA-4096)
extern const unsigned char THEMISDB_PLUGIN_SIGNATURE[] = {
    // Signature of DLL hash
};
extern const size_t THEMISDB_PLUGIN_SIGNATURE_LEN;

} // namespace plugins
} // namespace themis
```

### 3. Multi-Level Verification

```cpp
class EnhancedPluginSecurityVerifier {
public:
    enum class VerificationLevel {
        LEVEL_1_HASH_ONLY,           // Nur SHA-256 (schnell)
        LEVEL_2_EMBEDDED_SIGNATURE,  // Embedded Signature prüfen
        LEVEL_3_PLATFORM_SIGNATURE,  // PE/ELF Code-Signing
        LEVEL_4_FULL_CHAIN           // Komplette Zertifikatskette + CRL/OCSP
    };
    
    struct VerificationResult {
        bool passed = false;
        VerificationLevel level_achieved;
        std::string error_message;
        
        // Details
        bool hash_verified = false;
        bool embedded_signature_verified = false;
        bool platform_signature_verified = false;
        bool certificate_chain_verified = false;
        bool certificate_not_revoked = false;
        
        // Certificate info
        std::string issuer;
        std::string subject;
        std::chrono::system_clock::time_point valid_from;
        std::chrono::system_clock::time_point valid_until;
        bool is_themisdb_official = false;
    };
    
    /**
     * @brief Verify plugin with multi-level checks
     * @param plugin_path Path to DLL/SO
     * @param required_level Minimum verification level
     * @return Verification result with details
     */
    VerificationResult verifyPlugin(
        const std::string& plugin_path,
        VerificationLevel required_level = VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE
    );
    
private:
    // Level 1: Hash verification
    bool verifyHash(const std::string& plugin_path, VerificationResult& result);
    
    // Level 2: Read and verify embedded signature
    bool verifyEmbeddedSignature(const std::string& plugin_path, VerificationResult& result);
    
    // Level 3: Platform-specific code signing
    bool verifyPlatformSignature(const std::string& plugin_path, VerificationResult& result);
    
    // Level 4: Full certificate chain + revocation
    bool verifyFullChain(const std::string& plugin_path, VerificationResult& result);
    
    // Extract embedded certificate from DLL
    std::optional<std::vector<uint8_t>> extractEmbeddedCertificate(
        const std::string& plugin_path
    );
    
    // Extract embedded signature from DLL
    std::optional<std::vector<uint8_t>> extractEmbeddedSignature(
        const std::string& plugin_path
    );
    
    // Verify ThemisDB.org certificate
    bool isOfficialThemisDBCertificate(X509* cert);
};
```

## 📝 Implementation Details

### Phase 1: Certificate Infrastructure

1. **Erstelle ThemisDB.org CA**
   ```bash
   # Root CA für ThemisDB Plugin Signing
   openssl req -x509 -newkey rsa:4096 -sha256 -days 3650 \
     -keyout themisdb-plugin-ca.key \
     -out themisdb-plugin-ca.crt \
     -subj "/CN=ThemisDB Official Plugins CA/O=ThemisDB.org/C=DE"
   ```

2. **Erstelle Code-Signing-Zertifikat**
   ```bash
   # Code Signing Certificate
   openssl req -newkey rsa:4096 -sha256 \
     -keyout themisdb-plugin-signer.key \
     -out themisdb-plugin-signer.csr \
     -subj "/CN=ThemisDB Plugin Signer/O=ThemisDB.org/C=DE"
   
   # Sign with CA
   openssl x509 -req -in themisdb-plugin-signer.csr \
     -CA themisdb-plugin-ca.crt \
     -CAkey themisdb-plugin-ca.key \
     -out themisdb-plugin-signer.crt \
     -days 1825 -sha256 -set_serial 01 \
     -extensions v3_req -extfile openssl.cnf
   ```

3. **Sichere Speicherung**
   - Private Keys in HSM (Hardware Security Module)
   - Oder: Azure Key Vault / AWS KMS
   - Niemals im Git-Repository!

### Phase 2: Build-Integration

**CMake-Integration für Signierung:**

```cmake
# cmake/SignPlugin.cmake

function(sign_plugin TARGET_NAME)
    if(THEMIS_SIGN_PLUGINS)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            # Step 1: Calculate hash
            COMMAND ${CMAKE_COMMAND} -E sha256sum 
                $<TARGET_FILE:${TARGET_NAME}> > ${TARGET_NAME}.hash
            
            # Step 2: Sign with ThemisDB certificate
            COMMAND ${THEMIS_SIGN_TOOL} 
                --cert ${THEMISDB_CERT_PATH}
                --key ${THEMISDB_KEY_PATH}
                --input $<TARGET_FILE:${TARGET_NAME}>
                --output $<TARGET_FILE:${TARGET_NAME}>.signed
            
            # Step 3: Embed signature (platform-specific)
            COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan 
                "Signing plugin: ${TARGET_NAME}"
        )
        
        if(WIN32)
            # Windows: Authenticode signing
            find_program(SIGNTOOL signtool)
            if(SIGNTOOL)
                add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${SIGNTOOL} sign 
                        /f ${THEMISDB_PFX_PATH}
                        /p ${THEMISDB_PFX_PASSWORD}
                        /t http://timestamp.digicert.com
                        /v $<TARGET_FILE:${TARGET_NAME}>
                    COMMENT "Authenticode signing ${TARGET_NAME}"
                )
            endif()
        elseif(APPLE)
            # macOS: codesign
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND codesign --sign "Developer ID Application: ThemisDB.org"
                    --timestamp
                    --options runtime
                    $<TARGET_FILE:${TARGET_NAME}>
                COMMENT "Code signing ${TARGET_NAME} for macOS"
            )
        else()
            # Linux: GPG signature
            find_program(GPG gpg)
            if(GPG)
                add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${GPG} --armor --detach-sign 
                        --local-user plugins@themisdb.org
                        $<TARGET_FILE:${TARGET_NAME}>
                    COMMENT "GPG signing ${TARGET_NAME}"
                )
            endif()
        endif()
    endif()
endfunction()

# Usage in plugin CMakeLists.txt:
add_library(themis_plugin_onnx_clip SHARED onnx_clip.cpp)
sign_plugin(themis_plugin_onnx_clip)
```

### Phase 3: Signature Embedding

**Embedded Signature Generator:**

```cpp
// tools/embed_signature.cpp

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rsa.h>
#include <fstream>
#include <vector>

// Generate C++ header with embedded certificate and signature
void generateEmbeddedSignatureHeader(
    const std::string& cert_path,
    const std::string& dll_path,
    const std::string& output_header
) {
    // 1. Load certificate
    FILE* cert_file = fopen(cert_path.c_str(), "rb");
    X509* cert = d2i_X509_fp(cert_file, nullptr);
    fclose(cert_file);
    
    // 2. Extract DER-encoded cert
    unsigned char* cert_der = nullptr;
    int cert_len = i2d_X509(cert, &cert_der);
    
    // 3. Calculate DLL hash
    std::vector<uint8_t> dll_hash = calculateSHA256(dll_path);
    
    // 4. Sign hash with private key
    std::vector<uint8_t> signature = signWithPrivateKey(dll_hash, private_key);
    
    // 5. Generate header file
    std::ofstream header(output_header);
    header << "// Auto-generated: ThemisDB Plugin Embedded Signature\n";
    header << "// Generated: " << currentTimestamp() << "\n";
    header << "// Plugin: " << dll_path << "\n\n";
    
    header << "namespace themis::plugins {\n\n";
    
    // Certificate data
    header << "const unsigned char THEMISDB_PLUGIN_CERT[] = {\n";
    for (int i = 0; i < cert_len; i++) {
        if (i > 0) header << ", ";
        if (i % 12 == 0) header << "\n    ";
        header << "0x" << std::hex << std::setw(2) << std::setfill('0') 
               << (int)cert_der[i];
    }
    header << "\n};\n";
    header << "const size_t THEMISDB_PLUGIN_CERT_LEN = " << cert_len << ";\n\n";
    
    // Signature data
    header << "const unsigned char THEMISDB_PLUGIN_SIGNATURE[] = {\n";
    for (size_t i = 0; i < signature.size(); i++) {
        if (i > 0) header << ", ";
        if (i % 12 == 0) header << "\n    ";
        header << "0x" << std::hex << std::setw(2) << std::setfill('0') 
               << (int)signature[i];
    }
    header << "\n};\n";
    header << "const size_t THEMISDB_PLUGIN_SIGNATURE_LEN = " 
           << signature.size() << ";\n\n";
    
    header << "} // namespace themis::plugins\n";
    
    X509_free(cert);
    OPENSSL_free(cert_der);
}
```

### Phase 4: Verification Implementation

**Enhanced Verification:**

```cpp
bool EnhancedPluginSecurityVerifier::verifyEmbeddedSignature(
    const std::string& plugin_path,
    VerificationResult& result
) {
    // 1. Extract embedded certificate from DLL
    auto cert_data = extractEmbeddedCertificate(plugin_path);
    if (!cert_data) {
        result.error_message = "No embedded certificate found";
        return false;
    }
    
    // 2. Parse certificate
    const unsigned char* p = cert_data->data();
    X509* cert = d2i_X509(nullptr, &p, cert_data->size());
    if (!cert) {
        result.error_message = "Failed to parse embedded certificate";
        return false;
    }
    
    // 3. Verify certificate is official ThemisDB certificate
    if (!isOfficialThemisDBCertificate(cert)) {
        result.error_message = "Certificate is not official ThemisDB certificate";
        result.issuer = getCertificateIssuer(cert);
        X509_free(cert);
        return false;
    }
    
    // 4. Check certificate validity
    if (!isCertificateValid(cert)) {
        result.error_message = "Certificate has expired or not yet valid";
        X509_free(cert);
        return false;
    }
    
    // 5. Extract embedded signature
    auto signature_data = extractEmbeddedSignature(plugin_path);
    if (!signature_data) {
        result.error_message = "No embedded signature found";
        X509_free(cert);
        return false;
    }
    
    // 6. Calculate DLL hash (excluding signature section)
    std::vector<uint8_t> dll_hash = calculateHashExcludingSignature(plugin_path);
    
    // 7. Verify signature with certificate public key
    EVP_PKEY* pubkey = X509_get_pubkey(cert);
    bool signature_valid = verifyRSASignature(
        dll_hash, 
        *signature_data, 
        pubkey
    );
    EVP_PKEY_free(pubkey);
    X509_free(cert);
    
    if (!signature_valid) {
        result.error_message = "Embedded signature verification failed";
        return false;
    }
    
    result.embedded_signature_verified = true;
    result.is_themisdb_official = true;
    result.issuer = "CN=ThemisDB Official Plugins CA, O=ThemisDB.org, C=DE";
    
    return true;
}

bool EnhancedPluginSecurityVerifier::verifyPlatformSignature(
    const std::string& plugin_path,
    VerificationResult& result
) {
#ifdef _WIN32
    // Windows: Verify Authenticode signature
    return verifyAuthenticodeSignature(plugin_path, result);
#elif defined(__APPLE__)
    // macOS: Verify codesign signature
    return verifyMacOSCodeSignature(plugin_path, result);
#else
    // Linux: Verify GPG signature
    return verifyGPGSignature(plugin_path, result);
#endif
}
```

## ✅ Acceptance Criteria

### Phase 1: Infrastructure (Week 1)
- [ ] ThemisDB.org CA-Zertifikat erstellt
- [ ] Code-Signing-Zertifikat erstellt
- [ ] HSM oder Key Vault konfiguriert
- [ ] Zertifikatskette dokumentiert

### Phase 2: Build-Integration (Week 2-3)
- [ ] CMake `sign_plugin()` Funktion implementiert
- [ ] Authenticode-Signierung für Windows
- [ ] codesign-Integration für macOS
- [ ] GPG-Signierung für Linux
- [ ] CI/CD-Integration (GitHub Actions)
- [ ] Automatische Signierung im Release-Build

### Phase 3: Embedding (Week 3-4)
- [ ] Signature-Embedding-Tool implementiert
- [ ] Embedded-Signature-Extraktion implementiert
- [ ] PE/ELF-Parser für Signature-Section
- [ ] Test mit verschiedenen Plugin-Typen

### Phase 4: Verification (Week 4-5)
- [ ] `EnhancedPluginSecurityVerifier` implementiert
- [ ] Multi-Level-Verification (Level 1-4)
- [ ] Platform-spezifische Verification
- [ ] Certificate-Chain-Validation
- [ ] CRL/OCSP-Integration

### Phase 5: Testing & Documentation (Week 5-6)
- [ ] Unit-Tests für alle Verification-Levels
- [ ] Integration-Tests mit signierten Plugins
- [ ] Performance-Tests
- [ ] Security-Audit
- [ ] Dokumentation aktualisiert
- [ ] Migration-Guide für bestehende Plugins

## 🔗 Related

- Current implementation: `src/acceleration/plugin_security.cpp`
- Documentation: `docs/de/plugins/PLUGIN_SYSTEM_INTEGRATION.md`
- Related: Code-signing infrastructure (#TBD)
- Related: HSM integration (#TBD)

## 📊 Impact

**Security Benefits:**
- ✅ Authentizität garantiert (DLL ist wirklich von ThemisDB.org)
- ✅ Integrity garantiert (DLL wurde nicht manipuliert)
- ✅ Supply-Chain-Schutz (gefälschte Plugins werden erkannt)
- ✅ Compliance (erfüllt Security-Standards)

**Breaking Changes:**
- ❌ Alte unsignierte Plugins funktionieren nicht mehr (in Production)
- ❌ Build-Prozess wird komplexer
- ❌ Benötigt Code-Signing-Zertifikate

**Migration Path:**
- Development: `allowUnsigned = true` weiterhin erlaubt
- Production: Alle Plugins müssen signiert sein
- Grace Period: 3 Monate für Migration

## 🧪 Testing Strategy

1. **Unit Tests:**
   - Certificate loading und parsing
   - Signature embedding
   - Signature extraction
   - Verification für jeden Level

2. **Integration Tests:**
   - Signiere Test-Plugin
   - Lade und verifiziere Test-Plugin
   - Test mit manipuliertem Plugin (muss fehlschlagen)
   - Test mit abgelaufenem Zertifikat

3. **Security Tests:**
   - Fuzzing der Signature-Parser
   - Certificate-Spoofing-Versuche
   - Replay-Attacken
   - Penetration-Testing

4. **Platform Tests:**
   - Windows Authenticode
   - macOS codesign
   - Linux GPG
   - Cross-platform compatibility

## 📚 Additional Context

**Priority Justification:** P1 (High) - Critical security hardening requirement.

**Effort Estimate:** X-Large (5-6 weeks) - Requires:
- Certificate infrastructure setup
- Build system integration
- Platform-specific code signing
- Comprehensive testing
- Security audit

**Dependencies:**
- OpenSSL 1.1.1+
- Platform code-signing tools (signtool, codesign, gpg)
- HSM or Key Vault (optional but recommended)

**Standards Compliance:**
- X.509 PKI
- PKCS#7 / CMS signatures
- RFC 5652 (Cryptographic Message Syntax)
- Authenticode PE signature format
- ELF signature extensions

**Cost Estimate:**
- Code-Signing-Zertifikat: ~€300-500/Jahr
- HSM (optional): ~€500-2000 (einmalig)
- Development: 5-6 Wochen

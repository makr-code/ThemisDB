# Windows Authenticode Signing & Manufacturer Information

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🧩 Architecture

---

## 📑 Table of Contents

- [Overview](#overview)
- [Windows Trust Mechanisms](#windows-trust-mechanisms)
- [Implementation for ThemisDB Modules](#implementation-for-themisdb-modules)
- [Authenticode Signing Process](#1-authenticode-signing-process)

Windows uses several mechanisms to identify and trust executable files:

1. **Authenticode Signatures** - Digital signatures embedded in PE files
2. **Zone.Identifier (ADS)** - NTFS Alternate Data Stream marking downloads
3. **File Properties** - Manufacturer info in PE version resources
4. **SmartScreen** - Cloud-based reputation system

This document explains how ThemisDB integrates with these systems for modular DLLs.

## Windows Trust Mechanisms

### 1. Authenticode Signatures (PE Embedded)

Authenticode embeds digital signatures directly into Windows PE (Portable Executable) files (.exe, .dll, .sys).

**What Windows Shows**:
```
Digital Signatures Tab:
  ✓ This digital signature is OK
  Signed by: ThemisDB GmbH
  Timestamp: 2025-12-17 22:00:00
  
Properties → Details Tab:
  Company: ThemisDB GmbH
  Product Name: ThemisDB Database Server
  Product Version: 1.4.0
  File Version: 1.4.0.0
  Copyright: © 2025 ThemisDB GmbH
```

**How It Works**:
- Signature is embedded in PE file's Certificate Table
- Contains X.509 certificate chain
- Timestamped for long-term validity
- Verified by Windows before execution

### 2. Zone.Identifier (NTFS ADS)

When files are downloaded from the internet, Windows adds an Alternate Data Stream.

**File**: `themis_storage.dll:Zone.Identifier`
```ini
[ZoneTransfer]
ZoneId=3
ReferrerUrl=https://github.com/makr-code/ThemisDB/releases/download/v1.4.0/themis_storage.dll
HostUrl=https://github.com/makr-code/ThemisDB/releases/download/v1.4.0/themis_storage.dll
```

**Zone IDs**:
- 0 = Local Computer
- 1 = Local Intranet
- 2 = Trusted Sites
- 3 = Internet (triggers warnings)
- 4 = Restricted Sites

**What Windows Shows**:
```
Security Warning Dialog:
  ⚠️ Do you want to run this file?
  
  Name: themis_storage.dll
  Publisher: ThemisDB GmbH (if Authenticode signed)
  From: github.com
  
  [Run] [Don't Run]
```

### 3. PE Version Resources (Manufacturer Info)

Embedded in the PE file's resource section (.rsrc).

**Resource Structure**:
```cpp
VS_VERSION_INFO
  VS_FIXEDFILEINFO
    dwFileVersionMS: 1.4.0.0
    dwProductVersionMS: 1.4.0.0
  StringFileInfo
    CompanyName: "ThemisDB GmbH"
    FileDescription: "ThemisDB Storage Module"
    FileVersion: "1.4.0.0"
    InternalName: "themis_storage"
    LegalCopyright: "© 2025 ThemisDB GmbH"
    OriginalFilename: "themis_storage.dll"
    ProductName: "ThemisDB Database Server"
    ProductVersion: "1.4.0"
```

**How to View**:
- Right-click DLL → Properties → Details tab
- Shows company, version, copyright info

## Implementation for ThemisDB Modules

### Strategy

1. **Authenticode Signing** - Sign all modular DLLs with code signing certificate
2. **Version Resources** - Embed manufacturer info in PE files
3. **Zone.Identifier Support** - Handle downloaded modules correctly
4. **ModuleLoader Integration** - Verify Authenticode signatures before loading

### 1. Authenticode Signing Process

#### Obtain Code Signing Certificate

**Options**:
1. **EV Code Signing Certificate** (Extended Validation - recommended)
   - Hardware token (USB) required
   - Immediate SmartScreen reputation
   - Cost: ~€300-500/year
   - Vendors: DigiCert, Sectigo, GlobalSign

2. **Standard Code Signing Certificate**
   - Software-based certificate
   - Requires reputation building
   - Cost: ~€100-200/year

**Certificate Details**:
```
Subject: CN=ThemisDB GmbH, O=ThemisDB GmbH, L=Berlin, C=DE
Issuer: DigiCert SHA2 Assured ID Code Signing CA
Valid: 2025-01-01 to 2027-01-01
Key Usage: Digital Signature, Code Signing
Enhanced Key Usage: Code Signing (1.3.6.1.5.5.7.3.3)
```

#### Signing Command (Windows)

```powershell
# Sign themis_storage.dll
signtool sign `
    /f "ThemisDB-CodeSigning.pfx" `
    /p "PASSWORD" `
    /fd SHA256 `
    /tr "http://timestamp.digicert.com" `
    /td SHA256 `
    /d "ThemisDB Storage Module" `
    /du "https://github.com/makr-code/ThemisDB" `
    "themis_storage.dll"

# Verify signature
signtool verify /pa /v "themis_storage.dll"
```

**Output**:
```
Successfully signed: themis_storage.dll

Number of files successfully Signed: 1
Number of warnings: 0
Number of errors: 0
```

#### Automated Signing (CI/CD)

```yaml
# GitHub Actions workflow
name: Sign Modules

on:
  release:
    types: [created]

jobs:
  sign:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Import certificate
        run: |
          $pfxPath = "$env:TEMP\cert.pfx"
          [System.IO.File]::WriteAllBytes($pfxPath, [System.Convert]::FromBase64String($env:CERT_BASE64))
          Import-PfxCertificate -FilePath $pfxPath -CertStoreLocation Cert:\CurrentUser\My -Password (ConvertTo-SecureString -String $env:CERT_PASSWORD -AsPlainText -Force)
        env:
          CERT_BASE64: ${{ secrets.CODE_SIGNING_CERT }}
          CERT_PASSWORD: ${{ secrets.CERT_PASSWORD }}
      
      - name: Sign modules
        run: |
          $modules = Get-ChildItem -Path "build\lib" -Filter "themis_*.dll"
          foreach ($module in $modules) {
            signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /d "ThemisDB Module" $module.FullName
          }
      
      - name: Verify signatures
        run: |
          $modules = Get-ChildItem -Path "build\lib" -Filter "themis_*.dll"
          foreach ($module in $modules) {
            signtool verify /pa $module.FullName
          }
```

### 2. PE Version Resources (CMake)

Add version information to DLLs during build:

**CMakeLists.txt**:
```cmake
# Generate version resource file for Windows
if(WIN32)
    # Create version.rc for each module
    function(add_module_version_info TARGET_NAME MODULE_NAME MODULE_DESCRIPTION)
        set(VERSION_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}_version.rc")
        
        file(WRITE ${VERSION_RC_FILE}
"#include <winver.h>

VS_VERSION_INFO VERSIONINFO
FILEVERSION     ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0
PRODUCTVERSION  ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0
FILEFLAGSMASK   VS_FFI_FILEFLAGSMASK
FILEFLAGS       0x0L
FILEOS          VOS_NT_WINDOWS32
FILETYPE        VFT_DLL
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK \"StringFileInfo\"
    BEGIN
        BLOCK \"040904b0\"
        BEGIN
            VALUE \"CompanyName\", \"ThemisDB GmbH\"
            VALUE \"FileDescription\", \"${MODULE_DESCRIPTION}\"
            VALUE \"FileVersion\", \"${PROJECT_VERSION}\"
            VALUE \"InternalName\", \"${MODULE_NAME}\"
            VALUE \"LegalCopyright\", \"© 2025 ThemisDB GmbH\"
            VALUE \"OriginalFilename\", \"${MODULE_NAME}.dll\"
            VALUE \"ProductName\", \"ThemisDB Database Server\"
            VALUE \"ProductVersion\", \"${PROJECT_VERSION}\"
        END
    END
    BLOCK \"VarFileInfo\"
    BEGIN
        VALUE \"Translation\", 0x409, 1200
    END
END
")
        
        target_sources(${TARGET_NAME} PRIVATE ${VERSION_RC_FILE})
    endfunction()
    
    # Apply to all modules
    add_module_version_info(themis_storage "themis_storage" "ThemisDB Storage Module")
    add_module_version_info(themis_query "themis_query" "ThemisDB Query Engine Module")
    add_module_version_info(themis_security "themis_security" "ThemisDB Security Module")
    # ... repeat for all modules
endif()
```

### 3. Zone.Identifier Handling in ModuleLoader

Extend `ModuleLoader` to check and handle Zone.Identifier:

**include/themis/base/module_loader.h**:
```cpp
class ModuleLoader {
public:
    // ... existing methods ...
    
    /**
     * @brief Check if module has Zone.Identifier (downloaded from internet)
     * @param modulePath Path to module DLL
     * @return Zone ID (0-4), or -1 if no Zone.Identifier
     */
    int getZoneIdentifier(const std::string& modulePath) const;
    
    /**
     * @brief Remove Zone.Identifier (unblock file)
     * @param modulePath Path to module DLL
     * @return true if successful
     */
    bool removeZoneIdentifier(const std::string& modulePath);
    
    /**
     * @brief Check Authenticode signature (Windows)
     * @param modulePath Path to module DLL
     * @param signerInfo Output: certificate subject if signed
     * @return true if Authenticode signature valid
     */
    bool verifyAuthenticodeSignature(const std::string& modulePath, 
                                     std::string& signerInfo) const;
};
```

**Implementation (Windows-specific)**:
```cpp
#ifdef _WIN32
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>

#pragma comment(lib, "wintrust.lib")

int ModuleLoader::getZoneIdentifier(const std::string& modulePath) const {
    // Read Zone.Identifier alternate data stream
    std::string adsPath = modulePath + ":Zone.Identifier";
    
    HANDLE hFile = CreateFileA(
        adsPath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;  // No Zone.Identifier
    }
    
    char buffer[4096];
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        CloseHandle(hFile);
        return -1;
    }
    CloseHandle(hFile);
    
    buffer[bytesRead] = '\0';
    std::string content(buffer);
    
    // Parse ZoneId from INI content
    size_t pos = content.find("ZoneId=");
    if (pos != std::string::npos) {
        return std::stoi(content.substr(pos + 7, 1));
    }
    
    return -1;
}

bool ModuleLoader::removeZoneIdentifier(const std::string& modulePath) {
    std::string adsPath = modulePath + ":Zone.Identifier";
    return DeleteFileA(adsPath.c_str()) != 0;
}

bool ModuleLoader::verifyAuthenticodeSignature(const std::string& modulePath, 
                                               std::string& signerInfo) const {
    WINTRUST_FILE_INFO fileInfo = {};
    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    
    std::wstring wPath(modulePath.begin(), modulePath.end());
    fileInfo.pcwszFilePath = wPath.c_str();
    
    GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    
    WINTRUST_DATA trustData = {};
    trustData.cbStruct = sizeof(WINTRUST_DATA);
    trustData.dwUIChoice = WTD_UI_NONE;
    trustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    trustData.dwUnionChoice = WTD_CHOICE_FILE;
    trustData.pFile = &fileInfo;
    trustData.dwStateAction = WTD_STATEACTION_VERIFY;
    
    LONG status = WinVerifyTrust(NULL, &policyGUID, &trustData);
    
    bool verified = (status == ERROR_SUCCESS);
    
    if (verified) {
        // Extract signer information
        CRYPT_PROVIDER_DATA* provData = WTHelperProvDataFromStateData(trustData.hWVTStateData);
        if (provData) {
            CRYPT_PROVIDER_SGNR* signer = WTHelperGetProvSignerFromChain(provData, 0, FALSE, 0);
            if (signer && signer->psSigner) {
                CERT_INFO* certInfo = signer->psSigner->pCertInfo;
                if (certInfo) {
                    // Extract subject name
                    char subjectName[512];
                    CertNameToStrA(X509_ASN_ENCODING, &certInfo->Subject, 
                                  CERT_X500_NAME_STR, subjectName, sizeof(subjectName));
                    signerInfo = subjectName;
                }
            }
        }
    }
    
    // Cleanup
    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &policyGUID, &trustData);
    
    return verified;
}
#endif
```

### 4. Enhanced Module Verification Flow

Update `ModuleLoader::loadModule()` to use Authenticode:

```cpp
ModuleVerificationResult ModuleLoader::loadModule(const std::string& modulePath, 
                                                 const std::string& moduleName) {
    ModuleVerificationResult result;
    // ... existing checks ...
    
#ifdef _WIN32
    // Step 3a: Check Zone.Identifier
    int zoneId = getZoneIdentifier(modulePath);
    if (zoneId >= 0) {
        spdlog::info("Module {} has Zone.Identifier (Zone {})", moduleName, zoneId);
        
        if (zoneId == 3 || zoneId == 4) {  // Internet or Restricted
            spdlog::warn("Module downloaded from internet - verifying Authenticode signature");
            
            // Require Authenticode for internet downloads
            std::string signerInfo;
            if (!verifyAuthenticodeSignature(modulePath, signerInfo)) {
                result.errorMessage = "Module from internet lacks valid Authenticode signature";
                spdlog::critical("SECURITY: {}", result.errorMessage);
                result.success = false;
                return result;
            }
            
            spdlog::info("Authenticode signature valid: {}", signerInfo);
            
            // Optional: Remove Zone.Identifier after verification
            if (removeZoneIdentifier(modulePath)) {
                spdlog::info("Removed Zone.Identifier (module unblocked)");
            }
        }
    }
    
    // Step 3b: Verify Authenticode (always check if signature present)
    std::string signerInfo;
    if (verifyAuthenticodeSignature(modulePath, signerInfo)) {
        spdlog::info("Authenticode signature verified: {}", signerInfo);
        result.authenticodeSigner = signerInfo;
    } else {
#ifdef NDEBUG
        // Production: Require Authenticode
        result.errorMessage = "Module lacks valid Authenticode signature (required in production)";
        spdlog::critical("SECURITY: {}", result.errorMessage);
        result.success = false;
        return result;
#else
        spdlog::warn("Module not Authenticode signed (development mode - allowed)");
#endif
    }
#endif
    
    // Continue with existing verification (SHA-256, X.509, etc.)
    // ...
}
```

## Security Policy Matrix

| Scenario | Zone.Identifier | Authenticode | Action |
|----------|----------------|--------------|--------|
| Local build (dev) | No | No | ✅ Allow (dev mode) |
| Local build (prod) | No | No | ❌ Reject (require signature) |
| Downloaded (dev) | Yes (Zone 3) | No | ⚠️ Warn + Allow |
| Downloaded (prod) | Yes (Zone 3) | No | ❌ Reject |
| Downloaded (any) | Yes (Zone 3) | Yes | ✅ Verify + Unblock |
| Trusted source | Yes (Zone 2) | Yes | ✅ Allow |

## Benefits

### For Users
- ✅ **Windows Trust**: "Verified publisher" shown in UAC dialogs
- ✅ **SmartScreen**: No "Unknown publisher" warnings
- ✅ **Properties**: Company info visible in file properties
- ✅ **Confidence**: Official ThemisDB modules identifiable

### For Security
- ✅ **Authenticity**: Cryptographically proven origin
- ✅ **Integrity**: Detects tampering
- ✅ **Accountability**: Traceable to certificate owner
- ✅ **Compliance**: Meets enterprise security requirements

### For Distribution
- ✅ **Download Safety**: Users can verify before installation
- ✅ **Enterprise Deployment**: Passes Group Policy restrictions
- ✅ **Reputation**: Builds SmartScreen reputation over time

## Limitations

### Authenticode Limitations
- ❌ **Certificate Cost**: EV certificates €300-500/year
- ❌ **Initial Reputation**: Standard certificates need time to build trust
- ❌ **Revocation Checks**: Can fail if OCSP/CRL unavailable
- ❌ **Timestamp Dependency**: Timestamping service must be reachable

### Zone.Identifier Limitations
- ❌ **NTFS Only**: FAT32, exFAT don't support ADS
- ❌ **Easily Removed**: Users can manually delete ADS
- ❌ **Not Cryptographic**: Can be spoofed

## Recommendations

1. **Obtain EV Code Signing Certificate** (priority)
   - Immediate SmartScreen reputation
   - Hardware token security
   - Professional appearance

2. **Sign All Modules** (mandatory for production)
   - themis_*.dll files
   - themis_server.exe
   - installer packages

3. **Timestamp All Signatures** (critical)
   - Signatures remain valid after certificate expiration
   - Use DigiCert, GlobalSign, or Sectigo timestamp servers

4. **Embed Version Resources** (always)
   - Company name, copyright, version
   - Visible in Windows Explorer

5. **Handle Zone.Identifier** (defensive)
   - Check for internet downloads
   - Require Authenticode for Zone 3/4
   - Remove after successful verification

6. **Regular Certificate Renewal** (annual)
   - Certificate expires after 1-3 years
   - Plan renewal 2 months before expiration

## Implementation Checklist

- [ ] Obtain EV Code Signing Certificate
- [ ] Add version resources to CMake build
- [ ] Implement Authenticode verification in ModuleLoader
- [ ] Implement Zone.Identifier checking
- [ ] Add signing to CI/CD pipeline
- [ ] Test with downloaded modules
- [ ] Document signing process for releases
- [ ] Set up certificate renewal reminders

## References

- **Authenticode**: https://docs.microsoft.com/en-us/windows-hardware/drivers/install/authenticode
- **SignTool**: https://docs.microsoft.com/en-us/dotnet/framework/tools/signtool-exe
- **Zone.Identifier**: https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/6e3f7352-d11c-4d76-8c39-2516a9df36e8
- **WinVerifyTrust API**: https://docs.microsoft.com/en-us/windows/win32/api/wintrust/nf-wintrust-winverifytrust
- **Code Signing Best Practices**: https://docs.microsoft.com/en-us/windows-hardware/drivers/install/authenticode-signing-best-practices

---

**Status**: Design complete, ready for implementation  
**Priority**: High (required for Windows distribution)  
**Complexity**: Medium (certificate setup, build integration)  
**Timeline**: 1-2 weeks (including certificate acquisition)

# Linux Shared Library Signing & Verification

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🧩 Architecture

---

## 📑 Inhaltsverzeichnis

- [Overview](#overview)
- [Linux Trust Mechanisms](#linux-trust-mechanisms)
- [ELF Signing Methods](#1-elf-signing-methods)
- [Extended Attributes](#2-extended-attributes-ähnlich-zu-zoneidentifier)

Linux has mehrere Mechanismen für Signierung und Verifikation von shared libraries (.so), die ähnlich zu Windows Authenticode funktionieren, aber anders implementiert sind.

## Linux Trust Mechanisms

### Vergleich Windows vs. Linux

| Feature | Windows | Linux |
|---------|---------|-------|
| **Binary Format** | PE (Portable Executable) | ELF (Executable and Linkable Format) |
| **Signature Embedding** | Authenticode (in PE Certificate Table) | GPG detached signatures (.sig files) |
| **Download Marking** | Zone.Identifier (NTFS ADS) | Extended attributes (xattr) |
| **Metadata** | Version resources in PE | ELF notes / .comment section |
| **System Integration** | Built-in (WinVerifyTrust API) | Package manager dependent |

### 1. ELF Signing Methods

#### Option A: GPG Detached Signatures (Standard)

**Ähnlich zu**: Authenticode, aber als separate Datei

**How it works**:
```bash
# Sign a shared library
gpg --detach-sign --armor themis_storage.so
# Creates: themis_storage.so.asc (or .sig)

# Verify signature
gpg --verify themis_storage.so.asc themis_storage.so
```

**File structure**:
```
/opt/themisdb/lib/
  themis_storage.so         # Binary
  themis_storage.so.asc     # Signature (armored)
```

**Vorteile**:
- ✅ Standard Linux approach
- ✅ Works with package managers (dpkg, rpm)
- ✅ Widely supported tooling
- ✅ No binary modification needed

**Nachteile**:
- ❌ Separate file (can be separated)
- ❌ Not embedded in binary
- ❌ Requires GPG keyring management

#### Option B: ELF Embedded Signatures (osslsigncode)

**Ähnlich zu**: Authenticode (embedded)

**Tool**: `osslsigncode` (OpenSSL-based signing tool)

```bash
# Install
apt-get install osslsigncode

# Sign (creates new signed file)
osslsigncode sign \
    -certs themisdb-cert.pem \
    -key themisdb-key.pem \
    -in themis_storage.so \
    -out themis_storage.so.signed

# Verify
osslsigncode verify themis_storage.so.signed
```

**Vorteile**:
- ✅ Embedded in binary (like Authenticode)
- ✅ Single file
- ✅ X.509 certificates

**Nachteile**:
- ❌ Less common on Linux
- ❌ Modifies binary
- ❌ Limited distro support

#### Option C: IMA/EVM (Kernel-Level)

**Ähnlich zu**: Windows Code Integrity / Device Guard

**What it is**: Linux Integrity Measurement Architecture

```bash
# Enable IMA
echo "1" > /sys/kernel/security/ima/policy

# Sign with IMA signature
evmctl ima_sign --key /path/to/key.pem themis_storage.so

# Signature stored in extended attribute
getfattr -d -m security.ima themis_storage.so
```

**Vorteile**:
- ✅ Kernel-enforced integrity
- ✅ Cannot be bypassed in userspace
- ✅ Used in secure boot chains

**Nachteile**:
- ❌ Requires kernel support (CONFIG_IMA)
- ❌ Complex setup
- ❌ Not portable across systems

### 2. Extended Attributes (Ähnlich zu Zone.Identifier)

Linux kann **extended attributes (xattr)** nutzen, ähnlich zu NTFS Alternate Data Streams.

**Example**:
```bash
# Mark file as "downloaded from internet"
setfattr -n user.download.source -v "https://github.com/makr-code/ThemisDB" themis_storage.so
setfattr -n user.download.timestamp -v "2025-12-17T22:00:00Z" themis_storage.so

# Read attributes
getfattr -d themis_storage.so

# Output:
# user.download.source="https://github.com/makr-code/ThemisDB"
# user.download.timestamp="2025-12-17T22:00:00Z"
```

**Browser Integration**:
- Firefox/Chrome on Linux DO NOT automatically add download markers (unlike Windows)
- Custom solution needed for download tracking

**File System Support**:
- ext4, XFS, Btrfs: ✅ Full xattr support
- FAT, exFAT: ❌ No xattr support

### 3. ELF Metadata (Ähnlich zu PE Version Resources)

#### Option A: `.comment` Section

```bash
# Add metadata to .comment section
echo "ThemisDB GmbH - themis_storage v1.4.0" > comment.txt
objcopy --add-section .comment=comment.txt themis_storage.so themis_storage.so.new

# Read metadata
readelf -p .comment themis_storage.so
```

#### Option B: ELF Notes

```bash
# Add custom note section
objcopy --add-section .note.themisdb=metadata.bin themis_storage.so

# Read notes
readelf -n themis_storage.so
```

#### Option C: Build ID

```bash
# GCC/Clang automatically generates build ID
gcc -Wl,--build-id=sha1 ...

# Read build ID
readelf -n themis_storage.so | grep "Build ID"
```

**CMake Integration**:
```cmake
# Add build ID and metadata
if(UNIX)
    target_link_options(themis_storage PRIVATE
        -Wl,--build-id=sha1
    )
    
    # Add custom section with metadata
    add_custom_command(TARGET themis_storage POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "ThemisDB ${PROJECT_VERSION}" > ${CMAKE_BINARY_DIR}/metadata.txt
        COMMAND objcopy --add-section .themis.metadata=${CMAKE_BINARY_DIR}/metadata.txt 
                $<TARGET_FILE:themis_storage> 
                $<TARGET_FILE:themis_storage>.tmp
        COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:themis_storage>.tmp $<TARGET_FILE:themis_storage>
    )
endif()
```

### 4. Package Manager Signatures

**Debian/Ubuntu (dpkg)**:
```bash
# Packages (.deb) are signed with GPG
dpkg-sig --sign builder themisdb_1.4.0_amd64.deb

# Verify
dpkg-sig --verify themisdb_1.4.0_amd64.deb
```

**Red Hat/Fedora (rpm)**:
```bash
# RPM packages include GPG signatures
rpm --addsign themisdb-1.4.0-1.x86_64.rpm

# Verify
rpm --checksig themisdb-1.4.0-1.x86_64.rpm
```

## Recommended Approach for ThemisDB

### Production Deployment Strategy

**1. GPG Detached Signatures** (Primary)
- Sign all .so files with GPG
- Distribute .sig files alongside binaries
- ModuleLoader verifies GPG signatures

**2. Extended Attributes** (Download Tracking)
- Add xattr for download source
- Check in ModuleLoader
- Warn if from untrusted source

**3. ELF Metadata** (Version Info)
- Build ID via linker
- Custom .themis.metadata section
- Version, copyright, build info

### Implementation for ModuleLoader

**include/themis/base/module_loader.h**:
```cpp
class ModuleLoader {
public:
    // ... existing methods ...
    
#ifdef __linux__
    /**
     * @brief Verify GPG signature of module
     * @param modulePath Path to .so file
     * @param signaturePath Path to .sig/.asc file (optional, auto-detected)
     * @return true if signature valid
     */
    bool verifyGPGSignature(const std::string& modulePath,
                           const std::string& signaturePath = "") const;
    
    /**
     * @brief Check extended attributes (download marker)
     * @param modulePath Path to .so file
     * @return Map of attribute name → value
     */
    std::map<std::string, std::string> getExtendedAttributes(const std::string& modulePath) const;
    
    /**
     * @brief Read ELF metadata (Build ID, .comment, custom sections)
     * @param modulePath Path to .so file
     * @return Metadata structure
     */
    ELFMetadata readELFMetadata(const std::string& modulePath) const;
    
    /**
     * @brief Check IMA signature (if kernel supports)
     * @param modulePath Path to .so file
     * @return true if IMA signature valid
     */
    bool verifyIMASignature(const std::string& modulePath) const;
#endif
};
```

**Implementation**:
```cpp
#ifdef __linux__
#include <sys/xattr.h>
#include <elf.h>
#include <gelf.h>
#include <libelf.h>

bool ModuleLoader::verifyGPGSignature(const std::string& modulePath,
                                      const std::string& signaturePath) const {
    // Auto-detect signature file if not provided
    std::string sigPath = signaturePath;
    if (sigPath.empty()) {
        // Try .asc first, then .sig
        if (std::filesystem::exists(modulePath + ".asc")) {
            sigPath = modulePath + ".asc";
        } else if (std::filesystem::exists(modulePath + ".sig")) {
            sigPath = modulePath + ".sig";
        } else {
            spdlog::warn("No GPG signature file found for {}", modulePath);
            return false;
        }
    }
    
    // Verify using gpgme library
    gpgme_ctx_t ctx;
    gpgme_error_t err;
    
    err = gpgme_new(&ctx);
    if (err) {
        spdlog::error("GPG context creation failed: {}", gpgme_strerror(err));
        return false;
    }
    
    // Open signature and data files
    gpgme_data_t sig, text;
    FILE* sigFile = fopen(sigPath.c_str(), "r");
    FILE* dataFile = fopen(modulePath.c_str(), "r");
    
    gpgme_data_new_from_stream(&sig, sigFile);
    gpgme_data_new_from_stream(&text, dataFile);
    
    // Verify
    err = gpgme_op_verify(ctx, sig, text, nullptr);
    
    bool verified = false;
    if (!err) {
        gpgme_verify_result_t result = gpgme_op_verify_result(ctx);
        if (result && result->signatures) {
            gpgme_signature_t s = result->signatures;
            verified = (s->status == GPG_ERR_NO_ERROR);
            
            if (verified) {
                spdlog::info("GPG signature verified: {} (key: {})", 
                           modulePath, s->fpr);
            } else {
                spdlog::error("GPG signature invalid: {}", gpgme_strerror(s->status));
            }
        }
    }
    
    // Cleanup
    gpgme_data_release(sig);
    gpgme_data_release(text);
    gpgme_release(ctx);
    fclose(sigFile);
    fclose(dataFile);
    
    return verified;
}

std::map<std::string, std::string> ModuleLoader::getExtendedAttributes(
    const std::string& modulePath) const {
    
    std::map<std::string, std::string> attrs;
    
    // List all extended attributes
    ssize_t buflen = listxattr(modulePath.c_str(), nullptr, 0);
    if (buflen <= 0) {
        return attrs;  // No attributes
    }
    
    std::vector<char> buf(buflen);
    listxattr(modulePath.c_str(), buf.data(), buflen);
    
    // Parse attribute list (null-separated)
    size_t pos = 0;
    while (pos < buf.size()) {
        std::string attrName(&buf[pos]);
        if (attrName.empty()) break;
        
        // Read attribute value
        ssize_t vallen = getxattr(modulePath.c_str(), attrName.c_str(), nullptr, 0);
        if (vallen > 0) {
            std::vector<char> val(vallen + 1);
            getxattr(modulePath.c_str(), attrName.c_str(), val.data(), vallen);
            val[vallen] = '\0';
            attrs[attrName] = std::string(val.data());
        }
        
        pos += attrName.length() + 1;
    }
    
    return attrs;
}

ELFMetadata ModuleLoader::readELFMetadata(const std::string& modulePath) const {
    ELFMetadata meta;
    
    // Initialize libelf
    if (elf_version(EV_CURRENT) == EV_NONE) {
        return meta;
    }
    
    int fd = open(modulePath.c_str(), O_RDONLY);
    if (fd < 0) {
        return meta;
    }
    
    Elf* elf = elf_begin(fd, ELF_C_READ, nullptr);
    if (!elf) {
        close(fd);
        return meta;
    }
    
    // Read Build ID from notes
    Elf_Scn* scn = nullptr;
    while ((scn = elf_nextscn(elf, scn)) != nullptr) {
        GElf_Shdr shdr;
        gelf_getshdr(scn, &shdr);
        
        if (shdr.sh_type == SHT_NOTE) {
            Elf_Data* data = elf_getdata(scn, nullptr);
            if (data) {
                // Parse note section for Build ID
                GElf_Nhdr nhdr;
                size_t name_offset, desc_offset;
                size_t offset = 0;
                
                while ((offset = gelf_getnote(data, offset, &nhdr, 
                                             &name_offset, &desc_offset)) > 0) {
                    if (nhdr.n_type == NT_GNU_BUILD_ID) {
                        const unsigned char* build_id = 
                            (const unsigned char*)data->d_buf + desc_offset;
                        
                        std::stringstream ss;
                        for (size_t i = 0; i < nhdr.n_descsz; i++) {
                            ss << std::hex << std::setw(2) << std::setfill('0') 
                               << (int)build_id[i];
                        }
                        meta.buildId = ss.str();
                    }
                }
            }
        }
    }
    
    // Read .comment section
    scn = nullptr;
    while ((scn = elf_nextscn(elf, scn)) != nullptr) {
        GElf_Shdr shdr;
        gelf_getshdr(scn, &shdr);
        
        char* name = elf_strptr(elf, elf_getshdrstrndx(elf, nullptr), shdr.sh_name);
        if (name && std::string(name) == ".comment") {
            Elf_Data* data = elf_getdata(scn, nullptr);
            if (data) {
                meta.comment = std::string((char*)data->d_buf, data->d_size);
            }
        }
    }
    
    elf_end(elf);
    close(fd);
    
    return meta;
}
#endif
```

### CMake Build Integration

```cmake
# Linux: GPG signing and metadata
if(UNIX AND NOT APPLE)
    # Function to sign a module with GPG
    function(sign_module_gpg TARGET_NAME)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND gpg --detach-sign --armor $<TARGET_FILE:${TARGET_NAME}>
            COMMENT "Signing ${TARGET_NAME} with GPG"
        )
    endfunction()
    
    # Function to add ELF metadata
    function(add_elf_metadata TARGET_NAME)
        # Add build ID
        target_link_options(${TARGET_NAME} PRIVATE -Wl,--build-id=sha1)
        
        # Add custom metadata section
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo 
                "ThemisDB GmbH - ${TARGET_NAME} v${PROJECT_VERSION}" 
                > ${CMAKE_BINARY_DIR}/${TARGET_NAME}_meta.txt
            COMMAND objcopy 
                --add-section .themis.metadata=${CMAKE_BINARY_DIR}/${TARGET_NAME}_meta.txt
                $<TARGET_FILE:${TARGET_NAME}>
            COMMENT "Adding metadata to ${TARGET_NAME}"
        )
    endfunction()
    
    # Apply to all modules
    sign_module_gpg(themis_storage)
    sign_module_gpg(themis_query)
    # ... etc
    
    add_elf_metadata(themis_storage)
    add_elf_metadata(themis_query)
    # ... etc
endif()
```

## Distribution Best Practices

### Debian/Ubuntu Package

**debian/control**:
```
Package: themisdb
Version: 1.4.0
Architecture: amd64
Maintainer: ThemisDB GmbH <info@themisdb.com>
Description: ThemisDB Database Server
 Multi-model database with modular architecture
```

**Signing**:
```bash
# Sign package
dpkg-sig --sign builder themisdb_1.4.0_amd64.deb

# Verify
dpkg-sig --verify themisdb_1.4.0_amd64.deb
```

### RPM Package

**themisdb.spec**:
```spec
Name:           themisdb
Version:        1.4.0
Release:        1%{?dist}
Summary:        ThemisDB Database Server
License:        MIT
URL:            https://github.com/makr-code/ThemisDB

%description
ThemisDB multi-model database with modular architecture

%files
%{_libdir}/themis_*.so
%{_libdir}/themis_*.so.asc
```

**Signing**:
```bash
# Sign RPM
rpm --addsign themisdb-1.4.0-1.x86_64.rpm

# Verify
rpm --checksig themisdb-1.4.0-1.x86_64.rpm
```

## Security Comparison

| Security Feature | Windows | Linux |
|-----------------|---------|-------|
| **Embedded Signatures** | ✅ Authenticode in PE | ⚠️ osslsigncode (uncommon) |
| **Detached Signatures** | ❌ Not standard | ✅ GPG (.sig files) |
| **Download Marking** | ✅ Zone.Identifier (ADS) | ⚠️ xattr (manual) |
| **Version Info** | ✅ PE resources | ✅ ELF notes/.comment |
| **System Integration** | ✅ Built-in (WinVerifyTrust) | ⚠️ Manual (gpgme) |
| **Kernel Enforcement** | ✅ Code Integrity | ✅ IMA/EVM (opt-in) |
| **Package Manager** | ❌ No standard | ✅ dpkg/rpm signing |

## Empfehlung für ThemisDB

### Kurzzeitstrategie (v1.4.0)

1. **GPG Detached Signatures** für alle .so Dateien
2. **ELF Build ID** automatisch via Linker
3. **ModuleLoader GPG Verification** in Linux builds

### Langzeitstrategie (v1.5.0+)

1. **Package Repository** mit GPG-signierten Packages
2. **IMA/EVM Support** für high-security deployments
3. **Extended Attributes** für download tracking

### Code Examples

**Production Build**:
```bash
# Build with signing
cmake -DTHEMIS_BUILD_MODULAR=ON -DTHEMIS_SIGN_MODULES=ON ..
make

# Result:
# lib/themis_storage.so
# lib/themis_storage.so.asc (GPG signature)
```

**Verification**:
```cpp
themis::modules::ModuleLoader loader;

#ifdef __linux__
// Linux: GPG verification
auto result = loader.loadModule("/opt/themisdb/lib/themis_storage.so", "themis_storage");
// Automatically checks for themis_storage.so.asc
#endif

#ifdef _WIN32
// Windows: Authenticode verification
auto result = loader.loadModule("C:\\Program Files\\ThemisDB\\themis_storage.dll", "themis_storage");
// Checks embedded Authenticode signature
#endif

if (!result.success) {
    spdlog::critical("Module verification failed: {}", result.errorMessage);
    return 1;
}
```

## Zusammenfassung

**Ja, es gibt Linux-Äquivalente zu Windows Authenticode**:

| Windows Feature | Linux Equivalent | Implementation |
|----------------|------------------|----------------|
| Authenticode signatures | GPG detached signatures (.asc/.sig) | ✅ Recommended |
| PE Certificate Table | osslsigncode (embedded) | ⚠️ Less common |
| Zone.Identifier | Extended attributes (xattr) | ✅ Feasible |
| Version resources | ELF notes + .comment section | ✅ Standard |
| WinVerifyTrust API | gpgme library | ✅ Available |
| Code Integrity | IMA/EVM kernel subsystem | ⚠️ Advanced |

**Empfehlung**: GPG detached signatures als Primary-Methode für Linux, analog zu Authenticode für Windows.

## Referenzen

- **GPG Signing**: https://www.gnupg.org/documentation/
- **GPGME Library**: https://gnupg.org/software/gpgme/
- **ELF Format**: https://refspecs.linuxfoundation.org/elf/elf.pdf
- **IMA/EVM**: https://www.kernel.org/doc/html/latest/security/IMA-templates.html
- **Extended Attributes**: https://man7.org/linux/man-pages/man7/xattr.7.html
- **Debian Package Signing**: https://wiki.debian.org/SecureApt
- **RPM Signing**: https://rpm-software-management.github.io/rpm/manual/signatures.html

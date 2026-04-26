# ThemisDB — Update, Hotfix & Upgrade Guide

This document describes best practices for **operators** who want to apply
regular updates, hotfixes, and major upgrades to a running ThemisDB instance,
regardless of platform (Linux / Windows / macOS).

For the **developer / release manager** side (cutting releases, branching, tagging),
see [SOP.md](../../SOP.md) and [VERSIONING.md](../../VERSIONING.md).

---

## Table of Contents

1. [Version scheme quick reference](#1-version-scheme-quick-reference)
2. [Update types and urgency matrix](#2-update-types-and-urgency-matrix)
3. [Pre-update checklist](#3-pre-update-checklist)
4. [Applying a patch update (PATCH version)](#4-applying-a-patch-update-patch-version)
5. [Applying a hotfix](#5-applying-a-hotfix)
6. [Applying a minor feature update (MINOR version)](#6-applying-a-minor-feature-update-minor-version)
7. [Applying a major upgrade (MAJOR version)](#7-applying-a-major-upgrade-major-version)
8. [Rollback procedure](#8-rollback-procedure)
9. [Automated update strategies](#9-automated-update-strategies)
10. [Configuration drift management](#10-configuration-drift-management)
11. [Multi-node / clustered deployments](#11-multi-node--clustered-deployments)

---

## 1. Version scheme quick reference

ThemisDB uses [Semantic Versioning](https://semver.org/) — `MAJOR.MINOR.PATCH`:

| Increment | Meaning | Examples |
|-----------|---------|---------|
| **PATCH** | Bug fixes and security patches — backward-compatible | `1.8.0` → `1.8.1` |
| **MINOR** | New features — backward-compatible; config schema may gain new keys | `1.8.x` → `1.9.0` |
| **MAJOR** | Breaking changes — API, wire protocol, or storage format | `1.x` → `2.0.0` |

> **Rule of thumb:** PATCH/hotfix updates are safe to apply with a rolling restart.
> MINOR updates should follow the upgrade checklist. MAJOR updates always require
> reading the migration guide in `docs/migration/`.

Check the current installed version:

```bash
# Linux / macOS
themis_server --version
themisctl version

# Windows
themis_server.exe --version
themisctl.exe version
```

---

## 2. Update types and urgency matrix

| Type | Trigger | SLA | Config changes needed? | Data migration? |
|------|---------|-----|------------------------|-----------------|
| **Security hotfix (P0)** | Critical CVE / exploit in the wild | Apply within **24 h** | Rarely | Never |
| **Security patch (P1)** | High severity CVE | Apply within **7 days** | Rarely | Never |
| **Bug-fix patch** | Critical bug breaking core functionality | Apply within **1 release cycle** | Rarely | Never |
| **Regular patch** | Accumulated bug fixes | At next maintenance window | No | No |
| **Minor feature update** | New features / improvements | Planned maintenance window | Possibly | No |
| **Major upgrade** | Breaking changes | Planned, after testing in staging | Yes | Often |

Subscribe to [GitHub Releases](https://github.com/makr-code/ThemisDB/releases) and/or
[GitHub Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories)
to receive notifications.

---

## 3. Pre-update checklist

Before any update, complete all items:

```
[ ] 1. Read the CHANGELOG entry for the target version
        https://github.com/makr-code/ThemisDB/blob/main/CHANGELOG.md
[ ] 2. Review "Breaking Changes" section (MAJOR only)
[ ] 3. Back up the data directory
        Linux:   sudo rsync -a /var/lib/themisdb/ /var/lib/themisdb-backup-$(date +%Y%m%d)/
        Windows: robocopy "C:\ProgramData\ThemisDB\data" "D:\Backup\ThemisDB\data-%DATE%" /E
        macOS:   sudo rsync -a /var/lib/themisdb/ /var/lib/themisdb-backup-$(date +%Y%m%d)/
[ ] 4. Back up the configuration file
        Linux:   sudo cp /etc/themisdb/config.yaml /etc/themisdb/config.yaml.bak.$(date +%Y%m%d)
        Windows: copy "C:\ThemisDB\config\config.yaml" "C:\ThemisDB\config\config.yaml.bak"
[ ] 5. Verify backup integrity (spot-check that files are non-zero / readable)
[ ] 6. Note the current version: themis_server --version
[ ] 7. (Clustered) Ensure quorum and healthy replication before updating any node
[ ] 8. Download and verify the new package checksums
        sha256sum --check themisdb_1.8.1_amd64.deb.sha256  # Linux
```

---

## 4. Applying a patch update (PATCH version)

PATCH updates are backward-compatible. The standard procedure is:
**stop → install → start**. No configuration changes or data migrations are required.

### Linux — DEB (Debian/Ubuntu)

```bash
# Download the new .deb (replace version string)
wget https://github.com/makr-code/ThemisDB/releases/download/v1.8.1/themisdb_1.8.1_amd64.deb

# Install (dpkg upgrades in place; config files in /etc/themisdb/ are preserved)
sudo dpkg -i themisdb_1.8.1_amd64.deb

# Restart the service
sudo systemctl restart themisdb

# Confirm
sudo systemctl status themisdb
themis_server --version
```

### Linux — RPM (RHEL/Fedora/CentOS)

```bash
sudo dnf upgrade ./themisdb-1.8.1-1.x86_64.rpm
# or:
sudo rpm -Uvh themisdb-1.8.1-1.x86_64.rpm
sudo systemctl restart themisdb
```

### Linux — TGZ (generic)

```bash
sudo systemctl stop themisdb

# Replace binaries only — do NOT overwrite config or data
sudo tar -xzf themisdb-1.8.1-Linux-x86_64.tar.gz \
    -C /opt/themisdb --strip-components=1 \
    --exclude='*/config*'

sudo systemctl start themisdb
themis_server --version
```

### Windows — MSI

Run the new `.msi` as Administrator. The stable `CPACK_WIX_UPGRADE_GUID` ensures
in-place upgrade — the service is stopped, binaries replaced, and the service
restarted automatically.

```powershell
# Silent install
msiexec /i ThemisDB-1.8.1-Windows-x64.msi /quiet /qn REINSTALL=ALL REINSTALLMODE=vomus

# Verify
themis_server.exe --version
```

### Windows — ZIP (portable)

```powershell
sc.exe stop ThemisDB
# Replace executables only (preserve config)
Expand-Archive -Path ThemisDB-1.8.1-Windows-x64.zip -DestinationPath C:\ThemisDB -Force
sc.exe start ThemisDB
```

### macOS — TGZ

```bash
sudo launchctl unload /Library/LaunchDaemons/com.themisdb.server.plist

sudo tar -xzf themisdb-1.8.1-macOS-arm64.tar.gz \
    -C /usr/local/opt/themisdb --strip-components=1 \
    --exclude='*/config*'

sudo launchctl load -w /Library/LaunchDaemons/com.themisdb.server.plist
```

---

## 5. Applying a hotfix

Hotfixes follow the same procedure as patch updates (§4) but with **higher urgency**:

| Priority | Expected action |
|----------|----------------|
| P0 — Security / data-loss | Apply within **24 hours** of release, even outside normal maintenance windows |
| P1 — High severity | Apply within **7 days** |

### Identifying a hotfix release

Hotfix releases appear as PATCH increments (e.g., `v1.8.1`) in
[GitHub Releases](https://github.com/makr-code/ThemisDB/releases) and are labelled
`hotfix` or `security` in the release notes.

### Verify the hotfix before installing

```bash
# 1. Check the release notes for the affected component
#    and confirm your version is listed as vulnerable.

# 2. Download package + checksum
wget https://github.com/makr-code/ThemisDB/releases/download/v1.8.1/themisdb_1.8.1_amd64.deb
wget https://github.com/makr-code/ThemisDB/releases/download/v1.8.1/themisdb_1.8.1_amd64.deb.sha256

# 3. Verify checksum
sha256sum --check themisdb_1.8.1_amd64.deb.sha256

# 4. (Recommended) Verify GPG signature if published
gpg --verify themisdb_1.8.1_amd64.deb.asc themisdb_1.8.1_amd64.deb
```

Then follow the platform-specific steps in §4 with no maintenance window required
for P0 security hotfixes.

---

## 6. Applying a minor feature update (MINOR version)

MINOR updates add new functionality and may introduce new configuration keys, but
are always backward-compatible. Existing configuration files continue to work without
changes; new keys simply use built-in defaults if absent.

### Recommended procedure

```
[ ] 1. Complete the pre-update checklist (§3)
[ ] 2. Review CHANGELOG for new configuration options
[ ] 3. Apply the update using the platform procedure in §4
[ ] 4. Start the service and verify it boots cleanly
        sudo journalctl -u themisdb -n 50
[ ] 5. (Optional) Merge new default config keys into your config.yaml
        — compare with /etc/themisdb/config.yaml.rpmnew or the package-provided template
[ ] 6. Run a smoke test against the API
        curl http://localhost:8080/health
        curl http://localhost:8080/api/v1/status
[ ] 7. Monitor metrics and logs for 30 minutes after restart
```

### New configuration keys

When a new version introduces configuration options, the server logs a message at
`INFO` level for each key that is using its default value. Review the output:

```bash
sudo journalctl -u themisdb --since "5 minutes ago" | grep "using default"
```

Add any desired new keys to your `config.yaml` explicitly to lock in the values.

---

## 7. Applying a major upgrade (MAJOR version)

MAJOR upgrades may include breaking changes to the API, wire protocol, storage format,
or configuration schema. **Always test in a staging environment first.**

### Process

```
[ ] 1. Read the migration guide: docs/migration/vX-to-vY.md
        Available at: https://github.com/makr-code/ThemisDB/tree/main/docs/migration/
[ ] 2. Identify all breaking changes that affect your configuration / API usage
[ ] 3. Test the upgrade on a staging/dev instance with a copy of production data
[ ] 4. Validate all integrations (clients, SDKs, automation scripts) in staging
[ ] 5. Schedule a maintenance window with adequate time for rollback if needed
[ ] 6. Complete the pre-update checklist (§3) including a verified data backup
[ ] 7. If storage migration is required:
        sudo systemctl stop themisdb
        themisctl migrate --from vX --to vY --data-dir /var/lib/themisdb
[ ] 8. Install the new package (platform procedure in §4)
[ ] 9. Verify the service starts and passes health checks
[ ] 10. Run integration tests / smoke tests
[ ] 11. Monitor for at least 1 hour before declaring the upgrade complete
```

> **Wire Protocol:** Major upgrades may change the wire protocol version.
> If you run multiple ThemisDB nodes, upgrade them within a single maintenance
> window. Mixed-version clusters are only supported for rolling upgrades within
> the same MAJOR version.

### Rolling major upgrade (clustered)

1. Upgrade one follower node at a time (see §11).
2. Verify the node rejoins the cluster and replication is healthy.
3. Promote the upgraded follower to leader (if possible), then upgrade the old leader.
4. Wait for full replication before proceeding to the next node.

---

## 8. Rollback procedure

If an update introduces a regression, roll back to the previous version.

### Linux — DEB

```bash
# Stop the service
sudo systemctl stop themisdb

# Reinstall the previous version (have the old .deb on hand)
sudo dpkg -i themisdb_1.8.0_amd64.deb

# Restore data backup if the new version modified data on startup
sudo rsync -a /var/lib/themisdb-backup-20260425/ /var/lib/themisdb/

# Start the service
sudo systemctl start themisdb
```

### Linux — RPM

```bash
sudo systemctl stop themisdb
sudo rpm -Uvh --oldpackage themisdb-1.8.0-1.x86_64.rpm
sudo systemctl start themisdb
```

### Windows — MSI

```powershell
# Uninstall the current version silently
$productCode = (Get-WmiObject -Class Win32_Product | Where-Object { $_.Name -like "ThemisDB*" }).IdentifyingNumber
msiexec /x $productCode /quiet

# Install the previous version
msiexec /i ThemisDB-1.8.0-Windows-x64.msi /quiet
sc.exe start ThemisDB
```

### Linux / macOS — TGZ

```bash
# Stop service
sudo systemctl stop themisdb  # or launchctl unload on macOS

# Restore binaries from previous archive
sudo tar -xzf themisdb-1.8.0-Linux-x86_64.tar.gz \
    -C /opt/themisdb --strip-components=1 \
    --exclude='*/config*'

# Start service
sudo systemctl start themisdb
```

### Data rollback

If the new version performed a data migration (MAJOR upgrade):

```bash
sudo systemctl stop themisdb

# Restore from backup
sudo rsync -a --delete /var/lib/themisdb-backup-20260425/ /var/lib/themisdb/

# Verify ownership
sudo chown -R themisdb:themisdb /var/lib/themisdb

sudo systemctl start themisdb
```

> **Important:** Only downgrade data if the new version has modified the
> on-disk format (check the migration guide). A data backup from before the
> upgrade is required. Never restore data backups across non-compatible MAJOR versions
> without consulting the migration guide.

---

## 9. Automated update strategies

### Linux — unattended-upgrades (Debian/Ubuntu)

For DEB packages, `unattended-upgrades` can apply security hotfixes automatically.
Create `/etc/apt/apt.conf.d/52themisdb`:

```
// Auto-apply ThemisDB security hotfixes
Unattended-Upgrade::Allowed-Origins {
    "themisdb:stable";
};
Unattended-Upgrade::Package-Blacklist {
    // Add package names to exclude from auto-update
};
Unattended-Upgrade::Automatic-Reboot "false";
Unattended-Upgrade::Mail "ops@example.com";
```

> **Recommendation:** Enable automatic updates only for security patches (PATCH with
> `security` label). MINOR and MAJOR updates should always be applied manually after
> testing in staging.

### Linux — systemd timer for update check

```ini
# /etc/systemd/system/themisdb-update-check.service
[Unit]
Description=Check for ThemisDB updates

[Service]
Type=oneshot
ExecStart=/usr/bin/bash -c '\
  CURRENT=$(themis_server --version | grep -oP "[0-9]+\.[0-9]+\.[0-9]+"); \
  LATEST=$(curl -sf https://api.github.com/repos/makr-code/ThemisDB/releases/latest | grep -oP "(?<=\"tag_name\": \"v)[^\"]+"); \
  if [ "$CURRENT" != "$LATEST" ]; then \
    echo "ThemisDB update available: v$CURRENT -> v$LATEST" | systemd-cat -t themisdb-update; \
  fi'
```

```ini
# /etc/systemd/system/themisdb-update-check.timer
[Unit]
Description=Daily ThemisDB update check

[Timer]
OnCalendar=daily
Persistent=true

[Install]
WantedBy=timers.target
```

```bash
sudo systemctl enable --now themisdb-update-check.timer
```

### Windows — Scheduled Task

```powershell
$action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument @"
  \$current = (themis_server.exe --version) -replace '.*v([0-9.]+).*', '\$1'
  \$latest = (Invoke-RestMethod https://api.github.com/repos/makr-code/ThemisDB/releases/latest).tag_name -replace 'v',''
  if (\$current -ne \$latest) {
    Write-EventLog -LogName Application -Source ThemisDB -EventId 1001 -Message "Update available: \$current -> \$latest"
  }
"@
$trigger = New-ScheduledTaskTrigger -Daily -At "09:00"
Register-ScheduledTask -TaskName "ThemisDB Update Check" -Action $action -Trigger $trigger -RunLevel Highest
```

---

## 10. Configuration drift management

As versions evolve, the default `config.yaml` template gains new keys. To keep your
deployment config aligned:

### Check for new defaults

```bash
# After installing a new version, compare configs:
diff /etc/themisdb/config.yaml \
     /usr/share/doc/themisdb/config.yaml.example 2>/dev/null || \
diff /etc/themisdb/config.yaml \
     /opt/themisdb/config/config.yaml 2>/dev/null
```

On DEB upgrades, dpkg preserves your existing config and places the new template at
`/etc/themisdb/config.yaml.dpkg-new`. Review and merge manually:

```bash
sudo diff /etc/themisdb/config.yaml /etc/themisdb/config.yaml.dpkg-new
# Apply desired new keys, then remove the .dpkg-new file
sudo rm /etc/themisdb/config.yaml.dpkg-new
```

### Version-controlled configuration

Store your `config.yaml` in a version control system (git, Ansible, Puppet, etc.)
so that changes are tracked and auditable:

```bash
# Example: track config in a private git repo
git init /etc/themisdb
cd /etc/themisdb
git add config.yaml
git commit -m "initial config v1.8.0"
# On each update:
git diff config.yaml          # review changes before committing
git commit -am "updated for v1.8.1"
```

---

## 11. Multi-node / clustered deployments

For HA or distributed deployments, use a **rolling update** strategy to avoid downtime.

### Rolling update sequence (PATCH/MINOR)

```
For each follower node (one at a time):
  1. Drain the node (stop accepting new connections if possible):
       themisctl node drain <node-id>
  2. Apply the update package (§4) on that node
  3. Start ThemisDB on the node
  4. Wait for the node to rejoin the cluster and catch up on replication:
       themisctl cluster status  →  all nodes "healthy"
  5. Verify the node is serving traffic before proceeding to the next

After all followers are updated:
  6. Failover the leader to an already-updated follower:
       themisctl cluster transfer-leader
  7. Update the old leader node following the same per-node procedure
  8. Verify cluster health:
       themisctl cluster status
```

> **Downtime expectation:** A single leader failover takes a few seconds (leader
> election). Client SDKs with built-in retry logic handle this transparently.

### Zero-downtime constraint

| Update type | Rolling update possible? | Notes |
|-------------|--------------------------|-------|
| PATCH / Hotfix | ✅ Yes | Protocol-compatible; no cluster-wide coordination needed |
| MINOR | ✅ Yes | New features only become available after all nodes are updated |
| MAJOR | ⚠️ Limited | Check migration guide for mixed-version support window |

### Pre-flight cluster health check

Always verify cluster health before starting a rolling update:

```bash
themisctl cluster status
# Expected: all nodes "healthy", replication lag < 1 s, no pending repairs
```

---

## See also

- [DEPLOYMENT.md](DEPLOYMENT.md) — Initial installation guide (systemd, launchd, Windows Service)
- [CPACK.md](CPACK.md) — Building packages from source
- [VERSIONING.md](../../VERSIONING.md) — Versioning policy and release types
- [SOP.md](../../SOP.md) — Developer/release-manager procedures (SOP-01 through SOP-08)
- [SECURITY.md](../../SECURITY.md) — Security vulnerability reporting and patch SLA
- [CHANGELOG.md](../../CHANGELOG.md) — Full release history

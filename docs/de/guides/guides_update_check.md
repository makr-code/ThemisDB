---
category: "⚙️ Operations/Admin"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# ⚙️ Automatic Update Check

Guide to automatic version checking and update notifications.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Quick Start](#-quick-start)
- [📖 Manual & Automatic Checks](#-manual--automatic-checks)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

ThemisDB supports automatic version checking to notify administrators when new updates are available.

**Stand:** 6. April 2026  
**Version:** 1.3.0  
**Kategorie:** ⚙️ Operations/Admin

---

## ✨ Features

- 🔄 **Automatic Checks** - Periodic update polling
- 📱 **Version Notifications** - Alert admins of new releases
- 🔒 **Security Updates** - Prioritized security release detection
- 🔗 **Download Links** - Direct links to latest releases
- 📊 **Version Comparison** - Track current vs latest version
- ⚙️ **Configurable** - Customize check frequency

---

## 🚀 Quick Start

---

## Overview

ThemisDB instances can periodically check for updates by querying the version manifest hosted on GitHub. This allows you to:

- Be notified when new versions are released
- Check for security updates
- View download links for the latest version
- Compare your current version with the latest stable release

## Version Manifest Location

The version manifest is hosted at:
```
https://raw.githubusercontent.com/makr-code/ThemisDB/main/docs/VERSION.json
```

## Manual Version Check

You can manually check the current version of your ThemisDB instance:

```bash
# Using the version endpoint
curl http://localhost:8765/version

# Response:
{
  "version": "1.0.0",
  "build_date": "2025-12-02",
  "commit": "abc1234",
  "update_available": false
}
```

## Automatic Update Checking

### Enable Update Checks

ThemisDB can be configured to automatically check for updates:

**Via Environment Variable:**
```bash
export THEMIS_UPDATE_CHECK_ENABLED=true
export THEMIS_UPDATE_CHECK_INTERVAL=86400  # Check once per day (in seconds)
```

**Via Configuration File (`config/config.json`):**
```json
{
  "features": {
    "update_check": {
      "enabled": true,
      "interval_seconds": 86400,
      "notify_on_security_updates": true,
      "notify_on_breaking_changes": true,
      "channel": "stable"
    }
  }
}
```

### Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `enabled` | `false` | Enable automatic update checking |
| `interval_seconds` | `86400` | How often to check for updates (24 hours) |
| `notify_on_security_updates` | `true` | Log warnings for security updates |
| `notify_on_breaking_changes` | `true` | Log warnings for breaking changes |
| `channel` | `stable` | Update channel: `stable`, `beta`, `nightly` |
| `check_url` | (default) | Custom URL for version manifest |

### Update Check Behavior

When update checking is enabled:

1. **On Startup**: ThemisDB checks for updates when the server starts
2. **Periodic Checks**: Checks occur at the configured interval
3. **Logging**: Update information is logged to the server logs
4. **No Auto-Update**: ThemisDB never automatically updates itself - it only notifies

### Log Output Examples

**No Update Available:**
```
[themis] [info] Update check: Running version 1.0.0 (latest stable)
```

**Update Available:**
```
[themis] [warning] Update available: v1.1.0 is now available (current: 1.0.0)
[themis] [info] Download: https://github.com/makr-code/ThemisDB/releases/tag/v1.1.0
[themis] [info] Changelog: https://github.com/makr-code/ThemisDB/blob/main/CHANGELOG.md
```

**Security Update:**
```
[themis] [warning] SECURITY UPDATE AVAILABLE: v1.0.1 contains security fixes
[themis] [warning] Please update as soon as possible
[themis] [info] Security advisory: https://github.com/makr-code/ThemisDB/security/advisories/...
```

## API Endpoint

### GET /version

Returns the current version and update information.

**Request:**
```bash
curl http://localhost:8765/version
```

**Response:**
```json
{
  "current": {
    "version": "1.0.0",
    "build_date": "2025-12-02",
    "commit": "abc1234",
    "platform": "linux-x64"
  },
  "latest": {
    "version": "1.0.0",
    "release_date": "2025-12-02",
    "channel": "stable",
    "update_available": false,
    "breaking_changes": false,
    "security_updates": [],
    "announcement": "ThemisDB v1.0.0 - First stable release"
  },
  "downloads": {
    "linux_x64_zip": {
      "url": "https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb-1.0.0-linux-x64.zip",
      "sha256": "8B075931270487B493F9244738829CF84752D33FF7381B624044C988A00FCC80"
    },
    "debian_amd64": {
      "url": "https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/themisdb_1.0.0_amd64.deb",
      "sha256": "D922D21C4D7EAAD2FF18F784E4C447BF8411925B14F582883E4BE22369D4B5C3"
    }
  }
}
```

## Manual Update Process

ThemisDB does not support in-place updates. To update:

### Docker

```bash
# Pull latest image
docker pull themisdb/themisdb:latest

# Restart container
docker restart themisdb
```

### Debian/Ubuntu

```bash
# Download new .deb
wget https://github.com/makr-code/ThemisDB/releases/download/v1.1.0/themisdb_1.1.0_amd64.deb

# Stop service
sudo systemctl stop themisdb

# Backup data (optional but recommended)
sudo cp -r /var/lib/themisdb/data /var/lib/themisdb/data.backup

# Install update
sudo dpkg -i themisdb_1.1.0_amd64.deb

# Start service
sudo systemctl start themisdb
```

### RHEL/CentOS/Fedora

```bash
# Download new .rpm
wget https://github.com/makr-code/ThemisDB/releases/download/v1.1.0/themisdb-1.1.0-1.x86_64.rpm

# Stop service
sudo systemctl stop themisdb

# Backup data
sudo cp -r /var/lib/themisdb/data /var/lib/themisdb/data.backup

# Install update
sudo rpm -Uvh themisdb-1.1.0-1.x86_64.rpm

# Start service
sudo systemctl start themisdb
```

### Binary Installation

```bash
# Download new version
wget https://github.com/makr-code/ThemisDB/releases/download/v1.1.0/themisdb-1.1.0-linux-x64.zip
unzip themisdb-1.1.0-linux-x64.zip

# Stop current instance
pkill themis_server

# Backup data
cp -r data data.backup

# Replace binary
cp themisdb-1.1.0-linux-x64/themis_server ./themis_server

# Start new version
./themis_server
```

## Security Considerations

- Update checks are **read-only** and do not modify your installation
- No sensitive data is transmitted during update checks
- Only version information is requested from GitHub
- Update checks can be disabled if you prefer manual version management

## Troubleshooting

### Update Check Fails

If update checks fail, check:

1. **Network connectivity**: Can the server reach GitHub?
   ```bash
   curl -I https://raw.githubusercontent.com/makr-code/ThemisDB/main/docs/VERSION.json
   ```

2. **Firewall rules**: Ensure outbound HTTPS traffic is allowed

3. **Server logs**: Check for error messages
   ```bash
   sudo journalctl -u themisdb -n 100 | grep -i update
   ```

### Disable Update Checks

To completely disable update checking:

**Environment Variable:**
```bash
export THEMIS_UPDATE_CHECK_ENABLED=false
```

**Configuration File:**
```json
{
  "features": {
    "update_check": {
      "enabled": false
    }
  }
}
```

## Version Comparison Algorithm

ThemisDB uses semantic versioning (semver) for version comparison:

- **Major version** (1.x.x): Breaking changes
- **Minor version** (x.1.x): New features, backwards compatible
- **Patch version** (x.x.1): Bug fixes, backwards compatible

Update priority is determined by:
- **Critical**: Security updates
- **High**: Major version updates with breaking changes
- **Normal**: Minor/patch updates
- **Low**: Beta/preview releases

## Privacy

Update checks send minimal information to GitHub:
- No user data or database contents
- No server identification or hostname
- Only a HTTP GET request for the VERSION.json file

The request appears as a standard HTTP request to GitHub's raw content CDN.

## Related Documentation

- [Configuration Guide](configuration.md)
- [Deployment Guide](guides_deployment.md)
- [Changelog](../releases/CHANGELOG.md)
- [Security Policy](../SECURITY.md)

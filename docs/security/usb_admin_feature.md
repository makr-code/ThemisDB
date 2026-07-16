# USB Admin Security Feature

## Overview

The USB Admin Security Feature provides hardware-based authentication for administrative operations in ThemisDB. This feature requires a physical encrypted USB device with a valid license file to be present before allowing privileged administrative functions.

## Key Features

- **Hardware-Based Authentication**: Admin operations require physical USB device presence
- **License Validation**: USB must contain valid, non-expired license bound to specific hardware
- **Silent Failure Mode**: Unauthorized attempts fail silently without revealing USB requirement
- **Audit Logging**: All admin operation attempts are logged for security audit
- **Lockout Protection**: Automatic lockout after repeated failed attempts
- **Seamless Integration**: Integrates with existing AuthMiddleware and RBAC systems
- **USB Volume Hardening**: Three additional layers protect against FAT filesystem manipulation

## Security Benefits

1. **Physical Security Layer**: Adds physical dimension to authentication (something you have)
2. **Hardware Binding**: License tied to specific hardware ID prevents unauthorized transfer
3. **Tamper Prevention**: RSA signatures prevent license file manipulation
4. **Replay Protection**: Challenge-response mechanism prevents replay attacks
5. **Multi-Factor Authentication**: Combines token/JWT auth with physical USB requirement
6. **Zero Trust Architecture**: Admin operations denied by default without USB
7. **FAT Manipulation Protection**: Volume integrity hash detects any file-level tampering
8. **Anti-Cloning**: USB serial binding prevents `dd` clone attacks
9. **Live-Write Prevention**: Read-only mount enforcement blocks writes during authentication

## USB Volume Hardening

The following optional hardening features defend against an attacker who has physical access to the USB stick and attempts to manipulate the FAT filesystem:

### 1. Volume Integrity Hash (FAT-tamper detection)

Set `expected_volume_hash` to the SHA-256 hash of the license file computed at provisioning time.  Any subsequent modification — whether via a hex editor, FAT filesystem tools, or raw sector writes — will produce a different hash and the USB will be rejected before the license is parsed.

```cpp
USBAdminConfig config;
config.mount_path            = "/mnt/themis-admin";
config.expected_volume_hash  = "a3f2..."; // SHA-256 of license file at issue time
```

Compute the hash at provisioning time with:
```bash
sha256sum /mnt/themis-admin/themis_admin.lic
```

### 2. Read-Only Mount Enforcement

Set `require_readonly_mount = true` to reject any USB that is not mounted read-only.  A read-only mount prevents any process — including a compromised one running on the host — from writing to the stick while it is in use.

```bash
# Mount the USB read-only
mount -o ro /dev/sdb1 /mnt/themis-admin
```

```cpp
config.require_readonly_mount = true;
```

### 3. USB Device Serial Binding (anti-cloning)

Set `expected_usb_serial` to the hardware serial of the provisioned stick.  A `dd`-cloned copy will report a different USB string descriptor serial and be rejected.

```bash
# Read the serial on Linux
cat /sys/class/block/sdb/device/../../serial
```

```cpp
config.expected_usb_serial = "ABC123456789";
```

### Hardening Metrics

Three new `Metrics` counters track rejections from hardening checks:

| Counter | Description |
|---------|-------------|
| `usb_denied_not_readonly` | USB was writable (require_readonly_mount violated) |
| `usb_denied_volume_hash_mismatch` | License file was tampered (FAT manipulation detected) |
| `usb_denied_serial_mismatch` | Wrong USB device serial (cloned stick detected) |

## Configuration

### Enable USB Admin Authentication

```cpp
#include "server/auth_middleware.h"

// Create AuthMiddleware
AuthMiddleware auth;

// Enable USB admin authentication
auth.enableUSBAdminAuth("/mnt/themis-admin");
```

## License File Format

```json
{
  "license_key": "THEMIS-ENT-ADMIN-12345678-ABCDEF90",
  "organization": "Example Corporation",
  "hardware_id": "550e8400-e29b-41d4-a716-446655440000",
  "issued_date": "2026-01-01",
  "expiry_date": "2027-12-31",
  "admin_scopes": ["admin", "config:write", "cdc:admin"],
  "signature": "RSA-SHA256-SIGNATURE-HERE"
}
```

## Usage

Admin operations require both:
1. Valid API token with admin scope
2. Valid USB device present

```bash
curl -X POST https://themis-server:8765/admin/backup \
  -H "Authorization: Bearer admin-token-123"
```

For complete documentation, see [USB Admin Authentication Guide](usb_admin_authentication.md)


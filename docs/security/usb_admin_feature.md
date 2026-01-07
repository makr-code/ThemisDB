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

## Security Benefits

1. **Physical Security Layer**: Adds physical dimension to authentication (something you have)
2. **Hardware Binding**: License tied to specific hardware ID prevents unauthorized transfer
3. **Tamper Prevention**: RSA signatures prevent license file manipulation
4. **Replay Protection**: Challenge-response mechanism prevents replay attacks
5. **Multi-Factor Authentication**: Combines token/JWT auth with physical USB requirement
6. **Zero Trust Architecture**: Admin operations denied by default without USB

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

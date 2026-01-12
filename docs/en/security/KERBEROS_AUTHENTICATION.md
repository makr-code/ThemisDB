# Kerberos/GSSAPI Authentication Guide

## Overview

ThemisDB supports Kerberos/GSSAPI authentication for enterprise Single Sign-On (SSO) integration. This allows users to authenticate using their existing Kerberos credentials (from MIT Kerberos, Active Directory, or Heimdal) without managing separate database passwords.

## Features

- ✅ **MIT Kerberos 5** support
- ✅ **Active Directory** integration
- ✅ **Heimdal Kerberos** support
- ✅ **Principal-to-Role mapping** with RBAC
- ✅ **Wildcard patterns** for flexible role assignment
- ✅ **Fallback to basic authentication** when Kerberos is unavailable
- ✅ **Cross-platform** (Linux, Windows, macOS)

## Quick Start

### 1. Build with Kerberos Support

```bash
# Enable Kerberos support during build
cmake -DTHEMIS_ENABLE_KERBEROS=ON ..
make
```

### 2. Install Kerberos Client

**Ubuntu/Debian:**
```bash
sudo apt-get install krb5-user libkrb5-dev libgssapi-krb5-2
```

**RHEL/CentOS:**
```bash
sudo yum install krb5-workstation krb5-devel
```

**macOS:**
```bash
brew install krb5
```

### 3. Configure Kerberos

Create or edit `/etc/krb5.conf`:

```ini
[libdefaults]
    default_realm = EXAMPLE.COM
    dns_lookup_realm = false
    dns_lookup_kdc = false

[realms]
    EXAMPLE.COM = {
        kdc = kdc.example.com
        admin_server = kdc.example.com
    }

[domain_realm]
    .example.com = EXAMPLE.COM
    example.com = EXAMPLE.COM
```

### 4. Register Service Principal

On your Kerberos KDC:

```bash
# As Kerberos admin
kadmin.local
addprinc -randkey themisdb/hostname.example.com@EXAMPLE.COM
ktadd -k /tmp/themisdb.keytab themisdb/hostname.example.com@EXAMPLE.COM
quit
```

Copy the keytab to your ThemisDB server:

```bash
scp /tmp/themisdb.keytab root@dbserver:/etc/themisdb/themisdb.keytab
chmod 600 /etc/themisdb/themisdb.keytab
chown themisdb:themisdb /etc/themisdb/themisdb.keytab
```

### 5. Configure ThemisDB

Create or edit your authentication configuration file (e.g., `config/auth.yaml`):

```yaml
authentication:
  methods:
    - kerberos
    - jwt
    - basic
  
  kerberos:
    enabled: true
    service_principal: "themisdb/hostname.example.com@EXAMPLE.COM"
    keytab_file: "/etc/themisdb/themisdb.keytab"
    krb5_config: "/etc/krb5.conf"
    fallback_to_basic: true
    
    principal_mappings:
      - principal_pattern: "admin@EXAMPLE.COM"
        role: "admin"
      - principal_pattern: "*@EXAMPLE.COM"
        role: "readonly"
```

### 6. Load Configuration in Code

```cpp
#include "auth/gssapi_authenticator.h"
#include "server/auth_middleware.h"

// Configure Kerberos
themis::auth::KerberosConfig krb_config;
krb_config.enabled = true;
krb_config.service_principal = "themisdb/hostname.example.com@EXAMPLE.COM";
krb_config.keytab_file = "/etc/themisdb/themisdb.keytab";
krb_config.krb5_config = "/etc/krb5.conf";
krb_config.fallback_to_basic = true;

// Add principal mappings
krb_config.principal_mappings.push_back({
    "admin@EXAMPLE.COM",
    "admin"
});
krb_config.principal_mappings.push_back({
    "*@EXAMPLE.COM",
    "readonly"
});

// Enable in AuthMiddleware
themis::AuthMiddleware auth_middleware;
auth_middleware.enableKerberos(krb_config);
```

## Configuration Options

### Service Principal

The service principal follows the format: `service/hostname@REALM`

- `service`: Usually "themisdb" or your application name
- `hostname`: Fully qualified domain name of your server
- `REALM`: Kerberos realm in uppercase

Example: `themisdb/db01.company.com@COMPANY.COM`

### Principal Mappings

Map Kerberos principals to ThemisDB RBAC roles:

```yaml
principal_mappings:
  # Exact match
  - principal_pattern: "alice@EXAMPLE.COM"
    role: "operator"
  
  # Wildcard - all users in realm
  - principal_pattern: "*@EXAMPLE.COM"
    role: "readonly"
  
  # Multiple realms
  - principal_pattern: "dbadmin@AD.COMPANY.COM"
    role: "admin"
  - principal_pattern: "*@AD.COMPANY.COM"
    role: "analyst"
```

### Fallback Authentication

When `fallback_to_basic: true`, ThemisDB will try:
1. Kerberos authentication (if token looks like GSSAPI)
2. JWT validation (if token is a JWT)
3. Basic authentication (username/password)

## Client Usage

### Get Kerberos Ticket

First, obtain a Kerberos ticket:

```bash
kinit user@EXAMPLE.COM
# Enter password
klist  # Verify ticket
```

### Connect to ThemisDB

```cpp
#include "clients/themisdb_client.h"

// Client automatically uses Kerberos ticket from credential cache
ThemisDBClient client;
client.connect("hostname.example.com:9000", {
    .auth_method = "kerberos"
});
```

### Python Client Example

```python
import gssapi
from themisdb import Client

# Acquire GSSAPI credentials
service_name = gssapi.Name("themisdb/hostname.example.com@EXAMPLE.COM", 
                           gssapi.NameType.hostbased_service)
ctx = gssapi.SecurityContext(name=service_name, usage='initiate')

# Get authentication token
token = ctx.step()

# Connect to ThemisDB
client = Client()
client.connect("hostname.example.com:9000", auth_token=token)
```

## Active Directory Integration

### On AD Domain Controller

```powershell
# Register service principal
setspn -A themisdb/db01.company.com dbservice

# Generate keytab
ktpass -princ themisdb/db01.company.com@AD.COMPANY.COM `
       -mapuser dbservice `
       -pass * `
       -out themisdb.keytab `
       -ptype KRB5_NT_PRINCIPAL
```

### ThemisDB Configuration

```yaml
authentication:
  kerberos:
    enabled: true
    service_principal: "themisdb/db01.company.com@AD.COMPANY.COM"
    keytab_file: "/etc/themisdb/themisdb.keytab"
    
    principal_mappings:
      - principal_pattern: "*@AD.COMPANY.COM"
        role: "readonly"
```

## Troubleshooting

### Enable Debug Logging

```bash
export KRB5_TRACE=/dev/stdout
```

### Check Keytab

```bash
klist -k /etc/themisdb/themisdb.keytab
```

### Verify Principal

```bash
kvno themisdb/hostname.example.com@EXAMPLE.COM
```

### Check Time Synchronization

Kerberos requires time synchronization (within 5 minutes):

```bash
ntpdate -q pool.ntp.org
```

### Common Errors

**Error: "gss_acquire_cred failed"**
- Check keytab file permissions (should be 600)
- Verify keytab contains correct principal: `klist -k keytab_file`
- Ensure KRB5_KTNAME environment variable is set if needed

**Error: "Clock skew too great"**
- Synchronize clocks between client, server, and KDC
- Use NTP: `ntpdate kdc.example.com`

**Error: "Server not found in Kerberos database"**
- Service principal not registered with KDC
- Run `kadmin: addprinc -randkey service/host@REALM`

**Error: "Ticket expired"**
- Renew ticket: `kinit -R`
- Get new ticket: `kinit user@REALM`

## Security Considerations

### Keytab Protection

- Store keytab with restricted permissions (600)
- Use dedicated service account
- Rotate keytabs regularly (every 90 days)
- Never commit keytabs to version control

### Network Security

- Use encrypted channels (TLS) for all communications
- Restrict KDC access with firewall rules
- Monitor authentication failures

### Principal Naming

- Use fully qualified hostnames
- Follow your organization's naming conventions
- Document all service principals

## Performance

### Ticket Caching

ThemisDB caches validated GSSAPI security contexts to reduce authentication overhead:

- Average authentication time: <10ms
- Cached authentication: <1ms
- Cache invalidation on ticket expiration

### Monitoring

Monitor Kerberos authentication with Prometheus metrics:

```
themisdb_auth_kerberos_success_total
themisdb_auth_kerberos_failure_total
themisdb_auth_kerberos_duration_seconds
```

## Related Documentation

- [RBAC Configuration](../security/RBAC.md)
- [JWT Authentication](../security/JWT_AUTHENTICATION.md)
- [Audit Logging](../security/AUDIT_LOGGING.md)
- [MIT Kerberos Documentation](https://web.mit.edu/kerberos/)
- [RFC 4120: Kerberos V5](https://tools.ietf.org/html/rfc4120)
- [RFC 4121: Kerberos V5 GSSAPI](https://tools.ietf.org/html/rfc4121)

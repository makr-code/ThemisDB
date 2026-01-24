# ThemisDB API Versioning and Compatibility Strategy

## Overview

ThemisDB implements comprehensive API versioning to ensure backward compatibility and smooth upgrades for all API types:
- **REST API**: HTTP/HTTPS endpoints
- **gRPC API**: Protocol Buffers services
- **GraphQL API**: GraphQL queries and mutations

**Current Version**: v1.4.1  
**Minimum Supported Version**: v1.0.0  
**Version Format**: Semantic Versioning (Major.Minor.Patch)

## Version Negotiation

### REST API

Use the `Accept-Version` header to specify the desired API version:

```http
GET /api/entities/urn:themis:entity:123
Accept-Version: v1.3.0
Authorization: Bearer <token>
```

**Response** includes version information:

```http
HTTP/1.1 200 OK
API-Version: v1.3.0
Content-Type: application/json

{
  "data": { ... }
}
```

#### Supported Formats

- `v1.4.1` - Full version
- `v1.4` - Minor version (resolves to latest patch)
- `v1` - Major version (resolves to latest minor.patch)
- `latest` - Current stable version

If no `Accept-Version` header is provided, the current stable version is used.

### gRPC API

Use metadata to specify API version in gRPC calls:

```cpp
grpc::ClientContext context;
context.AddMetadata("api-version", "v1.4.0");

CreateRequest request;
CreateResponse response;
stub->Create(&context, request, &response);
```

**Response metadata** includes:

```
api-version: v1.4.0
```

### GraphQL API

Specify version in the request header or query parameter:

```graphql
# Header-based
Accept-Version: v1.4.0

# Query parameter
query {
  entities(version: "v1.4.0") {
    id
    data
  }
}
```

## Deprecation Policy

### 24-Month Deprecation Window

ThemisDB follows a **24-month deprecation policy**:

1. **Deprecation Announcement** (Month 0)
   - Feature/endpoint marked as deprecated
   - Deprecation headers added to responses
   - Migration guide published

2. **Deprecation Period** (Months 0-24)
   - Feature continues to work
   - Deprecation warnings in logs and responses
   - Migration support available

3. **Removal** (Month 24+)
   - Feature removed in next major version
   - Breaking change documented in CHANGELOG
   - Upgrade path clearly defined

### Deprecation Headers

When accessing deprecated endpoints, responses include:

```http
HTTP/1.1 200 OK
API-Version: v1.3.0
Deprecation: true; deprecated-version="v1.3.0"; removal-version="v2.0.0"
Sunset: Wed, 24 Jan 2028 06:00:00 GMT
Link: <https://docs.themisdb.com/migration/v1-to-v2>; rel="deprecation"
```

**Headers**:
- `Deprecation`: RFC draft - indicates deprecation status
- `Sunset`: RFC 8594 - removal date
- `Link`: Migration guide URL

## Compatibility Matrix

### Supported Versions

| Version | Status | Release Date | End of Support | Notes |
|---------|--------|--------------|----------------|-------|
| v1.4.x  | Current | 2026-01-19 | TBD | Production ready |
| v1.3.x  | Supported | 2025-09-15 | 2027-09-15 | 24-month support |
| v1.2.x  | Supported | 2025-03-10 | 2027-03-10 | 24-month support |
| v1.1.x  | Supported | 2024-09-01 | 2026-09-01 | 24-month support |
| v1.0.x  | Minimum | 2024-01-15 | 2026-01-15 | Basic compatibility |

### Breaking Changes by Version

#### v1.4.0 → v1.5.0 (Planned)
- **No breaking changes planned**
- Focus on feature additions and performance

#### v1.3.x → v1.4.0
- Extended context window (backward compatible)
- New LLM/LoRA endpoints (additive)
- Enhanced pagination (backward compatible)

#### v1.2.x → v1.3.0
- Query optimizer improvements (transparent)
- New authentication methods (additive)

#### v1.1.x → v1.2.0
- Enhanced transaction semantics (backward compatible)
- New sharding features (opt-in)

#### v1.0.x → v1.1.0
- Initial stable release to first feature update
- No breaking changes

### Forward Compatibility

Clients should be designed to handle:
- **Unknown fields**: Ignore fields not in their API version
- **New optional parameters**: Skip parameters they don't recognize
- **Deprecated warnings**: Log and plan migration

## Version Detection

### Client Detection

Detect server API version:

```bash
# REST
curl -I https://api.themisdb.com/api/status | grep "API-Version"

# Response
API-Version: v1.4.1
```

### Server Capabilities

Query server for supported versions:

```http
GET /api/status
```

```json
{
  "version": "1.4.1-dev",
  "api_version": {
    "major": 1,
    "minor": 4,
    "patch": 1
  },
  "supported_api_versions": [
    {"major": 1, "minor": 0, "patch": 0},
    {"major": 1, "minor": 1, "patch": 0},
    {"major": 1, "minor": 2, "patch": 0},
    {"major": 1, "minor": 3, "patch": 0},
    {"major": 1, "minor": 4, "patch": 0},
    {"major": 1, "minor": 4, "patch": 1}
  ]
}
```

## Client Libraries

### Official SDKs

All official SDKs support version negotiation:

```python
# Python
from themisdb import Client

client = Client(
    url="https://api.themisdb.com",
    api_version="v1.4.0"
)
```

```javascript
// JavaScript/TypeScript
import { ThemisClient } from 'themisdb-client';

const client = new ThemisClient({
  url: 'https://api.themisdb.com',
  apiVersion: 'v1.4.0'
});
```

```go
// Go
import "github.com/makr-code/themisdb-go"

client := themisdb.NewClient(
    themisdb.WithURL("https://api.themisdb.com"),
    themisdb.WithAPIVersion("v1.4.0"),
)
```

## Migration Guides

See detailed migration guides for version transitions:

- [v1.3.x to v1.4.x Migration Guide](../migration/v1.3-to-v1.4.md)
- [v1.2.x to v1.3.x Migration Guide](../migration/v1.2-to-v1.3.md)
- [v1.1.x to v1.2.x Migration Guide](../migration/v1.1-to-v1.2.md)
- [v1.0.x to v1.1.x Migration Guide](../migration/v1.0-to-v1.1.md)

## Best Practices

### For API Consumers

1. **Always specify API version** in production
2. **Monitor deprecation warnings** in logs
3. **Test against new versions** before upgrading
4. **Use version pinning** in configuration
5. **Subscribe to breaking changes** announcements

### For API Providers

1. **Document all breaking changes** in CHANGELOG
2. **Provide migration guides** for deprecated features
3. **Support multiple versions** during transition
4. **Use semantic versioning** consistently
5. **Communicate early** about deprecations

## FAQ

### What happens if I don't specify a version?

The current stable version (v1.4.1) will be used.

### Can I use multiple versions simultaneously?

Yes, different requests can use different API versions.

### How do I know if an endpoint is deprecated?

Check the `Deprecation` header in responses and review the [Deprecation Registry](./DEPRECATION_REGISTRY.md).

### What if my version is no longer supported?

You'll receive a 400 Bad Request with guidance to upgrade. Critical security updates may require immediate upgrade.

### Are there any performance differences between versions?

Generally no. Older versions may miss performance optimizations from newer releases.

## Support

For API versioning questions:
- **Documentation**: https://docs.themisdb.com/api/versioning
- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Community**: https://github.com/makr-code/ThemisDB/discussions

## References

- [Semantic Versioning 2.0.0](https://semver.org/)
- [RFC 8594 - Sunset Header](https://www.rfc-editor.org/rfc/rfc8594.html)
- [API Deprecation Best Practices](https://www.ietf.org/archive/id/draft-dalal-deprecation-header-01.html)

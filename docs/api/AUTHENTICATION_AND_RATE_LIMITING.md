# Authentication and Rate-Limiting Guide

## Overview

This guide describes how to configure and use authentication and rate-limiting in ThemisDB's REST/HTTP API. These features are essential for securing your database and preventing abuse.

## Authentication

### JWT-Based Authentication

ThemisDB uses JSON Web Tokens (JWT) for authentication. JWTs are cryptographically signed tokens that contain claims about the user and their permissions.

### Enabling Authentication

To enable authentication, set the following in your server configuration:

```cpp
HttpServer::Config config;
config.auth_enabled = true;
```

Or via configuration file:

```yaml
server:
  auth_enabled: true
```

### JWT Token Structure

A typical JWT token contains:

```json
{
  "header": {
    "alg": "HS256",
    "typ": "JWT"
  },
  "payload": {
    "sub": "user:123",
    "scopes": ["data:read", "data:write"],
    "groups": ["developers", "admins"],
    "iss": "themisdb",
    "aud": "themisdb-api",
    "exp": 1612345678,
    "iat": 1612342078
  },
  "signature": "..."
}
```

### Scopes and Permissions

#### Core Scopes

| Scope | Description | Use Cases |
|-------|-------------|-----------|
| `data:read` | Read data from entities, collections | GET /entities, /query |
| `data:write` | Create, update, delete data | PUT/POST/DELETE /entities |
| `content:read` | Read content and documents | GET /content, /contentfs |
| `content:write` | Upload and modify content | POST /content, /contentfs |
| `index:read` | View index information | GET /index/stats |
| `index:write` | Create and manage indexes | POST /index/create, /index/rebuild |

#### Advanced Scopes

| Scope | Description | Use Cases |
|-------|-------------|-----------|
| `vector:read` | Query vector embeddings | POST /vector/search |
| `vector:write` | Insert and update vectors | POST /vector/batch_insert |
| `graph:read` | Query graph relationships | POST /graph/traverse |
| `graph:write` | Create and delete edges | POST /graph/edge, DELETE /graph/edge |
| `timeseries:read` | Query time-series data | POST /timeseries/query |
| `timeseries:write` | Insert time-series data | POST /timeseries/insert |
| `cdc:read` | Subscribe to change streams | GET /changefeed/stream |
| `cache:read` | Query semantic cache | POST /cache/query |
| `llm:read` | Read prompt templates | GET /prompt |
| `llm:write` | Manage prompts | POST /prompt |

#### Administrative Scopes

| Scope | Description | Use Cases |
|-------|-------------|-----------|
| `admin` | Full administrative access | POST /admin/backup, /admin/restore |
| `config:read` | View server configuration | GET /config |
| `config:write` | Modify server configuration | POST /config |
| `audit:read` | Access audit logs | POST /api/audit/query |
| `pki:sign` | Sign documents | POST /api/pki/sign |
| `pki:verify` | Verify signatures | POST /api/pki/verify |
| `pii:read` | Access PII mappings | GET /pii |
| `pii:write` | Manage PII | POST /pii |

### Making Authenticated Requests

Include the JWT token in the `Authorization` header:

```http
GET /entities/user:123
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

Example with curl:

```bash
curl -H "Authorization: Bearer $TOKEN" \
     http://localhost:8080/entities/user:123
```

Example with Python:

```python
import requests

headers = {
    'Authorization': f'Bearer {token}',
    'Content-Type': 'application/json'
}

response = requests.get(
    'http://localhost:8080/entities/user:123',
    headers=headers
)
```

### Generating JWT Tokens

#### Using a JWT Library

**Python example:**

```python
import jwt
import datetime

def generate_token(user_id, scopes, secret_key):
    payload = {
        'sub': user_id,
        'scopes': scopes,
        'groups': [],
        'iss': 'themisdb',
        'aud': 'themisdb-api',
        'exp': datetime.datetime.utcnow() + datetime.timedelta(hours=1),
        'iat': datetime.datetime.utcnow()
    }
    
    token = jwt.encode(payload, secret_key, algorithm='HS256')
    return token

# Generate a token with data read/write access
token = generate_token(
    user_id='user:123',
    scopes=['data:read', 'data:write'],
    secret_key='your-secret-key'
)
```

**Node.js example:**

```javascript
const jwt = require('jsonwebtoken');

function generateToken(userId, scopes, secretKey) {
    const payload = {
        sub: userId,
        scopes: scopes,
        groups: [],
        iss: 'themisdb',
        aud: 'themisdb-api',
        exp: Math.floor(Date.now() / 1000) + (60 * 60), // 1 hour
        iat: Math.floor(Date.now() / 1000)
    };
    
    return jwt.sign(payload, secretKey, { algorithm: 'HS256' });
}

const token = generateToken(
    'user:123',
    ['data:read', 'data:write'],
    'your-secret-key'
);
```

### Authentication Errors

#### 401 Unauthorized

Returned when no valid JWT token is provided:

```json
{
  "error": "Unauthorized: Missing or invalid token",
  "code": "UNAUTHORIZED",
  "status": 401
}
```

#### 403 Forbidden

Returned when the token is valid but lacks required scopes:

```json
{
  "error": "Forbidden: Insufficient permissions. Required scope: data:write",
  "code": "FORBIDDEN",
  "status": 403
}
```

### Best Practices

1. **Use Short-Lived Tokens**: Set token expiration to 1 hour or less
2. **Implement Token Refresh**: Use refresh tokens to obtain new access tokens
3. **Rotate Secrets**: Regularly rotate JWT signing keys
4. **Use HTTPS**: Always use TLS/SSL in production to protect tokens
5. **Minimum Scopes**: Only grant the minimum required scopes
6. **Audit Token Usage**: Log authentication events for security monitoring

## Rate Limiting

### Overview

Rate limiting prevents abuse by restricting the number of requests a client can make within a time window. ThemisDB uses a token bucket algorithm for rate limiting.

### Enabling Rate Limiting

```cpp
HttpServer::Config config;
config.rate_limiting_enabled = true;
config.global_rate_limit_per_minute = 100;
config.global_rate_limit_burst = 100;
```

### Rate Limit Tiers

#### Global Rate Limit

Applies to all endpoints by default:

- **Rate**: 100 requests per minute per IP/user
- **Burst**: 100 requests (initial capacity)

#### Per-Endpoint Rate Limits

| Endpoint Pattern | Requests/Min | Burst | Reason |
|------------------|--------------|-------|--------|
| `/entities/*` | 1000 | 100 | High-traffic data access |
| `/query` | 500 | 50 | Standard queries |
| `/query/aql` | 300 | 30 | Complex queries |
| `/vector/*` | 500 | 50 | Vector operations |
| `/graph/*` | 500 | 50 | Graph traversals |
| `/content/*` | 500 | 50 | Content operations |
| `/timeseries/*` | 500 | 50 | Time-series data |
| `/cache/*` | 1000 | 100 | Cache queries (fast) |
| `/index/*` | 100 | 20 | Index management (expensive) |
| `/admin/*` | 50 | 10 | Administrative operations |
| `/api/audit/*` | 50 | 10 | Audit queries (expensive) |
| `/config` | 10 | 5 | Configuration changes |
| `/pii/*` | 50 | 10 | Sensitive PII operations |
| `/health` | 1000 | 100 | Health checks |
| `/metrics` | 100 | 20 | Metrics export |

### Rate Limit Headers

Every response includes rate limit headers:

```http
HTTP/1.1 200 OK
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 95
X-RateLimit-Reset: 1612345678
```

- `X-RateLimit-Limit`: Maximum requests allowed in the window
- `X-RateLimit-Remaining`: Requests remaining in current window
- `X-RateLimit-Reset`: Unix timestamp when the limit resets

### Rate Limit Exceeded

When rate limit is exceeded, a 429 response is returned:

```http
HTTP/1.1 429 Too Many Requests
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 0
X-RateLimit-Reset: 1612345678
Retry-After: 60
Content-Type: application/json

{
  "error": "Rate limit exceeded",
  "code": "RATE_LIMIT_EXCEEDED",
  "status": 429,
  "retry_after_seconds": 60
}
```

### Handling Rate Limits

#### Exponential Backoff

Implement exponential backoff when rate limited:

```python
import time
import requests

def make_request_with_backoff(url, max_retries=5):
    backoff_seconds = 1
    
    for retry in range(max_retries):
        response = requests.get(url)
        
        if response.status_code != 429:
            return response
        
        # Rate limited, wait and retry
        retry_after = int(response.headers.get('Retry-After', backoff_seconds))
        time.sleep(retry_after)
        backoff_seconds *= 2  # Exponential backoff
    
    raise Exception('Max retries exceeded')
```

#### Check Remaining Quota

Monitor rate limit headers to avoid hitting limits:

```python
def check_rate_limit(response):
    remaining = int(response.headers.get('X-RateLimit-Remaining', 0))
    reset_time = int(response.headers.get('X-RateLimit-Reset', 0))
    
    if remaining < 10:
        # Close to limit, slow down
        print(f'Warning: Only {remaining} requests remaining')
        print(f'Limit resets at {reset_time}')
        time.sleep(1)
```

### Whitelisting

Whitelist trusted IPs to bypass rate limiting:

```yaml
server:
  rate_limiting:
    whitelist_ips:
      - 10.0.0.0/8      # Internal network
      - 192.168.1.100   # Specific trusted host
```

### Custom Rate Limits

Configure custom rate limits for specific users or IPs:

```yaml
server:
  rate_limiting:
    custom_limits:
      "user:admin": 10000  # 10k requests/min for admin
      "192.168.1.100": 5000  # 5k requests/min for monitoring server
```

### Best Practices

1. **Monitor Headers**: Always check rate limit headers in responses
2. **Implement Backoff**: Use exponential backoff when rate limited
3. **Batch Operations**: Use batch endpoints to reduce request count
4. **Cache Results**: Cache frequently accessed data client-side
5. **Distribute Load**: Distribute requests evenly over time
6. **Plan for Limits**: Design applications to stay within limits

## Advanced Configuration

### Per-Endpoint Configuration

Create a custom configuration for specific endpoints:

```cpp
#include "server/api_auth_config.h"

// Create secure defaults
auto auth_config = ApiAuthConfig::createSecureDefaults();

// Override specific endpoint
EndpointAuthConfig custom_endpoint;
custom_endpoint.endpoint_pattern = "/api/custom/*";
custom_endpoint.required_scope = "custom:access";
custom_endpoint.action = "custom";
custom_endpoint.auth_required = true;
custom_endpoint.rate_limit_per_minute = 200;
custom_endpoint.rate_limit_burst = 40;

auth_config.endpoint_configs.push_back(custom_endpoint);
```

### Development vs Production

Use different configurations for development and production:

```cpp
#ifdef PRODUCTION
    auto auth_config = ApiAuthConfig::createSecureDefaults();
    auth_config.auth_enabled = true;
    auth_config.rate_limiting_enabled = true;
#else
    auto auth_config = ApiAuthConfig::createDevDefaults();
    auth_config.auth_enabled = false;  // Disabled for dev
    auth_config.rate_limiting_enabled = true;
    auth_config.global_rate_limit_per_minute = 10000;  // Lenient for dev
#endif
```

### Policy Engine Integration

ThemisDB can integrate with Apache Ranger or other policy engines for fine-grained authorization:

```cpp
// Policy engine checks are performed in requireAccess()
if (auto resp = requireAccess(req, "data:read", "read", "/entities/user:123")) {
    // User lacks permission based on policy engine
    return *resp;
}
```

### Metrics and Monitoring

Monitor authentication and rate limiting:

```bash
# Prometheus metrics
curl http://localhost:8080/metrics | grep -E "auth|rate_limit"

# Example metrics
themisdb_auth_requests_total{status="success"} 1000
themisdb_auth_requests_total{status="failed"} 5
themisdb_rate_limit_exceeded_total{endpoint="/query"} 10
```

## Troubleshooting

### Common Issues

#### "Unauthorized: Missing or invalid token"

**Cause**: No `Authorization` header or malformed token

**Solution**: Ensure the Authorization header is present and correctly formatted:
```http
Authorization: Bearer <token>
```

#### "Forbidden: Insufficient permissions"

**Cause**: Token lacks required scope

**Solution**: Check the required scope for the endpoint and ensure your token includes it.

#### "Rate limit exceeded"

**Cause**: Too many requests in time window

**Solution**: Implement exponential backoff and respect `Retry-After` header.

### Debug Mode

Enable debug logging for authentication:

```cpp
// In server configuration
themis::utils::Logger::setLevel(themis::utils::LogLevel::DEBUG);
```

Debug logs will show:
- Token validation results
- Scope checks
- Rate limit calculations
- Policy engine decisions

### Testing Authentication

Test authentication with curl:

```bash
# Generate a test token (requires JWT library)
TOKEN=$(python3 -c "import jwt; print(jwt.encode({'sub':'test','scopes':['data:read'],'exp':9999999999}, 'secret', algorithm='HS256'))")

# Test authenticated request
curl -H "Authorization: Bearer $TOKEN" \
     http://localhost:8080/entities/test:123

# Test without authentication (should fail)
curl http://localhost:8080/entities/test:123
```

## Security Considerations

### JWT Secret Management

1. **Never Hardcode**: Don't hardcode JWT secrets in source code
2. **Environment Variables**: Store secrets in environment variables
3. **Secret Rotation**: Implement secret rotation procedures
4. **Key Length**: Use at least 256-bit secrets

### HTTPS/TLS

1. **Always Use HTTPS**: Never transmit JWT tokens over HTTP
2. **Certificate Management**: Use valid TLS certificates
3. **HSTS**: Enable HTTP Strict Transport Security
4. **TLS 1.3**: Use TLS 1.3 or TLS 1.2 minimum

### Token Security

1. **Short Expiration**: Limit token lifetime to 1 hour or less
2. **Refresh Tokens**: Implement refresh token mechanism
3. **Token Revocation**: Implement token revocation/blacklisting
4. **Secure Storage**: Store tokens securely (httpOnly cookies, secure storage)

### Rate Limiting Bypass

1. **Distributed Systems**: Use Redis or similar for shared rate limit state
2. **IP Spoofing**: Validate client IPs and use X-Forwarded-For carefully
3. **Bot Detection**: Implement additional bot detection if needed

## Migration Guide

### Enabling Authentication in Existing Deployments

1. **Phase 1**: Enable authentication but use lenient defaults
   ```cpp
   auto config = ApiAuthConfig::createSecureDefaults();
   config.auth_enabled = true;
   // Initially set high rate limits for all endpoints
   for (auto &endpoint : config.endpoint_configs) {
       endpoint.rate_limit_per_minute *= 10;  // 10x normal limits
   }
   ```

2. **Phase 2**: Issue tokens to all clients and monitor usage

3. **Phase 3**: Enable enforcement gradually per endpoint
   ```cpp
   config.auth_enabled = true;
   // Disable auth for specific endpoints during migration
   for (auto &endpoint : config.endpoint_configs) {
       if (endpoint.endpoint_pattern == "/entities/*") {
           endpoint.auth_required = false;  // Still allow unauthenticated
       } else {
           endpoint.auth_required = true;
       }
   }
   ```

4. **Phase 4**: Full enforcement
   ```cpp
   // Use secure defaults - all endpoints require authentication
   auto config = ApiAuthConfig::createSecureDefaults();
   ```

### Adding Rate Limiting

1. **Start Lenient**: Begin with high limits
   ```cpp
   config.global_rate_limit_per_minute = 10000;
   ```

2. **Monitor Usage**: Track actual request patterns

3. **Adjust Gradually**: Lower limits based on usage data

4. **Set Production Limits**: Apply appropriate limits for production

## References

- [JWT Specification (RFC 7519)](https://tools.ietf.org/html/rfc7519)
- [OAuth 2.0 Scopes](https://tools.ietf.org/html/rfc6749#section-3.3)
- [Rate Limiting Best Practices](https://cloud.google.com/architecture/rate-limiting-strategies-techniques)
- [REST API Security](https://restfulapi.net/security-essentials/)

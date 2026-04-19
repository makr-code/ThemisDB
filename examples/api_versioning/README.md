> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# API Versioning Examples

This directory contains examples demonstrating API versioning in ThemisDB.

## Quick Examples

### REST API with Version Header

```bash
# Request specific API version
curl -H "Accept-Version: v1.4.0" \
     http://localhost:8080/api/health

# Response includes version header
# HTTP/1.1 200 OK
# API-Version: v1.4.0
```

### Python Client

```python
import requests

headers = {'Accept-Version': 'v1.4.0'}
response = requests.get('http://localhost:8080/api/health', headers=headers)
print(f"API Version: {response.headers.get('API-Version')}")
```

### gRPC Client (C++)

```cpp
grpc::ClientContext context;
context.AddMetadata("api-version", "v1.4.0");
stub->Create(&context, request, &response);
```

## Documentation

- [API Versioning Guide](../../docs/api/API_VERSIONING.md)
- [Migration Guides](../../docs/migration/README.md)

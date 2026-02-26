# ThemisDB OpenAPI Generated Client SDKs

This directory contains client SDKs automatically generated from the
[ThemisDB OpenAPI specification](../openapi.yaml) using
[openapi-generator](https://openapi-generator.tech/) v7.10.0.

> **Note:** These are auto-generated SDKs derived directly from the OpenAPI
> spec. For production-ready, hand-crafted clients with additional features
> (circuit breaker, retry logic, connection pooling), see
> [`clients/`](../../clients/).

## Available Generated SDKs

| Language   | Directory      | Generator        | Package / Module                                    |
|------------|----------------|------------------|-----------------------------------------------------|
| Python     | `python/`      | `python`         | `themisdb_client` (pip)                             |
| JavaScript | `javascript/`  | `typescript-fetch` | `@themisdb/openapi-client` (npm)                  |
| Go         | `go/`          | `go`             | `github.com/makr-code/ThemisDB/openapi/generated/go` |

## Regenerating the SDKs

SDKs are regenerated automatically by the
[`sdk-generation` CI workflow](../../.github/workflows/sdk-generation.yml)
whenever `openapi/openapi.yaml` changes.

To regenerate locally, run:

```bash
# Requires Docker (recommended) or Java 11+
./scripts/generate-sdks.sh          # all languages
./scripts/generate-sdks.sh --python
./scripts/generate-sdks.sh --javascript
./scripts/generate-sdks.sh --go
```

## Quick Start

### Python

```python
import themisdb_client
from themisdb_client.api import documents_api
from themisdb_client.model.create_document_request import CreateDocumentRequest

configuration = themisdb_client.Configuration(host="http://localhost:8765")
with themisdb_client.ApiClient(configuration) as api_client:
    api = documents_api.DocumentsApi(api_client)
    result = api.get_document("my-collection", "doc-id")
```

### JavaScript / TypeScript

```typescript
import { DocumentsApi, Configuration } from "@themisdb/openapi-client";

const config = new Configuration({ basePath: "http://localhost:8765" });
const api = new DocumentsApi(config);
const doc = await api.getDocument("my-collection", "doc-id");
```

### Go

```go
import (
    themisdb "github.com/makr-code/ThemisDB/openapi/generated/go"
)

cfg := themisdb.NewConfiguration()
cfg.Servers = []themisdb.ServerConfiguration{{URL: "http://localhost:8765"}}
client := themisdb.NewAPIClient(cfg)
doc, _, err := client.DocumentsAPI.GetDocument(ctx, "my-collection", "doc-id").Execute()
```

## Generator Version

Pinned to **openapi-generator-cli v7.10.0** for reproducible builds.
See [`openapitools.json`](../../openapitools.json) for the configuration.

## Source Specification

Generated from [`openapi/openapi.yaml`](../openapi.yaml) — ThemisDB HTTP API v0.1.0.

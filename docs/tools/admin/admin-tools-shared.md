# Themis.AdminTools.Shared - Shared Library for Admin Tools

## Overview

Shared .NET library providing common functionality for all ThemisDB admin tools, including API clients, data models, and utility functions.

## Components

### ThemisApiClient

HTTP client for communicating with themis_server REST API:

```csharp
var client = new ThemisApiClient("http://localhost:8080");
var results = await client.QueryAsync("FOR u IN users RETURN u");
```

### Models

Data Transfer Objects (DTOs):
- `AuditLogEntry` - Audit log records
- `QueryResult` - Query execution results
- `CollectionInfo` - Schema information
- `PIIDetectionResult` - PII scan results

### Utilities

Helper functions:
- JSON serialization/deserialization
- Configuration management
- Logging setup
- Connection pooling

## Installation

```bash
cd tools/Themis.AdminTools.Shared
dotnet restore
dotnet build
```

## Usage in Other Tools

```xml
<ItemGroup>
  <ProjectReference Include="../Themis.AdminTools.Shared/Themis.AdminTools.Shared.csproj" />
</ItemGroup>
```

## See Also

- [All Admin Tools](../../../tools/) - Tools using this library
- [Admin Tools User Guide](../../admin_tools_user_guide.md)

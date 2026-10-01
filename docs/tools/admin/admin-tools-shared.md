# Themis.AdminTools.Shared - Shared Library for Admin Tools

## Overview

The authoritative implementation of the shared admin-tools library lives in the git submodule at `projects/Themis.AdminTools.Shared`. It is backed by the dedicated repository `makr-code/themisdb_admin_tools` and is the single source of truth for the admin tool API client, DTOs, and shared helpers.

This repository acts as the integrator and should not keep a second local copy of the library in the parent tree.

## Source of Truth

```text
projects/Themis.AdminTools.Shared
```

The submodule is declared in `.gitmodules` and points to:

```text
https://github.com/makr-code/themisdb_admin_tools.git
```

## Components

### ThemisApiClient

HTTP client for communicating with the ThemisDB server API:

```csharp
var client = new ThemisApiClient("http://localhost:8080");
var results = await client.QueryAsync("FOR u IN users RETURN u");
```

### Models

Data transfer objects shared by the admin tools, including audit, retention, SAGA, key, and PII models.

### Utilities

Shared helper logic and DTO contracts used across the admin tool suite.

## Usage in Other Tools

When a consumer project references the shared library from the repository root, use the submodule path:

```xml
<ItemGroup>
  <ProjectReference Include="../../projects/Themis.AdminTools.Shared/Themis.AdminTools.Shared.csproj" />
</ItemGroup>
```

## Migration Status

- The authoritative repo is the dedicated `themisdb_admin_tools` submodule.
- Local duplicate admin-tools source trees are intentionally removed from the parent repo.
- Any tool project that needs the shared library should reference the submodule path above.

## See Also

- [All Admin Tools](../../../tools/) - Tools using this library
- [Admin Tools User Guide](../../admin_tools_user_guide.md)

# Themis.AuditLogViewer - Audit Log Browser

## Overview

WPF desktop application for viewing, filtering, and analyzing ThemisDB audit logs. Provides comprehensive audit trail for security, compliance, and troubleshooting purposes.

## Use Cases

- Security auditing and forensics
- Compliance reporting (SOC 2, ISO 27001)
- User activity monitoring
- Troubleshooting user issues
- Generate audit reports for regulators

## Requirements

- .NET 8.0+
- Windows, Linux, or macOS
- Access to ThemisDB server with audit API
- Appropriate audit log read permissions

## Features

- **Time-Range Filtering:** Filter logs by date/time range
- **User Filtering:** View actions by specific users
- **Action Filtering:** Filter by operation type (CREATE, READ, UPDATE, DELETE)
- **Entity Filtering:** Filter by entity type and ID
- **Success/Failure Filter:** View only successful or failed operations
- **Pagination:** Navigate large log sets efficiently
- **CSV Export:** Export filtered logs for external analysis
- **Real-Time Updates:** Auto-refresh for monitoring

## Installation

```bash
cd tools/Themis.AuditLogViewer
dotnet restore
dotnet build
dotnet run
```

## Basic Usage

1. **Connect:** Configure server URL (default: http://localhost:8080)
2. **Set Filters:** 
   - Time range: Last 24 hours, 7 days, 30 days, or custom
   - User: Filter by username
   - Action: CREATE, READ, UPDATE, DELETE, QUERY, etc.
   - Entity: Collection or entity type
3. **View Results:** Browse paginated audit entries
4. **Export:** Export to CSV for analysis
5. **Details:** Click entry for detailed information

## Configuration

Edit `appsettings.json`:

```json
{
  "ThemisServer": {
    "BaseUrl": "http://localhost:8080",
    "ApiKey": "",
    "Timeout": 30
  },
  "AuditViewer": {
    "PageSize": 100,
    "AutoRefreshInterval": 30,
    "EnableRealTimeUpdates": false
  }
}
```

## API Requirements

Server must provide:

### GET /api/audit

Query parameters:
- `start` - Start timestamp (ISO 8601)
- `end` - End timestamp (ISO 8601)
- `user` - Username filter
- `action` - Action type filter
- `entity_type` - Entity type filter
- `entity_id` - Entity ID filter
- `success` - Boolean success filter
- `page` - Page number
- `page_size` - Results per page

Response:
```json
{
  "entries": [
    {
      "timestamp": "2026-01-12T15:30:45Z",
      "user": "alice@example.com",
      "action": "UPDATE",
      "entity_type": "users",
      "entity_id": "user_123",
      "success": true,
      "changes": {"age": {"old": 30, "new": 31}}
    }
  ],
  "totalCount": 1234,
  "page": 1,
  "pageSize": 100,
  "hasMore": true
}
```

### GET /api/audit/export/csv

Same parameters as `/api/audit`, returns CSV file.

## Troubleshooting

### Connection Errors

- Verify server URL is correct
- Check API endpoint is accessible
- Ensure API key (if required) is configured

### No Logs Displayed

- Verify audit logging is enabled on server
- Check time range includes logged events
- Verify user has audit log read permissions

### Slow Performance

- Reduce time range
- Increase pagination page size
- Disable real-time updates
- Check server performance

## See Also

- [Audit and Retention Documentation](../../docs/security/audit_and_retention.md)
- [Admin Tools User Guide](../../docs/admin_tools_user_guide.md)
- [Security Best Practices](../../docs/security/best_practices.md)

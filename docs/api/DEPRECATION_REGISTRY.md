# API Deprecation Registry

This document tracks all deprecated APIs, endpoints, and features in ThemisDB, following our [24-month deprecation policy](./API_VERSIONING.md#24-month-deprecation-window).

**Last Updated**: 2026-04-06

## Active Deprecations

Currently, there are **no active deprecations** in ThemisDB v1.4.x.

All APIs and endpoints are fully supported.

## Planned Deprecations

### Future Deprecation Schedule

The following items are planned for deprecation in future releases:

| Feature | Deprecation Version | Removal Version | Deprecation Date | Removal Date | Alternative |
|---------|---------------------|-----------------|------------------|--------------|-------------|
| (None planned) | - | - | - | - | - |

## Past Deprecations (Completed Removals)

### v1.0.0 → v2.0.0 (If applicable in future)

When ThemisDB reaches v2.0.0, any features deprecated in v1.x will be documented here.

## How to Handle Deprecation Warnings

### 1. Identify Deprecated Usage

Monitor your application logs for deprecation warnings:

```
[WARN] Deprecated endpoint accessed: /api/v1/old-endpoint (version v1.4.1), 
       will be removed in version v2.0.0
```

### 2. Check Response Headers

Deprecated endpoints return special headers:

```http
Deprecation: true; deprecated-version="v1.4.0"; removal-version="v2.0.0"
Sunset: Wed, 24 Jan 2028 06:00:00 GMT
Link: <https://docs.themisdb.com/migration/endpoint-name>; rel="deprecation"
```

### 3. Review Migration Guide

Follow the link in the `Link` header or check the migration guides:
- [Migration Guide Index](../migration/README.md)

### 4. Plan Migration

You have 24 months from deprecation announcement to migrate:
- **Months 0-6**: Plan and design migration
- **Months 6-18**: Implement changes and test
- **Months 18-24**: Final testing and deployment
- **Month 24+**: Feature removed in next major version

## Deprecation Workflow

### For ThemisDB Maintainers

When deprecating an API:

1. **Create Issue**: Open GitHub issue with "API Deprecation" label
2. **Update Documentation**: Add entry to this registry
3. **Add Warning Headers**: Implement deprecation headers in code
4. **Create Migration Guide**: Document alternative approach
5. **Announce**: Post in GitHub Discussions and Release Notes
6. **Wait 24 Months**: Maintain feature during deprecation period
7. **Remove**: Remove in next major version
8. **Document Removal**: Update CHANGELOG with breaking change

## Deprecation Categories

### Critical Deprecations

Features with security implications or major bugs that may be removed earlier than 24 months:
- Security vulnerabilities
- Data corruption risks
- Legal/compliance issues

**Notice**: Critical deprecations will be clearly marked and may have accelerated timelines.

### Standard Deprecations

Normal feature lifecycle deprecations following the 24-month policy:
- Feature improvements
- API redesigns
- Performance optimizations

### Soft Deprecations

Features discouraged but not scheduled for removal:
- Superseded by better alternatives
- Limited use cases
- Maintenance burden

## Monitoring Deprecations

### Automated Alerts

Set up monitoring for deprecation warnings:

```python
# Example: Log monitoring for deprecation warnings
import logging

def monitor_deprecations(log_entry):
    if "Deprecated" in log_entry:
        send_alert("Deprecation warning detected", log_entry)
```

### Dashboard Integration

ThemisDB exposes deprecation metrics:

```http
GET /api/metrics/deprecations

{
  "deprecated_endpoint_calls": {
    "/api/old-endpoint": 150,
    "/api/legacy-feature": 42
  },
  "total_deprecation_warnings": 192
}
```

## Version-Specific Breaking Changes

### v1.4.1 (Current)

**Added**:
- API versioning infrastructure
- Deprecation warning system
- Migration guide framework

**No Breaking Changes**

### v1.4.0

**Added**:
- Extended context window support
- LLM/LoRA framework
- Enhanced pagination

**No Breaking Changes** - All changes are backward compatible

### v1.3.0

**Added**:
- Query optimizer improvements
- Enhanced authentication methods

**No Breaking Changes**

### v1.2.0

**Added**:
- Transaction enhancements
- Sharding features

**No Breaking Changes**

### v1.1.0

**Added**:
- Initial production features

**No Breaking Changes**

### v1.0.0

**Initial stable release**

## Contact

For questions about deprecations:
- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Email**: support@themisdb.com
- **Documentation**: https://docs.themisdb.com

## See Also

- [API Versioning Strategy](./API_VERSIONING.md)
- [Migration Guides](../migration/README.md)
- [CHANGELOG](../../CHANGELOG.md)
- [Breaking Changes Policy](../../CONTRIBUTING.md#breaking-changes)

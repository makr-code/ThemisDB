# Migration Guides

This directory contains version-specific migration guides for upgrading between ThemisDB versions.

## Available Migration Guides

### Current Guides

- [v1.7.x to v1.8.0](./v1.7-to-v1.8.md) - Wire Protocol V2, versioned API routing, ZSTD compression, SAGA engine
- [v1.3.x to v1.4.x](./v1.3-to-v1.4.md) - Extended context, LLM features, pagination
- *(Guides for v1.4→v1.5, v1.5→v1.6, v1.6→v1.7 will be added retroactively)*

## Migration Guide Template

When creating a new migration guide, follow this structure:

```markdown
# Migration Guide: vX.Y to vX.Y+1

## Overview

Brief summary of changes and why upgrade is beneficial.

## Breaking Changes

List of breaking changes (if any).

## New Features

Highlight new features available after upgrade.

## Deprecated Features

List of newly deprecated features.

## Step-by-Step Migration

1. Pre-migration checks
2. Backup procedures
3. Upgrade steps
4. Post-migration validation
5. Rollback procedure (if needed)

## Code Changes Required

Example code changes needed.

## Testing

How to test the migration.

## Support

Where to get help.
```

## General Migration Best Practices

### Before Migration

1. **Review Release Notes**: Read CHANGELOG.md for the target version
2. **Check Compatibility**: Review [API Compatibility Matrix](../api/API_VERSIONING.md#compatibility-matrix)
3. **Backup Data**: Always backup before major upgrades
4. **Test in Staging**: Test migration in non-production environment
5. **Review Deprecations**: Check [Deprecation Registry](../api/DEPRECATION_REGISTRY.md)

### During Migration

1. **Follow Guide**: Use version-specific migration guide
2. **Monitor Logs**: Watch for errors and warnings
3. **Incremental Approach**: Upgrade one minor version at a time
4. **Document Issues**: Note any unexpected behavior
5. **Keep Rollback Ready**: Have rollback plan prepared

### After Migration

1. **Verify Functionality**: Run integration tests
2. **Check Performance**: Compare metrics before/after
3. **Update Configuration**: Apply new recommended settings
4. **Update Dependencies**: Upgrade client libraries
5. **Monitor Production**: Watch metrics for first 48 hours

## Migration Strategies

### Blue-Green Deployment

```
[Production v1.3.x] ←→ Load Balancer ←→ [Staging v1.4.x]
                                ↓
                         Switch traffic gradually
                                ↓
[Staging v1.3.x] ←→ Load Balancer ←→ [Production v1.4.x]
```

### Rolling Update

```
Node 1: v1.3.x → v1.4.x (upgrade first)
Node 2: v1.3.x (still running)
Node 3: v1.3.x (still running)
   ↓
Node 1: v1.4.x (running)
Node 2: v1.3.x → v1.4.x (upgrade second)
Node 3: v1.3.x (still running)
   ↓
Node 1: v1.4.x (running)
Node 2: v1.4.x (running)
Node 3: v1.3.x → v1.4.x (upgrade last)
```

### Canary Deployment

```
95% traffic → v1.3.x (stable)
5% traffic  → v1.4.x (canary)
    ↓
Monitor metrics, error rates
    ↓
80% traffic → v1.3.x
20% traffic → v1.4.x
    ↓
Continue gradually
    ↓
100% traffic → v1.4.x
```

## Version-Specific Migration Paths

### Upgrading Multiple Versions

When upgrading across multiple versions, follow this path:

```
v1.0.x → v1.1.x → v1.2.x → v1.3.x → v1.4.x → v1.5.x → v1.6.x → v1.7.x → v1.8.x
```

**Important**: Do not skip minor versions. Upgrade sequentially to ensure proper migration of data structures and configurations.

### Skip-Level Migration (Not Recommended)

If you must skip versions:
1. **Review all intermediate CHANGELOGs**
2. **Combine all migration steps**
3. **Test extensively in staging**
4. **Have rollback plan ready**
5. **Consider professional support**

## Rollback Procedures

### When to Rollback

Rollback if:
- Critical bugs discovered
- Performance degradation >20%
- Data integrity issues
- Incompatibility with critical systems
- Failed validation tests

### How to Rollback

1. **Stop ThemisDB services**
2. **Restore backup** (data + config)
3. **Reinstall previous version**
4. **Verify data integrity**
5. **Resume services**
6. **Document issues** for future attempts

### Rollback Time Limits

- **<1 hour since upgrade**: Easy rollback
- **1-24 hours**: Moderate complexity
- **>24 hours**: May require data reconciliation
- **>7 days**: Rollback not recommended

## Support Resources

### Documentation

- [API Versioning](../api/API_VERSIONING.md)
- [Deprecation Registry](../api/DEPRECATION_REGISTRY.md)
- [CHANGELOG](../../CHANGELOG.md)

### Community Support

- **GitHub Discussions**: https://github.com/makr-code/ThemisDB/discussions
- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Stack Overflow**: Tag `themisdb`

### Professional Support

For enterprise customers:
- Email: enterprise@themisdb.com
- Priority support with SLA
- Dedicated migration assistance
- Custom migration planning

## Contribution

Found an issue in a migration guide?
1. Open an issue: https://github.com/makr-code/ThemisDB/issues
2. Submit a PR with corrections
3. Share your migration experience

## License

All migration guides are released under the same license as ThemisDB.
See [LICENSE](../../LICENSE) for details.

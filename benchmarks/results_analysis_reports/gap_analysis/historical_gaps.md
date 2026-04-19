> ⚠️ **Historische Lückenanalyse** – Gap-Analyse beschreibt den Stand zum Zeitpunkt der Erstellung.

# Historical Performance Gaps Analysis (v1.0.0)

**Date:** 2025-12-09 20:37:02
**Version Analyzed:** 1.0.0
**Total Gaps:** 36

## Summary Statistics

- **Critical:** 6
- **High:** 23
- **Medium:** 7
- **Low:** 0

## Gaps by Workload

| Workload | Total | Critical | High | Medium | Low |
|----------|-------|----------|------|--------|-----|
| relational | 36 | 6 | 23 | 7 | 0 |

## Performance Gaps by Competitor

| Competitor | Total | Critical | High | Medium | Low |
|------------|-------|----------|------|--------|-----|
| PostgreSQL 16 | 6 | 6 | 0 | 0 | 0 |
| CockroachDB | 6 | 0 | 6 | 0 | 0 |
| SingleStore | 6 | 0 | 5 | 1 | 0 |
| MySQL 8.0 | 6 | 0 | 4 | 2 | 0 |
| MariaDB 11 | 6 | 0 | 4 | 2 | 0 |
| TiDB | 6 | 0 | 4 | 2 | 0 |

## Critical Performance Gaps

### 1. Relational vs PostgreSQL 16 (grpc)

- **ThemisDB Latency:** 0.807ms
- **PostgreSQL 16 Latency:** 1.591ms
- **Performance Gap:** +49.3% slower
- **Latency Delta:** 0.784ms
- **Category:** latency

### 2. Relational vs PostgreSQL 16 (direct)

- **ThemisDB Latency:** 0.565ms
- **PostgreSQL 16 Latency:** 1.106ms
- **Performance Gap:** +48.9% slower
- **Latency Delta:** 0.541ms
- **Category:** latency

### 3. Relational vs PostgreSQL 16 (wire)

- **ThemisDB Latency:** 0.785ms
- **PostgreSQL 16 Latency:** 1.523ms
- **Performance Gap:** +48.5% slower
- **Latency Delta:** 0.739ms
- **Category:** latency

### 4. Relational vs PostgreSQL 16 (https)

- **ThemisDB Latency:** 1.064ms
- **PostgreSQL 16 Latency:** 2.056ms
- **Performance Gap:** +48.3% slower
- **Latency Delta:** 0.992ms
- **Category:** latency

### 5. Relational vs PostgreSQL 16 (tcp)

- **ThemisDB Latency:** 0.798ms
- **PostgreSQL 16 Latency:** 1.453ms
- **Performance Gap:** +45.1% slower
- **Latency Delta:** 0.655ms
- **Category:** latency

### 6. Relational vs PostgreSQL 16 (http)

- **ThemisDB Latency:** 1.010ms
- **PostgreSQL 16 Latency:** 1.806ms
- **Performance Gap:** +44.1% slower
- **Latency Delta:** 0.797ms
- **Category:** latency


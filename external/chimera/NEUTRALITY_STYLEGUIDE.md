# CHIMERA Suite: Neutrality Style Guide

**Version:** 1.0.0  
**Last Updated:** April 2026

---

## Introduction

This style guide ensures consistent vendor-neutral presentation across all CHIMERA materials. Follow these guidelines when creating code, documentation, reports, or visualizations.

---

## 1. System Naming

### ✅ DO

- Use generic identifiers: `system_multimodel_a`, `system_relational_b`
- Use descriptive types: `system_document_c`, `system_graph_d`
- Use alphabetical suffixes: `_a`, `_b`, `_c`

### ❌ DON'T

- Use vendor names: `themisdb`, `mongodb`, `postgresql`
- Use product names: `aurora`, `cosmos`, `dynamo`
- Use marketing terms: `enterprise`, `ultimate`, `pro`

---

## 2. Adapter Naming

### ✅ DO

- Use protocol names: `PostgreSQLAdapter` (PostgreSQL wire protocol)
- Use standard names: `MongoDBAdapter` (MongoDB wire protocol)
- Use generic names: `NativeAdapter`, `HTTPAdapter`, `gRPCAdapter`

### ❌ DON'T

- Use vendor prefixes: `ThemisDBAdapter`, `OracleAdapter`
- Use proprietary names: `ProprietaryProtocolAdapter`

---

## 3. Color Usage

### ✅ DO

- Use color-blind friendly palettes:
  - **Okabe-Ito**: `#0072B2`, `#D55E00`, `#009E73`, `#F0E442`
  - **Paul Tol Muted**: `#332288`, `#88CCEE`, `#44AA99`, `#117733`
- Assign colors systematically (palette order)
- Test with color blindness simulators

### ❌ DON'T

- Use vendor brand colors
- Use red/green for good/bad
- Assign colors by preference

---

## 4. Report Language

### ✅ DO

- Use objective descriptions:
  - "System with lowest latency"
  - "Statistically significant difference detected"
  - "95% confidence interval"
- Include complete statistics
- Show uncertainty

### ❌ DON'T

- Make value judgments:
  - "Best performer"
  - "Winner"
  - "Superior system"
  - "Recommended"

---

## 5. Sorting

### ✅ DO

- Sort alphabetically (default)
- Sort by metric value (when specified)
- Document sorting method

### ❌ DON'T

- Manually order systems
- Put "preferred" systems first
- Hide systems with poor performance

---

## 6. Documentation

### ✅ DO

- Include neutrality guarantees section
- Cite all methodologies
- Provide complete examples
- Use generic system names

### ❌ DON'T

- Use vendor-specific examples
- Promote specific systems
- Hide limitations

---

## 7. Code Style

### ✅ DO

```python
# Good - Generic naming
reporter.add_system_results(
    system_name="System Alpha",
    metric_name="Query Throughput",
    ...
)
```

### ❌ DON'T

```python
# Bad - Vendor-specific
reporter.add_system_results(
    system_name="ThemisDB Production",
    metric_name="ThemisDB Throughput",
    ...
)
```

---

## 8. Configuration

### ✅ DO

```yaml
systems:
  - system_id: "system_multimodel_a"
    adapter:
      type: "NativeAdapter"
```

### ❌ DON'T

```yaml
systems:
  - system_id: "themisdb_v1"
    adapter:
      type: "themisdb_adapter"
```

---

## 9. Copyright & Attribution

### ✅ DO

```
© 2026 CHIMERA Project. Released under MIT License.
Author: CHIMERA Development Team
```

### ❌ DON'T

```
© 2026 ThemisDB Project
Author: ThemisDB Team
```

---

## 10. Visual Design

### ✅ DO

- Clean, professional layout
- High contrast for accessibility
- Clear typography
- Neutral color scheme

### ❌ DON'T

- Vendor logos
- Brand colors
- Marketing imagery
- Promotional language

---

## Quick Reference

### Approved Terms

- System A, System B, System C
- system_multimodel_a, system_relational_b
- NativeAdapter, PostgreSQLAdapter, MongoDBAdapter
- CHIMERA Project, CHIMERA Development Team
- Query throughput, Vector search latency
- Statistical significance, Confidence interval

### Prohibited Terms

- Vendor names (ThemisDB, MongoDB, PostgreSQL, etc.)
- Product names
- Marketing terms (Enterprise, Ultimate, Pro)
- Value judgments (Best, Winner, Superior)
- Branded language

---

**Compliance:** All CHIMERA materials must follow this style guide.  
**Enforcement:** Automated linting + manual review  
**Updates:** Review quarterly

---

*Part of CHIMERA Neutrality Guarantees Framework*

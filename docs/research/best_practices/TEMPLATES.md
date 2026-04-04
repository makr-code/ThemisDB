# Best Practice Documentation Templates

Use the structure below for every best practice entry.  
A ready-to-copy starter file is at [_template_best_practice.md](_template_best_practice.md).

---

## Required Fields

| Field | Description | Example |
|-------|-------------|---------|
| `Source` | Origin of the practice | `RocksDB documentation`, `Apache Kafka design` |
| `URL` | Link to the original source | `https://github.com/facebook/rocksdb/wiki/...` |
| `Tags` | Comma-separated topics | `io, zero-copy, performance` |
| `ThemisDB-Versionen` | First version using this practice | `v1.4.1+` |
| `Status` | Implementation state | `Adopted` |

---

## Full Template Structure

```markdown
# [Best Practice Title]

**Metadaten:**
- Source: 
- URL: 
- Tags: [io, performance, security, ...]
- ThemisDB-Versionen: [v1.4.1+]
- Status: [ ] Identified | [ ] Partially Adopted | [ ] Fully Adopted

## 📋 Summary
(What is this best practice and why is it valuable for ThemisDB?)

## 🎯 Core Principles
- Principle 1
- Principle 2

## 🔗 Adoption in ThemisDB

### Affected Modules
- `src/module1/` — reason

### What Was Adopted?
(Specific patterns, configurations, or code structures)

### Deviations & Rationale
(Where did we deviate from the original and why?)

## ⚠️ Trade-offs & Limitations
- Trade-off 1:

## 🔬 Validation
- [ ] Code reviewed against source
- [ ] Tests written
- [ ] Performance measured
- [ ] Module README linked

## 📚 Related
- [Related Paper](../papers/related.md)
- [Architecture Decision](../architecture_decisions/related.md)

---
**Last Updated:** YYYY-MM-DD
```

---

## Status Values

| Value | Meaning |
|-------|---------|
| `Identified` | Practice identified, not yet applied |
| `Partially Adopted` | Partially applied |
| `Fully Adopted` | Fully applied and validated |

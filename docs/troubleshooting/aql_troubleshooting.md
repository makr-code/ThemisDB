# AQL Troubleshooting Guide

The `aql` module provides developer-facing AQL tools for ThemisDB, including an AQL query builder, autocomplete, confidence scorer, validator, syntax highlighter, LLM-based optimizer advisor, LoRA fine-tuner for AQL generation, migration assistant, and schema provider.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| Autocomplete suggestions missing | Schema not loaded by provider | Call `ANALYZE` on collections |
| `AqlValidator: query invalid` | Validator stricter than parser | Check `aql.validator.strict_mode` |
| LLM optimizer advice is wrong | LLM model not suited for AQL | Use a code-tuned model |
| `AqlLoraTuner: no training data` | Training set empty | Provide AQL query pairs |
| Migration assistant fails | Old AQL syntax not recognised | Update `aql.migration.source_version` |
| Confidence scorer always returns 0 | Scorer model not loaded | Set `aql.confidence_scorer.model_path` |
| Syntax highlighter returns plain text | Highlight output disabled | Set `aql.highlighter.format: ansi` |
| Schema provider returns stale schema | Cache TTL too long | Reduce `aql.schema_provider.cache_ttl_ms` |
| Query builder generates invalid AQL | Template mismatch | Update to current query builder templates |
| Conversation context not remembered | Context window too small | Increase `aql.conversation.max_turns` |

## Common Issues

### Issue 1: AQL Autocomplete Returns No Suggestions

**Description:** The AQL editor integration returns no autocomplete suggestions.

**Symptoms:**
- Empty suggestion list in IDE/editor integration
- Log: `AqlAutocomplete: schema not available for collection=users`

**Cause:** Schema provider has not yet loaded the collection schema.

**Solution:**
```bash
# Force schema refresh
themisdb-admin aql schema refresh --collection users

# Check schema provider status
themisdb-admin aql schema status
```
```yaml
aql:
  schema_provider:
    enabled: true
    cache_ttl_ms: 30000           # reduce from default 300000
    refresh_on_analyze: true
    eager_load_collections: true  # pre-load schemas at startup
```

---

### Issue 2: AQL Validator Rejects Valid Queries

**Description:** Queries that execute correctly are rejected by the validator.

**Symptoms:**
- `aql.validator.strict_mode: true` blocks queries with dynamic bind parameters
- Log: `AqlValidator: bind parameter @@ not allowed in strict mode`

**Cause:** Strict mode disallows dynamic collection names.

**Solution:**
```yaml
aql:
  validator:
    strict_mode: false             # allow dynamic collection names
    allow_dynamic_collections: true
    max_query_length: 65536
    check_collection_existence: true
```

---

### Issue 3: LLM Optimizer Advisor Returns Bad Suggestions

**Description:** The LLM-based optimizer advisor recommends queries that are slower or incorrect.

**Symptoms:**
- Suggested query has worse explain cost than original
- Log: `AqlOptimizerAdvisor: LLM confidence=0.40 (below threshold=0.70)`

**Cause:** General-purpose LLM not fine-tuned for AQL query optimization.

**Solution:**
```yaml
aql:
  optimizer_advisor:
    enabled: true
    model: aql-optimizer-lora      # use AQL-specific fine-tuned model
    min_confidence: 0.75           # only show suggestions above 75% confidence
    max_suggestions: 3
    verify_suggestions: true       # EXPLAIN suggested query before showing
    timeout_ms: 3000
```

---

### Issue 4: Migration Assistant Cannot Parse Old AQL

**Description:** AQL migration assistant fails to convert legacy queries.

**Symptoms:**
- Log: `AqlMigrationAssistant: unsupported syntax for source_version=1.2`
- Migration tool cannot process old query format

**Cause:** Source version not configured; migration rules missing for old syntax.

**Solution:**
```yaml
aql:
  migration_assistant:
    enabled: true
    source_version: "1.2"         # specify the old version
    target_version: "2.0"
    rule_set: comprehensive        # "minimal" | "comprehensive"
    dry_run: true
```
```bash
# Migrate a query file
themisdb-admin aql migrate \
  --input queries_v1.aql \
  --output queries_v2.aql \
  --source-version 1.2
```

---

### Issue 5: Schema Provider Cache Serves Stale Schema

**Description:** After adding a new field to a collection, the schema provider still shows the old schema.

**Symptoms:**
- Autocomplete does not show new field
- `aql.schema_provider` returns old field list

**Cause:** Schema cache TTL too long.

**Solution:**
```yaml
aql:
  schema_provider:
    cache_ttl_ms: 5000             # 5 seconds
    invalidate_on_schema_change: true
```
```bash
# Manually flush schema cache
themisdb-admin aql schema flush
```

---

### Issue 6: AQL Conversation Context Not Maintained

**Description:** Multi-turn AQL conversation assistant forgets previous context.

**Symptoms:**
- Follow-up question "Add a LIMIT to that" returns a new query instead of modifying previous
- Log: `AqlConversationContext: context_id not found – starting new session`

**Cause:** Context window is too small; session expired.

**Solution:**
```yaml
aql:
  conversation:
    enabled: true
    max_turns: 20                  # remember up to 20 turns
    session_ttl_ms: 1800000        # 30 minute session
    persist_sessions: false
```

## Diagnostic Commands

```bash
# Validate an AQL query
themisdb-admin aql validate \
  --query "FOR u IN users FILTER u.active == true RETURN u"

# Get optimizer advice
themisdb-admin aql optimize \
  --query "FOR o IN orders FILTER o.status == 'pending' RETURN o"

# Schema for collection
themisdb-admin aql schema show --collection users

# Confidence score for a generated query
themisdb-admin aql confidence-score \
  --query "FOR u IN users RETURN u.email"

# Tail AQL logs
journalctl -u themisdb -f | grep -E "aql|autocomplete|optimizer.advisor|migration.assist"
```

## Configuration Reference

```yaml
aql:
  validator:
    strict_mode: false
    max_query_length: 65536
  schema_provider:
    cache_ttl_ms: 30000
    eager_load_collections: false
  optimizer_advisor:
    enabled: false
    min_confidence: 0.75
  migration_assistant:
    enabled: false
    source_version: ""
  conversation:
    max_turns: 10
    session_ttl_ms: 900000
```

## Known Limitations

- AQL LoRA fine-tuner requires labelled query-result pairs; unsupervised fine-tuning is not supported.
- Autocomplete only covers the current collection schema; cross-collection joins require manual input.
- Migration assistant handles only well-known syntax changes; custom query patterns may not be migrated correctly.
- LLM optimizer advice may not account for data distribution; always verify with EXPLAIN.

## Related Documentation

- [AQL Module ROADMAP](../../src/aql/ROADMAP.md)
- [AQL Roadmap](../de/roadmap/aql_roadmap.md)
- [AQL Functions Implementation Status](../de/roadmap/aql_functions_implementation_status.md)
- [Query Troubleshooting](./query_troubleshooting.md)
- [Grammar Implementation Summary](../ARCHIVED/implementation-summaries/GRAMMAR_IMPLEMENTATION_SUMMARY.md)

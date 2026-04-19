# Exporters Troubleshooting Guide

The `exporters` module handles data export from ThemisDB to various formats and destinations, including Parquet, JSONL (for LLM training), HuggingFace datasets, and streaming writers with PII detection.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `ParquetExporter: library not found` | PyArrow/libparquet not installed | Install `libarrow-dev` |
| `HuggingfaceExporter: auth failure` | Missing HF token | Set `exporters.huggingface.token` |
| `PiiDetector: model not loaded` | PII detection model missing | Set `exporters.pii.model_path` |
| Export fails midway | Disk full on destination | Free disk space; check output path |
| `StreamWriter: connection refused` | Downstream not accepting data | Check downstream endpoint |
| `JsonlLlmExporter: schema invalid` | JSONL format mismatch | Check `exporters.jsonl.schema` |
| PII not being redacted | PII detection disabled | Enable `exporters.pii.enabled: true` |
| Export is very slow | No parallelism | Increase `exporters.parallel_workers` |
| `ExporterMetrics: counter not found` | Metrics not registered | Restart to re-register metrics |
| Exported Parquet file unreadable | Wrong compression codec | Use `snappy` for broad compatibility |

## Common Issues

### Issue 1: Parquet Exporter Library Not Found

**Description:** Parquet export fails because Apache Arrow library is not installed.

**Symptoms:**
- Log: `ParquetExporter: libarrow.so not found; Parquet export disabled`
- Parquet export API returns 503

**Cause:** Apache Arrow development libraries not installed.

**Solution:**
```bash
# Install Arrow
apt install libarrow-dev libparquet-dev

# Verify
ldconfig -p | grep arrow
```

---

### Issue 2: HuggingFace Export Authentication Failure

**Description:** Exporting to HuggingFace Hub fails with authentication error.

**Symptoms:**
- Log: `HuggingfaceExporter: HTTP 401 Unauthorized – check HF_TOKEN`
- Export to HF Hub fails

**Cause:** Missing or expired HuggingFace token.

**Solution:**
```yaml
exporters:
  huggingface:
    token: ${HF_TOKEN}            # use environment variable
    repo_type: dataset
    private: true
    commit_message: "ThemisDB export"
```
```bash
# Test token
curl -H "Authorization: Bearer $HF_TOKEN" \
  https://huggingface.co/api/whoami
```

---

### Issue 3: PII Not Redacted in Export

**Description:** Exported data contains personally identifiable information that should be redacted.

**Symptoms:**
- Email addresses visible in exported JSONL
- Log: `PiiDetector: disabled; skipping PII scan`

**Cause:** PII detection is disabled or model not loaded.

**Solution:**
```yaml
exporters:
  pii:
    enabled: true
    model_path: /var/lib/themisdb/models/pii-detector.onnx
    action: redact                # "redact" | "mask" | "block"
    entities:
      - EMAIL
      - PHONE
      - SSN
      - CREDIT_CARD
      - NAME
    redaction_char: "*"
```

---

### Issue 4: Slow Export Performance

**Description:** Exporting large collections takes excessively long.

**Symptoms:**
- Export rate < 10k rows/sec
- Log: `ParquetExporter: single-threaded export mode`

**Cause:** Parallel export not enabled.

**Solution:**
```yaml
exporters:
  parallel_workers: 8            # use 8 parallel export workers
  batch_size: 10000
  compression: snappy            # fast compression
  buffer_size_mb: 64
```

---

### Issue 5: JSONL LLM Export Schema Mismatch

**Description:** Exported JSONL does not match the expected format for LLM training.

**Symptoms:**
- Training pipeline rejects exported JSONL
- Log: `JsonlLlmExporter: schema=custom but required fields missing`

**Cause:** Export schema does not match the LLM training framework's required format.

**Solution:**
```yaml
exporters:
  jsonl:
    schema: alpaca                # "alpaca" | "sharegpt" | "openai" | "custom"
    # Alpaca format: {"instruction": ..., "input": ..., "output": ...}
    field_mapping:
      instruction: question
      output: answer
    filter_empty_fields: true
```

---

### Issue 6: Export Fails Midway on Large Collection

**Description:** Export starts but fails after processing part of the collection.

**Symptoms:**
- Partial output file written
- Log: `StreamWriter: write failed: ENOSPC`

**Cause:** Destination disk is full.

**Solution:**
```bash
# Check destination disk space
df -h /var/lib/themisdb/exports/

# Use streaming export to S3 instead of local disk
```
```yaml
exporters:
  output:
    backend: s3                   # "local" | "s3" | "gcs" | "azure"
    s3:
      bucket: themisdb-exports
      prefix: exports/
    resume_on_failure: true       # restart from last checkpoint
    checkpoint_interval: 10000
```

## Diagnostic Commands

```bash
# List export jobs
themisdb-admin exporters jobs list

# Export status
themisdb-admin exporters status

# Test PII detection
themisdb-admin exporters pii-test \
  --text "Contact alice@example.com at +1-555-1234"

# Export metrics
curl -s http://localhost:9100/metrics | grep themisdb_exporter

# Tail exporter logs
journalctl -u themisdb -f | grep -E "exporter|parquet|huggingface|pii|jsonl"
```

## Configuration Reference

```yaml
exporters:
  enabled: true
  parallel_workers: 4
  batch_size: 5000
  pii:
    enabled: true
    action: redact
  huggingface:
    token: ${HF_TOKEN}
  parquet:
    compression: snappy
  jsonl:
    schema: alpaca
```

## Known Limitations

- HuggingFace exporter requires internet access; use in air-gapped environments requires a local HF mirror.
- PII detection accuracy depends on model quality; always validate redacted output before publishing.
- Parquet export does not support nested arrays-of-arrays; only flat arrays are supported.
- Resume-on-failure for S3 exports requires consistent object naming; do not change output prefix mid-export.

## Related Documentation

- [Exporters Module ROADMAP](../../src/exporters/ROADMAP.md)
- [Exporters Roadmap](../de/roadmap/exporters_roadmap.md)
- [LLM Roadmap](../llm_roadmap.md)
- [Ingestion Troubleshooting](./ingestion_troubleshooting.md)

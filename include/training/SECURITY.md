<!-- Status: current | validated: 2026-04-06 -->

# Security — include/training/

Scope: security properties as expressed through the **public API**.
Implementation-level details: `../../src/training/SECURITY.md`.

---

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Training data poisoning | Corrupted model producing biased/malicious outputs | `DataSelector` validates samples against `LabelSpec` schema; malformed samples raise `ValidationError` |
| Checkpoint tampering | Loading a backdoored adapter into production | `LoRACheckpointManager` writes SHA-256 digest per checkpoint; `load()` verifies before deserialisation |
| Provenance forgery | False lineage records enabling audit evasion | `ProvenanceTracker` is append-only; no `update()` or `delete()` in public API |
| Knowledge graph injection | Poisoned graph edges corrupting enriched samples | `KGEnricher` is read-only; `EnrichmentSpec::allowed_relations` is an explicit allowlist (empty by default) |
| Path traversal in checkpoint I/O | Arbitrary file write/read | `LoRACheckpointManager` canonicalises all paths; traversal sequences raise `PathError` |
| Auto-labeler data leakage | Sensitive ThemisDB columns embedded in labels | `LabelSpec` requires explicit column projection; no wildcard projection exists |
| Malicious `ModalityParser` input (e.g. large blobs) | OOM / DoS | `ModalityParser` enforces `max_sample_bytes` (default: 64 MiB); limit exceeded returns `ParseError` |
| Dynamic plugin loading of malicious `ITrainer` | Arbitrary code execution | Plugin loading is not in the public API; all components are caller-constructed |

---

## Security Controls

### Checkpoint Integrity
- SHA-256 digest written in `CheckpointMeta` on every save
- `load()` verifies digest before deserialising tensor data; mismatch raises `CheckpointIntegrityError`
- Atomic write (temp file + `rename(2)`) prevents partial-write corruption

### Provenance Immutability
- `ProvenanceTracker` exposes only `append()` and `query()`
- Records stored in an append-only ThemisDB relation; engine enforces the constraint
- Each record includes a cryptographic hash of the input data snapshot

### Input Validation
- `ModalityParser` bounds every buffer to `max_sample_bytes`
- `AutoLabeler` validates outputs against `LabelSpec::schema` before returning
- `DataSelector` exhaustively checks `SelectionStrategy` enum via `static_assert`

### Path Safety
- `LoRACheckpointManager` calls `std::filesystem::canonical()` on all paths
- Paths escaping the configured `checkpoint_dir` root are rejected
- No `system()`, `popen()`, or shell-expansion paths in any public header

### Data Minimisation
- `LabelSpec` requires explicit column enumeration — no wildcard projection
- `KGEnricher::EnrichmentSpec::allowed_relations` defaults to empty; callers opt-in explicitly

---

## Known Limitations

1. **No gradient privacy in v1.x** — differential-privacy noise injection planned for `federated_trainer.h` (Q4 2026).
2. **Checkpoint not encrypted at rest** — integrity-protected (SHA-256) but not encrypted; filesystem-level encryption is the caller's responsibility. Planned for v2.0.
3. **Auto-labeler query injection** — parameterisation is enforced at the implementation layer. Do not construct `LabelSpec::query` by concatenating untrusted strings; use `LabelSpec::Builder`.
4. **KGEnricher read-only is semantic** — not enforced by storage-layer permissions in v1.x. Hardware-enforced read-only graph mounts are on the roadmap (Q4 2026).
5. **No mutual authentication for distributed training** — mTLS between workers is planned for `distributed_lora_trainer.h` (Q3 2026).

---

> Vulnerability reports: `/SECURITY.md` at repository root.
> Implementation details: `../../src/training/SECURITY.md`

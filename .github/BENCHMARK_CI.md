# Benchmark CI lanes

- `benchmark-gate.yml` is a Wave 4 **smoke**, despite its historical filename.
  It validates Google Benchmark JSON against all 13 registered W4A cases and
  five raw repetitions per case, checks the manifest, and runs variance
  diagnostics. Passing does not establish resilience coverage or performance.
- `benchmark-performance-gate.yml` is an expectations **diagnostic**, not a
  performance gate. Failure or incomplete comparison fails the diagnostic;
  success does not establish a comparable baseline.
- `benchmark-thematic-suites.yml` collects thematic results on hosted runners.
  Suite-map profile IDs are descriptive, not case-level coverage or evidence of
  workload conformance. Its comparisons and optional CTest validation are
  informational; their status is not a release decision. Hardware detection
  runs in a separate hosted job and its coarse CPU/GPU label does not prove
  the identity of the runner executing a suite.
- Benchmark CTest registration coverage and benchmark smokes do not measure
  performance. Skips are not performance evidence.

The expectations comparator's strict mode does not treat every `NO_MAPPING`
or warning verdict as incomplete. A diagnostic success therefore does not
establish that every expectation was mapped and numerically evaluated.

Wave 4 gate evaluation is disabled, including direct reporter invocations.
Legacy `report_variance.py` evaluation functions remain importable; their
results are not validated gate decisions. The existing baseline-guard unit test
imports those functions directly.
Before enabling any performance gate, establish reviewed, provenance-backed
baselines; collect and compare actual hardware, OS, compiler, flags, preset,
dependencies and benchmark configuration for both runs; require the exact
expected cases and complete successful repetitions; and validate the
statistical comparison and thresholds. Missing, failed, skipped or mismatched
evidence must fail closed. No thresholds or runner equivalence are inferred here.

## Optional identity and evidence preflight

`tools/ci/inspect_benchmark_evidence.py` is an offline, opt-in consistency
check; it is not connected to a workflow and does not enable any gate.
It accepts a policy, an existing workload-profile catalog, baseline/current
sidecars, and their raw Google Benchmark JSON files:

```text
python tools/ci/inspect_benchmark_evidence.py --policy POLICY.json --catalog CATALOG.json \
  --baseline-sidecar BASELINE_METADATA.json --baseline-result BASELINE.json \
  --current-sidecar CURRENT_METADATA.json --current-result CURRENT.json
```

The version-1 policy object has exactly `schema_version`, `workload_id`,
`repetitions` and `required_cases`. The last is a nonempty object mapping
**exact Google Benchmark iteration names**, including any `/real_time` suffix,
to nonempty arrays of required numeric metric names. The policy is a
caller-supplied inventory; maintainers must review its case coverage and
metrics. The catalog must resolve the ID uniquely to a `source-described`
profile; simulation-only and unverified profiles are rejected. This status is
source description, not proof of correctness or production readiness.

Each version-1 sidecar has exactly `schema_version`, `baseline_freeze`,
`hardware`, `build`, `workload` and `execution` objects:

| Object | Required fields |
|---|---|
| `baseline_freeze` | Existing scientific framework's `compiler`, `compiler_flags`, `preset`, `hardware_profile`, `os_image` |
| `hardware` | `cpu_model`, `architecture`, `logical_processors`, `memory_bytes`, `storage_identity`, `accelerator_identity` |
| `build` | `build_type`, `configuration_sha256`, `dependencies_sha256` |
| `workload` | `workload_id`, `catalog_sha256`, `dataset_sha256`, `configuration_sha256`, `seed`, `warmup`, `minimum_measurement_time`, `repetitions` |
| `execution` | `status`, `exit_code`, `run_id`, `source_revision`, `worktree_dirty`, `binary_sha256`, `runner_sha256`, `result_sha256`, `policy_sha256` |

SHA-256 values are lowercase hexadecimal. Catalog, policy and result hashes
are calculated over the exact supplied file bytes and must match both
sidecars. Dataset/configuration hashes must identify the actual dataset or
deterministic dataset specification and effective configurations. Each run
must declare a distinct run ID, a clean worktree, successful zero-exit
execution and source/binary/runner identities. Baseline/current freeze,
hardware, build, workload and runner hash must match exactly; source revision
and executable hash may differ. `seed` is a nonnegative integer, and
repetitions are positive integers. Explicit `none` is permitted for absent
storage or accelerators, not unknown identities.

The preflight reuses the smoke artifact checks and requires exactly one valid
iteration for every index `0..repetitions-1` of every policy case in each
result. Missing, duplicate, extra, failed or skipped iteration rows and
non-finite or absent required metrics fail. Aggregate rows cannot substitute
for iterations. It does not impose a repetition minimum or regression budget.

**Limitations:** No existing runner produces this sidecar automatically. Its
execution status, source, binary and hardware/build declarations are not
independently attested; dataset, configuration, dependency and binary hashes
are not recomputed from their inputs. Equality does not prove isolation,
representative hardware, workload correctness, semantic case mapping, sample
independence, baseline approval, or statistical validity. Existing Wave 4
baseline data do not satisfy this contract. A passing preflight is only a
necessary consistency check, never authorization to enable a gate.

### Opt-in local identity capture

`tools/ci/capture_benchmark_identity.py` can create a separate, local
self-description for a completed Google Benchmark result; it is not connected
to CI and does not run a benchmark:

```text
python tools/ci/capture_benchmark_identity.py --executable PATH --runner PATH \
  --result RESULT.json --configuration EFFECTIVE-CONFIG --target bench_mmdb \
  --output IDENTITY.json
```

The tool hashes the exact bytes of the supplied executable, runner, raw result,
and configuration. It derives a workload ID only when every non-aggregate raw
case resolves through the existing exact target/case links to one uniquely
`source-described` catalog profile. Unknown, mixed, malformed or unmapped cases
fail closed. The supplied target must exactly match the executable filename
stem; this is a consistency check, not proof that the executable produced the
result. The output contains those hashes, the profile-catalog hash, a UTC
capture time, and an OS/CPU/architecture/logical-processor/reported-memory
fingerprint. OS collection currently supports Windows and Linux and fails if
required identity fields cannot be observed. It does not record file paths,
configuration contents, environment variables, hostnames, storage identity or
accelerators; those are not inferred. Capture only a curated effective
configuration artifact, since the tool hashes it but does not inspect it for
secrets.

The output uses its own schema and is **not** a version-1 comparison sidecar.
It is editable local evidence, not cryptographic attestation, a dedicated
baseline, or proof that these artifacts produced the result. It does not run
the preflight, compare performance, add a gate, or make existing Wave-4
baselines comparable.

### Optional observation comparison

The offline preflight optionally accepts **both** `--baseline-observation
BASELINE_IDENTITY.json` and `--current-observation CURRENT_IDENTITY.json`;
supplying only one fails. With neither, existing behavior is unchanged. For
each capture-schema version-1 observation it rejects missing, additional,
unknown or incorrectly typed fields, including invalid hashes and host
fingerprints. It checks:

- observed executable and runner hashes against that run's sidecar binary and
  runner hashes;
- observed raw-result hash against both that sidecar and the exact result bytes
  validated by preflight;
- observed workload ID and catalog hash against the sidecar, with the catalog
  hash also checked against the exact supplied catalog bytes;
- CPU model, architecture, logical processor count and reported memory
  against that run's sidecar, and the two structured observed OS fingerprints
  against each other, all by exact equality.

The capture's `configuration_sha256` must be a valid hash but is **not
compared**: its one effective-configuration artifact is not unambiguously
either the sidecar's build configuration or workload configuration. The
structured observed OS fields cannot be unambiguously mapped to the sidecar's
free-form `os_image`; no OS-to-sidecar comparison is claimed. Storage,
accelerator, dependencies, dataset and compiler identity are not observed.
Neither inputs nor binaries are reread by this preflight; only supplied
result, catalog and policy bytes are hashed here.

Capture time, source revision, clean-worktree and execution status are
unattested claims. The timestamp's format is checked, not its truth or its
relationship to execution. Neither matching file hashes nor matching local
host fingerprints establish which executable or runner produced a result,
when or on which host it ran, or that a capture happened on that host. The
workload link is also a local claim, not independently resolved here from the
result's case names. A passing comparison gives no automatic approval and
cannot establish provenance or enable a gate. Complete hardware/build
identity and reviewed baseline provenance remain open.

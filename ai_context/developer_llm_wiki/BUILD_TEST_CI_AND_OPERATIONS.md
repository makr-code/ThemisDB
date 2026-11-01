---
Author: "ThemisDB Team"
Created: "2026-11-01"
Last Updated: "2026-11-01"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

# Developer LLM Wiki — Build/Test/CI/Operations

Datum: 2026-11-01
Status: Active

> Exact model backend not verifiable from final document text alone.

## PR merge-readiness cancellation

The canonical aggregator in
[gate-pr-merge-readiness.yml](../../.github/workflows/gate-pr-merge-readiness.yml)
uses `!cancelled()` so a cancelled workflow does not publish blocked/ready labels
or a merge-readiness comment. Active workflows still aggregate failed
prerequisites; a prerequisite reporting `cancelled` is not accepted as a pass.
Successful and intentionally skipped gates retain their existing behavior.
Focused regression checks: `python tests/test_merge_readiness_workflow.py -v`.

## Curated release delivery contracts

Canonical behavior sources are
[release-mainline.yml](../../.github/workflows/release-mainline.yml),
[release-mainline-approval.yml](../../.github/workflows/release-mainline-approval.yml),
[release-winget.yml](../../.github/workflows/release-winget.yml), and
[release-docker-image.yml](../../.github/workflows/release-docker-image.yml).
The approval policy remains defined by [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md).

- Packaging resolves the requested release tag to a commit and passes that commit
  and its edition through the CMake build matrix. Non-dry-run manual releases
  require an existing tag matching the selected edition.
- Community staging requires nonempty packages, checksums, and a release manifest.
  Upload or post-upload verification failure fails the job. New releases remain
  drafts; an approval issue is created only after all assets are verified.
  Published releases are immutable and are not overwritten by staging runs.
- The GitHub approval gate preserves prerelease suffixes and verifies downloaded
  checksums before making the draft public. Downstream review issues are created
  after publication. Packaging does not automatically trigger Docker publication;
  each registry retains its independent maintainer approval.
- WinGet consumes an already published stable Community release. Release files
  and manifests are transferred as Actions artifacts, not runner-local paths.
  The existing PowerShell generator supplies the WinGet version, installer and
  locale schema fields. Its optional `InstallerArchivePath` validates the ZIP
  checksum and derives the actual nested executable path, including rootless ZIPs.
  Submission requires `winget_pr_token` with access to the fork selected by
  `fork_owner` or `WINGET_FORK_OWNER`, and the `winget-production` environment.
- Docker honors explicit `tag_name` for manual and reusable calls, including
  inherited caller events. Registry publication requires an existing published
  GitHub release and builds the tag commit. The public consumer supports
  COMMUNITY and MINIMAL; private editions fail explicitly and require their
  provisioned private pipeline. MINIMAL aliases are isolated from Community
  aliases, and prerelease tags never update stable `latest`.
- Docker requires both distinct `linux/amd64` and `linux/arm64` digest artifacts
  before manifest creation. Pushing platform images uses `docker-production`;
  no-push validation uses `docker-validation`. Configure required reviewers on
  production environments; a workflow declaration alone does not configure them.

### Mandatory runtime payload in distributable packages

All canonical release-matrix lanes configure `THEMIS_DOCS_DB_MODE=BUILD`,
`THEMIS_MODELS_MODE=PACKAGE`, and TinyLlama inclusion. Each lane downloads the
baseline model before configure, explicitly builds `docs_database`, installs
the documentation and model components, and validates them before CPack.
After CPack, every generated ZIP, TGZ, DEB, RPM, MSI or DMG is inspected
individually by
[validate_runtime_payload.py](../../scripts/release/validate_runtime_payload.py).
A complete archive in another lane cannot compensate for an incomplete package.
Payload validation is blocking even on the experimental macOS lane.
Local development defaults in CMake remain unchanged.

The validator requires `data/docs.db/CURRENT`, its nonempty RocksDB manifest,
nonempty SST or WAL data, and a TinyLlama-named GGUF larger than 100 MB with
the GGUF magic header. It rejects missing, empty and truncated payloads.
Archive extraction uses Python for ZIP/TGZ, `dpkg-deb` for DEB, `bsdtar` for RPM,
`msiexec` on Windows (or `msiextract` on Linux) for MSI, and read-only `hdiutil`
mounts on macOS for DMG. These are structural/integrity guards, not a substitute
for inference or database semantic tests.

The unified Dockerfile builds and installs the compiled database and validates
the same runtime payload. It snapshots downloaded models out of the BuildKit
cache mount into `/opt/bundled-models`; both runtime and debug images receive
`/app/data/docs.db` and `/models/tinyllama.gguf`. `/models/default.gguf` points to
TinyLlama. Alternative model selection does not remove the TinyLlama baseline.
Disabling model inclusion fails the distributable image build.
WinGet validates its selected Windows ZIP against this contract before generating
its manifests; other distribution consumers inherit the validated package assets.

Focused payload verification: `python tests/test_release_runtime_payload.py -v`
(PyYAML; the compiled importer test additionally needs a C++ compiler, RocksDB
headers/libraries and nlohmann-json). Tests cover real ZIP/TGZ/DEB extraction,
missing/truncated payloads, an actual generated RocksDB database, all four
release-matrix lanes, and both Docker image stages. MSI/DMG extraction executes
on the corresponding release runners; no local cross-platform execution claim
is made.

Focused offline checks: `python -m unittest discover -s tests -p
'test_deployment_workflow_regressions.py' -v` (requires PyYAML and PowerShell),
`node --test tests/test_stage_github_release.js`, and `tools/bin/actionlint`.
These checks do not publish releases or contact external package registries.
Actual WinGet schema validation remains in the Windows job via `winget validate`.

## C++ build cache contracts

The reusable CMake workflow stores vcpkg download files and binary package
archives under the runner/architecture/compiler/triplet-scoped cache. The
binary archive cache is the portable dependency cache; do not point cache steps
at a root-level `vcpkg_installed` directory that manifest-mode builds do not
use. Windows release lanes must preserve `SCCACHE_GHA_ENABLED` from the reusable
workflow input instead of disabling it in their configure/build commands.
The standalone MSVC analysis lane initializes its vcpkg submodule before
restoring `vcpkg/installed`; its cache key includes the submodule commit and the
manifest/configuration hash. Cache-hit status, restored archive counts, and
sccache statistics are emitted as run diagnostics.

Dependency caches use explicit restore/save actions with run/attempt snapshot
keys. A relative-filename/size inventory fingerprint suppresses unchanged uploads;
it is not content integrity validation. The reusable checkpoints archive/download
and compiler caches before validation, even after configure/build failure, but
not cancellation. MSVC checkpoints its installed tree only after successful
dependency preparation, before analysis. Reusable restore prefixes stay within
the same OS/architecture/compiler/triplet lane. Snapshot growth and eviction
remain operational risks; online speedups still require measurement.
Dependency keys include overlay ports/triplets; the reusable also records the
checked-out source's vcpkg gitlink revision. MSVC setup precedes cache identity
and uses its toolset version instead of Git Bash GCC. The inventory helper uses
Python filesystem APIs to avoid GNU-only `find -printf` on macOS.
Pinned source builds sparse-checkout the inventory helper from the workflow
revision separately, so older release sources need not contain the new script.

Focused regression checks: `python -m pytest tools/tests/test_check_workflow_boundaries.py -q`.

Wiki health note: the generated Index source count/hash currently differs from
`WIKI_STATUS.json`; use the canonical workflow sources above for this contract.
Generated source inventories below are snapshots, not evidence that changed
workflows have executed successfully.

## .github/workflows/benchmark-gate.yml
- Kategorie: build_test_ci
- Hash: `aca5342a1a4719e8`
- Titel: (binary or unreadable)

## .github/workflows/benchmark-performance-gate.yml
- Kategorie: build_test_ci
- Hash: `0dda1e5dc15d07b0`
- Titel: (binary or unreadable)

## .github/workflows/benchmark-thematic-suites.yml
- Kategorie: build_test_ci
- Hash: `4924c5315f997ad1`
- Titel: (binary or unreadable)

## .github/workflows/build-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `71a3ed0d828b1264`
- Titel: (binary or unreadable)

## .github/workflows/build-clang-fast.yml
- Kategorie: build_test_ci
- Hash: `e3dc5830b7156750`
- Titel: (binary or unreadable)

## .github/workflows/build-content-regression.yml
- Kategorie: build_test_ci
- Hash: `7d5b8aeba93e383d`
- Titel: (binary or unreadable)

## .github/workflows/build-llm-inference.yml
- Kategorie: build_test_ci
- Hash: `cdbebf0d599d8288`
- Titel: (binary or unreadable)

## .github/workflows/build-mainline.yml
- Kategorie: build_test_ci
- Hash: `19f575aa854103d9`
- Titel: (binary or unreadable)

## .github/workflows/build-ollama-router.yml
- Kategorie: build_test_ci
- Hash: `0f6f7251a556c299`
- Titel: (binary or unreadable)

## .github/workflows/build-sanitizer-nightly.yml
- Kategorie: build_test_ci
- Hash: `18496274c11a21a3`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-a-gpu.yml
- Kategorie: build_test_ci
- Hash: `687a7b26386e4b3b`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-llm-benchmarks.yml
- Kategorie: build_test_ci
- Hash: `06d4bf8f34a54be2`
- Titel: (binary or unreadable)

## .github/workflows/build-wave-b-transaction.yml
- Kategorie: build_test_ci
- Hash: `8de8b1d5b9e3ceda`
- Titel: (binary or unreadable)

## .github/workflows/build-widget.yml
- Kategorie: build_test_ci
- Hash: `42880b1ffb7f94f7`
- Titel: (binary or unreadable)

## .github/workflows/codeql.yml
- Kategorie: build_test_ci
- Hash: `e578581a55abf58d`
- Titel: (binary or unreadable)

## .github/workflows/compliance-governance-gates.yml
- Kategorie: build_test_ci
- Hash: `5fbaf5d0c69ad1b8`
- Titel: (binary or unreadable)

## .github/workflows/compliance-supply-chain.yml
- Kategorie: build_test_ci
- Hash: `bc00b2b0ce899fbe`
- Titel: (binary or unreadable)

## .github/workflows/copilot-code-review.yml
- Kategorie: build_test_ci
- Hash: `67df9c187f101b65`
- Titel: (binary or unreadable)

## .github/workflows/coverage-report-nightly.yml
- Kategorie: build_test_ci
- Hash: `7e846289258c5dc7`
- Titel: (binary or unreadable)

## .github/workflows/crda.yml
- Kategorie: build_test_ci
- Hash: `bb919180ff98a17b`
- Titel: (binary or unreadable)

## .github/workflows/edition-hyperscaler-ci.yml
- Kategorie: build_test_ci
- Hash: `b74bf19be67a21e6`
- Titel: (binary or unreadable)

## .github/workflows/ga-promotion-signoff.yml
- Kategorie: build_test_ci
- Hash: `5578bfa9a2422da4`
- Titel: (binary or unreadable)

## .github/workflows/gate-copilot-regression.yml
- Kategorie: build_test_ci
- Hash: `26b5d8224705f9b9`
- Titel: (binary or unreadable)

## .github/workflows/gate-distributed-knowledge.yml
- Kategorie: build_test_ci
- Hash: `8cd46108cf2bbc01`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-benchmark-ctest.yml
- Kategorie: build_test_ci
- Hash: `2bea96d431b64131`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-community-failclosed.yml
- Kategorie: build_test_ci
- Hash: `a2ab1944bb366743`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-core.yml
- Kategorie: build_test_ci
- Hash: `e35592e861eb5198`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-doc-metadata.yml
- Kategorie: build_test_ci
- Hash: `aa6b2ce8ae57fc0f`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-doxygen-governance.yml
- Kategorie: build_test_ci
- Hash: `54b2aa7fb8588485`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-edition-license.yml
- Kategorie: build_test_ci
- Hash: `90502118f5c9afd8`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-final-runtime-api.yml
- Kategorie: build_test_ci
- Hash: `4c068252ac381b33`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-hash-sbom.yml
- Kategorie: build_test_ci
- Hash: `dd91b24239ddcf0e`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-merge-readiness.yml
- Kategorie: build_test_ci
- Hash: `95568a45e75c8174`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-module-doxygen-xml.yml
- Kategorie: build_test_ci
- Hash: `900637cefc00be9b`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-plugin-boundary.yml
- Kategorie: build_test_ci
- Hash: `a6ea80c1053c5585`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-primary-doc-structure.yml
- Kategorie: build_test_ci
- Hash: `fb2c7a7f3d0493f1`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-eval.yml
- Kategorie: build_test_ci
- Hash: `dd96084910101341`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase10.yml
- Kategorie: build_test_ci
- Hash: `1681c0bb656c122b`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase7.yml
- Kategorie: build_test_ci
- Hash: `df421c7580604c25`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase8.yml
- Kategorie: build_test_ci
- Hash: `a4974e1d52615e80`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-phase9.yml
- Kategorie: build_test_ci
- Hash: `6e8421e255ac7397`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-security.yml
- Kategorie: build_test_ci
- Hash: `2c3a03877b8a4233`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-rag-version.yml
- Kategorie: build_test_ci
- Hash: `c6460c02ca88e685`
- Titel: (binary or unreadable)

## .github/workflows/gate-pr-version-targeting.yml
- Kategorie: build_test_ci
- Hash: `fca23062af58da44`
- Titel: (binary or unreadable)

## .github/workflows/gate-release-benchmark-validation.yml
- Kategorie: build_test_ci
- Hash: `20c69f30312f2c38`
- Titel: (binary or unreadable)

## .github/workflows/gate-wave-closure.yml
- Kategorie: build_test_ci
- Hash: `4b71d91f930e92a8`
- Titel: (binary or unreadable)

## .github/workflows/gate-workflow-guidelines.yml
- Kategorie: build_test_ci
- Hash: `5afc2a42edbf47bb`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-ai-working.yml
- Kategorie: build_test_ci
- Hash: `26be4b7773d0202a`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-architecture-ci.yml
- Kategorie: build_test_ci
- Hash: `145c04435220c227`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-build-issues.yml
- Kategorie: build_test_ci
- Hash: `28cc8bae4f130a25`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-ci-health.yml
- Kategorie: build_test_ci
- Hash: `eeaaa0fef5ea8fb6`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-compendium-sync.yml
- Kategorie: build_test_ci
- Hash: `c715e87395b0e850`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-docs-db-build.yml
- Kategorie: build_test_ci
- Hash: `c902cb537996f423`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-docs.yml
- Kategorie: build_test_ci
- Hash: `a54b7b05d42b1d71`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-housekeeping.yml
- Kategorie: build_test_ci
- Hash: `21c263ba1be0aa9a`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-issues.yml
- Kategorie: build_test_ci
- Hash: `3eac9240951febcf`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-pr-failure-diagnosis.yml
- Kategorie: build_test_ci
- Hash: `79e30b70f70ed0ef`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-soll-ist-gap-issues.yml
- Kategorie: build_test_ci
- Hash: `82ad329604e11db4`
- Titel: (binary or unreadable)

## .github/workflows/maintenance-workflow-guardrails-observe.yml
- Kategorie: build_test_ci
- Hash: `b624de438556a4b0`
- Titel: (binary or unreadable)

## .github/workflows/msvc.yml
- Kategorie: build_test_ci
- Hash: `90395a27d5c2fffd`
- Titel: (binary or unreadable)

## .github/workflows/nightly-governance-audit.yml
- Kategorie: build_test_ci
- Hash: `ba578246c0558de1`
- Titel: (binary or unreadable)

## .github/workflows/publish-wiki.yml
- Kategorie: build_test_ci
- Hash: `cfbf27bb80fc0f6d`
- Titel: (binary or unreadable)

## .github/workflows/release-build-matrix.yml
- Kategorie: build_test_ci
- Hash: `14500612c5a032b4`
- Titel: (binary or unreadable)

## .github/workflows/release-changelog.yml
- Kategorie: build_test_ci
- Hash: `d97f477879f14fd3`
- Titel: (binary or unreadable)

## .github/workflows/release-docker-approval.yml
- Kategorie: build_test_ci
- Hash: `2aef3be51ad35c5c`
- Titel: (binary or unreadable)

## .github/workflows/release-docker-image.yml
- Kategorie: build_test_ci
- Hash: `c90c79e3a5308859`
- Titel: (binary or unreadable)

## .github/workflows/release-evidence-chain.yml
- Kategorie: build_test_ci
- Hash: `7f277b8a7015e3df`
- Titel: (binary or unreadable)

## .github/workflows/release-linux-distribution.yml
- Kategorie: build_test_ci
- Hash: `b57790a94b20eeeb`
- Titel: (binary or unreadable)

## .github/workflows/release-linux-distro-approval.yml
- Kategorie: build_test_ci
- Hash: `0d32919331508507`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline-approval.yml
- Kategorie: build_test_ci
- Hash: `1bd45850e1e6a3ba`
- Titel: (binary or unreadable)

## .github/workflows/release-mainline.yml
- Kategorie: build_test_ci
- Hash: `e156f24778d96463`
- Titel: (binary or unreadable)

## .github/workflows/release-nightly.yml
- Kategorie: build_test_ci
- Hash: `d30426adfb7a8ed0`
- Titel: (binary or unreadable)

## .github/workflows/release-promote.yml
- Kategorie: build_test_ci
- Hash: `a92910e7ccc60f25`
- Titel: (binary or unreadable)

## .github/workflows/release-rollback.yml
- Kategorie: build_test_ci
- Hash: `9d4c1cac6420e19c`
- Titel: (binary or unreadable)

## .github/workflows/release-windows-distribution.yml
- Kategorie: build_test_ci
- Hash: `1e353b6b8f278370`
- Titel: (binary or unreadable)

## .github/workflows/release-windows-distro-approval.yml
- Kategorie: build_test_ci
- Hash: `956b7f63c2049670`
- Titel: (binary or unreadable)

## .github/workflows/release-winget-approval.yml
- Kategorie: build_test_ci
- Hash: `bb4f1e6838c842b8`
- Titel: (binary or unreadable)

## .github/workflows/release-winget.yml
- Kategorie: build_test_ci
- Hash: `d97ea10c5b94d9f8`
- Titel: (binary or unreadable)

## .github/workflows/release-wordpress-press.yml
- Kategorie: build_test_ci
- Hash: `641b3d6490580dca`
- Titel: (binary or unreadable)

## .github/workflows/reusable-benchmark-build-run-report.yml
- Kategorie: build_test_ci
- Hash: `fb6c9e997f8c0092`
- Titel: (binary or unreadable)

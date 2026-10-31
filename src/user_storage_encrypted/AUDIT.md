---
Author: ThemisDB Contributors
Created: 2026-08-08
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Source and Documentation Review - user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This is a static source/configuration review for issue #7392, not a security
audit, build result, test pass, or production-readiness approval.

## Reviewed source surfaces

- `src/user_storage_encrypted/{gocryptfs_backend,key_derivation_service,key_rotation_scheduler,multi_level_storage,diagnostic_events}.cpp`
- `include/user_storage_encrypted/*.hpp`
- `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`
- `src/user_storage_encrypted/CMakeLists.txt`
- `tests/user_storage_encrypted/CMakeLists.txt` and scoped test sources
- `benchmarks/user_storage_encrypted/CMakeLists.txt` and benchmark sources

## Findings requiring owner attention

1. **Normative contract versus runtime:** the frozen contract promises atomic
   mount cleanup, unmount alerts/error codes, general key/salt zeroing,
   transactional rotation, per-container serialization, and callback/thread
   constraints. The operational API uses different namespaces/result types.
   `mountContainer()` does not clean up a failed mount; `unmountAll()` discards
   errors; only selected temporary buffers are cleared; and no per-container
   operation mutex is present. See the guarantee-by-guarantee comparison in
   `API.md`.
2. **Rotation API distinction:** `isRotationDue()` and
   `getNextRotationTime()` only inspect schedule timestamps.
   `triggerRotation()` invokes a scheduler callback (caller thread when called
   manually), not storage rotation. The separate `rotateKey()` operation
   mutates provider/container state; it is not wired to parsed
   scheduler configuration. It unmounts both containers before directory
   renames and remounts afterward; remount failure returns after the new
   directory is installed, without code rollback. This is neither a
   transactional nor zero-downtime guarantee.
3. **Scheduler callback behavior:** automatic callbacks run on its polling
   thread; manual `triggerRotation()` callbacks run on the caller's thread.
   Both are invoked while the scheduler mutex is held. Callback exceptions are
   swallowed and the timestamp advances after an attempt; scheduler re-entry
   can deadlock. Contract non-reentry language is a caller restriction, not an
   enforced runtime guard.
4. **Mount cleanup result propagation:** direct `unmountContainer()` returns an
   operational error on command failure, but `unmountAll()` ignores individual
   results. Some rotation unmount/rollback results are also ignored. Verify
   mount state independently.
5. **Key clearing:** `derive()` clears its temporary concatenated password,
   not caller-owned master-key or salt inputs. The byte-vector stdin helper
   clears one hex string buffer, but not every copy; the string overload does
   not clear its caller-owned input. The contract's general zeroing guarantee
   is not established.
6. **Health/metrics scope:** health checks mounted state/backend presence only;
   emitted container-size gauge is not updated.
7. **KDF integration:** multi-level plugin constructs a backend without a KDF
   service. The standalone backend supports one; plugin config does not turn
   it on. Build mode selects Argon2id or PBKDF2 fallback.
8. **Test evidence:** E2E cases unconditionally skip in current source.
   `test_user_storage_encrypted_phase1_hardening.cpp` is not registered in the
   module CMake file. Source presence is not proof of execution.
9. **Benchmark evidence:** lifecycle suite uses sleeps/placeholders and
   dedicated gates use stubs. Candidate labels are not enforced thresholds.
   Mount-latency source is not registered in this directory's CMake file.
10. **Platform claims:** `_WIN32` command/stdin paths return unsupported
    errors; WinFsp/Dokany support is absent. The macOS `pkgutil` probe only
    checks a package identifier and logs a warning on failure; it is not proof
    of a working mount lifecycle. The public-header platform summary was
    corrected to this source evidence.
11. **Rotation source wording:** the step-4 comment in
    `multi_level_storage.cpp` now describes sequential directory renames and
    best-effort rollback rather than an “atomic swap”; runtime behavior is
    unchanged.

## Validation status

- Static API/config/build/test/benchmark source comparison: performed for this
  documentation update.
- Build, CTest, gocryptfs/FUSE lifecycle, Vault integration, and benchmark
  execution: not performed here.
- Previous “production-ready,” “no critical blockers,” and “documentation
  synchronized” conclusions are not validated by this review and should not
  be inferred from this report.

## Generated and historical artifacts

- `DOXYGEN.md` identifies itself as generated from a Windows temporary XML
  directory and is dated 2026-09-20. It was not regenerated; treat its
  declarations and metrics as a snapshot, not current API evidence. The
  repository generator (`scripts/generate_module_doxygen_xml.py
  --write-module-markdown`) was attempted for this module, but cannot run here
  because the `doxygen` executable is absent; package installation is blocked
  by insufficient apt permissions.
- `MODULE_GAPS.md` is an auto-generated scanner snapshot dated 2026-06-04.
  Its finding counts and severities were not re-run or manually revalidated.
- `MODULE_EVIDENCE_SUMMARY.md` is retained as a historical issue #5681 record;
  its current-status claims are explicitly disclaimed at the top of that file.

## Canonical references

- `src/user_storage_encrypted/README.md`
- `src/user_storage_encrypted/API.md`
- `src/user_storage_encrypted/ROADMAP.md`
- `include/user_storage_encrypted/multi_level_storage.hpp`
- `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`
- `tests/user_storage_encrypted/README.md`
- `benchmarks/user_storage_encrypted/README.md`

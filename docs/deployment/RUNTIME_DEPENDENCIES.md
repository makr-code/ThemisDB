# Runtime dependency and startup gates

> Author: GitHub Copilot
> Created: 2026-11-03
> Last Updated: 2026-11-03
> Status: active

Release builds are not evidence that the installed server can load its libraries.
The release pipeline validates **each final package** on a separate runner, and
each Docker platform image before promoting its final tags.

## Deployment preflight

The Ubuntu 24.04 baseline is maintained in
`scripts/release/runtime-dependencies-ubuntu-24.04.txt`. It contains runtime
packages, not compiler or development dependencies, and is shared by the Docker
runtime image and the isolated Linux package gate.

On an Ubuntu 24.04 deployment host, install this baseline before deploying a
Linux archive:

```sh
sudo apt-get update
sudo xargs -r apt-get install -y --no-install-recommends \
  < scripts/release/runtime-dependencies-ubuntu-24.04.txt
python3 scripts/release/runtime_smoke_gate.py --root /opt/themisdb \
  --dependencies-only --output runtime-dependencies.json
```

The dependency-only check reports the actual ELF dependency graph with `ldd`,
rejects unresolved libraries, and executes the installed server's `--version`.
On Windows, execution uses only the Windows system directories on `PATH`, not
Visual Studio or vcpkg directories. Install the VC++ Redistributable required by
the WinGet manifest; other application DLLs must be shipped with the package.
macOS similarly removes build-specific dynamic-loader environment overrides.

For a full startup check:

```sh
python3 scripts/release/runtime_smoke_gate.py --root /opt/themisdb
python3 scripts/release/runtime_smoke_gate.py --packages ./release-packages
python3 scripts/release/runtime_smoke_gate.py --image themisdb:release
```

Python 3.12 or newer is required for safe tar extraction. DEB/RPM inspection
also requires `dpkg-deb`/`bsdtar`; MSI/DMG validation runs on Windows/macOS.
Keep `runtime_smoke_gate.py` and `validate_runtime_payload.py` together when
copying the preflight tools to a deployment host.
Only run these checks on trusted artifacts: they execute the supplied binaries.

## Blocking release checks

- `release-build-matrix.yml`: each Linux AMD64/ARM64 package is extracted and
  started in a clean Ubuntu 24.04 container with only the declared baseline and
  inspection tools. Windows and macOS use fresh jobs without build caches.
  Every generated TGZ, DEB, RPM, ZIP, MSI or DMG is checked independently.
- `reusable-docker-image-platform.yml`: checks the locally loaded or
  digest-pulled image before exporting the digest for final manifest promotion.
- `release-nightly.yml`: builds native AMD64/ARM64 candidates, validates both,
  and only then publishes the final multi-architecture nightly tags.
- `docker/Dockerfile.unified`: checks ELF resolution during the runtime-stage
  build and executes `--version` as the unprivileged `themis` user.

Reports contain version output, Linux library resolution, health responses,
startup logs and explicit errors, and are retained as workflow artifacts even
when validation fails. A missing package/server or an empty selection fails.

Package startup uses an isolated temporary database and a generated JSON config
with a loopback bind address, free port and two workers. It does not enable
degraded builds or bypass license/HSM requirements. Docker startup uses the
image's unmodified entrypoint, command and user, without mounting host libraries.
The unified image defaults to port 8080, matching its exposed port and healthcheck,
and provides a writable log directory for the unprivileged server.
The gate requires HTTP 200 and an exact healthy JSON status, then stops the server.

These checks prove loader resolution and basic startup, not full CRUD,
persistence, GPU inference, or compatibility with every Linux distribution.
DEB/RPM contents are tested on the Ubuntu baseline; package-manager installation
scripts and dependency metadata need separate distribution-specific coverage.
A successful runtime gate does not replace functional integration tests.

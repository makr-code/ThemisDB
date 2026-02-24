# Badge Documentation

This section explains the status badges shown at the top of the [ThemisDB README](../../../README.md).

Only the most important status indicators are displayed in the README header. Each badge links back to the corresponding page in this directory for more detail.

## Badges

| Badge | What it shows | Details |
|-------|---------------|---------|
| CI Status | Latest build result (Themis Core CI) | [ci-status.md](ci-status.md) |
| Version/Release | Current release tag | [version.md](version.md) |
| License | Project license | [license.md](license.md) |
| Docker Pulls | Total Docker Hub pulls for `themisdb/themisdb` | [docker.md](docker.md) |
| Lines of Code | Total source lines counted by Tokei | [loc.md](loc.md) |

## Why these five badges?

Keeping the header compact (5–8 badges) makes the most critical status information immediately visible without visual noise:

- **CI** – shows at a glance whether the core framework builds and tests are healthy.
- **Version** – communicates the current development cycle.
- **License** – instant confirmation of the open-source license for evaluators.
- **Docker Pulls** – reflects real-world adoption of the containerised distribution.
- **Lines of Code** – gives contributors a quick sense of project scale.

Capability information (supported models, technology stack, performance numbers) is documented in the README body and in the module documentation linked from there.

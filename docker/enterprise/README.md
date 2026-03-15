# ThemisDB Enterprise Docker Images

This directory contains Docker images for the **Enterprise Edition** and
**Military Edition** of ThemisDB.

## Status

Enterprise-specific Docker images will be added as the Enterprise Edition
runtime artefacts are finalized. For now, this directory serves as the
reserved path for edition-based Docker organisation.

## Planned images

| File                          | Description                            |
|-------------------------------|----------------------------------------|
| `Dockerfile`                  | Enterprise production image            |
| `Dockerfile.military`         | Military hardened image (FIPS, audit)  |
| `docker-compose.enterprise.yml` | Enterprise multi-node compose setup  |

## Usage (once available)

```bash
# Enterprise image
docker build -f docker/enterprise/Dockerfile \
  -t themisdb-enterprise:latest \
  -DTHEMIS_EDITION=ENTERPRISE \
  .

# Military hardened image
docker build -f docker/enterprise/Dockerfile.military \
  -t themisdb-military:latest \
  .
```

## Branch policy

Enterprise Docker images are **allowed in the `enterprise` release branch**
but are **blocked from `main`** (Community lane) by the PR path gate at
`.github/workflows/pr-path-gate-main.yml`.

They are also **blocked from `hyperscaler`** by the dedicated Enterprise path
gate if pure Hyperscaler content would overwrite them.

In CI, Enterprise Docker images are built and published via
`.github/workflows/publish-enterprise.yml`, which requires approval from the
`enterprise-prod` environment before any publishing step runs.

See [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) for details.

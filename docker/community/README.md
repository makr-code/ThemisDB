# ThemisDB Community Docker Images

Community Docker images are located at the **repository root** for backwards
compatibility and ease of use:

| File                         | Description                                  |
|------------------------------|----------------------------------------------|
| `../../Dockerfile`           | Production multi-stage image (COMMUNITY)     |
| `../../Dockerfile.community-simple` | Simplified image (faster local builds) |
| `../../docker-compose.yml`   | Community compose setup                      |

## Usage

```bash
# Production image
docker build -f Dockerfile -t themisdb:latest .

# Simplified image (faster, no vcpkg)
docker build -f Dockerfile.community-simple -t themisdb:community-simple .
```

## Branch policy

Community Docker images are the **only** Docker images allowed in the `main`
branch (Community release lane). Hyperscaler and Enterprise Docker images live
in `docker/hyperscaler/` and `docker/enterprise/` respectively and are blocked
from `main` by the PR path gate.

See [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) for details.

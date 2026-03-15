# ThemisDB Hyperscaler Docker Images

This directory contains Docker images for the **Hyperscaler Edition** of ThemisDB.

| File               | Description                                         |
|--------------------|-----------------------------------------------------|
| `Dockerfile`       | Full Hyperscaler image (LLM + GPU + distributed sharding) |
| `Dockerfile.lite`  | Lightweight variant for local cluster testing       |

## Usage

```bash
# Full Hyperscaler image
docker build -f docker/hyperscaler/Dockerfile \
  -t themisdb-hyperscaler:latest \
  --build-arg THEMIS_VERSION=local \
  --build-arg ENABLE_LLM=ON \
  .

# Hyperscaler Lite (for quick local testing)
docker build -f docker/hyperscaler/Dockerfile.lite \
  -t themisdb-hyperscaler:latest \
  .

# Start 10-shard cluster
./scripts/start-hyperscaler-docker.sh
```

## Branch policy

Hyperscaler Docker images are **blocked from the `main` branch** (Community
release lane) and **blocked from the `enterprise` branch** (Enterprise lane).
They are only allowed in the `hyperscaler` release lane.

In CI, Hyperscaler Docker images are built and published via
`.github/workflows/publish-hyperscaler.yml`, which requires approval from the
`hyperscaler-prod` environment before any publishing step runs.

See [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) for details.

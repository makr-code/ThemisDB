# Docker Badge

[![Docker Pulls](https://img.shields.io/docker/pulls/themisdb/themisdb)](https://hub.docker.com/r/themisdb/themisdb)

## What it shows

The total number of pulls of the official **ThemisDB** image from Docker Hub. A high pull count indicates active community adoption of the containerised distribution.

## What it does NOT guarantee

- The pull count includes all tags (including older versions) and automated pulls by CI pipelines.
- The badge reflects Docker Hub's reported count, which may lag by a short period.

## Source of truth

| Source | URL |
|--------|-----|
| Docker Hub repository | <https://hub.docker.com/r/themisdb/themisdb> |
| Local Dockerfile | [`Dockerfile`](../../../Dockerfile) in the repository root |
| Docker Compose setup | [`docker-compose.yml`](../../../docker-compose.yml) |

## How contributors can verify

```bash
docker pull themisdb/themisdb
docker images themisdb/themisdb
```

See the Docker Hub page for all available tags, image sizes, and the full pull history.

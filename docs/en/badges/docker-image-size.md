# Docker Image Size Badge

[![Docker Image Size](https://img.shields.io/docker/image-size/themisdb/themisdb/latest)](https://hub.docker.com/r/themisdb/themisdb/tags)

## What it shows

The compressed size of the `latest` tag of the official `themisdb/themisdb` Docker image on Docker Hub. This is the download size users incur when pulling the image for the first time (layers not already cached locally).

## What it does NOT guarantee

- The compressed image size does not equal the uncompressed on-disk size after `docker pull`.
- The badge reflects the `latest` tag; other tags (e.g., version-specific or `community`) may have different sizes.

## Source of truth

| Source | URL |
|--------|-----|
| Docker Hub tags | <https://hub.docker.com/r/themisdb/themisdb/tags> |
| Local Dockerfile | [`Dockerfile`](../../../Dockerfile) in the repository root |

## How contributors can verify

```bash
docker pull themisdb/themisdb:latest
docker inspect themisdb/themisdb:latest --format='{{.Size}}' | numfmt --to=iec
```

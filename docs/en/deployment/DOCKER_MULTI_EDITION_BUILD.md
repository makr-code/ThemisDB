# ThemisDB Multi-Edition Docker Builds

## Übersicht

ThemisDB kann in 3 Editionen gebaut werden:
- **COMMUNITY** (Standard) - Open Source Edition
- **ENTERPRISE** - Enterprise Edition mit erweiterten Features
- **HYPERSCALER** - Cloud-optimierte Edition für große Deployments

Jede Edition kann für beide Architekturen gebaut werden:
- **linux/amd64** (x86_64)
- **linux/arm64** (ARM64/aarch64)

## Quick Build - Community Edition (Standard)

```bash
# Single Architecture (aktuelle Plattform)
docker build -t themisdb:1.3.0-community .

# Multi-Architecture (amd64 + arm64)
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themisdb:1.3.0-community \
  --build-arg THEMIS_EDITION=COMMUNITY \
  .
```

## Alle Editionen bauen

### 1. Community Edition

```bash
# AMD64
docker buildx build \
  --platform linux/amd64 \
  -t themisdb:1.3.0-community-amd64 \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --load \
  .

# ARM64
docker buildx build \
  --platform linux/arm64 \
  -t themisdb:1.3.0-community-arm64 \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --load \
  .

# Multi-Arch (beide Architekturen)
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themisdb:1.3.0-community \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --push \
  .
```

### 2. Enterprise Edition

```bash
# AMD64
docker buildx build \
  --platform linux/amd64 \
  -t themisdb:1.3.0-enterprise-amd64 \
  --build-arg THEMIS_EDITION=ENTERPRISE \
  --load \
  .

# ARM64
docker buildx build \
  --platform linux/arm64 \
  -t themisdb:1.3.0-enterprise-arm64 \
  --build-arg THEMIS_EDITION=ENTERPRISE \
  --load \
  .

# Multi-Arch (beide Architekturen)
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themisdb:1.3.0-enterprise \
  --build-arg THEMIS_EDITION=ENTERPRISE \
  --push \
  .
```

### 3. Hyperscaler Edition

```bash
# AMD64
docker buildx build \
  --platform linux/amd64 \
  -t themisdb:1.3.0-hyperscaler-amd64 \
  --build-arg THEMIS_EDITION=HYPERSCALER \
  --load \
  .

# ARM64
docker buildx build \
  --platform linux/arm64 \
  -t themisdb:1.3.0-hyperscaler-arm64 \
  --build-arg THEMIS_EDITION=HYPERSCALER \
  --load \
  .

# Multi-Arch (beide Architekturen)
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themisdb:1.3.0-hyperscaler \
  --build-arg THEMIS_EDITION=HYPERSCALER \
  --push \
  .
```

## Mit LLM Support (llama.cpp)

Alle Editionen können mit optionalem LLM Support gebaut werden:

```bash
# Community + LLM
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themisdb:1.3.0-community-llm \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=ON \
  --push \
  .

# Enterprise + LLM
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themisdb:1.3.0-enterprise-llm \
  --build-arg THEMIS_EDITION=ENTERPRISE \
  --build-arg ENABLE_LLM=ON \
  --push \
  .

# Hyperscaler + LLM
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themisdb:1.3.0-hyperscaler-llm \
  --build-arg THEMIS_EDITION=HYPERSCALER \
  --build-arg ENABLE_LLM=ON \
  --push \
  .
```

## Batch Build - Alle Editionen und Architekturen

```bash
#!/bin/bash
# build-all-editions.sh

EDITIONS=("COMMUNITY" "ENTERPRISE" "HYPERSCALER")
VERSION="1.3.0"
PLATFORMS="linux/amd64,linux/arm64"

for EDITION in "${EDITIONS[@]}"; do
  EDITION_LOWER=$(echo "$EDITION" | tr '[:upper:]' '[:lower:]')
  
  echo "Building ThemisDB ${EDITION} Edition..."
  
  # Standard (ohne LLM)
  docker buildx build \
    --platform ${PLATFORMS} \
    -t themisdb:${VERSION}-${EDITION_LOWER} \
    --build-arg THEMIS_EDITION=${EDITION} \
    --push \
    .
  
  # Mit LLM Support
  docker buildx build \
    --platform ${PLATFORMS} \
    -t themisdb:${VERSION}-${EDITION_LOWER}-llm \
    --build-arg THEMIS_EDITION=${EDITION} \
    --build-arg ENABLE_LLM=ON \
    --push \
    .
  
  echo "✓ ${EDITION} Edition fertig"
done

echo "Alle Editionen erfolgreich gebaut!"
```

## Image Überprüfung

```bash
# Edition anzeigen
docker inspect themisdb:1.3.0-community | jq '.[0].Config.Labels'

# Edition zur Laufzeit prüfen
docker run --rm themisdb:1.3.0-community printenv THEMIS_EDITION
```

## Docker Compose - Edition auswählen

```yaml
version: "3.8"
services:
  # Community Edition
  themisdb-community:
    image: themisdb:1.3.0-community
    ports:
      - "8080:8080"
      - "18765:18765"
    volumes:
      - themis_community:/var/lib/themisdb
    environment:
      THEMIS_EDITION: COMMUNITY

  # Enterprise Edition
  themisdb-enterprise:
    image: themisdb:1.3.0-enterprise
    ports:
      - "8081:8080"
      - "18766:18765"
    volumes:
      - themis_enterprise:/var/lib/themisdb
    environment:
      THEMIS_EDITION: ENTERPRISE

  # Hyperscaler Edition
  themisdb-hyperscaler:
    image: themisdb:1.3.0-hyperscaler
    ports:
      - "8082:8080"
      - "18767:18765"
    volumes:
      - themis_hyperscaler:/var/lib/themisdb
    environment:
      THEMIS_EDITION: HYPERSCALER

volumes:
  themis_community:
  themis_enterprise:
  themis_hyperscaler:
```

## Editions-Features

### Community Edition (Standard)
- Alle Core-Features
- Open Source
- Keine Beschränkungen
- Community Support

### Enterprise Edition
- Alle Community Features
- Enterprise Sharding
- GPU-Beschleunigung
- Advanced Analytics
- Enterprise Support

### Hyperscaler Edition
- Alle Enterprise Features
- Cloud-optimierte Performance
- Extreme Skalierbarkeit
- Multi-Region Support
- Premium Support

## Buildx Setup (einmalig)

```bash
# Buildx builder erstellen (wenn noch nicht vorhanden)
docker buildx create --name multiarch --use
docker buildx inspect --bootstrap

# Verify
docker buildx ls
```

## Troubleshooting

### QEMU für ARM auf AMD64 (falls benötigt)
```bash
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
```

### Build Cache löschen
```bash
docker builder prune -af
```

### Spezifische Edition testen
```bash
# Lokal testen (eine Architektur)
docker buildx build \
  --platform linux/amd64 \
  -t themisdb:test-enterprise \
  --build-arg THEMIS_EDITION=ENTERPRISE \
  --load \
  .

docker run --rm themisdb:test-enterprise printenv THEMIS_EDITION
```

## CI/CD Integration

### GitHub Actions Beispiel

```yaml
name: Build Multi-Edition Docker Images

on:
  push:
    tags:
      - 'v*'

jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        edition: [COMMUNITY, ENTERPRISE, HYPERSCALER]
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3
      
      - name: Login to Docker Hub
        uses: docker/login-action@v3
        with:
          username: ${{ secrets.DOCKER_USERNAME }}
          password: ${{ secrets.DOCKER_PASSWORD }}
      
      - name: Build and push
        uses: docker/build-push-action@v5
        with:
          context: .
          platforms: linux/amd64,linux/arm64
          push: true
          tags: |
            themisdb:${{ github.ref_name }}-${{ matrix.edition }}
          build-args: |
            THEMIS_EDITION=${{ matrix.edition }}
            THEMIS_VERSION=${{ github.ref_name }}
```

## Best Practices

1. **Edition-Naming**: Nutze konsistente Tag-Struktur: `version-edition-arch`
2. **Multi-Arch Default**: Baue immer beide Architekturen für maximale Kompatibilität
3. **LLM Optional**: Trenne Standard und LLM Builds für kleinere Images
4. **Cache**: Nutze Docker BuildKit Cache für schnellere Builds
5. **Testing**: Teste jede Edition auf beiden Architekturen vor Release

## Performance

| Edition | Build Zeit (amd64) | Build Zeit (arm64) | Image Größe |
|---------|-------------------|-------------------|-------------|
| Community | ~15 min | ~20 min | ~500 MB |
| Enterprise | ~18 min | ~25 min | ~600 MB |
| Hyperscaler | ~20 min | ~30 min | ~650 MB |
| +LLM Support | +5 min | +8 min | +200 MB |

*Zeiten sind Richtwerte und hängen von Hardware und Cache ab*

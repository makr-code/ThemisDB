# ThemisDB v1.3.0 - Linux x64 Docker Build Instructions

## Quick Build with Docker

### Option 1: Use Official Docker Image
```bash
docker pull ghcr.io/makr-code/themisdb:v1.3.0
docker run -d -p 8765:8765 -v themis_data:/data ghcr.io/makr-code/themisdb:v1.3.0
```

### Option 2: Build from Source (WSL2/Ubuntu)
```bash
# 1. Clone and setup
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
git clone https://github.com/ggerganov/llama.cpp.git llama.cpp

# 2. Configure
cmake -B build-linux \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_CORE_SHARED=OFF

# 3. Build
cmake --build build-linux -j$(nproc)

# 4. Run
./build-linux/themis_server --config config/config.json
```

### Option 3: Multi-Arch Docker Build (amd64 + arm64)
```bash
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themisdb:v1.3.0 \
  --build-arg ENABLE_LLM=ON \
  .
```

## Run Container with LLM Support

```bash
docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -e THEMIS_LLM_ENABLE=true \
  -v themis_data:/var/lib/themis \
  -v ~/.models:/models \
  ghcr.io/makr-code/themisdb:v1.3.0
```

### Download Model
```bash
mkdir -p ~/.models
wget https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF/resolve/main/mistral-7b-instruct-v0.2.Q4_K_M.gguf \
  -O ~/.models/mistral-7b.gguf
```

## Docker Compose
```yaml
version: '3.8'
services:
  themisdb:
    image: ghcr.io/makr-code/themisdb:v1.3.0
    ports:
      - "8765:8765"
    volumes:
      - themis_data:/var/lib/themis
      - ~/.models:/models
    environment:
      - THEMIS_LLM_ENABLE=true
    restart: unless-stopped

volumes:
  themis_data:
```

## Notes
- Default port: **8765**
- Data directory: **/var/lib/themis**
- Model directory: **/models**

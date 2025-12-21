# Linux x64 Build Guide

## Build Options

### WSL2 (Windows)
```bash
cd /mnt/c/VCC/themis
rm -rf build-linux
cmake -B build-linux \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DTHEMIS_ENABLE_LLM=ON

cmake --build build-linux -j8
./build-linux/themis_server --help
```

### Native Linux (Ubuntu 22.04+)
```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
git clone https://github.com/ggerganov/llama.cpp.git llama.cpp

cmake -B build-linux -DCMAKE_BUILD_TYPE=Release -DTHEMIS_ENABLE_LLM=ON
cmake --build build-linux -j$(nproc)

./build-linux/themis_server --help
```

### Docker (Recommended)
See themisdb-v1.3.0-linux-x64-docker-build.md

## Package Installation

After building, package with:
```bash
tar -czf themisdb-v1.3.0-linux-x64.tar.gz \
  build-linux/themis_server \
  config/ \
  docs/ \
  LICENSE \
  README.md

# Install
sudo mkdir -p /opt/themisdb
sudo tar -xzf themisdb-v1.3.0-linux-x64.tar.gz -C /opt/themisdb

# Run
/opt/themisdb/build-linux/themis_server --config /opt/themisdb/config/config.json
```

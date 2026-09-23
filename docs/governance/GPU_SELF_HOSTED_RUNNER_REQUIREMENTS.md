# GPU Self-Hosted Runner Infrastructure Requirements

**Document Type:** Infrastructure Configuration — Wave A GPU Baseline Capture  
**Status:** 🔴 NOT DEPLOYED — Requirements specification ready for implementation  
**Target Completion:** Q4 2026

---

## Overview

To execute the Wave A GPU baseline capture (`bench_gpu_a8_baselines.cpp`, CUDA reduction quantification, and latency/throughput measurements), at least one self-hosted GitHub Actions runner with NVIDIA CUDA 12.x capability must be deployed and registered.

This document specifies the exact configuration requirements.

---

## Runner Specification

### Runner Identity

```yaml
Name: gpu-cuda
Type: self-hosted
OS: Linux
Architecture: x86_64
Labels:
  - self-hosted
  - gpu-cuda
  - linux
```

### Hardware Requirements

| Component | Minimum Spec | Recommended | Notes |
|-----------|--------------|-------------|-------|
| GPU | NVIDIA Compute Capability 8.0+ (A100) | NVIDIA A100 or H100 | For representative A8-class baseline |
| VRAM | 32 GB | 40+ GB | Support multi-GPU and benchmark scaling |
| System RAM | 64 GB | 128 GB | Compilation + execution headroom |
| Storage | 500 GB SSD | 1 TB NVMe | Fast CMake cache, benchmark runs |
| CPU | 32+ cores | 64+ cores | Parallel compilation optimization |
| Network | 1 Gbps | 10 Gbps+ | Artifact upload performance |

### Software Requirements

| Component | Version | Purpose | Validation |
|-----------|---------|---------|-----------|
| Ubuntu | 20.04 LTS or later | Base OS | `lsb_release -a` |
| CUDA Toolkit | 12.x (≥12.0) | GPU compute | `nvcc --version` |
| cuDNN | 8.x (recommended) | ML acceleration | `nvidia-smi` with CUDA 12.x |
| NCCL | 2.20+ | Multi-GPU collective ops | `libnccl-dev` package |
| GCC | 11+ or Clang 14+ | C++ compilation | `gcc --version` |
| CMake | 3.22+ | Build system | `cmake --version` |
| Ninja | 1.11+ | Build acceleration | `ninja --version` |
| Python | 3.9+ | Benchmark scripting | `python3 --version` |

---

## Installation Steps

### 1. Hardware Setup

```bash
# Verify GPU hardware
nvidia-smi
# Expected output: NVIDIA driver version, GPU model (A100/H100), CUDA Capability

# Check available VRAM
nvidia-smi --query-gpu=memory.total --format=csv
# Expected: ≥32 GB per GPU
```

### 2. CUDA Toolkit 12.x Installation

```bash
# Ubuntu 20.04 / 22.04
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt-get update
sudo apt-get install -y cuda-12-2 cuda-runtime-12-2

# Verify installation
nvcc --version
# Expected: nvcc: NVIDIA (R) Cuda compiler driver, Version 12.x
```

### 3. NCCL and cuDNN Installation

```bash
# NCCL (for collective operations)
sudo apt-get install -y libnccl2 libnccl-dev

# cuDNN (if needed for full feature validation)
# Download from NVIDIA Developer Program and install per instructions
# Required for training/inference GPU paths

# Verify
ldconfig -p | grep nccl
# Expected: libnccl.so.2 in output
```

### 4. Build Tools Installation

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  git \
  sccache \
  libfmt-dev \
  libspdlog-dev \
  nlohmann-json3-dev \
  librocksdb-dev \
  libssl-dev \
  zlib1g-dev \
  libbz2-dev \
  liblz4-dev \
  libzstd-dev \
  libboost-all-dev \
  libtbb-dev \
  libcpp-httplib-dev \
  libpugixml-dev \
  libyaml-cpp-dev \
  libmimalloc-dev

# Verify key tools
cmake --version
ninja --version
gcc --version
```

### 5. GitHub Actions Runner Installation

```bash
# Create runner directory
mkdir -p /opt/github-runners/gpu-cuda
cd /opt/github-runners/gpu-cuda

# Download runner (replace VERSION with latest)
curl -o actions-runner-linux-x64-VERSION.tar.gz \
  https://github.com/actions/runner/releases/download/vVERSION/actions-runner-linux-x64-VERSION.tar.gz

tar xzf actions-runner-linux-x64-VERSION.tar.gz

# Configure runner
./config.sh \
  --url https://github.com/makr-code/ThemisDB \
  --token <PAT_TOKEN> \
  --name gpu-cuda \
  --labels self-hosted,gpu-cuda,linux \
  --runnergroup Default \
  --unattended

# Verify configuration
./run.sh
# Runner should start and connect to GitHub
```

### 6. Validation Script

Create `/home/gpu-runner/validate_gpu_runner.sh`:

```bash
#!/bin/bash
set -e

echo "=== GPU Runner Validation ==="
echo ""

# Check GPU hardware
echo "1. GPU Hardware:"
nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv,noheader
echo "   ✓ GPU detected"

# Check CUDA version
echo ""
echo "2. CUDA Toolkit:"
nvcc --version | grep release
echo "   ✓ CUDA 12.x installed"

# Check NCCL
echo ""
echo "3. NCCL:"
ldconfig -p | grep nccl.so.2
echo "   ✓ NCCL installed"

# Check build tools
echo ""
echo "4. Build Tools:"
echo "   CMake: $(cmake --version | head -1)"
echo "   Ninja: $(ninja --version)"
echo "   GCC: $(gcc --version | head -1)"
echo "   ✓ Build tools ready"

# Check compilation capability
echo ""
echo "5. CUDA Compilation:"
cat > /tmp/test_cuda.cu <<'EOF'
#include <iostream>
int main() { std::cout << "CUDA OK\n"; return 0; }
EOF
nvcc /tmp/test_cuda.cu -o /tmp/test_cuda 2>&1 && /tmp/test_cuda
echo "   ✓ CUDA compilation works"

echo ""
echo "=== Validation Complete ==="
echo "Runner is ready for GPU benchmark execution"
```

Run validation:
```bash
chmod +x /home/gpu-runner/validate_gpu_runner.sh
/home/gpu-runner/validate_gpu_runner.sh
```

---

## GitHub Actions Integration

### Runner Labels

The runner must be registered with these labels:
- `self-hosted` (automatic)
- `gpu-cuda` (manual, for GPU-specific jobs)
- `linux` (manual, for OS)

### Workflow Configuration

Jobs requiring the GPU runner should use:

```yaml
jobs:
  gpu-baseline-capture:
    name: "GPU A8 Baseline Capture"
    runs-on: [self-hosted, gpu-cuda, linux]
    if: |
      github.event_name == 'workflow_dispatch' &&
      github.event.inputs.run_representative_hardware == 'true'
    steps:
      - uses: actions/checkout@v5
      
      - name: "Build GPU benchmarks"
        run: |
          cmake --preset community-release \
            -DTHEMIS_BUILD_BENCHMARKS=ON \
            -DTHEMIS_ENABLE_CUDA=ON
          cmake --build build-release --target bench_gpu_a8_baselines
      
      - name: "Execute baseline capture"
        run: |
          ./build-release/bin/bench_gpu_a8_baselines \
            --benchmark_out=gpu_baselines_a8.json \
            --benchmark_out_format=json
      
      - name: "Upload baseline evidence"
        uses: actions/upload-artifact@v5
        with:
          name: gpu-baselines-a8
          path: gpu_baselines_a8.json
```

### Safety Configuration

**Important:** Self-hosted runners should be isolated on internal network if possible.

```bash
# Restrict network access
sudo ufw allow from 140.82.112.0/20 to any port 22,443  # GitHub IP ranges
sudo ufw enable

# Use secrets for any credentials
# GitHub Actions Secrets interface for access tokens
```

---

## Monitoring and Health Checks

### CI/CD Health Check

Add to `.github/workflows/gpu-runner-health.yml`:

```yaml
name: "GPU Runner Health Check"
on:
  schedule:
    - cron: "0 */6 * * *"  # Every 6 hours
  workflow_dispatch:

jobs:
  health-check:
    runs-on: [self-hosted, gpu-cuda, linux]
    steps:
      - name: "Verify GPU availability"
        run: nvidia-smi
      
      - name: "Check NCCL"
        run: ldconfig -p | grep nccl
      
      - name: "Quick CUDA compile test"
        run: |
          echo "int main() { return 0; }" > test.cu
          nvcc test.cu -o test_binary
```

### Manual Verification

```bash
# SSH to runner host
# Verify GPU utilization
watch -n 1 nvidia-smi

# Check runner process
ps aux | grep runner

# Review recent logs
tail -f /opt/github-runners/gpu-cuda/_diag/Runner_*.log
```

---

## Maintenance

### Regular Updates

```bash
# CUDA driver updates (monthly)
sudo apt-get update
sudo apt-get upgrade cuda-runtime-12-2

# Runner updates (automatic via GitHub)
# Manual: cd /opt/github-runners/gpu-cuda && ./config.sh

# CMake/Ninja (quarterly)
sudo apt-get upgrade cmake ninja-build
```

### Capacity Planning

| Metric | Threshold | Action |
|--------|-----------|--------|
| GPU Memory Used | >80% | Increase VRAM or parallelize jobs |
| Disk Free | <100 GB | Clear old CMake cache |
| CPU Load | >90% | Reduce parallel jobs |
| Temp > 80°C | Alert | Check cooling, throttle benchmarks |

---

## Rollback Plan

If runner becomes unavailable:

1. **Immediate:** Disable gpu-cuda label in workflows
2. **Fallback:** CPU-only baseline runs will execute (slower, but validates correctness)
3. **Recovery:** Fix infrastructure issue and re-enable
4. **Documentation:** Update GA_PROMOTION_SIGN_OFF.md with timeline

---

## Related Documentation

- GPU Module Roadmap: `src/gpu/ROADMAP.md`
- CUDA Reduction Tracking: `src/gpu/GPU_CUDA_REDUCTION_TRACKING.md`
- GA Sign-Off Process: `docs/governance/GA_PROMOTION_SIGN_OFF.md`
- CI Policy: `.github/WORKFLOW_GUIDELINES.md`

---

*Document Status: Template — Ready for deployment  
*Owner: Platform Infrastructure Team*  
*Review Date: 2026-10-01*

# Production Deployment Guide

**Version:** 1.8.0-rc1  
**Last Updated:** April 2026  
**Target Audience:** DevOps Engineers, System Administrators

## Table of Contents

1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Prerequisites](#prerequisites)
4. [Installation](#installation)
5. [GPU Configuration](#gpu-configuration)
6. [Network Setup](#network-setup)
7. [Storage Configuration](#storage-configuration)
8. [Security Hardening](#security-hardening)
9. [Validation](#validation)
10. [Troubleshooting](#troubleshooting)

---

## Overview

This guide covers production deployment of ThemisDB with GPU acceleration for LLM training and inference workloads. It includes hardware compatibility, driver installation, multi-GPU setup, and production-ready configurations.

### Deployment Scenarios

- **Single GPU**: Development and small-scale inference
- **Multi-GPU (Single Node)**: Training workloads, high-throughput inference
- **Multi-Node**: Distributed training, large-scale deployments
- **RAID Configurations**: High-availability storage with data redundancy

---

## Hardware Requirements

### GPU Compatibility Matrix

| GPU Model | VRAM | CUDA Compute | Recommended Use Case |
|-----------|------|--------------|----------------------|
| RTX 3090 | 24GB | 8.6 | Development, small models |
| RTX 4090 | 24GB | 8.9 | Single-GPU training, inference |
| A100 40GB | 40GB | 8.0 | Production training |
| A100 80GB | 80GB | 8.0 | Large model training |
| H100 SXM5 | 80GB | 9.0 | High-performance training |
| H100 NVL | 94GB | 9.0 | Extreme-scale workloads |
| L40S | 48GB | 8.9 | Inference-optimized |
| A6000 | 48GB | 8.6 | Professional workstations |

### Minimum System Requirements

**Single GPU Setup:**
- CPU: 8+ cores (Intel Xeon, AMD EPYC, or equivalent)
- RAM: 64GB DDR4/DDR5
- Storage: 500GB NVMe SSD
- Network: 10 GbE
- PSU: 850W+ (check GPU TDP)

**Multi-GPU Setup (4x GPU):**
- CPU: 32+ cores
- RAM: 256GB DDR4/DDR5
- Storage: 2TB+ NVMe SSD (RAID recommended)
- Network: 25 GbE or InfiniBand
- PSU: 2000W+ (check total GPU TDP)

**Multi-Node Setup:**
- Network: 100 GbE or InfiniBand (RDMA)
- Storage: Distributed filesystem (Lustre, BeeGFS, or NFS)
- Switch: Non-blocking fabric for GPU-to-GPU communication

### VRAM Requirements by Model Size

| Model Size | Training (FP32) | Training (FP16) | Training (BF16) | Inference (FP16) |
|------------|-----------------|-----------------|-----------------|------------------|
| 7B params  | 28GB | 14GB | 14GB | 7GB |
| 13B params | 52GB | 26GB | 26GB | 13GB |
| 30B params | 120GB | 60GB | 60GB | 30GB |
| 65B params | 260GB | 130GB | 130GB | 65GB |
| 70B params | 280GB | 140GB | 140GB | 70GB |

> **Note:** Add 20-30% overhead for gradients, optimizer states, and activations during training.

---

## Prerequisites

### Operating System Support

**Linux (Recommended):**
- Ubuntu 20.04/22.04 LTS
- CentOS 8/Rocky Linux 8
- Red Hat Enterprise Linux 8/9
- Debian 11/12

**Windows:**
- Windows Server 2019/2022
- Windows 10/11 Pro (development only)

**Note:** Linux provides better GPU driver support and performance.

### CUDA Installation

#### Required Versions

- **CUDA Toolkit:** 11.8+ or 12.x (recommended: 12.3+)
- **cuDNN:** 8.9+ (9.x recommended for best performance)
- **NVIDIA Driver:** 
  - RTX 30/40 series: 525.x+
  - A100/H100: 525.x+ (535.x+ recommended)

#### Install CUDA on Ubuntu 22.04

```bash
# Remove old CUDA installations
sudo apt-get --purge remove "*cuda*" "*cublas*" "*cufft*" "*curand*" "*cusolver*" "*cusparse*" "*npp*" "*nvjpeg*"

# Add NVIDIA package repositories
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt-get update

# Install CUDA Toolkit 12.3
sudo apt-get install -y cuda-toolkit-12-3

# Install cuDNN
sudo apt-get install -y libcudnn9-cuda-12

# Set environment variables
echo 'export PATH=/usr/local/cuda-12.3/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-12.3/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

# Verify installation
nvidia-smi
nvcc --version
```

#### Install CUDA on CentOS/RHEL 8

```bash
# Add NVIDIA repository
sudo dnf config-manager --add-repo https://developer.download.nvidia.com/compute/cuda/repos/rhel8/x86_64/cuda-rhel8.repo

# Install CUDA
sudo dnf install -y cuda-toolkit-12-3

# Install cuDNN
sudo dnf install -y libcudnn9-cuda-12

# Configure environment
echo 'export PATH=/usr/local/cuda-12.3/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-12.3/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

#### Verify GPU Setup

```bash
# Check NVIDIA driver
nvidia-smi

# Expected output:
# +-----------------------------------------------------------------------------+
# | NVIDIA-SMI 535.129.03   Driver Version: 535.129.03   CUDA Version: 12.3    |
# |-------------------------------+----------------------+----------------------+
# | GPU  Name        Persistence-M| Bus-Id        Disp.A | Volatile Uncorr. ECC |
# | Fan  Temp  Perf  Pwr:Usage/Cap|         Memory-Usage | GPU-Util  Compute M. |
# |===============================+======================+======================|
# |   0  NVIDIA A100-SXM4-40GB   On   | 00000000:00:04.0 Off |                    0 |
# | N/A   30C    P0    50W / 400W |      0MiB / 40960MiB |      0%      Default |
# +-------------------------------+----------------------+----------------------+

# Test CUDA
cat << 'EOF' > test_cuda.cu
#include <stdio.h>

__global__ void hello() {
    printf("Hello from GPU!\n");
}

int main() {
    hello<<<1, 1>>>();
    cudaDeviceSynchronize();
    return 0;
}
EOF

nvcc test_cuda.cu -o test_cuda
./test_cuda
rm test_cuda test_cuda.cu
```

---

## Installation

### Docker Installation (Recommended)

#### Prerequisites

```bash
# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Install NVIDIA Container Toolkit
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | \
    sudo tee /etc/apt/sources.list.d/nvidia-docker.list

sudo apt-get update
sudo apt-get install -y nvidia-container-toolkit

# Configure Docker to use NVIDIA runtime
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker

# Test GPU access in Docker
docker run --rm --gpus all nvidia/cuda:12.3.0-base-ubuntu22.04 nvidia-smi
```

#### Deploy ThemisDB with GPU Support

```bash
# Pull latest image
docker pull themisdb/themisdb:latest-gpu

# Single GPU deployment
docker run -d \
  --name themisdb-gpu \
  --gpus all \
  -p 8080:8080 \
  -p 18765:18765 \
  -p 4318:4318 \
  -v themisdb_data:/data \
  -v themisdb_models:/models \
  -e THEMIS_GPU_ENABLED=true \
  -e THEMIS_GPU_DEVICE=0 \
  -e THEMIS_CUDA_VISIBLE_DEVICES=0 \
  themisdb/themisdb:latest-gpu

# Multi-GPU deployment (4 GPUs)
docker run -d \
  --name themisdb-multigpu \
  --gpus '"device=0,1,2,3"' \
  -p 8080:8080 \
  -p 18765:18765 \
  -p 4318:4318 \
  -v themisdb_data:/data \
  -v themisdb_models:/models \
  -e THEMIS_GPU_ENABLED=true \
  -e THEMIS_GPU_DEVICES=0,1,2,3 \
  -e THEMIS_MULTI_GPU_STRATEGY=data_parallel \
  -e NCCL_DEBUG=INFO \
  themisdb/themisdb:latest-gpu
```

### Native Installation

#### Build from Source

```bash
# Install build dependencies
sudo apt-get install -y build-essential cmake git \
  librocksdb-dev libssl-dev libcurl4-openssl-dev \
  ninja-build pkg-config

# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Configure with GPU support
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda-12.3 \
  -DTHEMIS_ENABLE_LLAMA=ON \
  -DTHEMIS_ENABLE_LORA=ON

# Build (use all cores)
cmake --build build -j$(nproc)

# Install
sudo cmake --install build
```

---

## GPU Configuration

### Single GPU Setup

Edit `/etc/themisdb/config.yaml`:

```yaml
gpu:
  enabled: true
  devices:
    - id: 0
      memory_limit: 0.9  # Use 90% of VRAM
      compute_capability: 8.6
  
llm:
  backend: cuda
  model_path: /models/llama-2-7b-chat.gguf
  context_length: 4096
  batch_size: 512
  n_gpu_layers: 35  # Offload all layers to GPU
  
lora:
  enabled: true
  adapter_path: /adapters
  max_adapters: 8
  gpu_enabled: true
```

### Multi-GPU Setup (Data Parallel)

```yaml
gpu:
  enabled: true
  multi_gpu:
    enabled: true
    strategy: data_parallel  # or: model_parallel, pipeline_parallel
    devices:
      - id: 0
        memory_limit: 0.9
      - id: 1
        memory_limit: 0.9
      - id: 2
        memory_limit: 0.9
      - id: 3
        memory_limit: 0.9
  
  # NCCL configuration for multi-GPU communication
  nccl:
    enabled: true
    debug: false
    p2p_enabled: true  # GPU-to-GPU direct transfers
    ib_enabled: false  # Enable for InfiniBand
    net_plugin: ""     # Leave empty for default

llm:
  backend: cuda
  model_path: /models/llama-2-70b-chat.gguf
  context_length: 4096
  batch_size: 2048  # Larger batch with multiple GPUs
  n_gpu_layers: -1  # Distribute across all GPUs
  tensor_split: [0.25, 0.25, 0.25, 0.25]  # Equal distribution
```

### Multi-Node Distributed Setup

**Coordinator Node** (`config.yaml`):

```yaml
distributed:
  enabled: true
  mode: coordinator
  coordinator_address: 192.168.1.100:9000
  
  nodes:
    - id: node-1
      address: 192.168.1.101:9000
      gpus: 4
    - id: node-2
      address: 192.168.1.102:9000
      gpus: 4
    - id: node-3
      address: 192.168.1.103:9000
      gpus: 4

gpu:
  enabled: true
  multi_gpu:
    enabled: true
    strategy: model_parallel
```

**Worker Node** (`config.yaml`):

```yaml
distributed:
  enabled: true
  mode: worker
  coordinator_address: 192.168.1.100:9000
  node_id: node-1

gpu:
  enabled: true
  devices:
    - id: 0
    - id: 1
    - id: 2
    - id: 3
```

---

## Network Setup

### Single Node Multi-GPU

For GPUs connected via NVLink or PCIe:

```bash
# Check GPU topology
nvidia-smi topo -m

# Expected for NVLink-connected GPUs:
#         GPU0    GPU1    GPU2    GPU3
# GPU0     X      NV12    NV12    NV12
# GPU1    NV12     X      NV12    NV12
# GPU2    NV12    NV12     X      NV12
# GPU3    NV12    NV12    NV12     X

# Enable persistence mode
sudo nvidia-smi -pm 1

# Set application clocks (A100 example)
sudo nvidia-smi -ac 1215,1410
```

### Multi-Node Setup with InfiniBand

```bash
# Install InfiniBand drivers
sudo apt-get install -y infiniband-diags perftest

# Configure NCCL for InfiniBand
export NCCL_IB_DISABLE=0
export NCCL_IB_HCA=mlx5_0
export NCCL_SOCKET_IFNAME=ib0
export NCCL_DEBUG=INFO

# Test InfiniBand connectivity
ibstatus
ibping <remote_host>

# Test RDMA bandwidth
ib_write_bw
```

### Firewall Configuration

```bash
# Open required ports
sudo firewall-cmd --permanent --add-port=8080/tcp     # HTTP API
sudo firewall-cmd --permanent --add-port=18765/tcp    # Binary protocol
sudo firewall-cmd --permanent --add-port=4318/tcp     # Metrics
sudo firewall-cmd --permanent --add-port=9000/tcp     # Distributed coordinator
sudo firewall-cmd --permanent --add-port=9100-9199/tcp  # Worker nodes
sudo firewall-cmd --reload
```

---

## Storage Configuration

### Local NVMe Setup

```bash
# Check available NVMe drives
lsblk -d -o NAME,SIZE,MODEL | grep nvme

# Format and mount
sudo mkfs.ext4 -F /dev/nvme0n1
sudo mkdir -p /data/themisdb
sudo mount /dev/nvme0n1 /data/themisdb

# Add to /etc/fstab for persistence
echo '/dev/nvme0n1 /data/themisdb ext4 defaults,noatime 0 2' | sudo tee -a /etc/fstab
```

### RAID Configuration (High Availability)

See [examples/raid_configuration.yaml](examples/raid_configuration.yaml) for detailed RAID setup.

**RAID 5 (Minimum 3 drives):**

```bash
# Create RAID 5 array
sudo mdadm --create --verbose /dev/md0 \
  --level=5 \
  --raid-devices=4 \
  /dev/nvme0n1 /dev/nvme1n1 /dev/nvme2n1 /dev/nvme3n1

# Format and mount
sudo mkfs.ext4 -F /dev/md0
sudo mkdir -p /data/themisdb
sudo mount /dev/md0 /data/themisdb
```

### Distributed Storage

**NFS Setup:**

```bash
# On storage server
sudo apt-get install -y nfs-kernel-server
echo '/data/themisdb *(rw,sync,no_subtree_check,no_root_squash)' | sudo tee -a /etc/exports
sudo exportfs -ra
sudo systemctl restart nfs-kernel-server

# On compute nodes
sudo apt-get install -y nfs-common
sudo mount 192.168.1.200:/data/themisdb /data/themisdb
```

---

## Security Hardening

### GPU Access Control

```yaml
security:
  gpu:
    # Limit GPU access to specific users/groups
    access_control:
      enabled: true
      allowed_users: [themisdb, admin]
      allowed_groups: [gpu-users]
    
    # GPU memory isolation
    memory_isolation:
      enabled: true
      per_process_limit: 0.25  # Max 25% VRAM per process
```

### Encryption

**Data at Rest:**

```yaml
storage:
  encryption:
    enabled: true
    algorithm: AES-256-GCM
    key_provider: hsm  # or: file, env, vault
    key_rotation:
      enabled: true
      interval_days: 90
```

**Data in Transit (mTLS):**

```yaml
network:
  tls:
    enabled: true
    version: "1.3"
    cert_file: /etc/themisdb/certs/server.crt
    key_file: /etc/themisdb/certs/server.key
    ca_file: /etc/themisdb/certs/ca.crt
    
    # Mutual TLS
    client_auth: required
    client_cert_file: /etc/themisdb/certs/client.crt
```

### Audit Logging

```yaml
logging:
  audit:
    enabled: true
    level: info
    output: /var/log/themisdb/audit.log
    rotate:
      max_size: 100MB
      max_age: 30
      max_backups: 10
    
    events:
      - gpu_access
      - model_load
      - training_start
      - checkpoint_save
      - authentication
      - authorization
```

---

## Validation

### Post-Installation Checks

```bash
# Check ThemisDB service
sudo systemctl status themisdb

# Verify GPU access
themisdb-cli system gpu-info

# Test API endpoint
curl http://localhost:8080/health

# Check logs
sudo journalctl -u themisdb -f
```

### GPU Validation

```bash
# Run built-in GPU test
themisdb-cli test gpu --device 0

# Expected output:
# GPU 0: NVIDIA A100-SXM4-40GB
# Compute Capability: 8.0
# VRAM: 40960 MB
# CUDA Version: 12.3
# Driver Version: 535.129.03
# Status: READY
```

### Load Test

```bash
# Single GPU inference test
themisdb-cli benchmark inference \
  --model llama-2-7b \
  --batch-size 32 \
  --sequence-length 512 \
  --duration 60

# Training benchmark
themisdb-cli benchmark training \
  --model llama-2-7b \
  --dataset wikitext \
  --epochs 1 \
  --batch-size 16
```

---

## Troubleshooting

### Common Issues

**Issue: GPU not detected**

```bash
# Check NVIDIA driver
nvidia-smi

# If not working, reinstall driver
sudo apt-get purge nvidia-*
sudo apt-get install -y nvidia-driver-535
sudo reboot
```

**Issue: Out of Memory (OOM)**

```bash
# Reduce batch size in config
# OR enable gradient accumulation
# OR reduce model layers on GPU
```

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for detailed solutions.

---

## Next Steps

1. **Performance Tuning**: See [PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md)
2. **Monitoring Setup**: See [MONITORING.md](MONITORING.md)
3. **Security Hardening**: See [SECURITY.md](SECURITY.md)
4. **Operational Procedures**: See [RUNBOOKS.md](RUNBOOKS.md)

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Next Review:** April 2026

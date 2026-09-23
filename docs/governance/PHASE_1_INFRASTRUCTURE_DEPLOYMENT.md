# Phase 1: GPU Infrastructure Deployment Guide

**Author:** ThemisDB Contributors  
**Created:** 2026-09-23  
**Last Updated:** 2026-09-23  
**Status:** active  

**Issue:** makr-code/ThemisDB#6575  
**Phase:** 1 (Infrastructure Prep)  
**Timeline:** 2026-09-23 to 2026-10-21 (2–4 weeks)  
**Owner:** Platform Team + Infrastructure  
**Status:** 🟡 IN PROGRESS — Documentation complete, hardware deployment pending  

---

## Overview

Phase 1 prepares the GPU infrastructure required for CUDA reduction optimization (Phase 2), baseline capture (Phase 3), and sign-off (Phase 4). All requirements are fully documented; this phase is **operations-dependent** (hardware procurement, installation, CI/CD integration).

---

## Prerequisites

### Decision Gate
Before starting Phase 1:
- [ ] Platform Team approves GPU hardware acquisition (A100 or H100)
- [ ] Budget and procurement timeline confirmed
- [ ] Datacenter or cloud environment identified (on-premises self-hosted or cloud provider)

If prerequisites cannot be met, escalate to release team for deferral decision per `ROADMAP.md` §Wave A GPU Non-Blocking Status.

---

## Deliverables

### 1. GPU Hardware Acquisition & Setup

**Hardware Specification** (from `docs/governance/GPU_SELF_HOSTED_RUNNER_REQUIREMENTS.md`):

| Component | Requirement | Rationale |
|-----------|-------------|-----------|
| **GPU** | NVIDIA A100 or H100 (Compute Capability 8.0+, ≥40 GB VRAM) | Representative hardware for Wave A workload baseline; H100 optional for 2025+ performance targets |
| **System RAM** | 128 GB minimum (256 GB recommended) | Buffer pool + CUDA operations overhead |
| **Storage** | 1 TB NVMe SSD (scratch + benchmark results) | Fast I/O for benchmark runs and data transfer |
| **CPU** | 64+ cores (AMD EPYC 7004 or Intel Xeon Platinum) | Parallelism for CPU fallback validation; benchmark harness |
| **Network** | 10 Gbps Ethernet (25 Gbps recommended) | Data transfer latency baseline; multi-GPU NCCL testing |
| **OS** | Ubuntu 20.04 LTS or 22.04 LTS | Tested build environment matching CI/CD |

**Procurement Steps:**
1. [ ] Submit hardware request to procurement with specification (link to GPU_SELF_HOSTED_RUNNER_REQUIREMENTS.md)
2. [ ] Track delivery and installation timeline
3. [ ] Allocate isolated network segment (security: no internet-facing services)
4. [ ] Configure IPMI/out-of-band management for remote access

**Estimated Effort:** 1–2 weeks  
**Blocker Risk:** HIGH — External dependency on hardware availability

---

### 2. Software Stack Installation

Follow `docs/governance/GPU_SELF_HOSTED_RUNNER_REQUIREMENTS.md` §Installation Steps (lines 45–95).

#### 2.1 CUDA Toolkit Installation

**Versions:** CUDA 12.x with cuDNN 8.x, NCCL 2.20+

```bash
# Example for CUDA 12.5 on Ubuntu 22.04 LTS (adapt for your OS version)
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.0-1_all.deb
dpkg -i cuda-keyring_1.0-1_all.deb
apt update && apt install -y cuda-toolkit-12-5

# Verify installation
nvcc --version
nvidia-smi
```

**Steps:**
- [ ] Install CUDA 12.x from NVIDIA repositories
- [ ] Install cuDNN 8.x (`libcudnn8` + development headers)
- [ ] Install NCCL 2.20+ (`libnccl2` + development headers)
- [ ] Verify with `nvcc --version`, `nvidia-smi`, and NCCL test binary

**Estimated Effort:** 1–2 days  
**Risk:** Driver version conflicts with OS kernel

**Mitigation:**
- Pin NVIDIA driver to tested versions (470.x, 510.x, 530.x+)
- Test in isolated environment before production deployment
- Maintain rollback driver image

#### 2.2 Build Tools Installation

```bash
apt update && apt install -y \
  cmake 3.22+ \
  ninja-build 1.11+ \
  gcc 11+ / clang 14+ \
  libfmt-dev \
  libspdlog-dev \
  nlohmann-json3-dev
```

**Steps:**
- [ ] Install CMake 3.22+
- [ ] Install Ninja 1.11+
- [ ] Install GCC 11+ or Clang 14+
- [ ] Install build dependencies (fmt, spdlog, nlohmann-json)
- [ ] Verify via `cmake --version`, `ninja --version`, `gcc --version`

**Estimated Effort:** 1 day

#### 2.3 GitHub Actions Runner Installation

**Runner Requirements:**
- GitHub Actions self-hosted runner (v2.311.0+)
- Service account with minimal permissions (no sudo or package manager access)

```bash
# Create runner user (non-root)
useradd -m -s /bin/bash github-runner

# Download and install runner
cd /opt/actions-runner
wget https://github.com/actions/runner/releases/download/v2.311.0/actions-runner-linux-x64-2.311.0.tar.gz
tar xzf actions-runner-linux-x64-2.311.0.tar.gz
chown -R github-runner:github-runner /opt/actions-runner

# Register runner (interactive, provide GitHub org/repo and token)
sudo -u github-runner ./config.sh --url https://github.com/makr-code/ThemisDB \
  --token <RUNNER_REGISTRATION_TOKEN> \
  --name gpu-cuda-runner-01 \
  --labels gpu-cuda,linux,cuda-12,a100

# Install and start as systemd service
sudo ./svc.sh install github-runner
sudo systemctl start github-runner
sudo systemctl enable github-runner
```

**Steps:**
- [ ] Create service account (`github-runner`)
- [ ] Download GitHub Actions runner v2.311.0+
- [ ] Register runner with labels: `gpu-cuda`, `linux`, `cuda-12`, `a100` (or `h100`)
- [ ] Install as systemd service
- [ ] Verify runner online in GitHub (Settings → Actions → Runners)

**Estimated Effort:** 1 day  
**Critical Label:** The runner MUST have label `gpu-cuda` for CI/CD job selection

---

### 3. Health Check & Validation

Execute validation script from `GPU_SELF_HOSTED_RUNNER_REQUIREMENTS.md` §Validation Script (lines 97–127).

**Validation Steps:**

1. **GPU Detection & Capabilities**
   ```bash
   nvidia-smi --query-gpu=index,name,memory.total,compute_cap --format=csv,noheader
   ```
   - Expected: A100 or H100 listed; ≥40 GB memory; Compute Capability 8.0+

2. **CUDA Version & Libraries**
   ```bash
   nvcc --version
   ldconfig -p | grep cudart  # CUDA runtime
   ldconfig -p | grep cublas  # CUBLAS
   ldconfig -p | grep nccl    # NCCL
   ```

3. **Build Tools Verification**
   ```bash
   cmake --version  # 3.22+
   ninja --version  # 1.11+
   gcc --version    # 11+
   ```

4. **NCCL Functional Test**
   ```bash
   # If nccl-tests installed:
   /usr/local/cuda/bin/nccl-tests_gpu_arch_check
   ```

5. **Repository Build Test**
   ```bash
   cd /home/runner/work/ThemisDB/ThemisDB
   cmake --preset community-release -DTHEMIS_ENABLE_CUDA=ON -B /tmp/phase1-validation-build
   cmake --build /tmp/phase1-validation-build --target gpu_memory_allocator -j4
   ```
   - Expected: Clean build with no CUDA-related compile errors

**Checklist:**
- [ ] GPU detection successful (nvidia-smi output)
- [ ] CUDA 12.x installed (`nvcc --version`)
- [ ] cuDNN 8.x libraries present
- [ ] NCCL 2.20+ libraries present
- [ ] CMake 3.22+, Ninja 1.11+, GCC 11+
- [ ] Repository builds cleanly with CUDA enabled
- [ ] GitHub Actions runner online and healthy

**Estimated Effort:** 1 day

---

### 4. Benchmark Environment Setup

Prepare benchmark infrastructure for Phase 3.

**Tasks:**
- [ ] Clone benchmark binaries from repository
  ```bash
  cd /home/runner/work/ThemisDB/ThemisDB
  cmake --preset community-release -DTHEMIS_BUILD_BENCHMARKS=ON -B /tmp/phase1-bench-build
  cmake --build /tmp/phase1-bench-build --target bench_gpu_a8_baselines
  ```

- [ ] Verify benchmark binary exists
  ```bash
  ls -lh /tmp/phase1-bench-build/bin/bench_gpu_a8_baselines
  ```

- [ ] Test single benchmark run (1-minute baseline)
  ```bash
  /tmp/phase1-bench-build/bin/bench_gpu_a8_baselines --benchmark_min_time=60s --benchmark_out=/tmp/phase1-warmup.json
  ```

- [ ] Verify JSON output format matches `benchmarks/wave8/GPU_BASELINES_2026_Q4.json` schema

**Estimated Effort:** 1 day

---

### 5. Network & Security Configuration

**Isolation:**
- [ ] Runner on isolated network segment (no internet-facing services)
- [ ] SSH access restricted to authorized users only
- [ ] No secrets or credentials in runner home directory

**GitHub Integration:**
- [ ] Register runner token is temporary (expires after use)
- [ ] Runner service runs as unprivileged user (`github-runner`)
- [ ] Actions secrets and variables synced to runner environment only at job runtime

**Estimated Effort:** 1 day

---

## Timeline & Checkpoints

| Week | Task | Owner | Status |
|------|------|-------|--------|
| Week 1 (09-23) | Hardware procurement submitted | Platform Team | 🔄 Pending |
| Week 1-2 | Hardware delivery & installation | Datacenter/Cloud Ops | 🔄 Pending |
| Week 2 (09-30) | CUDA 12.x + build tools installation | Infrastructure | ⏳ Queued |
| Week 2 (10-02) | GitHub Actions runner registration | Infrastructure | ⏳ Queued |
| Week 2-3 (10-07) | Validation script execution | GPU Module Team | ⏳ Queued |
| Week 3 (10-14) | Benchmark environment ready | GPU Module Team | ⏳ Queued |
| Week 3 (10-21) | Phase 1 complete, Phase 2 unblocked | Platform Team | 🔴 Blocked |

**Phase 1 Complete Criteria:**
- ✅ GPU hardware online and healthy
- ✅ CUDA 12.x + cuDNN + NCCL installed and verified
- ✅ GitHub Actions runner registered and reporting online
- ✅ Validation script passes all checks
- ✅ Repository builds cleanly with CUDA enabled
- ✅ Benchmark binaries compiled and tested

---

## Risk Mitigation

| Risk | Severity | Mitigation |
|------|----------|-----------|
| **Hardware procurement delayed** | HIGH | Pre-identify backup hardware option; consider cloud GPU provider as fallback |
| **CUDA driver conflicts** | MEDIUM | Test in isolated environment; maintain rollback driver image; pin to tested versions |
| **Network connectivity issues** | MEDIUM | Test NCCL on multi-GPU setup (if available); verify 10 Gbps link latency |
| **Runner registration fails** | MEDIUM | Verify GitHub token permissions; check runner connectivity; consult GitHub Actions docs |
| **Build environment incompatible** | MEDIUM | Cross-check CMake/Ninja/GCC versions against CI/CD specifications in README |

---

## Escalation Path

| Issue | Owner | Resolution |
|-------|-------|-----------|
| Hardware unavailable beyond 2026-10-21 | Platform Team | Escalate to release team; evaluate CPU-only deferral per `ROADMAP.md` |
| CUDA installation fails | Infrastructure | Consult NVIDIA driver compatibility matrix; test in sandbox environment |
| CI/CD runner offline | Infrastructure Team | Activate backup runner; trigger manual baseline execution |
| Benchmark build fails | GPU Module Team | File issue against GPU module; extend Phase 1 timeline if needed |

---

## Sign-Off Criteria

**Phase 1 is complete when:**

```markdown
- [ ] Hardware specification: NVIDIA A100/H100 with ≥40 GB VRAM confirmed
- [ ] CUDA 12.x + cuDNN 8.x + NCCL 2.20+ installed and verified
- [ ] GitHub Actions self-hosted runner (gpu-cuda) online and healthy
- [ ] Validation script passes all checks
- [ ] Repository builds successfully with CUDA enabled
- [ ] Benchmark binaries compiled and tested on representative hardware
- [ ] Network isolation and security configuration complete
- [ ] Owner sign-off: <name> on <date> at <time> UTC
```

---

## Next Phase

Upon Phase 1 completion:
- **Phase 2 Kickoff:** CUDA Reduction code refactoring begins (parallel effort with infrastructure validation)
- **Phase 3 Readiness:** Representative hardware available for baseline capture (follow-up: 1–2 weeks after Phase 2)

---

**Document Type:** Operational Guide  
**Owner:** Platform Team + Infrastructure  
**Review Cadence:** Weekly during Phase 1 execution  
**Escalation:** platform-release@themisdb  

# Troubleshooting Guide

**Version:** 1.8.0-rc1  
**Last Updated:** April 2026  
**Target Audience:** Engineers, DevOps, Support

## Table of Contents

1. [Quick Diagnostic Commands](#quick-diagnostic-commands)
2. [GPU Errors](#gpu-errors)
3. [Out of Memory (OOM)](#out-of-memory-oom)
4. [CUDA Compatibility](#cuda-compatibility)
5. [Network Issues](#network-issues)
6. [Storage Problems](#storage-problems)
7. [Training Issues](#training-issues)
8. [Inference Problems](#inference-problems)
9. [Performance Degradation](#performance-degradation)
10. [Data Corruption](#data-corruption)

---

## Quick Diagnostic Commands

### System Health Check

```bash
# ThemisDB status
themisdb-cli status

# GPU status
nvidia-smi

# Detailed GPU info
nvidia-smi -q

# Check CUDA
nvcc --version

# Check cuDNN
ldconfig -p | grep cudnn

# Check disk space
df -h /data/themisdb

# Check memory
free -h

# Check processes
ps aux | grep themis

# Check logs
sudo journalctl -u themisdb -f
```

### GPU Diagnostic

```bash
# Run GPU self-test
themisdb-cli test gpu --full

# Check GPU topology
nvidia-smi topo -m

# Monitor GPU in real-time
nvidia-smi dmon -s pucvmet

# Check GPU errors
nvidia-smi --query-gpu=gpu_name,ecc.errors.corrected.aggregate.total --format=csv

# Test GPU memory
cuda-memcheck themisdb-server
```

---

## GPU Errors

### Error: "CUDA error: out of memory"

**Symptom:**
```
RuntimeError: CUDA out of memory. Tried to allocate 2.00 GiB 
(GPU 0; 23.70 GiB total capacity; 21.50 GiB already allocated)
```

**Causes:**
1. Batch size too large
2. Model too large for GPU
3. Memory leak
4. Fragmentation

**Solutions:**

```bash
# 1. Reduce batch size
# Edit config.yaml
training:
  micro_batch_size: 8  # Reduce from 16
  gradient_accumulation_steps: 8  # Increase to maintain effective batch size

# 2. Enable gradient checkpointing
training:
  gradient_checkpointing:
    enabled: true
    checkpoint_segments: 4

# 3. Use 8-bit optimizer
training:
  optimizer: adamw_8bit

# 4. Clear GPU cache
themisdb-cli gpu clear-cache

# 5. Restart service
sudo systemctl restart themisdb
```

**Prevention:**
```yaml
# Set memory limit
gpu:
  memory_limit: 0.85  # Use max 85% of VRAM
  
  # Enable OOM protection
  oom_protection:
    enabled: true
    reserved_memory_mb: 2048
```

### Error: "CUDA error: device-side assert triggered"

**Symptom:**
```
RuntimeError: CUDA error: device-side assert triggered
CUDA kernel errors might be asynchronously reported
```

**Causes:**
1. Invalid tensor indices
2. NaN/Inf in computations
3. Wrong tensor dimensions
4. Label out of range

**Solutions:**

```bash
# 1. Enable CUDA error checking
export CUDA_LAUNCH_BLOCKING=1

# 2. Run with debug mode
themisdb-cli train --debug --check-numerics

# 3. Check for NaN/Inf
training:
  check_numerics:
    enabled: true
    check_interval: 10  # Every 10 steps
    
  gradient_clipping:
    enabled: true
    max_norm: 1.0

# 4. Validate data
themisdb-cli validate dataset --path /data/train
```

### Error: "GPU has fallen off the bus"

**Symptom:**
```
Unable to determine the device handle for GPU 0000:01:00.0: GPU is lost
```

**Causes:**
1. GPU hardware failure
2. Power supply issue
3. Overheating
4. Driver problem
5. PCIe connection issue

**Solutions:**

```bash
# 1. Check GPU temperature
nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader

# 2. Check power
nvidia-smi --query-gpu=power.draw,power.limit --format=csv

# 3. Reseat GPU (if accessible)
# - Power off system
# - Remove and reinsert GPU
# - Ensure PCIe power connectors are secure

# 4. Reset GPU
sudo nvidia-smi -r

# 5. Reload driver
sudo modprobe -r nvidia_uvm nvidia_drm nvidia_modeset nvidia
sudo modprobe nvidia nvidia_modeset nvidia_drm nvidia_uvm

# 6. Check dmesg for hardware errors
dmesg | grep -i nvidia
dmesg | grep -i pcie

# 7. Run hardware diagnostic
nvidia-smi --id=0 --reset-gpu
```

**Prevention:**
```bash
# Enable persistence mode
sudo nvidia-smi -pm 1

# Set power limit (prevent power spikes)
sudo nvidia-smi -pl 300  # 300W limit

# Monitor temperature
sudo nvidia-smi --query-gpu=temperature.gpu --loop=1
```

### Error: "NCCL initialization failed"

**Symptom:**
```
NCCL error: unhandled system error
NCCL version 2.18.3
```

**Causes:**
1. Network misconfiguration
2. Firewall blocking ports
3. GPU topology issues
4. NCCL version mismatch

**Solutions:**

```bash
# 1. Check GPU visibility
export CUDA_VISIBLE_DEVICES=0,1,2,3
nvidia-smi

# 2. Enable NCCL debug
export NCCL_DEBUG=INFO
export NCCL_DEBUG_SUBSYS=ALL

# 3. Check network interface
export NCCL_SOCKET_IFNAME=eth0

# 4. Disable P2P if causing issues
export NCCL_P2P_DISABLE=1

# 5. Verify GPU topology
nvidia-smi topo -m

# 6. Test NCCL
/usr/local/cuda/bin/nccl-tests/all_reduce_perf -b 8 -e 256M -f 2 -g 4
```

**Configuration:**
```yaml
multi_gpu:
  nccl:
    enabled: true
    debug: true
    p2p_enabled: true
    ib_enabled: false  # Set true for InfiniBand
    socket_ifname: "eth0"
```

---

## Out of Memory (OOM)

### System Memory OOM

**Symptom:**
```
kernel: Out of memory: Kill process 12345 (themisdb-server)
```

**Solutions:**

```bash
# 1. Check memory usage
free -h
ps aux --sort=-%mem | head

# 2. Increase swap
sudo fallocate -l 32G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# 3. Reduce dataloader workers
dataloader:
  num_workers: 4  # Reduce from 8
  prefetch_factor: 2  # Reduce from 4

# 4. Enable CPU offloading
training:
  cpu_offload:
    enabled: true
    offload_optimizer: true
```

### GPU Memory Fragmentation

**Symptom:**
```
CUDA out of memory despite having free memory
```

**Solutions:**

```bash
# 1. Clear fragmented memory
themisdb-cli gpu defragment

# 2. Enable memory pool
gpu:
  memory_pool:
    enabled: true
    initial_size_mb: 4096
    max_split_size_mb: 512

# 3. Restart with fresh allocation
sudo systemctl restart themisdb

# 4. Use unified memory
gpu:
  unified_memory: true
```

---

## CUDA Compatibility

### CUDA Version Mismatch

**Symptom:**
```
version `libcuda.so.1.1' not found
CUDA driver version is insufficient for CUDA runtime version
```

**Solutions:**

```bash
# 1. Check versions
nvidia-smi  # Shows driver CUDA version
nvcc --version  # Shows toolkit version

# 2. Install compatible CUDA
sudo apt-get install cuda-toolkit-12-3

# 3. Update LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/cuda-12.3/lib64:$LD_LIBRARY_PATH

# 4. Rebuild ThemisDB
cd /path/to/ThemisDB
cmake -B build -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda-12.3
cmake --build build
```

### cuDNN Not Found

**Symptom:**
```
Could not load dynamic library 'libcudnn.so.8'
```

**Solutions:**

```bash
# 1. Check cuDNN installation
ldconfig -p | grep cudnn

# 2. Install cuDNN
sudo apt-get install libcudnn9-cuda-12

# 3. Link manually if needed
sudo ln -s /usr/local/cuda-12.3/lib64/libcudnn.so.9 /usr/lib/x86_64-linux-gnu/

# 4. Update LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/cuda-12.3/lib64:$LD_LIBRARY_PATH
```

---

## Network Issues

### Multi-GPU Communication Timeout

**Symptom:**
```
NCCL error: Timeout on rank 1
```

**Solutions:**

```bash
# 1. Increase timeout
export NCCL_TIMEOUT=3600  # 1 hour

# 2. Check network bandwidth
iperf3 -s  # On server
iperf3 -c <server_ip>  # On client

# 3. Disable firewall temporarily
sudo systemctl stop firewalld

# 4. Check for packet loss
ping -c 100 <node_ip>

# 5. Use different network interface
export NCCL_SOCKET_IFNAME=ib0  # For InfiniBand
```

### Distributed Training Connection Refused

**Symptom:**
```
Connection refused when connecting to coordinator at 192.168.1.100:9000
```

**Solutions:**

```bash
# 1. Check coordinator is running
ps aux | grep themisdb

# 2. Check port is listening
sudo netstat -tlnp | grep 9000

# 3. Open firewall ports
sudo firewall-cmd --permanent --add-port=9000/tcp
sudo firewall-cmd --reload

# 4. Check coordinator config
distributed:
  mode: coordinator
  bind_address: "0.0.0.0:9000"  # Not 127.0.0.1

# 5. Test connectivity
telnet 192.168.1.100 9000
nc -zv 192.168.1.100 9000
```

---

## Storage Problems

### Disk Full During Training

**Symptom:**
```
No space left on device
```

**Solutions:**

```bash
# 1. Check disk usage
df -h
du -sh /data/themisdb/*

# 2. Clean old checkpoints
find /data/themisdb/checkpoints -mtime +7 -delete

# 3. Enable checkpoint rotation
training:
  checkpoint:
    max_keep: 3  # Keep only 3 latest
    auto_cleanup: true

# 4. Move to larger disk
sudo rsync -avh /data/themisdb/ /mnt/large-disk/themisdb/

# 5. Enable compression
storage:
  checkpoint_compression: true
  compression_level: 6
```

### Slow I/O Performance

**Symptom:**
```
Training throughput drops periodically
iostat shows high iowait
```

**Solutions:**

```bash
# 1. Check I/O stats
iostat -x 1

# 2. Use NVMe if available
lsblk -d -o NAME,ROTA
# ROTA=0 means SSD/NVMe

# 3. Increase dataloader workers
dataloader:
  num_workers: 16
  prefetch_factor: 4
  pin_memory: true

# 4. Cache dataset in memory
training:
  dataset_cache:
    enabled: true
    cache_size_gb: 64

# 5. Use tmpfs for temporary files
sudo mount -t tmpfs -o size=32G tmpfs /tmp/themisdb-cache
```

### RAID Degradation

**Symptom:**
```
mdadm: RAID array degraded
```

**Solutions:**

```bash
# 1. Check RAID status
cat /proc/mdstat
sudo mdadm --detail /dev/md0

# 2. Identify failed drive
sudo mdadm --detail /dev/md0 | grep -i fail

# 3. Remove failed drive
sudo mdadm /dev/md0 --fail /dev/nvme0n1
sudo mdadm /dev/md0 --remove /dev/nvme0n1

# 4. Add replacement drive
sudo mdadm /dev/md0 --add /dev/nvme4n1

# 5. Monitor rebuild
watch cat /proc/mdstat

# 6. Enable hot spare
sudo mdadm /dev/md0 --add-spare /dev/nvme5n1
```

---

## Training Issues

### Loss is NaN or Inf

**Symptom:**
```
Training loss: nan
```

**Causes:**
1. Learning rate too high
2. Gradient explosion
3. Numerical instability
4. Bad data

**Solutions:**

```bash
# 1. Reduce learning rate
training:
  learning_rate: 1e-5  # Reduce from 3e-4

# 2. Enable gradient clipping
training:
  gradient_clipping:
    enabled: true
    max_norm: 1.0

# 3. Use BF16 instead of FP16
training:
  precision: bf16  # More stable than FP16

# 4. Check data
themisdb-cli validate dataset --check-nan

# 5. Add warmup
training:
  learning_rate:
    warmup_steps: 1000
    warmup_ratio: 0.1

# 6. Use stable optimizer
training:
  optimizer: adamw
  optimizer_config:
    eps: 1e-8  # Increase from 1e-10
```

### Training Not Converging

**Symptom:**
```
Loss plateaus or oscillates
```

**Solutions:**

```bash
# 1. Adjust learning rate
training:
  learning_rate: 1e-4  # Try different values
  learning_rate_schedule: cosine_with_warmup

# 2. Increase batch size
training:
  batch_size: 64  # Larger batch = more stable

# 3. Check data quality
themisdb-cli analyze dataset --stats

# 4. Reduce regularization
training:
  weight_decay: 0.01  # Reduce from 0.1

# 5. Try different optimizer
training:
  optimizer: lion  # or: sophia, adafactor
```

### Checkpoint Save Failures

**Symptom:**
```
Failed to save checkpoint: Permission denied
```

**Solutions:**

```bash
# 1. Check permissions
ls -la /data/themisdb/checkpoints
sudo chown -R themisdb:themisdb /data/themisdb

# 2. Check disk space
df -h /data/themisdb

# 3. Enable async saving
training:
  checkpoint:
    async_save: true
    save_timeout: 300

# 4. Test write access
touch /data/themisdb/checkpoints/test && rm /data/themisdb/checkpoints/test
```

---

## Inference Problems

### High Latency

**Symptom:**
```
P95 latency > 500ms
```

**Solutions:**

```bash
# 1. Enable continuous batching
inference:
  continuous_batching:
    enabled: true
    max_batch_size: 32

# 2. Use smaller model
# Or enable quantization
inference:
  quantization: int8

# 3. Increase GPU allocation
gpu:
  memory_limit: 0.95

# 4. Enable KV cache
inference:
  kv_cache:
    enabled: true
    max_tokens: 65536

# 5. Profile and optimize
themisdb-cli profile inference --model llama-2-7b
```

### Incorrect Outputs

**Symptom:**
```
Model generates gibberish or repeats
```

**Solutions:**

```bash
# 1. Check model was loaded correctly
themisdb-cli model info --name llama-2-7b

# 2. Adjust sampling parameters
inference:
  temperature: 0.7
  top_p: 0.9
  top_k: 50
  repetition_penalty: 1.1

# 3. Reload model
themisdb-cli model reload --name llama-2-7b

# 4. Verify model hash
sha256sum /models/llama-2-7b.gguf
```

---

## Performance Degradation

### Gradual Slowdown

**Symptom:**
```
Training starts fast but slows down over time
```

**Solutions:**

```bash
# 1. Check memory leaks
nvidia-smi dmon -s m -c 1000

# 2. Monitor fragmentation
themisdb-cli gpu memory-stats

# 3. Restart periodically
# Add to crontab
0 3 * * * systemctl restart themisdb

# 4. Clear caches regularly
training:
  cache_clearing:
    enabled: true
    interval: 1000  # Every 1000 steps
```

### GPU Throttling

**Symptom:**
```
nvidia-smi shows reduced clock speeds
```

**Solutions:**

```bash
# 1. Check temperature
nvidia-smi --query-gpu=temperature.gpu,clocks_throttle_reasons.* --format=csv

# 2. Improve cooling
# - Check fans
# - Clean heatsinks
# - Improve airflow

# 3. Reduce power limit temporarily
sudo nvidia-smi -pl 300

# 4. Set persistence mode
sudo nvidia-smi -pm 1
```

---

## Data Corruption

### Checkpoint Corruption

**Symptom:**
```
Failed to load checkpoint: Invalid format
```

**Solutions:**

```bash
# 1. Verify checksum
sha256sum /data/checkpoints/latest.ckpt

# 2. Use backup checkpoint
themisdb-cli restore --checkpoint /data/checkpoints/previous.ckpt

# 3. Enable checksums
storage:
  checksums:
    enabled: true
    algorithm: sha256

# 4. Use RAID for redundancy
# See examples/raid_configuration.yaml
```

### Database Corruption

**Symptom:**
```
RocksDB error: Corruption detected
```

**Solutions:**

```bash
# 1. Run repair
themisdb-cli repair --data-dir /data/themisdb

# 2. Restore from backup
themisdb-cli restore --backup /backup/latest

# 3. Check filesystem
sudo fsck /dev/nvme0n1

# 4. Enable WAL
storage:
  wal:
    enabled: true
    sync_mode: fsync
```

---

## Emergency Procedures

### Quick Recovery Checklist

```bash
# 1. Stop service
sudo systemctl stop themisdb

# 2. Backup current state
sudo tar czf /tmp/themisdb-emergency-$(date +%Y%m%d-%H%M%S).tar.gz /data/themisdb

# 3. Check hardware
nvidia-smi
df -h
free -h

# 4. Clear GPU memory
sudo nvidia-smi --gpu-reset

# 5. Start in safe mode
themisdb-server --safe-mode --no-gpu

# 6. Restore from backup if needed
themisdb-cli restore --backup /backup/latest

# 7. Start service
sudo systemctl start themisdb
```

---

## Getting Help

### Information to Collect

```bash
# System info
uname -a
nvidia-smi
cat /etc/os-release

# ThemisDB version
themisdb-cli version

# Logs
sudo journalctl -u themisdb --since "1 hour ago" > themisdb.log

# Configuration
cat /etc/themisdb/config.yaml

# GPU diagnostics
nvidia-smi -q > gpu-info.txt

# Create support bundle
themisdb-cli support-bundle --output /tmp/support-bundle.tar.gz
```

### Support Channels

1. **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
2. **Documentation**: https://makr-code.github.io/ThemisDB/
3. **Community Forum**: https://github.com/makr-code/ThemisDB/discussions

---

## Next Steps

- **Monitoring**: Set up proactive monitoring ([MONITORING.md](MONITORING.md))
- **Performance**: Optimize your configuration ([PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md))
- **Runbooks**: Create incident response procedures ([RUNBOOKS.md](RUNBOOKS.md))

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Next Review:** April 2026

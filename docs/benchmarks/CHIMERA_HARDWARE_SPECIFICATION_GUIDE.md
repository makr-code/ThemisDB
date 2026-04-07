# CHIMERA Suite: Hardware Specification Guide

**Version:** 1.0.0  
**Standards Compliance:** IEEE Std 2807-2022  
**Last Updated:** April 2026

---

## Table of Contents

1. [Introduction](#introduction)
2. [Processor Specification](#processor-specification)
3. [Memory Configuration](#memory-configuration)
4. [Storage Specification](#storage-specification)
5. [Network Configuration](#network-configuration)
6. [System Configuration](#system-configuration)
7. [Hardware Profile Examples](#hardware-profile-examples)
8. [Verification and Validation](#verification-and-validation)

---

## 1. Introduction

### 1.1 Purpose

This guide explains how to specify hardware configurations for CHIMERA Suite benchmarks in compliance with IEEE Std 2807-2022 requirements for performance benchmarking.

### 1.2 Importance of Complete Hardware Specification

**Why it matters:**
- Enables reproducibility of benchmark results
- Allows fair comparisons across studies
- Documents environmental factors affecting performance
- Supports scientific peer review
- Meets publication standards

**IEEE Std 2807-2022 Requirements:**
> "Complete hardware and software configuration must be documented to enable independent verification and reproduction of results."

### 1.3 Vendor Neutrality in Hardware Specification

**Guidelines:**
- Anonymize vendor names when appropriate
- Use generic model descriptions
- Focus on objective technical specifications
- Document all performance-relevant features

---

## 2. Processor Specification

### 2.1 Complete Processor Configuration

```yaml
hardware:
  processor:
    # Architecture
    architecture: "x86_64"              # ISA: x86_64, ARM64, RISC-V, PowerPC
    vendor: "Generic"                    # Anonymized vendor name
    model: "8-Core High-Performance CPU" # Generic model description
    microarchitecture: "Modern x86"      # Architecture generation
    
    # Core configuration
    cores:
      physical: 8                        # Physical cores
      logical: 16                        # With SMT/Hyper-Threading
      smt_enabled: true                  # Simultaneous Multi-Threading
      
    # Frequency
    frequency:
      base_ghz: 3.6                      # Base frequency
      turbo_ghz: 5.0                     # Maximum turbo frequency
      turbo_cores: 2                     # Cores at max turbo
      all_core_turbo_ghz: 4.5            # All-core turbo frequency
      
    # Cache hierarchy
    cache:
      l1_instruction_kb: 32              # L1 instruction cache per core
      l1_data_kb: 32                     # L1 data cache per core
      l2_kb: 512                         # L2 cache per core
      l3_mb: 16                          # L3 cache (shared)
      l3_shared: true                    # Shared across cores
      
    # Extensions and features
    features:
      - "SSE4.2"                         # SIMD extensions
      - "AVX"
      - "AVX2"
      - "AVX512F"                        # AVX-512 Foundation
      - "AVX512DQ"                       # AVX-512 DQ
      - "AES-NI"                         # Hardware AES
      - "SHA"                            # Hardware SHA
      - "FMA"                            # Fused Multiply-Add
      - "BMI1"                           # Bit Manipulation 1
      - "BMI2"                           # Bit Manipulation 2
      
    # Power and thermal
    tdp_watts: 125                       # Thermal Design Power
    max_temp_celsius: 100                # Maximum temperature
    
    # Process technology
    process_nm: 10                       # Manufacturing process (nm)
```

### 2.2 Architecture Types

**x86_64 (AMD64):**
- Intel and AMD processors
- Dominant in servers and desktops
- Rich instruction set

**ARM64 (AArch64):**
- ARM Cortex, Graviton, Apple Silicon
- Growing in servers and mobile
- Energy efficient

**RISC-V:**
- Open-source ISA
- Emerging architecture
- Customizable

**PowerPC:**
- IBM POWER processors
- High-performance computing
- Legacy systems

### 2.3 SIMD Extension Capabilities

**Why it matters:** Vector operations significantly affect database performance.

**x86_64 Extensions:**

| Extension | Width | Year | Impact |
|-----------|-------|------|--------|
| SSE | 128-bit | 1999 | Basic SIMD |
| SSE2 | 128-bit | 2001 | Double precision |
| SSE4.2 | 128-bit | 2008 | String operations |
| AVX | 256-bit | 2011 | 2x throughput |
| AVX2 | 256-bit | 2013 | Integer operations |
| AVX-512 | 512-bit | 2016 | 4x throughput |

**Database Relevance:**
- Hash computations
- Compression/decompression
- Vector similarity search
- Aggregation operations
- Sorting algorithms

### 2.4 SMT (Simultaneous Multi-Threading)

```yaml
cores:
  physical: 8
  logical: 16
  smt_enabled: true
```

**Trade-offs:**
- **Enabled:** Higher throughput for parallel workloads
- **Disabled:** More consistent latency, better for OLTP

**Benchmark Note:** Document SMT status as it significantly affects results.

---

## 3. Memory Configuration

### 3.1 Complete Memory Specification

```yaml
hardware:
  memory:
    # Capacity
    total_gb: 64                         # Total RAM
    available_gb: 62                     # Available after OS
    
    # Memory type
    type: "DDR4"                         # DDR4, DDR5, LPDDR4, etc.
    generation: 4                        # Generation number
    
    # Speed and timing
    speed_mhz: 3200                      # Effective speed
    cas_latency: 16                      # CAS Latency (CL)
    timing: "16-18-18-38"                # CL-tRCD-tRP-tRAS
    
    # Configuration
    channels: 4                          # Memory channels
    dimms_per_channel: 1                 # DIMMs per channel
    rank_per_dimm: 2                     # Ranks per DIMM (SR/DR)
    
    # Features
    ecc_enabled: true                    # Error-Correcting Code
    ecc_type: "SEC-DED"                  # Single Error Correction, Double Error Detection
    
    # NUMA configuration
    numa_nodes: 1                        # Number of NUMA nodes
    memory_per_node_gb: 64               # Memory per NUMA node
    
    # Bandwidth (measured)
    bandwidth:
      theoretical_gbps: 102.4            # Theoretical bandwidth
      measured_read_gbps: 95.2           # Measured read bandwidth
      measured_write_gbps: 88.7          # Measured write bandwidth
      measured_copy_gbps: 92.1           # Measured copy bandwidth
```

### 3.2 Memory Type Characteristics

**DDR4:**
- Mature technology
- Speeds: 2133-3200 MHz (mainstream), up to 4266 MHz
- Lower power than DDR3
- Common in current servers

**DDR5:**
- Newer technology
- Speeds: 4800-6400 MHz
- Higher bandwidth
- On-die ECC
- Growing adoption

**LPDDR (Low Power DDR):**
- Mobile and embedded systems
- Lower power consumption
- Soldered (not upgradeable)

### 3.3 ECC (Error-Correcting Code)

```yaml
ecc_enabled: true
ecc_type: "SEC-DED"
```

**ECC Types:**
- **SEC-DED:** Single Error Correction, Double Error Detection
- **Chipkill:** Advanced multi-bit error correction
- **None:** No error correction

**Impact on Benchmarks:**
- Slight performance overhead (~2-3%)
- Critical for data integrity
- Required for production systems
- Document ECC status

### 3.4 NUMA (Non-Uniform Memory Access)

```yaml
numa_nodes: 2
memory_per_node_gb: 32
```

**Multi-Socket Systems:**
- Each CPU has local memory
- Remote memory access is slower
- Affects database performance

**NUMA Awareness:**
- Pin processes to NUMA nodes
- Allocate memory locally
- Document topology

**Measurement:**
```bash
# Check NUMA topology
numactl --hardware

# Measure memory latency
numactl --membind=0 --cpubind=0 stream
numactl --membind=1 --cpubind=0 stream  # Remote access
```

### 3.5 Memory Bandwidth Measurement

**Tool: STREAM Benchmark**

```bash
# Download and compile STREAM
gcc -O3 -fopenmp stream.c -o stream

# Run benchmark
./stream
```

**Report Results:**
```yaml
bandwidth:
  theoretical_gbps: 102.4
  measured_read_gbps: 95.2
  measured_write_gbps: 88.7
  measured_copy_gbps: 92.1
```

---

## 4. Storage Specification

### 4.1 Complete Storage Configuration

```yaml
hardware:
  storage:
    primary:
      # Type and interface
      type: "NVMe SSD"                   # HDD, SATA SSD, NVMe SSD, Optane
      interface: "PCIe 4.0 x4"           # Interface type
      protocol: "NVMe 1.4"               # Protocol version
      
      # Physical
      capacity_gb: 1000                  # Total capacity
      form_factor: "M.2"                 # M.2, 2.5", 3.5", U.2
      
      # Performance characteristics
      performance:
        sequential_read_mbps: 7000       # Sequential read
        sequential_write_mbps: 5000      # Sequential write
        random_read_iops: 1000000        # 4K random read IOPS
        random_write_iops: 700000        # 4K random write IOPS
        queue_depth: 32                  # Test queue depth
        
        # Latency (measured)
        avg_read_latency_us: 50          # Average read latency
        avg_write_latency_us: 80         # Average write latency
        p99_read_latency_us: 200         # 99th percentile read
        p99_write_latency_us: 400        # 99th percentile write
        
      # Endurance
      tbw: 600                           # Terabytes Written (warranty)
      dwpd: 1                            # Drive Writes Per Day
      
      # Filesystem
      filesystem: "ext4"                 # ext4, XFS, Btrfs, ZFS, NTFS
      mount_options: "noatime,nodiratime" # Mount options
      block_size_kb: 4                   # Filesystem block size
      
    # Optional: Secondary storage
    secondary:
      type: "SATA SSD"
      interface: "SATA 3.0"
      capacity_gb: 2000
      performance:
        sequential_read_mbps: 550
        sequential_write_mbps: 520
```

### 4.2 Storage Types

**HDD (Hard Disk Drive):**
- Rotating magnetic storage
- Low cost per GB
- Seq: 100-200 MB/s, IOPS: 100-200
- High latency (10-20 ms)
- Use case: Cold storage, archives

**SATA SSD:**
- SATA interface (6 Gbps limit)
- Moderate performance
- Seq: 500-550 MB/s, IOPS: 50K-100K
- Latency: 50-100 µs
- Use case: General purpose, legacy systems

**NVMe SSD:**
- PCIe interface (high bandwidth)
- High performance
- PCIe 3.0: up to 3.5 GB/s
- PCIe 4.0: up to 7 GB/s
- PCIe 5.0: up to 14 GB/s
- IOPS: 100K-1M+
- Latency: 10-100 µs
- Use case: High-performance databases

**Intel Optane (3D XPoint):**
- Lower latency than NAND
- Higher endurance
- Expensive
- Latency: ~10 µs
- Use case: Latency-sensitive workloads

### 4.3 Performance Measurement

**Sequential Throughput:**
```bash
# Using fio
fio --name=seqread --rw=read --bs=1M --size=10G --numjobs=1
fio --name=seqwrite --rw=write --bs=1M --size=10G --numjobs=1
```

**Random IOPS:**
```bash
# 4K random read
fio --name=randread --rw=randread --bs=4k --size=10G --numjobs=16 --iodepth=32

# 4K random write
fio --name=randwrite --rw=randwrite --bs=4k --size=10G --numjobs=16 --iodepth=32
```

**Latency:**
```bash
# Latency test
fio --name=latency --rw=randread --bs=4k --size=1G --numjobs=1 --iodepth=1
```

### 4.4 Filesystem Selection

**ext4:**
- Mature and stable
- Good all-around performance
- Default for many Linux distributions

**XFS:**
- Better for large files
- Parallel I/O
- Good for databases

**Btrfs:**
- Copy-on-write
- Snapshots
- Compression
- Still maturing

**ZFS:**
- Enterprise features
- Data integrity
- Compression
- Requires more memory

**NTFS:**
- Windows default
- Good compatibility
- Reasonable performance

**Recommendation:** Document rationale for filesystem choice.

### 4.5 Mount Options

```yaml
mount_options: "noatime,nodiratime,nobarrier"
```

**Important Options:**
- `noatime`: Don't update access time (reduces writes)
- `nodiratime`: Don't update directory access time
- `nobarrier`: Disable write barriers (only if UPS/BBU)
- `discard`: Enable TRIM for SSDs
- `relatime`: Update atime on modify or once per day

**Performance Impact:**
- `noatime`: 5-15% improvement for read-heavy workloads
- `nobarrier`: Higher risk but better write performance

---

## 5. Network Configuration

### 5.1 Complete Network Specification

```yaml
hardware:
  network:
    # Interface
    interface: "10GbE"                   # 1GbE, 10GbE, 25GbE, 40GbE, 100GbE
    nic_model: "Generic 10 Gigabit NIC"  # NIC model (anonymized)
    
    # Bandwidth
    bandwidth_gbps: 10                   # Link bandwidth
    measured_throughput_gbps: 9.4        # Measured throughput
    
    # Frame configuration
    mtu_bytes: 9000                      # Jumbo frames
    
    # Latency
    latency_us: 10                       # Measured latency
    measured_ping_us: 8                  # Ping latency
    
    # Topology
    topology: "single_node"              # single_node, cluster, distributed
    
    # For distributed setups
    nodes: 1                             # Number of nodes
    interconnect: "N/A"                  # Ethernet, InfiniBand, RoCE
    
    # TCP configuration
    tcp_window_kb: 512                   # TCP window size
    tcp_congestion_control: "cubic"      # cubic, bbr, reno
    
    # Offloading
    offload_features:
      - "tcp_segmentation_offload"       # TSO
      - "generic_segmentation_offload"   # GSO
      - "checksum_offload"               # Hardware checksum
```

### 5.2 Network Types

**Ethernet:**
- 1 GbE: 125 MB/s
- 10 GbE: 1.25 GB/s
- 25 GbE: 3.125 GB/s
- 40 GbE: 5 GB/s
- 100 GbE: 12.5 GB/s

**InfiniBand:**
- Very low latency (~1 µs)
- High bandwidth (up to 400 Gbps)
- HPC and storage

**RoCE (RDMA over Converged Ethernet):**
- RDMA benefits
- Ethernet infrastructure
- Lower latency than TCP

### 5.3 MTU (Maximum Transmission Unit)

**Standard MTU:**
```yaml
mtu_bytes: 1500                          # Standard Ethernet
```

**Jumbo Frames:**
```yaml
mtu_bytes: 9000                          # Jumbo frames
```

**Impact:**
- Larger MTU reduces CPU overhead
- Improves throughput for large transfers
- Requires network infrastructure support

### 5.4 TCP Configuration

**Window Size:**
```yaml
tcp_window_kb: 512
```
- Larger window = better throughput
- For high bandwidth-delay product

**Congestion Control:**
```yaml
tcp_congestion_control: "cubic"          # or "bbr"
```
- `cubic`: Default, general purpose
- `bbr`: Better for high-latency networks
- `reno`: Legacy, conservative

---

## 6. System Configuration

### 6.1 Operating System

```yaml
hardware:
  system:
    # OS identification
    operating_system: "Linux"
    distribution: "Generic Linux Distribution"
    distribution_version: "6.5"
    kernel_version: "6.5.0"
    
    # Memory management
    page_size_kb: 4                      # Memory page size
    huge_pages_enabled: true             # Transparent Huge Pages
    huge_page_size_mb: 2                 # THP size
    
    # CPU management
    cpu_governor: "performance"          # performance, powersave, ondemand
    cpu_frequency_scaling: false         # Frequency scaling disabled
    
    # Power management
    power_profile: "maximum_performance"
    turbo_boost_enabled: true
    c_states_disabled: true              # Disable deep C-states
    
    # I/O scheduler
    io_scheduler: "mq-deadline"          # none, mq-deadline, kyber, bfq
    
    # NUMA
    numa_balancing: false                # Disable automatic NUMA balancing
    
    # Swapping
    swappiness: 1                        # Minimize swapping (0-100)
    
    # Security
    selinux: "disabled"                  # or "enforcing", "permissive"
    apparmor: "disabled"
    firewall: "disabled"                 # For benchmark isolation
```

### 6.2 CPU Governor

**Performance:**
```yaml
cpu_governor: "performance"
```
- CPU always at maximum frequency
- Best for benchmarks
- Higher power consumption

**Powersave:**
```yaml
cpu_governor: "powersave"
```
- Minimize power consumption
- Not suitable for benchmarks

**Ondemand:**
```yaml
cpu_governor: "ondemand"
```
- Dynamic frequency scaling
- Introduces variability

### 6.3 Huge Pages

```yaml
huge_pages_enabled: true
huge_page_size_mb: 2
```

**Benefits:**
- Reduced TLB misses
- Lower page table overhead
- Better for large datasets
- 5-10% performance improvement

**Configuration:**
```bash
# Enable Transparent Huge Pages
echo always > /sys/kernel/mm/transparent_hugepage/enabled

# Or for explicit huge pages
echo 8192 > /proc/sys/vm/nr_hugepages  # 16 GB with 2MB pages
```

### 6.4 I/O Scheduler

**mq-deadline:**
```yaml
io_scheduler: "mq-deadline"
```
- Default for SSDs
- Fair latency
- Good all-around

**none (noop):**
```yaml
io_scheduler: "none"
```
- No scheduling
- Best for NVMe
- Lowest latency

**kyber:**
```yaml
io_scheduler: "kyber"
```
- Latency-focused
- Good for fast storage

---

## 7. Hardware Profile Examples

### 7.1 High-End Server Profile

```yaml
hardware:
  processor:
    architecture: "x86_64"
    cores:
      physical: 32
      logical: 64
    frequency:
      base_ghz: 2.5
      turbo_ghz: 4.0
    cache:
      l3_mb: 48
    features: ["AVX512"]
    tdp_watts: 280
    
  memory:
    total_gb: 512
    type: "DDR4"
    speed_mhz: 3200
    channels: 8
    ecc_enabled: true
    numa_nodes: 2
    
  storage:
    primary:
      type: "NVMe SSD"
      capacity_gb: 4000
      interface: "PCIe 4.0 x4"
      performance:
        sequential_read_mbps: 7000
        random_read_iops: 1000000
        
  network:
    interface: "25GbE"
    mtu_bytes: 9000
```

### 7.2 Mid-Range Server Profile

```yaml
hardware:
  processor:
    architecture: "x86_64"
    cores:
      physical: 8
      logical: 16
    frequency:
      base_ghz: 3.6
      turbo_ghz: 5.0
    cache:
      l3_mb: 16
    features: ["AVX2"]
    
  memory:
    total_gb: 64
    type: "DDR4"
    speed_mhz: 3200
    ecc_enabled: true
    
  storage:
    primary:
      type: "NVMe SSD"
      capacity_gb: 1000
      interface: "PCIe 4.0 x4"
        
  network:
    interface: "10GbE"
```

### 7.3 ARM Server Profile

```yaml
hardware:
  processor:
    architecture: "ARM64"
    vendor: "Generic"
    model: "ARM-based High-Performance CPU"
    cores:
      physical: 64
      logical: 64
    frequency:
      base_ghz: 2.5
    cache:
      l3_mb: 32
    features: ["NEON", "AES", "SHA2"]
    tdp_watts: 150
    
  memory:
    total_gb: 128
    type: "DDR4"
    speed_mhz: 3200
    ecc_enabled: true
```

---

## 8. Verification and Validation

### 8.1 Hardware Information Collection

**Linux Commands:**
```bash
# CPU info
lscpu
cat /proc/cpuinfo

# Memory info
free -h
dmidecode --type memory

# Storage info
lsblk
nvme list
hdparm -I /dev/nvme0n1

# Network info
ip link
ethtool eth0
```

**System Tools:**
```bash
# Install tools
apt-get install hwinfo lshw dmidecode

# Comprehensive hardware info
hwinfo --short
lshw -short
```

### 8.2 Performance Validation

**CPU Performance:**
```bash
# Install sysbench
apt-get install sysbench

# CPU benchmark
sysbench cpu --cpu-max-prime=20000 run
```

**Memory Bandwidth:**
```bash
# STREAM benchmark
./stream
```

**Storage Performance:**
```bash
# Install fio
apt-get install fio

# Quick test
fio --name=test --rw=randread --bs=4k --size=1G --numjobs=4 --iodepth=32
```

**Network Throughput:**
```bash
# Install iperf3
apt-get install iperf3

# Server side
iperf3 -s

# Client side
iperf3 -c server_ip
```

### 8.3 Documentation Checklist

- [ ] Complete processor specifications
- [ ] Memory configuration and topology
- [ ] Storage type and performance characteristics
- [ ] Network configuration and measured bandwidth
- [ ] Operating system and kernel version
- [ ] All relevant system settings (CPU governor, huge pages, etc.)
- [ ] Measured performance baselines
- [ ] Power and thermal configuration

---

**Document Version:** 1.0.0  
**Last Updated:** April 2026  
**License:** MIT  
**Maintainer:** CHIMERA Development Team

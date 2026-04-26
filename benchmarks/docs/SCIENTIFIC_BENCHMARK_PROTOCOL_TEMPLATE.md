> ⚠️ **Historischer Plan** – Dieser Plan beschreibt den Entwicklungsstand zum Zeitpunkt der Erstellung.
> Für aktuellen Teststatus: `ctest --preset linux-ninja-release --test-dir build/linux-ninja-release -R <pattern>` verwenden.

# Scientific Benchmark Protocol Template - ThemisDB

**Version:** 1.0  
**Standard:** ISO/IEC 14756:2015 (Database Performance Measurement)  
**Reproducibility:** Full parameter disclosure for peer review

---

## 1. Test Environment Specification

### 1.1 Hardware Configuration

**Processor:**
- Model: {CPU_MODEL} (e.g., Intel Core i7-11700K)
- Architecture: {ARCHITECTURE} (e.g., x86_64)
- Cores: {PHYSICAL_CORES} physical, {LOGICAL_CORES} logical
- Base Frequency: {BASE_FREQ} GHz
- Turbo Frequency: {TURBO_FREQ} GHz
- Cache L1: {L1_SIZE} KB
- Cache L2: {L2_SIZE} KB
- Cache L3: {L3_SIZE} MB
- SIMD Extensions: {EXTENSIONS} (e.g., AVX-512, AVX2)
- Virtualization: {VIRT_TECH} (e.g., VT-x enabled)

**Memory:**
- Total Capacity: {RAM_TOTAL} GB
- Type: {RAM_TYPE} (e.g., DDR4)
- Speed: {RAM_SPEED} MHz
- Channels: {RAM_CHANNELS} (e.g., Dual-channel)
- ECC: {ECC_ENABLED} (Yes/No)
- NUMA Nodes: {NUMA_NODES}

**Storage:**
- Type: {STORAGE_TYPE} (e.g., NVMe SSD)
- Model: {STORAGE_MODEL}
- Capacity: {STORAGE_SIZE} TB
- Interface: {INTERFACE} (e.g., PCIe 4.0 x4)
- Sequential Read: {SEQ_READ} MB/s
- Sequential Write: {SEQ_WRITE} MB/s
- Random Read IOPS: {RAND_READ_IOPS}
- Random Write IOPS: {RAND_WRITE_IOPS}
- Queue Depth: {QUEUE_DEPTH}
- File System: {FILESYSTEM} (e.g., ext4, NTFS)

**Network:**
- Interface: {NIC_MODEL}
- Bandwidth: {BANDWIDTH} Gbps
- MTU: {MTU} bytes
- TCP Window Size: {TCP_WINDOW} KB
- TCP Congestion Control: {TCP_ALGO} (e.g., cubic)

### 1.2 Software Configuration

**Operating System:**
- Distribution: {OS_NAME} (e.g., Windows 11 Pro)
- Version: {OS_VERSION} (e.g., Build 23120)
- Kernel: {KERNEL_VERSION}
- Page Size: {PAGE_SIZE} KB
- Scheduler: {SCHEDULER} (e.g., CFS)
- Power Profile: {POWER_PROFILE} (e.g., High Performance)

**Docker Infrastructure:**
- Docker Engine: {DOCKER_VERSION} (e.g., 29.1.2)
- Docker Compose: {COMPOSE_VERSION} (e.g., v2.40.3)
- Containerd: {CONTAINERD_VERSION}
- Runtime: {RUNTIME} (e.g., runc)
- Storage Driver: {STORAGE_DRIVER} (e.g., overlay2)
- Network Driver: {NETWORK_DRIVER} (e.g., bridge)
- Docker Resources:
  - CPUs: {DOCKER_CPUS}
  - Memory: {DOCKER_MEMORY} GB
  - Swap: {DOCKER_SWAP} GB

**Python Environment:**
- Python Version: {PYTHON_VERSION} (e.g., 3.13.6)
- asyncio: {ASYNCIO_VERSION}
- docker-py: {DOCKER_PY_VERSION}
- aiohttp: {AIOHTTP_VERSION}
- grpcio: {GRPC_VERSION}
- Other dependencies: {DEPENDENCIES}

---

## 2. Database Under Test (DUT) Configuration

### 2.1 ThemisDB Configuration

**Version:** {THEMIS_VERSION} (e.g., v1.0.1, commit hash: {COMMIT_HASH})

**Build Configuration:**
- Compiler: {COMPILER} (e.g., MSVC 19.39, GCC 12.2)
- Optimization Level: {OPT_LEVEL} (e.g., -O3, /O2)
- Build Type: {BUILD_TYPE} (Debug/Release)
- SIMD Support: {SIMD_ENABLED} (AVX-512: Yes/No)
- Assertions: {ASSERTIONS} (Enabled/Disabled)

**Runtime Configuration:**
```yaml
themis_config:
  port: {PORT}
  max_connections: {MAX_CONN}
  worker_threads: {WORKER_THREADS}
  query_cache_size: {CACHE_SIZE} MB
  buffer_pool_size: {BUFFER_POOL} GB
  wal_enabled: {WAL_ENABLED}
  wal_buffer_size: {WAL_BUFFER} MB
  checkpoint_interval: {CHECKPOINT_INTERVAL} seconds
  compression_enabled: {COMPRESSION}
  compression_algorithm: {COMPRESSION_ALGO}
  encryption_enabled: {ENCRYPTION}
  
  # Index Configuration
  index_type: {INDEX_TYPE} (e.g., HNSW, B-Tree)
  index_cache_size: {INDEX_CACHE} MB
  
  # Vector Configuration (if applicable)
  vector_dimensions: {VECTOR_DIM}
  similarity_metric: {SIMILARITY_METRIC} (e.g., L2, cosine)
  ef_construction: {EF_CONSTRUCTION}
  ef_search: {EF_SEARCH}
  m_neighbors: {M_NEIGHBORS}
  
  # Memory Limits
  max_memory: {MAX_MEMORY} GB
  tmp_dir_size: {TMP_SIZE} GB
```

**Container Configuration:**
```yaml
docker_container:
  image: themisdb:{IMAGE_TAG}
  cpus: {CONTAINER_CPUS}
  memory: {CONTAINER_MEMORY} GB
  memory_swap: {CONTAINER_SWAP} GB
  shm_size: {SHM_SIZE} GB
  ulimits:
    nofile: {NOFILE_LIMIT}
    memlock: {MEMLOCK}
  environment:
    - THEMIS_LOG_LEVEL={LOG_LEVEL}
    - THEMIS_METRICS_ENABLED={METRICS}
```

### 2.2 Competitor Database Configurations

**PostgreSQL {PG_VERSION}:**
```yaml
postgresql_config:
  max_connections: {PG_MAX_CONN}
  shared_buffers: {PG_SHARED_BUF} GB
  effective_cache_size: {PG_CACHE} GB
  work_mem: {PG_WORK_MEM} MB
  maintenance_work_mem: {PG_MAINT_MEM} GB
  checkpoint_completion_target: {PG_CHECKPOINT}
  wal_buffers: {PG_WAL_BUF} MB
  random_page_cost: {PG_RAND_COST}
  effective_io_concurrency: {PG_IO_CONCUR}
```

**MongoDB {MONGO_VERSION}:**
```yaml
mongodb_config:
  storage_engine: {MONGO_ENGINE} (e.g., wiredTiger)
  cache_size: {MONGO_CACHE} GB
  journal_enabled: {MONGO_JOURNAL}
  compression: {MONGO_COMPRESSION}
  max_connections: {MONGO_MAX_CONN}
```

**MySQL {MYSQL_VERSION}:**
```yaml
mysql_config:
  innodb_buffer_pool_size: {MYSQL_BUFFER} GB
  innodb_log_file_size: {MYSQL_LOG_SIZE} MB
  innodb_flush_method: {MYSQL_FLUSH}
  max_connections: {MYSQL_MAX_CONN}
```

*(Repeat for all competitor databases)*

---

## 3. Test Data Specification

### 3.1 Relational Workload Data

**Dataset Size:** {RELATIONAL_SIZE} records

**Schema:**
```sql
CREATE TABLE benchmark_table (
    id BIGINT PRIMARY KEY,
    field1 VARCHAR({VARCHAR_SIZE}),
    field2 INTEGER,
    field3 FLOAT,
    field4 TIMESTAMP,
    field5 TEXT ({TEXT_SIZE} bytes avg),
    INDEX idx_field2 (field2),
    INDEX idx_field4 (field4)
);
```

**Data Generation:**
- Distribution: {DISTRIBUTION} (e.g., Uniform, Zipf, Normal)
- Skew Factor: {SKEW} (for Zipf distribution)
- Null Values: {NULL_PERCENT}%
- Unique Values (field2): {UNIQUE_VALUES}
- Average Row Size: {AVG_ROW_SIZE} bytes
- Total Data Size: {TOTAL_SIZE} GB
- Index Size: {INDEX_SIZE} GB

**Random Seed:** {RANDOM_SEED} (for reproducibility)

### 3.2 Vector Workload Data

**Dataset Size:** {VECTOR_COUNT} vectors

**Specifications:**
- Dimensions: {VECTOR_DIM}
- Data Type: {VECTOR_DTYPE} (e.g., float32)
- Distribution: {VECTOR_DIST} (e.g., Random Gaussian)
- Mean: {VECTOR_MEAN}
- Standard Deviation: {VECTOR_STDDEV}
- Normalization: {NORMALIZED} (Yes/No)
- Ground Truth: {GROUND_TRUTH} (Precomputed neighbors)
- Query Set Size: {QUERY_SET_SIZE}

### 3.3 Graph Workload Data

**Dataset Size:** {NODE_COUNT} nodes, {EDGE_COUNT} edges

**Specifications:**
- Graph Type: {GRAPH_TYPE} (e.g., Social Network, Knowledge Graph)
- Degree Distribution: {DEGREE_DIST} (e.g., Power-law, β={BETA})
- Average Degree: {AVG_DEGREE}
- Clustering Coefficient: {CLUSTERING}
- Diameter: {DIAMETER}
- Node Properties: {NODE_PROPS}
- Edge Properties: {EDGE_PROPS}

### 3.4 Geo-Spatial Workload Data

**Dataset Size:** {GEO_POINTS} points

**Specifications:**
- Coordinate System: {COORD_SYS} (e.g., WGS84)
- Bounds: {BOUNDS} (lat/lon ranges)
- Distribution: {GEO_DIST} (e.g., Clustered, Uniform)
- Cluster Centers: {CLUSTER_COUNT}
- Point Types: {POINT_TYPES}
- Polygon Count: {POLYGON_COUNT}

### 3.5 Document Workload Data

**Dataset Size:** {DOC_COUNT} documents

**Specifications:**
- Average Document Size: {AVG_DOC_SIZE} KB
- Document Structure: {DOC_STRUCTURE} (Nested/Flat)
- Max Nesting Depth: {NEST_DEPTH}
- Field Count (avg): {FIELD_COUNT}
- Text Field Size: {TEXT_FIELD_SIZE} bytes
- Array Fields: {ARRAY_FIELDS}

---

## 4. Test Execution Parameters

### 4.1 Test Duration & Phases

**Warm-up Phase:**
- Duration: {WARMUP_DURATION} seconds
- Operations: {WARMUP_OPS} (e.g., Sequential reads to warm cache)
- Purpose: Stabilize cache, eliminate cold-start effects

**Measurement Phase:**
- Duration: {MEASUREMENT_DURATION} seconds
- Iterations: {ITERATIONS}
- Operations per iteration: {OPS_PER_ITER}
- Total operations: {TOTAL_OPS}

**Cool-down Phase:**
- Duration: {COOLDOWN_DURATION} seconds
- Purpose: Allow metrics collection, flush buffers

**Repetitions:**
- Independent runs: {REPETITIONS} (recommended: ≥3)
- Variance threshold: {VARIANCE_THRESHOLD}% (max acceptable CV)

### 4.2 Workload Mix

**Relational Operations:**
```yaml
relational_mix:
  insert: {INSERT_PERCENT}% ({INSERT_COUNT} ops)
  select_pk: {SELECT_PK_PERCENT}% ({SELECT_PK_COUNT} ops)
  select_range: {SELECT_RANGE_PERCENT}% ({SELECT_RANGE_COUNT} ops)
  update: {UPDATE_PERCENT}% ({UPDATE_COUNT} ops)
  delete: {DELETE_PERCENT}% ({DELETE_COUNT} ops)
  join: {JOIN_PERCENT}% ({JOIN_COUNT} ops)
  aggregate: {AGG_PERCENT}% ({AGG_COUNT} ops)
```

**Query Complexity:**
- Simple (PK lookup): {SIMPLE_PERCENT}%
- Medium (Range + Filter): {MEDIUM_PERCENT}%
- Complex (Multi-join + Agg): {COMPLEX_PERCENT}%

### 4.3 Concurrency & Load

**Client Configuration:**
- Concurrent clients: {CONCURRENT_CLIENTS}
- Threads per client: {THREADS_PER_CLIENT}
- Connection pooling: {POOL_ENABLED}
- Pool size: {POOL_SIZE}
- Connection timeout: {CONN_TIMEOUT} ms
- Query timeout: {QUERY_TIMEOUT} ms

**Rate Limiting:**
- Max operations/sec: {MAX_OPS_SEC} (or "unlimited")
- Rate distribution: {RATE_DIST} (e.g., Constant, Poisson)
- Think time: {THINK_TIME} ms (between operations)

### 4.4 Protocol Configuration

**TCP Protocol:**
- Keep-alive: {TCP_KEEPALIVE}
- Nagle's algorithm: {NAGLE} (Enabled/Disabled)
- Socket buffer: {SOCKET_BUFFER} KB

**HTTP Protocol:**
- HTTP Version: {HTTP_VERSION} (e.g., HTTP/2)
- Connection reuse: {CONN_REUSE}
- Compression: {HTTP_COMPRESSION}
- Max payload size: {MAX_PAYLOAD} MB

**gRPC Protocol:**
- Max message size: {GRPC_MAX_MSG} MB
- Keepalive time: {GRPC_KEEPALIVE} seconds
- Compression: {GRPC_COMPRESSION}

---

## 5. Measurement Methodology

### 5.1 Latency Measurement

**Timing Method:** {TIMING_METHOD} (e.g., `time.perf_counter()`, RDTSC)

**Measurement Points:**
1. **Client-side latency:** Time from request send to response received
2. **Server-side latency:** Time spent in database query execution
3. **Network latency:** Difference between (1) and (2)

**Resolution:** {TIMER_RESOLUTION} nanoseconds

**Percentiles Calculated:**
- P50 (Median)
- P90
- P95
- P99
- P99.9
- Min/Max

**Outlier Detection:**
- Method: {OUTLIER_METHOD} (e.g., 3-sigma rule, IQR)
- Outlier threshold: {OUTLIER_THRESHOLD}
- Outlier handling: {OUTLIER_HANDLING} (Remove/Report)

### 5.2 Throughput Measurement

**Calculation:**
```
Throughput = Total_Operations / Measurement_Duration
Unit: operations/second (ops/sec)
```

**Measurement interval:** {THROUGHPUT_INTERVAL} seconds (for time-series)

### 5.3 Resource Monitoring

**Sampling Rate:** {SAMPLING_RATE} Hz (samples per second)

**Metrics Collected:**

**CPU:**
- User time: {CPU_USER}%
- System time: {CPU_SYS}%
- Idle time: {CPU_IDLE}%
- I/O wait: {CPU_IOWAIT}%
- Per-core utilization

**Memory:**
- RSS (Resident Set Size)
- VSZ (Virtual Size)
- Shared memory
- Page faults (major/minor)
- Cache hit rate

**Disk I/O:**
- Read throughput (MB/s)
- Write throughput (MB/s)
- Read IOPS
- Write IOPS
- Queue depth
- Latency (read/write)

**Network I/O:**
- Bytes sent/received
- Packets sent/received
- Errors/Dropped packets
- Connection count

**Monitoring Tools:**
- Primary: {MONITORING_TOOL} (e.g., Prometheus, docker stats)
- Secondary: {SECONDARY_TOOLS}

### 5.4 Statistical Validation

**Hypothesis Testing:**
- Null Hypothesis (H₀): No performance difference between ThemisDB and Competitor
- Alternative Hypothesis (H₁): ThemisDB is faster (one-tailed) or different (two-tailed)
- Significance level (α): {ALPHA} (commonly 0.01 or 0.05)
- Test statistic: {TEST_STAT} (e.g., Two-sample t-test, Mann-Whitney U)

**Effect Size:**
- Metric: Cohen's d
- Calculation: d = (μ₁ - μ₂) / σ_pooled
- Interpretation:
  - |d| < 0.2: Negligible
  - 0.2 ≤ |d| < 0.5: Small
  - 0.5 ≤ |d| < 0.8: Medium
  - |d| ≥ 0.8: Large

**Confidence Intervals:**
- Level: {CONFIDENCE_LEVEL}% (commonly 95% or 99%)
- Method: {CI_METHOD} (e.g., Bootstrap, Normal approximation)

**Sample Size Calculation:**
- Power: {POWER} (commonly 0.80 or 0.90)
- Minimum detectable effect: {MIN_EFFECT}%
- Required samples: {REQUIRED_SAMPLES}

---

## 6. Quality Assurance Procedures

### 6.1 Pre-Test Validation

**Checklist:**
- [ ] All containers started successfully
- [ ] Health checks passing (timeout: {HEALTH_TIMEOUT} seconds)
- [ ] Network connectivity verified
- [ ] Test data loaded (checksum: {DATA_CHECKSUM})
- [ ] Indexes built and verified
- [ ] Resource baseline recorded
- [ ] Monitoring stack operational
- [ ] Disk space sufficient (>{MIN_DISK_SPACE} GB free)

**Automated Checks:**
```bash
# Container health
docker ps --filter "status=running" | grep -E "themis|postgres|mongo"

# Network connectivity
ping -c 5 {CONTAINER_IP}

# Data integrity
SELECT COUNT(*), SUM(CHECKSUM(*)) FROM benchmark_table;

# Resource availability
df -h
free -h
```

### 6.2 During-Test Monitoring

**Real-time Checks:**
- Error rate: Must remain < {MAX_ERROR_RATE}%
- Memory growth: Must be linear or flat (no leaks)
- CPU throttling: Must not occur
- Disk full: Must not occur
- Network saturation: Must not occur

**Anomaly Detection:**
- Method: {ANOMALY_METHOD} (e.g., 3-sigma from rolling mean)
- Window size: {ANOMALY_WINDOW} samples
- Action on anomaly: {ANOMALY_ACTION} (Log/Abort)

### 6.3 Post-Test Verification

**Validation Steps:**
1. Verify data integrity (checksums match pre-test)
2. Check for error logs
3. Verify result file completeness
4. Calculate coefficient of variation (CV):
   - CV = (σ / μ) × 100%
   - Acceptable: CV < {MAX_CV}%
5. Compare resource usage to baseline

**Reproducibility Check:**
- Re-run test with same parameters
- Compare results: Δ < {MAX_DELTA}%

---

## 7. Reporting Standards

### 7.1 Required Metrics

**For each test:**
- Test name
- Workload type
- Database/competitor
- Protocol used
- Latency (P50, P95, P99) in milliseconds
- Throughput in ops/sec
- Success rate (%)
- Error count
- Resource usage (CPU %, Memory MB)
- Timestamp

### 7.2 Result Files

**Formats:**
1. JSON (machine-readable)
2. CSV (for analysis tools)
3. Markdown (human-readable)
4. HTML (with charts)

**File Naming:**
```
benchmark_results_{WORKLOAD}_{DATE}_{TIME}.{EXT}
```

**Metadata in Results:**
```json
{
  "metadata": {
    "benchmark_version": "{VERSION}",
    "timestamp": "{ISO8601_TIMESTAMP}",
    "hostname": "{HOSTNAME}",
    "git_commit": "{COMMIT_HASH}",
    "parameters": { ... },
    "environment": { ... }
  },
  "results": [ ... ]
}
```

### 7.3 Version Control

**All configuration files must be versioned:**
- Docker Compose files
- Database configuration files
- Test scripts
- Data generation scripts

**Git commit hash:** {GIT_COMMIT}  
**Repository:** {GIT_REPO}  
**Branch:** {GIT_BRANCH}

---

## 8. Reproducibility Guarantee

### 8.1 Steps to Reproduce

```bash
# 1. Clone repository
git clone {REPO_URL}
cd {REPO_DIR}
git checkout {COMMIT_HASH}

# 2. Verify environment
python3 --version  # Should be {PYTHON_VERSION}
docker --version   # Should be {DOCKER_VERSION}

# 3. Install dependencies
pip install -r requirements.txt

# 4. Generate test data
python3 scripts/generate_test_data.py --seed {RANDOM_SEED}

# 5. Start containers
docker compose -f {COMPOSE_FILE} up -d

# 6. Run benchmark
python3 {BENCHMARK_SCRIPT} --config {CONFIG_FILE}

# 7. Collect results
ls -lh results/
```

### 8.2 Expected Variance

**Acceptable variance between runs:**
- Latency: ±{LATENCY_VARIANCE}%
- Throughput: ±{THROUGHPUT_VARIANCE}%
- Resource usage: ±{RESOURCE_VARIANCE}%

**Factors affecting variance:**
- Background processes
- CPU thermal throttling
- Network congestion
- Disk cache state

### 8.3 Data & Code Availability

**Test data:** {DATA_URL} (or generation script with seed)  
**Source code:** {CODE_URL}  
**Results archive:** {RESULTS_URL}  
**Docker images:** {DOCKER_REGISTRY}/{IMAGE}:{TAG}

---

## 9. Peer Review Checklist

**For scientific validation, reviewers should verify:**

- [ ] All hardware specifications disclosed
- [ ] All software versions specified
- [ ] Database configurations documented
- [ ] Test data generation reproducible
- [ ] Random seeds provided
- [ ] Measurement methodology described
- [ ] Statistical methods appropriate
- [ ] Sample size justified
- [ ] Outlier handling disclosed
- [ ] Confidence intervals reported
- [ ] Effect sizes calculated
- [ ] All code/scripts available
- [ ] Results reproducible (±{VARIANCE}%)

---

## 10. References

**Standards:**
- ISO/IEC 14756:2015 - Database Performance Measurement
- TPC Benchmark Standards (TPC-C, TPC-H)
- YCSB: Yahoo! Cloud Serving Benchmark

**Statistical Methods:**
- Student's t-test
- Cohen's d effect size
- Bootstrap confidence intervals

**Tools:**
- Docker: https://docs.docker.com/
- Prometheus: https://prometheus.io/
- Python asyncio: https://docs.python.org/3/library/asyncio.html

---

**Document Version:** 1.0  
**Last Updated:** {LAST_UPDATED}  
**Authors:** {AUTHORS}  
**License:** {LICENSE}

---

## Appendix A: Configuration File Templates

*(Include actual configuration files used)*

## Appendix B: Raw Data Samples

*(Include sample raw data for verification)*

## Appendix C: Statistical Analysis Code

*(Include Python/R code for statistical tests)*

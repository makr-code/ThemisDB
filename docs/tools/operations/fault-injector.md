# fault_injector.py - Fault Tolerance Testing Tool

## Overview

`fault_injector.py` simulates various failure scenarios in a sharded ThemisDB cluster to measure resilience, recovery time, and impact on performance. It can inject replica failures, network impairments, and other faults while monitoring throughput and latency.

## Use Cases

- **Resilience Testing:** Verify cluster continues operating under failure conditions
- **Recovery Time Measurement:** Measure how quickly the cluster recovers from failures
- **Chaos Engineering:** Proactively discover weaknesses in distributed system
- **SLA Validation:** Verify uptime and availability guarantees
- **Documentation:** Generate failure scenario documentation for operations teams

## Requirements

- Python 3.8 or later
- SSH access to cluster nodes (for replica kill scenarios)
- Network control capabilities (for network impairment scenarios)
- Optional: `tc` (traffic control) command for network simulation
- Optional: YAML configuration file support (`pip install pyyaml`)

## Fault Scenarios

| Scenario | Description | Impact | Recovery |
|----------|-------------|--------|----------|
| **replica_kill** | Terminate shard replica process | Temporary unavailability | Auto-restart + resync |
| **network_partition** | Simulate network split-brain | Reduced availability | Network healing |
| **network_latency** | Add 100-500ms network delay | Increased latency | Latency removal |
| **network_packet_loss** | Drop 5-20% of packets | Retries + timeouts | Stop packet loss |
| **disk_full** | Simulate storage exhaustion | Write failures | Space cleanup |
| **slow_query** | Inject artificial query delays | Increased latency | Remove delays |

## Installation

```bash
# Clone repository
cd /path/to/ThemisDB

# Install dependencies
pip install pyyaml requests psutil
```

## Basic Usage

### Single Fault Scenario

```bash
python3 tools/fault_injector.py \
  --scenario replica_kill \
  --target shard_2 \
  --duration 60 \
  --output fault_replica_kill.json
```

### Network Latency Injection

```bash
python3 tools/fault_injector.py \
  --scenario network_latency \
  --target all \
  --latency 200ms \
  --duration 120 \
  --output fault_network_latency.json
```

### Multiple Faults

```bash
# Run all fault scenarios
for scenario in replica_kill network_partition network_latency; do
  python3 tools/fault_injector.py \
    --scenario $scenario \
    --duration 90 \
    --output fault_${scenario}.json
  sleep 120  # Wait for recovery between tests
done
```

## Command-Line Options

```
usage: fault_injector.py [-h] --scenario SCENARIO [--target TARGET]
                        [--duration DURATION] [--output OUTPUT]
                        [--config CONFIG] [--baseline-duration BASELINE]

Options:
  --scenario SCENARIO    Fault type: replica_kill, network_partition, etc.
  --target TARGET        Target shard/replica (default: random selection)
  --duration DURATION    Fault duration in seconds (default: 60)
  --output OUTPUT        Output JSON file (default: fault_results.json)
  --config CONFIG        YAML configuration file
  --baseline-duration    Collect baseline metrics for N seconds before fault
  --latency LATENCY      Network latency to inject (e.g., 100ms, 500ms)
  --packet-loss RATE     Packet loss percentage (e.g., 10, 20)
```

## Configuration

Create `fault_injector_config.yaml`:

```yaml
cluster:
  shards:
    - id: shard_0
      host: node1.example.com
      ssh_user: admin
      api_url: http://node1.example.com:8080
    - id: shard_1
      host: node2.example.com
      ssh_user: admin
      api_url: http://node2.example.com:8080
    - id: shard_2
      host: node3.example.com
      ssh_user: admin
      api_url: http://node3.example.com:8080

fault_scenarios:
  replica_kill:
    enabled: true
    process_name: themis_server
    restart_command: "systemctl restart themis-server"
  
  network_latency:
    enabled: true
    latency_ms: 200
    interface: eth0
    tc_command: "/sbin/tc"
  
  network_partition:
    enabled: true
    partition_groups:
      - [shard_0, shard_1]
      - [shard_2]

monitoring:
  metrics_interval: 5  # seconds
  baseline_duration: 30
  recovery_threshold: 0.95  # 95% of baseline throughput
```

Use with:
```bash
python3 tools/fault_injector.py \
  --config fault_injector_config.yaml \
  --scenario replica_kill
```

## Output Format

```json
{
  "metadata": {
    "timestamp": "2026-01-12T11:00:00Z",
    "scenario": "replica_kill",
    "target": "shard_2",
    "duration_sec": 60
  },
  "results": {
    "scenario": "replica_kill",
    "fault_type": "replica_kill",
    "duration_sec": 60,
    "throughput_before_ops_sec": 8000.0,
    "throughput_during_ops_sec": 6200.0,
    "throughput_after_ops_sec": 7800.0,
    "latency_p99_before_ms": 8.5,
    "latency_p99_during_ms": 45.2,
    "latency_p99_after_ms": 12.1,
    "recovery_time_sec": 23.5,
    "data_loss": false,
    "availability": 0.975,
    "error_rate_during": 0.0023
  },
  "timeline": [
    {"time": 0, "phase": "baseline", "throughput": 8000, "latency_p99": 8.5},
    {"time": 30, "phase": "fault_injected", "throughput": 6200, "latency_p99": 45.2},
    {"time": 90, "phase": "recovering", "throughput": 7100, "latency_p99": 28.0},
    {"time": 113, "phase": "recovered", "throughput": 7800, "latency_p99": 12.1}
  ]
}
```

## Advanced Usage

### Cascading Failures

Test multiple simultaneous failures:

```bash
# Kill two replicas simultaneously
python3 tools/fault_injector.py \
  --scenario replica_kill \
  --target shard_1,shard_3 \
  --duration 60 \
  --output fault_cascading.json
```

### Gradual Network Degradation

```bash
# Start with low latency, gradually increase
for latency in 50 100 200 500; do
  python3 tools/fault_injector.py \
    --scenario network_latency \
    --latency ${latency}ms \
    --duration 120 \
    --output fault_latency_${latency}ms.json
done
```

### Scheduled Chaos Testing

```bash
#!/bin/bash
# chaos_schedule.sh - Run daily chaos tests

scenarios=("replica_kill" "network_latency" "network_partition")
day_of_week=$(date +%u)  # 1-7

scenario=${scenarios[$((day_of_week % 3))]}

python3 tools/fault_injector.py \
  --scenario $scenario \
  --duration 300 \
  --output chaos_$(date +%Y%m%d)_${scenario}.json

# Alert on-call if recovery > 60s
```

## Recovery Metrics

### Recovery Time

Time until throughput returns to ≥95% of baseline:

- **Excellent:** < 10 seconds
- **Good:** 10-30 seconds
- **Acceptable:** 30-60 seconds
- **Poor:** > 60 seconds

### Data Loss

Any data loss during fault injection:

- **Target:** Zero data loss for replica_kill, network_partition
- **Acceptable:** Minimal loss during disk_full scenario

### Availability

Percentage of requests successfully handled:

- **High Availability:** ≥ 99.9%
- **Standard:** ≥ 99%
- **Degraded:** < 99%

## Troubleshooting

### SSH Access Denied

**Symptoms:** Cannot execute remote commands

**Solutions:**
- Verify SSH key authentication configured
- Check SSH user has necessary permissions
- Test: `ssh user@host "systemctl status themis-server"`

### TC Command Not Found

**Symptoms:** Network simulation fails

**Solutions:**
```bash
# Install traffic control tools
sudo apt-get install iproute2  # Debian/Ubuntu
sudo yum install iproute        # RHEL/CentOS
```

### Cluster Doesn't Recover

**Symptoms:** Throughput remains low after fault

**Solutions:**
- Manually verify replica status
- Check logs for errors
- Restart affected replicas: `systemctl restart themis-server`
- Verify replication catching up

### Inaccurate Baseline

**Symptoms:** Baseline metrics don't match expected

**Solutions:**
- Increase `--baseline-duration` to 60+ seconds
- Verify no other workloads running
- Check for ongoing compaction/maintenance

## Best Practices

1. **Staging First:** Test fault injection on staging environment before production
2. **Maintenance Window:** Schedule chaos tests during low-traffic periods
3. **Monitoring:** Have real-time monitoring dashboard open during tests
4. **Notification:** Alert team before running chaos tests
5. **Documentation:** Document all fault scenarios and expected outcomes
6. **Gradual Rollout:** Start with mild faults, increase severity gradually
7. **Backup Plan:** Have rollback procedure ready

## Safety Features

The fault injector includes safety mechanisms:

- **Duration Limits:** Maximum fault duration (default: 10 minutes)
- **Auto-Recovery:** Automatically removes faults after duration
- **Dry-Run Mode:** Preview fault without executing (`--dry-run`)
- **Emergency Stop:** Ctrl+C removes fault immediately
- **Health Checks:** Aborts if cluster health below threshold

## Integration

### CI/CD Chaos Testing

```yaml
# .github/workflows/chaos.yml
name: Chaos Testing
on:
  schedule:
    - cron: '0 2 * * *'  # Daily at 2 AM

jobs:
  chaos:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run Chaos Test
        run: |
          python3 tools/fault_injector.py \
            --scenario replica_kill \
            --duration 120 \
            --output chaos_results.json
      - name: Validate Recovery
        run: |
          python3 scripts/validate_chaos.py \
            --results chaos_results.json \
            --max-recovery-time 60
```

### Monitoring Integration

```bash
# Send results to monitoring system
python3 tools/fault_injector.py \
  --scenario replica_kill \
  --output fault_results.json

# Parse and send metrics
jq '.results | {
  recovery_time: .recovery_time_sec,
  availability: .availability,
  data_loss: .data_loss
}' fault_results.json | \
  curl -X POST https://metrics.example.com/chaos \
    -H "Content-Type: application/json" \
    -d @-
```

## See Also

- [shard_bench.py](shard-bench.md) - Performance benchmarking
- [aggregate_shard_results.py](aggregate-shard-results.md) - Results analysis
- [High Availability Guide](../../deployment/high_availability.md)
- [Disaster Recovery](../../operations/disaster_recovery.md)
- [Monitoring Setup](../../operations/monitoring.md)

## License

Part of ThemisDB, licensed under the project's main license.

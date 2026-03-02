# Scheduler Module - Future Enhancements

## Scope

- Periodic and one-shot task execution driven by Cron expressions (6-field format, including year)
- Distributed leader election (gossip-based, with planned Raft-based upgrade for stronger consistency)
- Task DAG dependency resolution: topological sort, parallel fan-out, cascading-failure propagation
- Retention management: data expiry policies and compaction scheduling
- Priority-based task queuing with starvation prevention via aging
- Multi-tenancy: per-tenant quota tracking and task namespace isolation
- Observability hooks: Prometheus metrics, event streaming, and execution timeline

## Design Constraints

- [ ] Cron next-execution calculation must be O(1) per tick; no full-scan of all tasks
- [ ] Leader election must guarantee exactly-one-runner semantics; duplicate execution is a correctness violation
- [ ] DAG cycle detection must run at registration time, not at execution time
- [ ] Scheduler loop must not allocate heap memory on the hot path (task dispatch loop)
- [ ] Distributed state (task registry, leader lease) must tolerate up to N/2 node failures without data loss
- [ ] All public scheduler APIs must be thread-safe; concurrent `schedule`/`cancel` must not corrupt state
- [ ] Resource limits (CPU, memory) enforced via cgroups must not require root privileges

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `ITaskScheduler` | DB retention manager, replication | Core `schedule`, `cancel`, `pause`, `resume` |
| `ICronExpression` | Schedule builder | Parse + validate 6-field cron; return `next_execution` |
| `IDistributedCoordinator` | Multi-node cluster | Leader election, task-state replication |
| `ITaskDAG` | Pipeline executor | Register task graph; compute execution order |
| `IRetryPolicy` | Task executor | Exponential / Fibonacci backoff with jitter |
| `IRetentionManager` | Storage engine | Schedule and execute data expiry/compaction |
| `ITaskObserver` | Metrics / alerting | Receive start/stop/fail events per task |

This file tracks planned enhancements for the Scheduler module implementation.

For detailed feature descriptions and API proposals, see:
[../include/scheduler/FUTURE_ENHANCEMENTS.md](../include/scheduler/FUTURE_ENHANCEMENTS.md)

## High Priority

### ~~Distributed Cron Leader Election (one runner per cluster)~~ ✅ Implemented (v1.x)
**Implemented via:** `DistributedTaskCoordinator` + gossip-based `DistributedCoordinator`

Leader election semantics:
- [x] Leader-based task coordination (only leader schedules tasks)
- [x] Automatic failover when leader fails
- [x] Distributed task registry on all nodes
- [x] One runner per cluster guarantee

### Distributed Task Coordination with Raft
**Target:** v1.7.0 (enhanced Raft-based implementation)

Note: Gossip-based leader election is already implemented (v1.x).
This entry covers the advanced Raft-based alternative for stronger consistency.

Implementation tasks:
- [ ] Raft cluster integration for leader election
- [ ] Task state replication via Raft log
- [ ] Clock synchronization handling

### Priority-Based Scheduling
**Target:** v1.7.0

Implementation tasks:
- [ ] Replace FIFO queue with priority queue
- [ ] Priority-based resource allocation
- [ ] Starvation prevention via aging
- [ ] Dynamic priority adjustment
- [ ] Per-priority slot reservation

### Task Dependencies and DAG Execution
**Target:** v1.7.0

Implementation tasks:
- [x] Task dependency graph data structure
- [x] Topological sort for execution order
- [x] Parallel execution of independent tasks
- [x] Cascading failure handling
- [x] Conditional execution based on results

### Dynamic Resource Allocation
**Target:** v1.8.0

Implementation tasks:
- [ ] cgroups integration (Linux)
- [ ] CPU quota enforcement
- [ ] Memory limit enforcement
- [ ] I/O throttling
- [ ] Dynamic limit adjustment based on usage

### Multi-Tenancy Support
**Target:** v1.7.0

Implementation tasks:
- [ ] Tenant quota tracking
- [ ] Resource isolation per tenant
- [ ] Task namespace separation
- [ ] Billing/accounting integration

## Medium Priority

### Cron Expression Parser
**Target:** v1.5.0 ✅ Completed

Implementation tasks:
- [x] Cron syntax parser (6-field format): wildcards, ranges, lists (with embedded ranges/steps), start/step syntax
- [x] Month name aliases (JAN–DEC, JANUARY–DECEMBER, case-insensitive)
- [x] Weekday name aliases (SUN–SAT, SUNDAY–SATURDAY, case-insensitive)
- [x] Next run time calculation (`getNextExecution`)
- [x] Timezone support (`getNextExecution` with `tz_offset_seconds`)
- [x] Special expressions (@daily, @midnight, @hourly, @weekly, @monthly, @yearly, @annually, @reboot)
- [x] Validation and error messages (`CronExpression::validate`)
- [x] 6-field year constraint (optional year field, range 1970–2199)

### ~~Advanced Retry Policies~~ ✅ Implemented (v0.0.32)
**Target:** v1.6.0

Implementation tasks:
- [x] Exponential backoff implementation
- [x] Jittered backoff for thundering herd
- [x] Fibonacci backoff
- [x] Conditional retry based on error type
- [x] Per-task retry strategy configuration

### Task Templates and Parameterization
**Target:** v1.8.0

Implementation tasks:
- [ ] Template definition schema
- [ ] Parameter validation
- [ ] Template instantiation
- [ ] Template versioning
- [ ] Template library management

### Task Checkpointing and Resume
**Target:** v1.8.0

Implementation tasks:
- [ ] Checkpoint persistence API
- [ ] Automatic resume on failure
- [ ] Progress tracking
- [ ] Incremental processing support

### Observability Enhancements
**Target:** v1.7.0

Implementation tasks:
- [ ] Prometheus metrics export
- [ ] Grafana dashboard
- [ ] Real-time event streaming (WebSocket)
- [ ] Task execution timeline visualization
- [ ] Dependency graph visualization

## Low Priority

### Task Versioning and Rollback
**Target:** v1.9.0

Implementation tasks:
- [ ] Version storage schema
- [ ] Version diff calculation
- [ ] Rollback mechanism
- [ ] Version history retention policy

### ~~Event-Driven Task Execution~~ ✅ CDC Integration Implemented (v0.0.32)

Implementation tasks:
- [ ] Event source abstraction
- [x] CDC integration (changefeed → task execution via `EventTrigger` / `EventTriggerManager`)
- [ ] Webhook receiver
- [ ] Message queue integration
- [ ] Event filter DSL

## Research Projects

### Machine Learning for Task Optimization
**Timeline:** 2025+

Research tasks:
- [ ] Task execution time prediction (LSTM/Prophet)
- [ ] Failure prediction model
- [ ] Auto-tuning resource limits
- [ ] Anomaly detection in task behavior

### Serverless Task Execution
**Timeline:** 2025+

Research tasks:
- [ ] AWS Lambda integration
- [ ] Azure Functions integration
- [ ] Google Cloud Functions integration
- [ ] Cost optimization analysis

## Implementation Guidelines

### Adding New Features

1. **Design Phase:**
   - Create design document in discussions
   - API proposal with examples
   - Performance impact analysis
   - Security considerations

2. **Implementation Phase:**
   - Feature branch from main
   - Unit tests (>80% coverage)
   - Integration tests
   - Documentation updates

3. **Review Phase:**
   - Code review
   - Performance benchmarks
   - Security review
   - Documentation review

4. **Deployment Phase:**
   - Feature flag for gradual rollout
   - Monitoring and alerts
   - Rollback plan

### Testing Requirements

All new features must include:
- [ ] Unit tests with >80% coverage
- [ ] Integration tests
- [ ] Performance benchmarks
- [ ] Security tests (if applicable)
- [ ] Documentation with examples

### Performance Benchmarks

Track these metrics for new features:
- Scheduler loop overhead (CPU %)
- Task startup latency (ms)
- Memory usage per task (KB)
- Throughput (tasks/sec)
- 99th percentile latency

### Security Checklist

For features with security impact:
- [ ] Threat model documented
- [ ] Input validation
- [ ] Authentication/authorization
- [ ] Audit logging
- [ ] Resource limits
- [ ] Injection attack prevention

## Community Contributions

We welcome contributions! Popular community requests:

1. **Calendar-aware scheduling** - Skip holidays/weekends
2. **Task chaining** - Simple pipelines without DAG
3. **Manual approval gates** - Pause for human decision
4. **Task simulation** - Dry-run mode
5. **Task import/export** - Backup/restore definitions

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines.

## Tracking

- Feature requests: GitHub Issues with label `enhancement`
- Design discussions: GitHub Discussions
- Progress tracking: GitHub Projects
- Release planning: Milestones

## Test Strategy

- Unit tests: ≥ 80% line coverage on `CronExpression`, `TaskDAG`, `RetryPolicy`, and `DistributedCoordinator`
- Cron parser: exhaustive tests covering all special expressions, edge dates (DST transitions, leap years, year 2199)
- DAG execution: property-based tests for topological correctness on random acyclic graphs (≥ 10,000 graphs)
- Leader election: partition-simulation tests; verify exactly-one-leader invariant under network splits
- Retention manager: inject mock clock to verify expiry fires within ≤ 1 scheduler tick of due time
- Priority scheduler: starvation tests running ≥ 10,000 ticks; verify low-priority tasks always eventually execute
- Performance regression gate: CI blocks merge if scheduler-loop overhead exceeds 1% CPU or p99 dispatch latency regresses > 20%
- Chaos tests: random task-executor crashes; verify correct cascading-failure propagation in DAGs

## Performance Targets

- Scheduler loop tick latency: p99 ≤ 1 ms for up to 10,000 registered tasks
- Task dispatch (from due-time to first instruction): p99 ≤ 5 ms
- Cron `next_execution` calculation: ≤ 10 µs per call
- Leader election convergence after node failure: ≤ 5 s in a 5-node cluster
- DAG topological sort: ≤ 1 ms for graphs with ≤ 10,000 nodes
- Throughput: ≥ 5,000 task dispatches/second on a single scheduler node
- Memory overhead: ≤ 1 KB per registered task (excluding task payload)

## References

- [Task Scheduler Implementation](./task_scheduler.cpp)
- [Hybrid Retention Manager](./hybrid_retention_manager.cpp)
- [Public API Documentation](../include/scheduler/README.md)
- [Detailed Feature Descriptions](../include/scheduler/FUTURE_ENHANCEMENTS.md)

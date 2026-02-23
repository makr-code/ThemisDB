# Scheduler Module - Future Enhancements

This file tracks planned enhancements for the Scheduler module implementation.

For detailed feature descriptions and API proposals, see:
[../include/scheduler/FUTURE_ENHANCEMENTS.md](../include/scheduler/FUTURE_ENHANCEMENTS.md)

## High Priority

### Distributed Task Coordination with Raft
**Target:** v1.7.0

Implementation tasks:
- [ ] Raft cluster integration for leader election
- [ ] Task state replication via Raft log
- [ ] Leader-based task scheduling
- [ ] Automatic failover on leader failure
- [ ] Distributed task execution across nodes
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

### Advanced Retry Policies
**Target:** v1.6.0

Implementation tasks:
- [ ] Exponential backoff implementation
- [ ] Jittered backoff for thundering herd
- [ ] Fibonacci backoff
- [ ] Conditional retry based on error type
- [ ] Per-task retry strategy configuration

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

### Event-Driven Task Execution
**Target:** v1.9.0

Implementation tasks:
- [ ] Event source abstraction
- [ ] CDC integration
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

## References

- [Task Scheduler Implementation](./task_scheduler.cpp)
- [Hybrid Retention Manager](./hybrid_retention_manager.cpp)
- [Public API Documentation](../include/scheduler/README.md)
- [Detailed Feature Descriptions](../include/scheduler/FUTURE_ENHANCEMENTS.md)

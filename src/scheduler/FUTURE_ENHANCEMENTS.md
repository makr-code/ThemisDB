<!-- Status: current | validated: 2026-04-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Scheduler Module - Future Enhancements

- Periodic and one-shot task execution driven by Cron expressions (6-field format, including year)
- Distributed leader election (gossip-based, with planned Raft-based upgrade for stronger consistency)
- Task DAG dependency resolution: topological sort, parallel fan-out, cascading-failure propagation
- Retention management: data expiry policies and compaction scheduling
- Priority-based task queuing with starvation prevention via aging
- SLO-based adaptive retry policy ✅ implemented v1.9.0 (`ScheduledTask::SloRetryConfig`)
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

### `TaskScheduler`: Propagate Authenticated User Context to Audit Events
**Priority:** High
**Target Version:** v1.8.0
**Status:** ✅ Implemented

`task_scheduler.cpp` had several TODOs for user context propagation — audit events hardcoded `"system"` as the actor instead of the actual requesting user.

**Implementation Notes:**
- `[x]` Added `TaskScheduler::RequestContext` struct (`user_id`, `client_ip`) to `include/scheduler/task_scheduler.h`.
- `[x]` Implemented `static setRequestContext(ctx)`, `clearRequestContext()`, `currentUserId(fallback)`, `currentClientIp()` using `thread_local TLSRequestContext` in `src/scheduler/task_scheduler.cpp`.
- `[x]` `setDefaultAuditContext()` helper updated to read from `currentUserId()` / `currentClientIp()` instead of hardcoded `"system"` / `"localhost"`.
- `[x]` All remaining `"system"` hardcodings in `logTaskSchedulerEvent()` calls replaced with `TaskScheduler::currentUserId()`.
- `[x]` `clearRequestContext()` design: HTTP handlers call `setRequestContext({user_id, client_ip})` before scheduler ops and `clearRequestContext()` after; scheduler thread falls back to `"system"`.
- `[x]` `sandbox_execution` config flag implemented: when `true`, wraps user-provided task functions in `modules::ModuleSandbox` (cgroups v2 memory/CPU limits, seccomp-bpf syscall filtering on Linux; graceful fallback when sandbox launch fails).

---



### ~~Distributed Cron Leader Election (one runner per cluster)~~ ✅ Implemented (v1.x)
**Implemented via:** `DistributedTaskCoordinator` + gossip-based `DistributedCoordinator`

Leader election semantics:
- [x] Leader-based task coordination (only leader schedules tasks)
- [x] Automatic failover when leader fails
- [x] Distributed task registry on all nodes
- [x] One runner per cluster guarantee

### Distributed Task Coordination with Raft
**Target:** v1.7.0 (enhanced Raft-based implementation)
**Research Basis:** *(see [1][2][3])*

Note: Gossip-based leader election is already implemented (v1.x).
This entry covers the advanced Raft-based alternative for stronger consistency.

Implementation tasks:
- [ ] Raft cluster integration for leader election
- [ ] Task state replication via Raft log
- [ ] Clock synchronization handling

### Priority-Based Scheduling
**Target:** v1.7.0
**Research Basis:** *(see [4][5])*

Implementation tasks:
- [x] Priority-ordered dispatch – `schedulerLoop()` sorts `tasks_to_execute` by `ScheduledTask::priority` (HIGH → NORMAL → LOW) before launching threads (`std::sort` descending, added v1.7.0)
- [ ] Full lock-free priority queue to replace the pre-sort approach
- [ ] Priority-based resource allocation (CPU/memory slot reservations per priority tier)
- [ ] Starvation prevention via aging (dynamically boost low-priority task priority after N idle ticks)
- [ ] Dynamic priority adjustment at runtime
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
**Research Basis:** *(see [6][7][8])*

Implementation tasks:
- [ ] cgroups integration (Linux)
- [ ] CPU quota enforcement
- [ ] Memory limit enforcement
- [ ] I/O throttling
- [ ] Dynamic limit adjustment based on usage

### Multi-Tenancy Support
**Target:** v1.7.0
**Research Basis:** *(see [9][10])*

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
**Research Basis:** *(see [11][12])*

Implementation tasks:
- [ ] Checkpoint persistence API
- [ ] Automatic resume on failure
- [ ] Progress tracking
- [ ] Incremental processing support

### Observability Enhancements
**Target:** v1.7.0
**Research Basis:** *(see [13][14])*

Implementation tasks:
- [x] Prometheus metrics export (`TaskScheduler::exportMetrics()` — emits `themis_scheduler_*` gauges/counters in Prometheus text format, including `themis_scheduler_concurrency_limit` and `themis_scheduler_queue_depth`)
- [x] Grafana dashboard (`config/grafana/dashboards/themisdb-scheduler-dashboard.json` — covers all `themis_scheduler_*` metrics: stat overview, concurrency/queue-depth timeseries, success/failure rate, per-task duration, last-run timestamps, enabled-status table, `$task_name` filter variable)
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
**Research Basis:** *(see [15][16][17])*

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

### Internal Implementation References

- [Task Scheduler Implementation](./task_scheduler.cpp)
- [Hybrid Retention Manager](./hybrid_retention_manager.cpp)
- [Public API Documentation](../include/scheduler/README.md)
- [Detailed Feature Descriptions](../include/scheduler/FUTURE_ENHANCEMENTS.md)

---

## Scientific References (IEEE Format)

The following papers underpin the algorithms and design decisions for the planned features in this module. References are grouped by the feature they support.

### Distributed Consensus and Raft-Based Coordination

[1] D. Ongaro and J. Ousterhout, "In Search of an Understandable Consensus Algorithm," in *Proc. 2014 USENIX Annual Technical Conf. (USENIX ATC '14)*, Philadelphia, PA, USA, 2014, pp. 305–319. [Online]. Available: https://www.usenix.org/system/files/conference/atc14/atc14-paper-ongaro.pdf *(Raft consensus algorithm; direct foundation for the planned `DistributedTaskCoordinator` Raft-based leader election and log replication, see [1])*

[2] L. Lamport, "Paxos Made Simple," *ACM SIGACT News (Distributed Computing Column)*, vol. 32, no. 4, pp. 51–58, Dec. 2001. [Online]. Available: https://lamport.azurewebsites.net/pubs/paxos-simple.pdf *(Foundational distributed consensus protocol; Raft is derived from Paxos; provides correctness guarantees for exactly-once task scheduling across replicas)*

[3] A. Boutin et al., "Apollo: Scalable and Coordinated Scheduling for Cloud-Scale Computing," in *Proc. 11th USENIX Symp. on Operating Systems Design and Implementation (OSDI '14)*, Broomfield, CO, USA, 2014, pp. 285–300. [Online]. Available: https://www.usenix.org/system/files/conference/osdi14/osdi14-paper-boutin_0.pdf *(Distributed task scheduling at scale with coordination; models task state replication and leader coordination patterns applicable to `DistributedTaskCoordinator`)*

### Priority-Based Scheduling and Starvation Prevention

[4] R. D. Blumofe and C. E. Leiserson, "Scheduling Multithreaded Computations by Work Stealing," *J. ACM*, vol. 46, no. 5, pp. 720–748, Sep. 1999, doi: 10.1145/324133.324234. *(Work-stealing thread pool scheduler; proves optimal time and space bounds for fully strict computations; the `TaskScheduler` thread pool follows this pattern, underpins priority-ordered dispatch)*

[5] S. Baruah, G. Koren, D. Mao, B. Mishra, A. Raghunathan, L. Rosier, D. Shasha, and F. Wang, "On the Competitiveness of On-Line Real-Time Task Scheduling," *Real-Time Syst.*, vol. 4, no. 2, pp. 125–144, 1992, doi: 10.1007/BF00365326. *(Formal analysis of preemptive priority scheduling and starvation avoidance; provides theoretical foundation for the planned priority-aging mechanism that prevents indefinite postponement of `LOW`-priority tasks)*

### Dynamic Resource Allocation and cgroups

[6] A. Verma, L. Pedrosa, M. Korupolu, D. Oppenheimer, E. Tune, and J. Wilkes, "Large-Scale Cluster Management at Google with Borg," in *Proc. 10th European Conf. on Computer Systems (EuroSys '15)*, Bordeaux, France, 2015, pp. 1–17, doi: 10.1145/2741948.2741964. *(Resource isolation via cgroups in a large-scale cluster scheduler; directly applicable to the planned `cgroups integration (Linux)` for CPU and memory quota enforcement per task)*

[7] B. Burns, B. Grant, D. Oppenheimer, E. Brewer, and J. Wilkes, "Borg, Omega, and Kubernetes," *ACM Queue*, vol. 14, no. 1, pp. 70–93, Jan. 2016, doi: 10.1145/2898442.2898444. *(Evolution of resource-isolation and scheduling techniques from Borg to Kubernetes; informs cgroups-v2 integration strategy and dynamic limit adjustment design)*

[8] B. Hindman et al., "Mesos: A Platform for Fine-Grained Resource Sharing in the Data Center," in *Proc. 8th USENIX Symp. on Networked Systems Design and Implementation (NSDI '11)*, Boston, MA, USA, 2011, pp. 295–308. [Online]. Available: https://www.usenix.org/legacy/events/nsdi11/tech/full_papers/Hindman_new.pdf *(Fine-grained per-task resource offers and enforcement model; basis for `TaskResourceLimits` CPU/memory/I/O throttling design)*

### Multi-Tenancy and Resource Isolation

[9] P. Barham, B. Dragovic, K. Fraser, S. Hand, T. Harris, A. Ho, R. Neugebauer, I. Pratt, and A. Warfield, "Xen and the Art of Virtualization," in *Proc. 19th ACM Symp. on Operating Systems Principles (SOSP '03)*, Bolton Landing, NY, USA, 2003, pp. 164–177, doi: 10.1145/945445.945462. *(Foundational resource partitioning and isolation; scheduling isolation principles applicable to per-tenant task namespace separation and quota enforcement)*

[10] G. Tesauro, N. K. Jong, R. Das, and M. N. Bennani, "A Hybrid Reinforcement Learning Approach to Autonomic Resource Allocation," in *Proc. 3rd Int. Conf. on Autonomic Computing (ICAC '06)*, Dublin, Ireland, 2006, pp. 65–73, doi: 10.1109/ICAC.2006.1662383. *(Autonomic per-tenant resource allocation and billing integration; models quota tracking and dynamic adjustment relevant to multi-tenancy support)*

### Task Checkpointing and Fault-Tolerant Resumption

[11] I. P. Egwutuoha, D. Levy, B. Selic, and S. Chen, "A Survey of Fault Tolerance Mechanisms and Checkpoint/Restart Implementations for High Performance Computing Systems," *J. Supercomput.*, vol. 65, no. 3, pp. 1302–1326, Sep. 2013, doi: 10.1007/s11227-013-0884-0. *(Comprehensive survey of checkpoint strategies; directly informs `ITaskCheckpoint` API design for persistence and automatic resume on failure)*

[12] J. S. Plank, M. Beck, G. Kingsley, and K. Li, "Libckpt: Transparent Checkpointing Under Unix," in *Proc. 1995 USENIX Winter Technical Conf.*, New Orleans, LA, USA, 1995, pp. 213–223. [Online]. Available: https://www.usenix.org/legacy/publications/library/proceedings/wtec95/full_papers/plank.ps *(Transparent process-level checkpointing; foundational techniques for checkpoint persistence API and progress tracking design)*

### Real-Time Event Streaming and Observability

[13] J. Kreps, N. Narkhede, and J. Rao, "Kafka: A Distributed Messaging System for Log Processing," in *Proc. 6th Int. Workshop on Networking Meets Databases (NetDB '11)*, Athens, Greece, 2011, pp. 1–7. [Online]. Available: https://notes.stephenholiday.com/Kafka.pdf *(Persistent distributed event log; relevant to task event streaming design for WebSocket and event-pipeline backends)*

[14] M. Zaharia, T. Das, H. Li, T. Hunter, S. Shenker, and I. Stoica, "Discretized Streams: Fault-Tolerant Streaming Computation at Scale," in *Proc. 24th ACM Symp. on Operating Systems Principles (SOSP '13)*, Farminton, PA, USA, 2013, pp. 423–438, doi: 10.1145/2517349.2522737. *(Fault-tolerant micro-batch streaming; informs real-time task execution timeline streaming and event replay guarantees for the planned WebSocket endpoint)*

### Machine Learning for Task Optimization

[15] S. Hochreiter and J. Schmidhuber, "Long Short-Term Memory," *Neural Comput.*, vol. 9, no. 8, pp. 1735–1780, Nov. 1997, doi: 10.1162/neco.1997.9.8.1735. *(LSTM sequence model; basis for planned task execution time prediction model using historical execution durations)*

[16] S. J. Taylor and B. Letham, "Forecasting at Scale," *Am. Stat.*, vol. 72, no. 1, pp. 37–45, Jan. 2018, doi: 10.1080/00031305.2017.1380080. *(Prophet forecasting model for seasonality-aware time series; applicable to `Task execution time prediction (LSTM/Prophet)` for adaptive concurrency limit scaling)*

[17] Y. Diao, J. L. Hellerstein, S. Parekh, R. Griffith, G. Kaiser, and D. Phung, "Self-Managing Systems: A Control Theory Foundation," in *Proc. 12th IEEE Int. Conf. and Workshop on the Engineering of Computer Based Systems (ECBS '05)*, Greenbelt, MD, USA, 2005, pp. 441–448, doi: 10.1109/ECBS.2005.68. *(Control-theoretic approach to autonomous resource tuning and anomaly detection; basis for `auto-tuning resource limits` and `anomaly detection in task behavior` research items)*

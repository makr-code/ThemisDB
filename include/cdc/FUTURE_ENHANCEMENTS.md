# CDC Module - Future Header Enhancements
<!-- Status: current | validated: 2026-03-09 -->
<!-- Links: ../../src/cdc/README.md · ../../src/cdc/ARCHITECTURE.md · ../../src/cdc/FUTURE_ENHANCEMENTS.md · docs/de/cdc/ -->

## Scope

- `ICDCListener` interface extensions for delivery guarantees and event filtering
- Delivery guarantee interface (`IDeliveryGuaranteeConfig`) for at-least-once and exactly-once semantics
- Schema-aware event type API (`ICDCEventSchema`) supporting Avro and Protobuf encoding descriptors
- Materialized view hook (`ICDCMaterializedViewHook`) for CDC-driven view maintenance integration
- Pause/resume control API (`ICDCPauseControl`) for atomic stream suspension and resumption
- Backpressure signal interface (`ICDCBackpressureSignal`) for consumer-side flow control

## Design Constraints

- `[ ]` CDC event delivery is strictly ordered per collection; `ICDCListener` callbacks for the same collection are never invoked concurrently
- `[ ]` Pause/resume via `ICDCPauseControl` is atomic; a paused stream buffers events internally without losing them
- `[ ]` Schema-aware events are validated against the registered schema before delivery; invalid events invoke the error callback, not the main listener
- `[ ]` Materialized view hooks registered via `ICDCMaterializedViewHook` must be `noexcept`; exceptions terminate the process
- `[ ]` Backpressure signals are advisory; the CDC layer may still deliver events when a signal is active, but reduces throughput
- `[ ]` All `ICDCListener` method signatures use `std::span` for event batches to avoid unnecessary copies in the public API

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `ICDCListener` | `ReplicationLayer`, `MaterializedViewEngine`, `OutboxProcessor` | Base listener interface; `onEvents(std::span<CDCEvent>)` is the primary delivery method |
| `ICDCEventSchema` | `SchemaRegistry`, `ICDCListener` implementations | Describes the Avro/Protobuf schema for a collection's change events; immutable after registration |
| `ICDCPauseControl` | `AdminAPI`, `BackpressureManager` | Provides `pause()` and `resume()` for atomic stream suspension; `isPaused()` is thread-safe |
| `ICDCMaterializedViewHook` | `MaterializedViewEngine`, `QueryPlanner` | Receives pre-validated `CDCEvent` batches; all methods are `noexcept` |
| `IDeliveryGuaranteeConfig` | `ICDCListener` factory, `OutboxProcessor` | Configures at-least-once vs. exactly-once delivery semantics per listener registration |
| `ICDCBackpressureSignal` | `ICDCListener` implementations, `BackpressureManager` | Advisory signal; consumer calls `signalBackpressure()` and `clearBackpressure()` |

## Planned Features

### CDC Pause/Resume Control API

- `[ ]` Define `ICDCPauseControl::pause() -> Result<void>` and `resume() -> Result<void>` in public header
- `[ ]` Add `isPaused() -> bool` as a thread-safe query method; must not block
- `[ ]` Expose `PauseReason` as a strongly-typed enum class: `AdminRequest`, `Backpressure`, `SchemaEvolution`
- `[ ]` Document that events arriving during pause are buffered up to a configured `maxBufferBytes` limit; overflow triggers `Result::error(BufferFull)`

### Backpressure Signaling Interface

- `[ ]` Define `ICDCBackpressureSignal` with `signalBackpressure(BackpressureLevel)` and `clearBackpressure()`
- `[ ]` Expose `BackpressureLevel` as a strongly-typed enum class: `Low`, `Medium`, `High`, `Critical`
- `[ ]` Add `currentLevel() -> BackpressureLevel` as a non-blocking query method
- `[ ]` Document that `Critical` level triggers automatic `ICDCPauseControl::pause()` if configured to do so

### Multi-Source Fan-In API

- `[ ]` Define `ICDCFanIn` with `addSource(CollectionId, ICDCListener&)` and `removeSource(CollectionId)`
- `[ ]` Expose `FanInEvent` as a tagged-event value type that carries the originating `CollectionId`
- `[ ]` Add `ICDCFanIn::setMergePolicy(IFanInMergePolicy&)` to configure conflict resolution across sources
- `[ ]` Document ordering guarantee: events from the same source collection are delivered in order; cross-collection ordering is best-effort

### Schema Evolution Hook

- `[ ]` Define `ICDCEventSchema::onSchemaEvolution(SchemaEvolutionDescriptor, ISchemaEvolutionCallback&)`
- `[ ]` Expose `SchemaEvolutionDescriptor` with old/new schema versions, migration strategy enum, and affected field list
- `[ ]` Add `ISchemaEvolutionCallback::onCompatible()` and `onIncompatible(SchemaConflict)` as pure-virtual methods
- `[ ]` Document that incompatible schema evolution triggers an automatic `ICDCPauseControl::pause()` until resolved

### Delivery Guarantee Configuration

- `[ ]` Define `IDeliveryGuaranteeConfig` with `setMode(DeliveryMode)` and `setAckTimeout(std::chrono::milliseconds)`
- `[ ]` Expose `DeliveryMode` as a strongly-typed enum class: `AtLeastOnce`, `ExactlyOnce`
- `[ ]` Add `setDeduplicationWindow(std::chrono::milliseconds)` for exactly-once dedup window configuration
- `[ ]` Document that `ExactlyOnce` mode requires the listener to implement `IIdempotentCDCListener`; missing impl is a compile-time error

## Test Strategy

- Ordering tests deliver 10k events to `ICDCListener` from a single collection across multiple threads and assert arrival order matches insertion order
- Pause/resume tests assert that zero events are lost during a pause/resume cycle with concurrent producers
- Schema validation tests inject malformed events and verify they are routed to the error callback, not the main `onEvents` path
- Materialized view hook tests use `noexcept` static analysis to verify all hook implementations satisfy the `noexcept` contract
- Backpressure tests ramp producer rate until `Critical` level is reached and assert that `pause()` is invoked automatically when configured
- Fan-in ordering tests verify that events from the same source arrive in order when merged from 4 concurrent source collections

## Performance Targets

- `ICDCListener::onEvents` batch delivery overhead ≤ 500 µs for a batch of 1000 events at the public interface boundary
- `ICDCPauseControl::pause()` and `resume()` round-trip latency ≤ 10 ms including buffer flush acknowledgement
- `ICDCEventSchema` schema validation per event ≤ 100 µs; batch validation of 1000 events ≤ 50 ms
- `ICDCBackpressureSignal::signalBackpressure()` call overhead ≤ 1 µs (non-blocking signal update)
- `ICDCFanIn` fan-in merge overhead per event ≤ 200 µs including `CollectionId` tagging
- `IDeliveryGuaranteeConfig` deduplication check per event in `ExactlyOnce` mode ≤ 10 µs using a rolling hash window

## Security / Reliability

- CDC events contain only changed fields; unchanged fields are not included in the `CDCEvent` payload delivered via `ICDCListener`
- Event stream access is controlled by collection-level ACL; `ICDCListener` registration is rejected if the caller lacks read permission on the collection
- Sensitive field redaction hook (`ISensitiveFieldRedactor`) is invocable before delivery; registered at listener factory time
- Schema validation prevents malformed events from reaching `ICDCMaterializedViewHook`; invalid events never propagate past the validation boundary
- `ICDCPauseControl::pause()` is privilege-gated; only callers with `CDC_PAUSE` capability can invoke it
- Audit log entries are written for every `pause()`, `resume()`, and schema-evolution event to support forensic analysis

---

## Scientific References

References for the planned header interfaces. IEEE/ACM format, numbered.

[1] T. Akidau, R. Bradshaw, C. Chambers, S. Chernyak, R. J. Fernández-Moctezuma, R. Lax, S. McVeety, D. Mills, F. Perry, E. Schmidt, and S. Whittle, "The Dataflow Model: A Practical Approach to Balancing Correctness, Latency, and Cost in Massive-Scale, Unbounded, Out-of-Order Data Processing," *Proc. VLDB Endowment*, vol. 8, no. 12, pp. 1792–1803, 2015. https://doi.org/10.14778/2824032.2824076
*(Exactly-once delivery semantics and delivery mode configuration; basis for `IDeliveryGuaranteeConfig::DeliveryMode::ExactlyOnce` and `IIdempotentCDCListener`.)*

[2] M. Zaharia, T. Das, H. Li, T. Hunter, S. Shenker, and I. Stoica, "Discretized Streams: Fault-Tolerant Streaming Computation at Scale," in *Proc. 24th ACM Symp. Operating Systems Principles (SOSP)*, Farmington, PA, 2013, pp. 423–438. https://doi.org/10.1145/2517349.2522737
*(Backpressure levels and pause/resume mechanics; informs `ICDCBackpressureSignal` (`BackpressureLevel`) and `ICDCPauseControl` designs.)*

[3] J. Li, K. Tufte, V. Shkapenyuk, V. Papadimos, T. Johnson, and D. Maier, "Out-of-Order Processing: A New Architecture for High-Performance Stream Systems," *Proc. VLDB Endowment*, vol. 1, no. 1, pp. 274–288, 2008. https://doi.org/10.14778/1453856.1453886
*(Multi-source event ordering and merge policies; directly applicable to `ICDCFanIn` and `IFanInMergePolicy` design.)*

[4] Apache Software Foundation, "Apache Avro Specification, v1.11," 2023. [Online]. Available: https://avro.apache.org/docs/current/spec.html
*(Schema compatibility rules (FORWARD, BACKWARD, FULL); basis for `ICDCEventSchema::onSchemaEvolution` and `SchemaEvolutionDescriptor` design.)*

[5] J. M. Hellerstein, M. Stonebraker, and J. Hamilton, "Architecture of a Database System," *Foundations and Trends in Databases*, vol. 1, no. 2, pp. 141–259, 2007. https://doi.org/10.1561/1900000002
*(Foundational streaming listener and subscription model; background for `ICDCListener::onEvents(std::span<CDCEvent>)` batch delivery design.)*

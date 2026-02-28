# Change Data Capture (CDC) Module

Change Data Capture and changefeed implementation for ThemisDB.

## Module Purpose

Implements Change Data Capture for ThemisDB, providing real-time change notifications via SSE streaming, filtered subscriptions, change log management, historical change replay, and CDC-driven incremental materialized view maintenance.

## Subsystem Scope

**In scope:** Changefeed engine, SSE event streaming, per-collection/per-key filtering, change log persistence, historical replay, subscription lifecycle management, cross-collection aggregated streams, CDC-based materialized view maintenance, Kafka producer transport (`KafkaCDCProducer`), `ICDCTransport` abstract interface.

**Out of scope:** WebSocket HTTP server endpoint wiring (transport class exists, wiring is a follow-up).

## Relevant Interfaces

- `changefeed.cpp` — core change capture engine
- `sse_streamer.cpp` — Server-Sent Events transport
- `change_log.cpp` — persistent change log
- `subscription_manager.cpp` — subscription lifecycle
- `cross_collection_stream.cpp` — cross-collection change aggregation (`CrossCollectionStream`)
- `cdc_materialized_view.cpp` — CDC-driven incremental materialized view maintenance (`CDCMaterializedViewMaintainer`)

## Current Delivery Status

**Maturity:** 🟢 Production — SSE-based changefeeds, filtered subscriptions, WebSocket transport, consumer groups, and Kafka producer integration operational.

## Components

- Changefeed implementation
- Server-Sent Events (SSE) streaming
- Change log management
- Subscription management
- Cross-collection change aggregation (`CrossCollectionStream`)
- CDC-based incremental materialized view maintenance (`CDCMaterializedViewMaintainer`)

## Features

- Real-time change notifications
- SSE-based event streaming
- Filtered change subscriptions
- Historical change replay
- Cross-collection merged event streams with per-collection resume cursors
- CDC-driven incremental materialized view maintenance (GROUP BY aggregations updated in O(1) per change)

## Documentation

For CDC documentation, see:
- [Changefeed](../../docs/src/cdc/changefeed.cpp.md)
- [Change Data Capture](../../docs/change_data_capture.md)
- [CDC Documentation](../../docs/cdc.md)
- [Changefeed Development](../../docs/development/changefeed/)

## Scientific References

1. Stonebraker, M., Rowe, L. A., & Hirohama, M. (1990). **The Implementation of Postgres**. *IEEE Transactions on Knowledge and Data Engineering*, 2(1), 125–142. https://doi.org/10.1109/69.43410

2. Kleppmann, M. (2017). **Designing Data-Intensive Applications: The Big Ideas Behind Reliable, Scalable, and Maintainable Systems**. O'Reilly Media. ISBN: 978-1-449-37332-0

3. Mohan, C., Haderle, D., Lindsay, B., Pirahesh, H., & Schwarz, P. (1992). **ARIES: A Transaction Recovery Method Supporting Fine-Granularity Locking and Partial Rollbacks Using Write-Ahead Logging**. *ACM Transactions on Database Systems*, 17(1), 94–162. https://doi.org/10.1145/128765.128770

4. Flink Community. (2015). **Apache Flink: Stream and Batch Processing in a Single Engine**. *IEEE Data Engineering Bulletin*, 38(4), 28–38. http://sites.computer.org/debull/A15dec/p28.pdf

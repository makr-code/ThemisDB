> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Time Series Module

Time series data management and compression implementation for ThemisDB.

## Module Purpose

Provides time series data management and compression for ThemisDB, offering Gorilla compression, continuous aggregation, retention management, and automatic batching for high-frequency single-point inserts.

## Subsystem Scope

**In scope:** Time series storage (TSStore), Gorilla delta-delta compression, continuous aggregation for downsampling, time-based retention, TSAutoBuffer for auto-batching.

**Out of scope:** General temporal data (handled by temporal module), event streaming (handled by cdc module), raw metrics collection (handled by observability module).

## Relevant Interfaces

- `tsstore.h/cpp` — time series storage backend
- `gorilla.h/cpp` — Gorilla compression codec
- `continuous_agg.h/cpp` — continuous aggregation engine
- `retention.h/cpp` — retention policy enforcement
- `ts_auto_buffer.h/cpp` — automatic batching buffer

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — TSStore, Gorilla compression, continuous aggregation, retention policies, and auto-batching are operational.

## Components

- **Time series storage** (`tsstore.h/cpp`)
- **Continuous aggregation** (`continuous_agg.h/cpp`)
- **Gorilla compression** (`gorilla.h/cpp`)
- **Retention management** (`retention.h/cpp`)
- **Auto-batching buffer** (`ts_auto_buffer.h/cpp`) - NEW!
- TSStore

## Features

- Optimized time series storage
- Gorilla time series compression
- **Automatic batching for single-point inserts** (TSAutoBuffer)
- Continuous aggregation for downsampling
- Time-based retention policies
- High-frequency data ingestion
- Configurable compression strategies

## Documentation

For time series documentation, see:
- [Time Series Storage Methods](../../docs/timeseries/STORAGE_METHODS.md) - How data is stored
- [Auto-Batching Buffer](../../docs/timeseries/AUTO_BUFFER.md) - Automatic compression for single points
- [Time Series](../../docs/src/timeseries/timeseries.cpp.md)
- [Continuous Aggregation](../../docs/src/timeseries/continuous_agg.cpp.md)
- [Gorilla Compression](../../docs/src/timeseries/gorilla.cpp.md)
- [Retention](../../docs/src/timeseries/retention.cpp.md)
- [TSStore](../../docs/src/timeseries/tsstore.cpp.md)
- [Time Series Documentation](../../docs/time_series.md)
- [Temporal Graphs](../../docs/temporal_graphs.md)

## Scientific References

1. Pelkonen, T., Franklin, S., Teller, J., Cavallaro, P., Huang, Q., Meza, J., & Veeraraghavan, K. (2015). **Gorilla: A Fast, Scalable, In-Memory Time Series Database**. *Proceedings of the VLDB Endowment*, 8(12), 1816–1827. https://doi.org/10.14778/2824032.2824078

2. Elias, P. (1975). **Universal Codeword Sets and Representations of the Integers**. *IEEE Transactions on Information Theory*, 21(2), 194–203. https://doi.org/10.1109/TIT.1975.1055349

3. Ding, R., Wang, Q., Dang, Y., Fu, Q., Zhang, H., & Zhang, D. (2015). **YADING: Fast Clustering of Large-Scale Time Series Data**. *Proceedings of the VLDB Endowment*, 8(5), 473–484. https://doi.org/10.14778/2735479.2735481

4. Keogh, E., & Ratanamahatana, C. A. (2005). **Exact Indexing of Dynamic Time Warping**. *Knowledge and Information Systems*, 7(3), 358–386. https://doi.org/10.1007/s10115-004-0154-9

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/timeseries/README.md`](../../include/timeseries/README.md) for the public API.

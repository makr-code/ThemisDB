# Time Series Module

Time series data management and compression implementation for ThemisDB.

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

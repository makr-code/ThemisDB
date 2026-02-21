# Acceleration Module

GPU and hardware acceleration implementations for ThemisDB.

## Module Purpose

Provides GPU and hardware acceleration for compute-intensive database operations including vector similarity search, geospatial queries, graph algorithms, and matrix operations using CUDA and Vulkan backends.

## Subsystem Scope

**In scope:** CUDA kernel implementations for HNSW/ANN search, Vulkan compute shaders for cross-platform GPU, GPU device detection and capability negotiation, CPU fallback paths.

**Out of scope:** CPU-only algorithm implementations (handled by index/geo/graph modules), GPU driver management.

## Relevant Interfaces

- `cuda/ann_kernels.cu` — CUDA ANN search kernels (stub)
- `vulkan/compute_pipeline.cpp` — Vulkan compute shaders (stub)
- `device_manager` — runtime device detection

## Current Delivery Status

**Maturity:** 🔴 Alpha — Infrastructure scaffolding complete; CUDA and Vulkan kernel implementations are stubs pending production implementation.

## Components

- **cuda/** - NVIDIA CUDA implementations for GPU-accelerated operations
- **vulkan/** - Vulkan compute implementations for cross-platform GPU acceleration

## Features

- Vector similarity search acceleration
- Geospatial query acceleration
- Parallel graph algorithms
- Matrix operations for embeddings

## Documentation

For detailed acceleration documentation, see:
- [CUDA Backend](../../docs/performance/CUDA_BACKEND.md)
- [Vulkan Backend](../../docs/performance/VULKAN_BACKEND.md)
- [Hardware Acceleration Plan](../../docs/performance/HARDWARE_ACCELERATION.md)
- [GPU Acceleration Plan](../../docs/performance/GPU_ACCELERATION_PLAN.md)

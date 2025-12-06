# GPU Acceleration Configuration

ThemisDB supports GPU acceleration for vector search, graph operations, and spatial operations through various compute backends. This document describes how to configure and use GPU acceleration.

## Configuration File

The acceleration configuration is stored in `config/acceleration.yaml`. Here's an example:

```yaml
acceleration:
  # Preference for backend selection
  # auto: Automatically select best available backend (GPU if available, CPU fallback)
  # gpu: Prefer GPU backends, fail if not available (unless gpu_fallback=true)
  # cpu: Force CPU-only execution
  prefer: auto
  
  # Enable automatic CPU fallback when GPU operations fail
  # When true, operations will retry on CPU if GPU execution fails
  # When false, GPU errors will propagate to the caller
  gpu_fallback: true
  
  # Minimum batch size to activate GPU acceleration
  # Operations with fewer items will use CPU (reduces GPU overhead for small batches)
  min_batch_size: 1000
```

## Configuration Options

### `prefer`
Controls which backend type to prefer:
- `auto` (default): Automatically selects the best available backend. Prefers GPU if available, falls back to CPU.
- `gpu`: Only use GPU backends. If no GPU is available and `gpu_fallback` is false, operations will fail.
- `cpu`: Force CPU-only execution. Useful for testing or when GPU is reserved for other tasks.

### `gpu_fallback`
Boolean flag controlling automatic fallback behavior:
- `true` (default): If a GPU operation fails, automatically retry on CPU
- `false`: GPU errors propagate to the caller (no automatic fallback)

This is useful in production environments where you want to ensure operations complete even if the GPU is temporarily unavailable.

### `min_batch_size`
Minimum number of operations required to use GPU acceleration (default: 1000).

GPU acceleration has overhead (memory transfer, kernel launch). For small batches, CPU execution is often faster. This threshold prevents inefficient GPU usage for small operations.

Example:
- Batch size = 100: Uses CPU (below threshold)
- Batch size = 5000: Uses GPU (above threshold)

## Supported GPU Backends

ThemisDB supports multiple GPU compute backends, automatically selected based on availability:

1. **CUDA** (NVIDIA GPUs) - Best performance on NVIDIA hardware
2. **HIP** (AMD GPUs) - Native AMD solution
3. **ZLUDA** (AMD GPUs) - CUDA compatibility layer for AMD
4. **Vulkan** - Cross-platform modern graphics API
5. **DirectX 12** - Windows-native compute shaders
6. **ROCm** - AMD compute platform
7. **Metal** - Apple hardware acceleration
8. **OpenCL** - Generic cross-platform acceleration
9. **CPU** - Always available fallback

## Docker Support

### Standard Image (CPU-only)
```bash
docker run -p 8765:8765 themisdb/themisdb:latest
```

### CUDA-enabled Image
For NVIDIA GPU acceleration using Faiss GPU:

```bash
docker run --gpus all -p 8765:8765 themisdb/themisdb:cuda
```

Requirements:
- NVIDIA GPU with CUDA 12.3+ support
- NVIDIA Container Toolkit installed
- `--gpus all` flag to expose GPUs to container

### Building CUDA Image
```bash
docker build -f Dockerfile.cuda -t themisdb:cuda .
```

## Runtime Detection

ThemisDB automatically detects available GPU backends at startup. Check logs for:

```
[INFO] Auto-detecting acceleration backends...
[INFO] Registered backend: CUDA (Type: 1)
[INFO] Total backends available: 4
[INFO]   - CUDA (Vector:Yes Graph:Yes Geo:Yes)
[INFO]   - CPU (Vector:Yes Graph:Yes Geo:Yes)
```

## Vector Search with Faiss GPU

When CUDA backend is available, ThemisDB uses Faiss GPU for accelerated vector search:

```cpp
// Automatic GPU selection for large batch
auto* backend = BackendRegistry::instance().getBestVectorBackend(10000);
// Will use CUDA/GPU backend if available and batch size >= min_batch_size

// Force CPU for small batch
auto* cpuBackend = BackendRegistry::instance().getBestVectorBackend(100);
// Will use CPU backend (below threshold)
```

## DirectX Compute Shaders (Windows)

On Windows systems without NVIDIA/AMD GPUs, ThemisDB can use DirectX 12 Compute Shaders for acceleration:

Requirements:
- Windows 10 version 1809 or later
- DirectX 12 compatible GPU
- Graphics driver with compute shader support

Configuration:
```yaml
acceleration:
  prefer: auto  # Will select DirectX if CUDA/Vulkan not available
  directx:
    enabled: true
    adapter_index: 0
```

## Performance Tuning

### Batch Size Threshold
Adjust `min_batch_size` based on your GPU:

- **High-end GPUs** (RTX 4090, A100): 500-1000
- **Mid-range GPUs** (RTX 3060, RX 6700): 1000-2000
- **Low-end GPUs** (GTX 1650, integrated): 2000-5000

Test with your workload to find optimal value.

### Memory Management
Configure GPU memory limits in backend-specific settings:

```yaml
acceleration:
  cuda:
    memory_limit_gb: 8  # Maximum VRAM to use
```

## Monitoring

ThemisDB logs backend selection decisions:

```
[DEBUG] Batch size 500 below threshold 1000, using CPU
[DEBUG] Selected GPU backend: CUDA
[WARN] GPU backend requested but not available and fallback disabled
```

Monitor these logs to ensure optimal backend selection.

## Troubleshooting

### GPU not detected
1. Verify GPU is available: `nvidia-smi` (NVIDIA) or `rocm-smi` (AMD)
2. Check driver installation
3. Ensure CUDA/ROCm toolkit is installed
4. Verify container runtime has GPU access (`--gpus all`)

### Performance not improving
1. Check batch size is above threshold
2. Monitor GPU utilization
3. Verify data transfer overhead isn't dominating (use pinned memory)
4. Consider increasing `min_batch_size`

### Out of memory errors
1. Reduce `memory_limit_gb`
2. Decrease batch size
3. Enable `gpu_fallback: true` for automatic CPU fallback

## Example Use Cases

### High-throughput Vector Search
```yaml
acceleration:
  prefer: gpu
  gpu_fallback: true
  min_batch_size: 500
  cuda:
    batch_size: 10000
    async_compute: true
```

### Development/Testing (CPU-only)
```yaml
acceleration:
  prefer: cpu
```

### Production (reliable with fallback)
```yaml
acceleration:
  prefer: auto
  gpu_fallback: true
  min_batch_size: 1000
```

## API Integration

The acceleration configuration is automatically loaded by `themis_server` from:
1. `--config` command line argument
2. `./config/acceleration.yaml`
3. `./acceleration.yaml`
4. `/etc/themisdb/acceleration.yaml`

No code changes required - just deploy the configuration file.

# GPU Acceleration Quick Reference

## Enable GPU Acceleration

### 1. Configuration File
Edit `config/acceleration.yaml`:
```yaml
acceleration:
  prefer: auto          # auto | gpu | cpu
  gpu_fallback: true    # Enable CPU fallback
  min_batch_size: 1000  # GPU threshold
```

### 2. Docker (CUDA)
```bash
# Build CUDA image
docker build -f Dockerfile.cuda -t themisdb:cuda .

# Run with GPU
docker run --gpus all -p 8765:8765 themisdb:cuda
```

### 3. Build from Source (with CUDA)
```bash
cmake -B build -DTHEMIS_ENABLE_CUDA=ON -DTHEMIS_ENABLE_GPU=ON
cmake --build build
```

## Configuration Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `prefer` | `auto`, `gpu`, `cpu` | `auto` | Backend preference |
| `gpu_fallback` | `true`, `false` | `true` | CPU fallback on GPU errors |
| `min_batch_size` | integer | `1000` | Min operations for GPU |

## Supported Backends (Priority Order)

1. **CUDA** - NVIDIA GPUs (best performance)
2. **HIP** - AMD GPUs
3. **Vulkan** - Cross-platform
4. **DirectX 12** - Windows GPU
5. **CPU** - Always available (fallback)

## Examples

### Force CPU-only
```yaml
acceleration:
  prefer: cpu
```

### Require GPU (no fallback)
```yaml
acceleration:
  prefer: gpu
  gpu_fallback: false
```

### Optimize for small batches
```yaml
acceleration:
  prefer: auto
  min_batch_size: 500  # Lower threshold
```

## Verification

Check logs on startup:
```
[INFO] Acceleration configured: prefer=AUTO, gpu_fallback=true, min_batch_size=1000
[INFO] GPU acceleration is AVAILABLE
[INFO] Registered backend: CUDA (Type: 1)
```

## Performance Tips

- **High-end GPU**: `min_batch_size: 500-1000`
- **Mid-range GPU**: `min_batch_size: 1000-2000`
- **Low-end GPU**: `min_batch_size: 2000-5000`

## Troubleshooting

**GPU not detected:**
- Verify: `nvidia-smi` (NVIDIA) or `rocm-smi` (AMD)
- Check Docker: `docker run --gpus all nvidia/cuda:12.3.0-base nvidia-smi`
- Ensure drivers installed

**Performance not improving:**
- Check batch size is above threshold
- Monitor GPU utilization
- Review logs for backend selection

**Out of memory:**
- Reduce `memory_limit_gb` in config
- Decrease batch size
- Enable `gpu_fallback: true`

## Documentation

- Full guide: [docs/features/gpu_acceleration.md](gpu_acceleration.md)
- Config reference: [config/acceleration.yaml](../../config/acceleration.yaml)
- Example config: [config/config.example.gpu.yaml](../../config/config.example.gpu.yaml)

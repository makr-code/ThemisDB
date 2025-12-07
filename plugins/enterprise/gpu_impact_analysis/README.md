# GPU Impact Analysis Plugin

**Enterprise Feature for ThemisDB**

GPU-accelerated FEM-inspired cause-effect analysis for document changes and their impact in the graph.

## Quick Start

### Installation

1. **Copy plugin files:**
   ```bash
   # Windows
   Copy-Item themis_gpu_impact_analysis.dll "C:\Program Files\ThemisDB\plugins\enterprise\"
   
   # Linux
   sudo cp libthemis_gpu_impact_analysis.so /usr/local/lib/themis/plugins/enterprise/
   ```

2. **Configure:**
   ```bash
   cp config.yaml /etc/themis/plugins/gpu_impact_analysis.yaml
   ```

3. **Activate license:**
   ```sql
   LOAD PLUGIN 'themis.enterprise.gpu_impact_analysis' 
   WITH LICENSE 'YOUR-LICENSE-KEY';
   ```

### Basic Usage

```sql
-- Analyze document change impact
LET impact = GPU_ANALYZE_IMPACT(
  {
    document_id: 'products/123',
    change_type: 'update',
    magnitude: 0.8
  },
  {max_depth: 5, use_gpu: true}
)

RETURN {
  total_affected: impact.total_affected_count,
  max_impact: impact.max_impact_score
}
```

## Features

- ✅ FEM-based graph propagation (10-50x GPU speedup)
- ✅ Monte Carlo risk assessment (100-1000x GPU speedup)
- ✅ Time series forecasting (ARIMA on GPU)
- ✅ Pattern detection (FFT, 100-500x speedup)
- ✅ Anomaly detection (Isolation Forest, 20-50x speedup)
- ✅ What-If scenario analysis
- ✅ Root cause analysis
- ✅ Sensitivity analysis

## Documentation

See [GPU Impact Analysis Plugin Documentation](../../../docs/enterprise/gpu_impact_analysis_plugin.md) for complete guide.

## Building

```bash
cd plugins/enterprise/gpu_impact_analysis
cmake -B build -DTHEMIS_ENABLE_CUDA=ON
cmake --build build --config Release
```

## License

ThemisDB Enterprise License  
Contact: enterprise-support@themisdb.com

## Support

- Documentation: https://docs.themisdb.com/enterprise/gpu-impact-analysis
- Email: enterprise-support@themisdb.com
- Issues: https://github.com/makr-code/ThemisDB/issues

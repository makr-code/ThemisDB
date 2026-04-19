> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# RoPE Visualization Examples

This directory contains example scripts and notebooks demonstrating the RoPE visualization tools.

## Files

### 1. `basic_visualization_example.py`
Basic introduction to RoPE visualization with simple examples.

**Features:**
- 2D embedding space visualization
- Rotation trail plotting
- Similarity heatmap generation
- Theta distribution visualization
- 3D embedding space

**Usage:**
```bash
python examples/rope_visualization/basic_visualization_example.py
```

**Output:** Saves visualizations to `/tmp/rope_*.png`

---

### 2. `cli_usage_demo.py`
Demonstrates all CLI commands and options.

**Features:**
- Shows how to use each CLI subcommand
- Demonstrates different configuration options
- Examples of custom titles and parameters

**Usage:**
```bash
python examples/rope_visualization/cli_usage_demo.py
```

**Output:** Saves visualizations to `/tmp/cli_demo_*.png`

---

### 3. `advanced_analysis_example.py`
Advanced analysis workflows and comparative studies.

**Features:**
- Compare different `base_theta` values
- Analyze position separation effectiveness
- Study impact of embedding dimensions
- Generate comprehensive reports

**Usage:**
```bash
python examples/rope_visualization/advanced_analysis_example.py
```

**Output:** Saves analysis plots to `/tmp/rope_*.png`

---

### 4. `rope_visualization_tutorial.ipynb`
Interactive Jupyter notebook with step-by-step tutorial.

**Features:**
- Interactive exploration with sliders
- Mathematical property verification
- Detailed explanations and commentary
- Hands-on exercises

**Usage:**
```bash
jupyter notebook examples/rope_visualization/rope_visualization_tutorial.ipynb
```

---

### 5. `rope_config.json`
Example configuration file for RoPE parameters.

**Contents:**
```json
{
  "hidden_dim": 128,
  "base_theta": 10000.0,
  "num_rotation_pairs": 64,
  "normalize_after": false
}
```

**Usage:**
```bash
python tools/rope_visualizer/cli.py visualize \
  --embeddings embeddings.npy \
  --config examples/rope_visualization/rope_config.json \
  --output output.png
```

---

## Quick Start

### Option 1: Run Basic Example
```bash
# Install dependencies
pip install numpy matplotlib seaborn scikit-learn

# Run basic visualization
python examples/rope_visualization/basic_visualization_example.py

# View outputs
ls /tmp/rope_*.png
```

### Option 2: Try CLI Commands
```bash
# First, generate test embeddings
python -c "import numpy as np; np.save('/tmp/test_emb.npy', np.random.randn(100, 128))"

# Visualize with PCA
python tools/rope_visualizer/cli.py visualize \
  --embeddings /tmp/test_emb.npy \
  --method pca \
  --output /tmp/viz.png

# Generate heatmap
python tools/rope_visualizer/cli.py heatmap \
  --embeddings /tmp/test_emb.npy \
  --positions 0,50,100,200 \
  --output /tmp/heatmap.png

# Plot rotation trail
python tools/rope_visualizer/cli.py trail \
  --embeddings /tmp/test_emb.npy \
  --positions 0-500 \
  --output /tmp/trail.png
```

### Option 3: Interactive Jupyter Notebook
```bash
# Install Jupyter (if not already installed)
pip install jupyter ipywidgets

# Launch notebook
jupyter notebook examples/rope_visualization/rope_visualization_tutorial.ipynb
```

---

## Common Use Cases

### 1. Debugging RoPE Implementation
Verify your RoPE implementation by visualizing rotation effects:

```python
from rope_visualizer import RopeVisualizer
import numpy as np

# Your embeddings
embeddings = your_rope_function(...)

# Visualize
viz = RopeVisualizer({'hidden_dim': 128, 'base_theta': 10000.0})
viz.plot_embeddings(embeddings, positions=list(range(100)), method='pca')
```

### 2. Comparing Different Configurations
Find optimal `base_theta` for your use case:

```python
for theta in [1000, 10000, 100000]:
    viz = RopeVisualizer({'base_theta': theta})
    viz.plot_rotation_trail(embedding, positions, 
                           title=f'Theta = {theta}')
```

### 3. Educational Demonstrations
Create visualizations for presentations or documentation:

```python
# High-quality figures for papers
viz.plot_embeddings(embeddings, positions, 
                   save_path='figure1.png',
                   title='RoPE Effect on Embedding Space')
```

### 4. Analyzing Position Encoding Quality
Check if positions are well-separated:

```python
# Compare similarity at different position gaps
viz.plot_similarity_matrix(embedding, 
                          positions=[0, 10, 20, 50, 100, 200],
                          metric='cosine')
```

---

## Tips and Tricks

### Performance
- Use PCA for quick exploration (fastest)
- Use UMAP for better separation (requires `umap-learn`)
- Use t-SNE for publication-quality plots (slowest)

### Visualization Quality
- Normalize embeddings before visualization
- Limit to 30-50 points for 3D plots
- Use heatmap for detailed position comparison
- Adjust `figsize` in config for larger plots

### Debugging
- Check theta distribution first (`theta` command)
- Verify rotation with trail plot
- Use similarity heatmap to find issues
- Compare with reference implementation

---

## Dependencies

### Required
```
numpy>=1.20.0
matplotlib>=3.3.0
seaborn>=0.11.0
scikit-learn>=0.24.0
```

### Optional
```
umap-learn>=0.5.0     # For UMAP projections
jupyter>=1.0.0        # For notebooks
ipywidgets>=7.6.0     # For interactive widgets
```

Install all:
```bash
pip install -r tools/rope_visualizer/requirements.txt
```

---

## Troubleshooting

**Q: ImportError: No module named 'umap'**  
A: Install UMAP with `pip install umap-learn` or use `--method pca` instead.

**Q: Plots don't show up**  
A: Either use `--no-show` flag and check saved files, or ensure you have a display server running.

**Q: ValueError: Need at least N samples**  
A: Increase the number of embeddings or reduce `n_components`.

**Q: Similarity matrix looks wrong**  
A: Make sure embeddings are normalized to unit length.

---

## Related Documentation

- [RoPE Visualizer README](../../tools/rope_visualizer/README.md)
- [RoPE Implementation](../../include/index/rotary_embeddings.h)
- [Learnable RoPE](../../include/index/learnable_rope.h)
- [LoRA RoPE](../../include/index/lora_rope.h)

---

## Contributing

Found a bug or have a feature request? Please open an issue on GitHub!

Want to add a new example? Follow these guidelines:
1. Create a descriptive filename (e.g., `use_case_specific_example.py`)
2. Add docstring explaining the purpose
3. Include usage instructions in comments
4. Test thoroughly before submitting
5. Update this README with your example

---

## License

These examples are part of ThemisDB and follow the same MIT license.

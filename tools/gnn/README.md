> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# GNN Tools for ThemisDB

This directory contains **verification and testing tools** for Graph Neural Network (GNN) implementations in ThemisDB.

## Purpose

**Important:** These Python tools are **NOT** the primary training implementation. They serve to:

- ✅ **Verify** native C++ GNN training implementations
- ✅ **Test** GNN algorithms against PyTorch Geometric reference implementations
- ✅ **Benchmark** ThemisDB's native implementation vs. standard frameworks
- ✅ **Prototype** new GNN architectures before C++ implementation

**Primary Implementation:** ThemisDB uses native C++ training integrated with `DistributedTrainingCoordinator` and the existing LLM training infrastructure.

## Overview

ThemisDB supports GNN-based embeddings for graph data through the `GNNEmbeddingManager` class with native C++ training. The Python tools in this directory provide:

- **Reference Implementations**: PyTorch Geometric versions for comparison
- **Verification Scripts**: Validate C++ training produces correct results
- **Testing Utilities**: Generate test data and validate embeddings
- **Benchmarking**: Compare performance between implementations

## Quick Start (Verification Workflow)

### 1. Train Native C++ Model (Primary)

First, train using ThemisDB's native C++ implementation:

```cpp
// C++ Training (Primary Implementation)
GnnTrainingEngine trainer(db, pgm, coordinator);
TrainingConfig config{
    .model_type = "graphsage",
    .embedding_dim = 128,
    .epochs = 100
};
trainer.trainGraphSAGE("social_network", config);
```

### 2. Verify with Python Reference (Optional)

For verification, compare against PyTorch Geometric reference:

```bash
# Export data for verification
python export_graph_data.py \
    --host localhost \
    --port 8080 \
    --graph social_network \
    --output data/social_network.parquet

# Train reference model
python train_gnn.py \
    --input data/social_network.parquet \
    --model graphsage \
    --embedding-dim 128 \
    --epochs 100 \
    --output models/reference_graphsage.pth

# Compare embeddings
python verify_embeddings.py \
    --cpp-embeddings data/cpp_embeddings.npy \
    --pytorch-embeddings models/reference_graphsage.pth \
    --threshold 0.95
```

### 3. Benchmark Performance

Compare training performance:

```bash
python benchmark_training.py \
    --cpp-host localhost \
    --cpp-port 8080 \
    --graph social_network \
    --iterations 10
```

## Available Tools

### Verification Scripts

- **`train_gnn.py`**: PyTorch Geometric reference implementation for verification
- **`verify_embeddings.py`**: Compare C++ vs PyTorch embeddings
- **`benchmark_training.py`**: Performance comparison between implementations

### Testing Utilities

- **`gnn_example.py`**: Standalone PyTorch example for understanding GNN concepts
- **`export_graph_data.py`**: Export ThemisDB graphs for external testing
- **`export_to_onnx.py`**: Convert PyTorch models (for cross-validation)

### Analysis Tools

- **`evaluate_model.py`**: Evaluate embedding quality
- **`visualize_embeddings.py`**: Visualize embeddings with t-SNE/UMAP

## Dependencies

Install required Python packages:

```bash
pip install torch torch-geometric pyarrow pandas onnx
```

For full functionality:

```bash
pip install torch torch-geometric pyarrow pandas onnx scikit-learn matplotlib seaborn umap-learn
```

## Model Types

### GraphSAGE (Recommended)

- **Best for**: Large graphs, inductive learning, production use
- **Pros**: Scalable, works with new nodes, good performance
- **Cons**: Requires sampling during training

```bash
python train_gnn.py --model graphsage --embedding-dim 128
```

### GCN (Graph Convolutional Network)

- **Best for**: Small to medium graphs, transductive learning
- **Pros**: Simple, well-understood, fast inference
- **Cons**: Doesn't generalize to new nodes

```bash
python train_gnn.py --model gcn --embedding-dim 128
```

### GAT (Graph Attention Network)

- **Best for**: Graphs with varying edge importance
- **Pros**: Attention mechanism, interpretable, high accuracy
- **Cons**: Slower training, more memory

```bash
python train_gnn.py --model gat --embedding-dim 128
```

## Configuration

### Training Configuration

Create a `config.yaml` file:

```yaml
model:
  type: graphsage
  hidden_dim: 256
  num_layers: 3
  embedding_dim: 128
  dropout: 0.5

training:
  epochs: 100
  batch_size: 512
  learning_rate: 0.001
  weight_decay: 0.0005

data:
  num_neighbors: [10, 10]  # 2-hop sampling
  negative_sampling: true
```

Then run:

```bash
python train_gnn.py --config config.yaml
```

## Current Status

### ✅ C++ Native Implementation (Primary)

- Native GNN Training via `DistributedTrainingCoordinator`
- Integration with existing LLM training infrastructure
- Multi-GPU support via LoRA-RAID system
- Direct training from RocksDB without export
- Production-ready sharding-aware distributed training

### ✅ Python Verification Tools (This Directory)

- PyTorch Geometric reference implementations
- Verification scripts for correctness testing
- Benchmarking tools for performance comparison
- Testing utilities for algorithm validation

### 🚧 In Progress

- Verification script integration with C++ training API
- Automated correctness testing pipeline
- Performance benchmarking framework

### 🔮 Planned

- Additional GNN architectures (GIN, RGCN)
- Temporal GNN verification
- Multi-modal embedding verification

## Examples

See `examples/` directory for complete working examples:

- `examples/fraud_detection.py` - Use GNN embeddings for fraud detection
- `examples/recommendation.py` - Build a recommendation system
- `examples/knowledge_graph.py` - Knowledge graph reasoning

## Performance

Expected performance (based on research literature):

- **Embedding Generation**: ~1000 nodes/sec (CPU), ~10,000 nodes/sec (GPU)
- **Similarity Search**: <5ms for Top-10 in 1M nodes (HNSW index)
- **Training Time**: ~1-4 hours for 1M nodes on single GPU

## Troubleshooting

### Model doesn't converge

- Try reducing learning rate: `--learning-rate 0.0001`
- Increase model capacity: `--hidden-dim 512 --num-layers 4`
- Check data quality: Ensure features are normalized

### Out of memory during training

- Reduce batch size: `--batch-size 256`
- Use smaller model: `--hidden-dim 128 --num-layers 2`
- Enable gradient checkpointing: `--gradient-checkpointing`

### Poor embedding quality

- Train longer: `--epochs 200`
- Use more neighbors: `--num-neighbors 20,20,20` (3-hop)
- Try different model: `--model gat`

## References

- [Research Document](../../docs/research/GNN_BASED_INDEXING_AND_EMBEDDINGS.md)
- [PyTorch Geometric Documentation](https://pytorch-geometric.readthedocs.io/)
- [GraphSAGE Paper](https://arxiv.org/abs/1706.02216)
- [GCN Paper](https://arxiv.org/abs/1609.02907)
- [GAT Paper](https://arxiv.org/abs/1710.10903)

## Contributing

To add a new GNN model:

1. Create model class in `models/<model_name>.py`
2. Implement `forward()` method following PyTorch Geometric conventions
3. Add model to `train_gnn.py` model registry
4. Test with `python test_model.py --model <model_name>`
5. Document in this README

## License

See [LICENSE](../../LICENSE) file in repository root.

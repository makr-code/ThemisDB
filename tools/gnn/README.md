# GNN Tools for ThemisDB

This directory contains tools for training and using Graph Neural Network (GNN) models with ThemisDB.

## Overview

ThemisDB supports GNN-based embeddings for graph data through the `GNNEmbeddingManager` class. This enables:

- **Node Embeddings**: Vector representations of graph nodes based on their features and structure
- **Edge Embeddings**: Vector representations of graph edges
- **Similarity Search**: Find similar nodes/edges based on embedding similarity
- **Query Optimization**: Use GNN-based embeddings to improve query performance

## Quick Start

### 1. Export Graph Data

First, export your graph data from ThemisDB for training:

```bash
python export_graph_data.py \
    --host localhost \
    --port 8080 \
    --graph social_network \
    --output data/social_network.parquet
```

### 2. Train GNN Model

Train a GraphSAGE model on your exported data:

```bash
python train_gnn.py \
    --input data/social_network.parquet \
    --model graphsage \
    --embedding-dim 128 \
    --epochs 100 \
    --output models/social_network_graphsage.pth
```

### 3. Export to ONNX

Convert the trained PyTorch model to ONNX format for C++ inference:

```bash
python export_to_onnx.py \
    --model models/social_network_graphsage.pth \
    --output models/social_network_graphsage.onnx \
    --embedding-dim 128
```

### 4. Register Model in ThemisDB

Register the ONNX model in ThemisDB (currently uses simple feature-based embeddings as MVP):

```cpp
// C++ API
GNNEmbeddingManager gnn(db, pgm, vim);
gnn.registerModel("social_graphsage", "graphsage", 128, "{}");

// Generate embeddings
gnn.generateNodeEmbeddings("social_network", "Person", "social_graphsage");

// Find similar nodes
auto [st, similar] = gnn.findSimilarNodes("alice", "social_network", 10, "social_graphsage");
```

## Available Tools

### Training Scripts

- **`train_gnn.py`**: Train GNN models (GraphSAGE, GCN, GAT)
- **`export_graph_data.py`**: Export graph data from ThemisDB
- **`export_to_onnx.py`**: Convert PyTorch models to ONNX

### Utilities

- **`gnn_example.py`**: End-to-end example of training and using GNN embeddings
- **`evaluate_model.py`**: Evaluate model quality and performance
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

### ✅ Implemented (MVP)

- Basic `GNNEmbeddingManager` C++ class
- Feature extraction from graph nodes
- Simple embedding computation (feature-based)
- Vector index integration for similarity search
- Batch processing support

### 🚧 In Progress

- Python training pipeline (this directory)
- ONNX model inference integration
- Multi-hop neighbor aggregation
- Incremental embedding updates

### 🔮 Planned

- Distributed training (multi-GPU)
- Temporal graph support
- Multi-modal embeddings (text + graph + image)
- LLM integration for semantic search

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

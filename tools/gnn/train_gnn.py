"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            train_gnn.py                                       ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:49:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     298                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
GNN Model Training Script for ThemisDB

This script trains Graph Neural Network models on graph data exported from ThemisDB.
Supports GraphSAGE, GCN, and GAT architectures.

Usage:
    python train_gnn.py --input data/graph.parquet --model graphsage --embedding-dim 128
"""

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Dict, List, Tuple

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch_geometric.data import Data
from torch_geometric.nn import SAGEConv, GCNConv, GATConv
from torch_geometric.loader import NeighborLoader
import pandas as pd
import pyarrow.parquet as pq


class GraphSAGEModel(nn.Module):
    """GraphSAGE model for node embedding generation."""
    
    def __init__(self, in_channels: int, hidden_channels: int, out_channels: int, num_layers: int = 2, dropout: float = 0.5):
        super().__init__()
        self.convs = nn.ModuleList()
        self.convs.append(SAGEConv(in_channels, hidden_channels))
        for _ in range(num_layers - 2):
            self.convs.append(SAGEConv(hidden_channels, hidden_channels))
        self.convs.append(SAGEConv(hidden_channels, out_channels))
        self.dropout = dropout
        
    def forward(self, x, edge_index):
        for i, conv in enumerate(self.convs[:-1]):
            x = conv(x, edge_index)
            x = F.relu(x)
            x = F.dropout(x, p=self.dropout, training=self.training)
        x = self.convs[-1](x, edge_index)
        return x


class GCNModel(nn.Module):
    """Graph Convolutional Network model."""
    
    def __init__(self, in_channels: int, hidden_channels: int, out_channels: int, num_layers: int = 2, dropout: float = 0.5):
        super().__init__()
        self.convs = nn.ModuleList()
        self.convs.append(GCNConv(in_channels, hidden_channels))
        for _ in range(num_layers - 2):
            self.convs.append(GCNConv(hidden_channels, hidden_channels))
        self.convs.append(GCNConv(hidden_channels, out_channels))
        self.dropout = dropout
        
    def forward(self, x, edge_index):
        for i, conv in enumerate(self.convs[:-1]):
            x = conv(x, edge_index)
            x = F.relu(x)
            x = F.dropout(x, p=self.dropout, training=self.training)
        x = self.convs[-1](x, edge_index)
        return x


class GATModel(nn.Module):
    """Graph Attention Network model."""
    
    def __init__(self, in_channels: int, hidden_channels: int, out_channels: int, num_layers: int = 2, dropout: float = 0.5, heads: int = 4):
        super().__init__()
        self.convs = nn.ModuleList()
        self.convs.append(GATConv(in_channels, hidden_channels, heads=heads, dropout=dropout))
        for _ in range(num_layers - 2):
            self.convs.append(GATConv(hidden_channels * heads, hidden_channels, heads=heads, dropout=dropout))
        self.convs.append(GATConv(hidden_channels * heads, out_channels, heads=1, concat=False, dropout=dropout))
        self.dropout = dropout
        
    def forward(self, x, edge_index):
        for i, conv in enumerate(self.convs[:-1]):
            x = conv(x, edge_index)
            x = F.elu(x)
        x = self.convs[-1](x, edge_index)
        return x


def load_graph_data(parquet_path: str) -> Tuple[Data, Dict]:
    """
    Load graph data from Parquet file exported from ThemisDB.
    
    Expected schema:
        - node_id: int64
        - node_type: string
        - features: list<float>
        - edges: list<struct<src:int64, dst:int64, type:string>>
    
    Returns:
        PyTorch Geometric Data object and metadata dict
    """
    print(f"Loading graph data from {parquet_path}...")
    table = pq.read_table(parquet_path)
    df = table.to_pandas()
    
    # Extract node features
    node_ids = df['node_id'].values
    node_features = torch.tensor(df['features'].tolist(), dtype=torch.float)
    
    # Build node ID to index mapping
    node_id_to_idx = {node_id: idx for idx, node_id in enumerate(node_ids)}
    
    # Extract edges
    edge_list = []
    for edges in df['edges']:
        if edges:
            for edge in edges:
                src_idx = node_id_to_idx.get(edge['src'])
                dst_idx = node_id_to_idx.get(edge['dst'])
                if src_idx is not None and dst_idx is not None:
                    edge_list.append([src_idx, dst_idx])
    
    edge_index = torch.tensor(edge_list, dtype=torch.long).t().contiguous()
    
    # Create PyG Data object
    data = Data(x=node_features, edge_index=edge_index)
    
    metadata = {
        'num_nodes': len(node_ids),
        'num_edges': edge_index.shape[1],
        'num_features': node_features.shape[1],
        'node_ids': node_ids.tolist()
    }
    
    print(f"Loaded graph: {metadata['num_nodes']} nodes, {metadata['num_edges']} edges, {metadata['num_features']} features")
    return data, metadata


def train_epoch(model, data, optimizer, device):
    """Train model for one epoch."""
    model.train()
    optimizer.zero_grad()
    
    # Forward pass
    out = model(data.x.to(device), data.edge_index.to(device))
    
    # Self-supervised loss: Link prediction
    # Positive samples: actual edges
    pos_edge_index = data.edge_index.to(device)
    pos_scores = (out[pos_edge_index[0]] * out[pos_edge_index[1]]).sum(dim=1)
    
    # Negative samples: random non-edges
    num_neg = pos_edge_index.shape[1]
    neg_edge_index = torch.randint(0, data.num_nodes, (2, num_neg), device=device)
    neg_scores = (out[neg_edge_index[0]] * out[neg_edge_index[1]]).sum(dim=1)
    
    # Binary cross-entropy loss
    pos_loss = F.binary_cross_entropy_with_logits(pos_scores, torch.ones_like(pos_scores))
    neg_loss = F.binary_cross_entropy_with_logits(neg_scores, torch.zeros_like(neg_scores))
    loss = pos_loss + neg_loss
    
    loss.backward()
    optimizer.step()
    
    return loss.item()


@torch.no_grad()
def evaluate(model, data, device):
    """Evaluate model performance."""
    model.eval()
    out = model(data.x.to(device), data.edge_index.to(device))
    
    # Link prediction evaluation
    pos_edge_index = data.edge_index.to(device)
    pos_scores = (out[pos_edge_index[0]] * out[pos_edge_index[1]]).sum(dim=1)
    pos_acc = (pos_scores > 0).float().mean()
    
    num_neg = pos_edge_index.shape[1]
    neg_edge_index = torch.randint(0, data.num_nodes, (2, num_neg), device=device)
    neg_scores = (out[neg_edge_index[0]] * out[neg_edge_index[1]]).sum(dim=1)
    neg_acc = (neg_scores < 0).float().mean()
    
    accuracy = (pos_acc + neg_acc) / 2
    return accuracy.item()


def main():
    parser = argparse.ArgumentParser(description='Train GNN model for ThemisDB')
    parser.add_argument('--input', type=str, required=True, help='Input Parquet file')
    parser.add_argument('--model', type=str, default='graphsage', choices=['graphsage', 'gcn', 'gat'], help='Model type')
    parser.add_argument('--embedding-dim', type=int, default=128, help='Embedding dimension')
    parser.add_argument('--hidden-dim', type=int, default=256, help='Hidden layer dimension')
    parser.add_argument('--num-layers', type=int, default=2, help='Number of GNN layers')
    parser.add_argument('--dropout', type=float, default=0.5, help='Dropout rate')
    parser.add_argument('--epochs', type=int, default=100, help='Number of training epochs')
    parser.add_argument('--lr', type=float, default=0.001, help='Learning rate')
    parser.add_argument('--weight-decay', type=float, default=5e-4, help='Weight decay')
    parser.add_argument('--output', type=str, required=True, help='Output model path (.pth)')
    parser.add_argument('--device', type=str, default='cuda' if torch.cuda.is_available() else 'cpu', help='Device')
    
    args = parser.parse_args()
    
    # Load data
    data, metadata = load_graph_data(args.input)
    device = torch.device(args.device)
    data = data.to(device)
    
    # Create model
    in_channels = metadata['num_features']
    if args.model == 'graphsage':
        model = GraphSAGEModel(in_channels, args.hidden_dim, args.embedding_dim, args.num_layers, args.dropout)
    elif args.model == 'gcn':
        model = GCNModel(in_channels, args.hidden_dim, args.embedding_dim, args.num_layers, args.dropout)
    elif args.model == 'gat':
        model = GATModel(in_channels, args.hidden_dim, args.embedding_dim, args.num_layers, args.dropout)
    else:
        raise ValueError(f"Unknown model type: {args.model}")
    
    model = model.to(device)
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr, weight_decay=args.weight_decay)
    
    print(f"\nTraining {args.model.upper()} model:")
    print(f"  Device: {device}")
    print(f"  Embedding dim: {args.embedding_dim}")
    print(f"  Hidden dim: {args.hidden_dim}")
    print(f"  Num layers: {args.num_layers}")
    print(f"  Epochs: {args.epochs}")
    print(f"  Learning rate: {args.lr}\n")
    
    # Training loop
    best_acc = 0.0
    for epoch in range(1, args.epochs + 1):
        loss = train_epoch(model, data, optimizer, device)
        acc = evaluate(model, data, device)
        
        if acc > best_acc:
            best_acc = acc
            # Save best model
            os.makedirs(os.path.dirname(args.output), exist_ok=True)
            torch.save({
                'model_state_dict': model.state_dict(),
                'model_type': args.model,
                'embedding_dim': args.embedding_dim,
                'hidden_dim': args.hidden_dim,
                'num_layers': args.num_layers,
                'in_channels': in_channels,
                'metadata': metadata,
                'epoch': epoch,
                'accuracy': acc
            }, args.output)
        
        if epoch % 10 == 0:
            print(f"Epoch {epoch:03d}: Loss={loss:.4f}, Accuracy={acc:.4f}, Best={best_acc:.4f}")
    
    print(f"\nTraining complete! Best accuracy: {best_acc:.4f}")
    print(f"Model saved to: {args.output}")
    
    # Save metadata
    metadata_path = args.output.replace('.pth', '_metadata.json')
    with open(metadata_path, 'w') as f:
        json.dump({
            'model_type': args.model,
            'embedding_dim': args.embedding_dim,
            'hidden_dim': args.hidden_dim,
            'num_layers': args.num_layers,
            'best_accuracy': best_acc,
            **metadata
        }, f, indent=2)
    print(f"Metadata saved to: {metadata_path}")


if __name__ == '__main__':
    main()

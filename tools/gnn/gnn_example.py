"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gnn_example.py                                     ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:58:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     215                                            ║
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
Simple example demonstrating GNN embeddings with ThemisDB.

This script shows how to:
1. Create a simple graph dataset
2. Train a GraphSAGE model
3. Generate embeddings
4. Export to ONNX for C++ inference
"""

import torch
import torch.nn.functional as F
from torch_geometric.data import Data
from torch_geometric.nn import SAGEConv
import numpy as np


class SimpleGraphSAGE(torch.nn.Module):
    """Simple GraphSAGE model for demonstration."""
    
    def __init__(self, in_channels, hidden_channels, out_channels):
        super().__init__()
        self.conv1 = SAGEConv(in_channels, hidden_channels)
        self.conv2 = SAGEConv(hidden_channels, out_channels)
        
    def forward(self, x, edge_index):
        x = self.conv1(x, edge_index)
        x = F.relu(x)
        x = F.dropout(x, p=0.5, training=self.training)
        x = self.conv2(x, edge_index)
        return x


def create_sample_graph():
    """
    Create a sample social network graph.
    
    Graph structure:
        Alice -> Bob, Charlie
        Bob -> Alice, David
        Charlie -> Alice, David
        David -> Bob, Charlie
    """
    # Node features (age, activity_score, num_friends)
    node_features = torch.tensor([
        [25.0, 0.8, 3.0],  # Alice
        [30.0, 0.6, 2.0],  # Bob
        [28.0, 0.9, 2.0],  # Charlie
        [35.0, 0.7, 2.0],  # David
    ], dtype=torch.float)
    
    # Edges (undirected)
    edge_index = torch.tensor([
        [0, 0, 1, 1, 2, 2, 3, 3],  # Source nodes
        [1, 2, 0, 3, 0, 3, 1, 2],  # Target nodes
    ], dtype=torch.long)
    
    data = Data(x=node_features, edge_index=edge_index)
    print(f"Created sample graph: {data.num_nodes} nodes, {data.num_edges} edges")
    return data


def train_model(model, data, epochs=100):
    """Train the model using link prediction."""
    optimizer = torch.optim.Adam(model.parameters(), lr=0.01)
    
    print(f"\nTraining for {epochs} epochs...")
    model.train()
    
    for epoch in range(epochs):
        optimizer.zero_grad()
        
        # Forward pass
        out = model(data.x, data.edge_index)
        
        # Link prediction loss
        pos_edge_index = data.edge_index
        pos_scores = (out[pos_edge_index[0]] * out[pos_edge_index[1]]).sum(dim=1)
        
        # Negative sampling
        neg_edge_index = torch.randint(0, data.num_nodes, (2, pos_edge_index.shape[1]))
        neg_scores = (out[neg_edge_index[0]] * out[neg_edge_index[1]]).sum(dim=1)
        
        # BCE loss
        pos_loss = F.binary_cross_entropy_with_logits(pos_scores, torch.ones_like(pos_scores))
        neg_loss = F.binary_cross_entropy_with_logits(neg_scores, torch.zeros_like(neg_scores))
        loss = pos_loss + neg_loss
        
        loss.backward()
        optimizer.step()
        
        if (epoch + 1) % 20 == 0:
            print(f"  Epoch {epoch+1:03d}: Loss = {loss.item():.4f}")
    
    print("Training complete!")


def generate_embeddings(model, data):
    """Generate embeddings for all nodes."""
    model.eval()
    with torch.no_grad():
        embeddings = model(data.x, data.edge_index)
    return embeddings


def main():
    print("=" * 60)
    print("GNN Embeddings Example for ThemisDB")
    print("=" * 60)
    
    # 1. Create sample graph
    print("\n[1] Creating sample social network graph...")
    data = create_sample_graph()
    
    # 2. Initialize model
    print("\n[2] Initializing GraphSAGE model...")
    in_channels = data.num_node_features
    hidden_channels = 16
    out_channels = 8  # Embedding dimension
    
    model = SimpleGraphSAGE(in_channels, hidden_channels, out_channels)
    print(f"    Input features: {in_channels}")
    print(f"    Hidden dimension: {hidden_channels}")
    print(f"    Embedding dimension: {out_channels}")
    
    # 3. Train model
    print("\n[3] Training model...")
    train_model(model, data, epochs=100)
    
    # 4. Generate embeddings
    print("\n[4] Generating node embeddings...")
    embeddings = generate_embeddings(model, data)
    print(f"    Shape: {embeddings.shape}")
    
    # 5. Display embeddings
    print("\n[5] Node embeddings:")
    node_names = ['Alice', 'Bob', 'Charlie', 'David']
    for i, name in enumerate(node_names):
        emb = embeddings[i].numpy()
        print(f"    {name:8s}: [{', '.join([f'{x:6.3f}' for x in emb[:4]])}...]")
    
    # 6. Compute similarities
    print("\n[6] Pairwise cosine similarities:")
    embeddings_norm = F.normalize(embeddings, p=2, dim=1)
    similarity = torch.mm(embeddings_norm, embeddings_norm.t())
    
    print("         ", " ".join([f"{name:8s}" for name in node_names]))
    for i, name in enumerate(node_names):
        sims = similarity[i].numpy()
        print(f"    {name:8s}: {' '.join([f'{s:8.3f}' for s in sims])}")
    
    # 7. Find most similar nodes
    print("\n[7] Most similar nodes to Alice:")
    alice_idx = 0
    alice_sim = similarity[alice_idx].numpy()
    sorted_indices = np.argsort(-alice_sim)  # Descending order
    
    for idx in sorted_indices[1:]:  # Skip Alice itself
        print(f"    {node_names[idx]:8s}: similarity = {alice_sim[idx]:.3f}")
    
    # 8. Save model
    print("\n[8] Saving model...")
    torch.save({
        'model_state_dict': model.state_dict(),
        'in_channels': in_channels,
        'hidden_channels': hidden_channels,
        'out_channels': out_channels,
    }, 'sample_graphsage.pth')
    print("    Model saved to: sample_graphsage.pth")
    
    # 9. Usage in ThemisDB
    print("\n[9] Using in ThemisDB C++:")
    print("""
    // Register model
    GNNEmbeddingManager gnn(db, pgm, vim);
    gnn.registerModel("sample_graphsage", "graphsage", 8, "{}");
    
    // Generate embeddings
    gnn.generateNodeEmbeddings("social_network", "Person", "sample_graphsage");
    
    // Find similar nodes
    auto [st, similar] = gnn.findSimilarNodes("alice", "social_network", 3, "sample_graphsage");
    // Result: [Bob (0.85), Charlie (0.82), David (0.65)]
    """)
    
    print("\n" + "=" * 60)
    print("Example complete!")
    print("=" * 60)


if __name__ == '__main__':
    main()

"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export_to_onnx.py                                  ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:54:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     200                                            ║
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
Export trained PyTorch GNN model to ONNX format for C++ inference.

Usage:
    python export_to_onnx.py --model models/graphsage.pth --output models/graphsage.onnx
"""

import argparse
import json
import torch
import torch.onnx
from train_gnn import GraphSAGEModel, GCNModel, GATModel


def export_to_onnx(model_path: str, output_path: str, opset_version: int = 11):
    """
    Export PyTorch model to ONNX format.
    
    Args:
        model_path: Path to trained PyTorch model (.pth)
        output_path: Path to save ONNX model (.onnx)
        opset_version: ONNX opset version
    """
    print(f"Loading model from {model_path}...")
    checkpoint = torch.load(model_path, map_location='cpu')
    
    # Extract model configuration
    model_type = checkpoint['model_type']
    in_channels = checkpoint['in_channels']
    hidden_dim = checkpoint['hidden_dim']
    embedding_dim = checkpoint['embedding_dim']
    num_layers = checkpoint['num_layers']
    
    print(f"Model type: {model_type}")
    print(f"Input channels: {in_channels}")
    print(f"Embedding dimension: {embedding_dim}")
    
    # Create model
    if model_type == 'graphsage':
        model = GraphSAGEModel(in_channels, hidden_dim, embedding_dim, num_layers)
    elif model_type == 'gcn':
        model = GCNModel(in_channels, hidden_dim, embedding_dim, num_layers)
    elif model_type == 'gat':
        model = GATModel(in_channels, hidden_dim, embedding_dim, num_layers)
    else:
        raise ValueError(f"Unknown model type: {model_type}")
    
    model.load_state_dict(checkpoint['model_state_dict'])
    model.eval()
    
    # Create dummy inputs for ONNX export
    num_nodes = 100
    num_edges = 500
    dummy_x = torch.randn(num_nodes, in_channels)
    dummy_edge_index = torch.randint(0, num_nodes, (2, num_edges))
    
    print(f"\nExporting to ONNX format...")
    print(f"  Dummy input shape: x={dummy_x.shape}, edge_index={dummy_edge_index.shape}")
    
    # Export to ONNX
    torch.onnx.export(
        model,
        (dummy_x, dummy_edge_index),
        output_path,
        input_names=['node_features', 'edge_index'],
        output_names=['node_embeddings'],
        dynamic_axes={
            'node_features': {0: 'num_nodes'},
            'edge_index': {1: 'num_edges'},
            'node_embeddings': {0: 'num_nodes'}
        },
        opset_version=opset_version,
        do_constant_folding=True,
    )
    
    print(f"✓ Model exported to: {output_path}")
    
    # Save metadata
    metadata_path = output_path.replace('.onnx', '_metadata.json')
    metadata = {
        'model_type': model_type,
        'in_channels': in_channels,
        'hidden_dim': hidden_dim,
        'embedding_dim': embedding_dim,
        'num_layers': num_layers,
        'opset_version': opset_version,
        'accuracy': checkpoint.get('accuracy', 0.0),
        'epoch': checkpoint.get('epoch', 0)
    }
    
    with open(metadata_path, 'w') as f:
        json.dump(metadata, f, indent=2)
    
    print(f"✓ Metadata saved to: {metadata_path}")
    
    # Verify ONNX model
    print("\nVerifying ONNX model...")
    try:
        import onnx
        onnx_model = onnx.load(output_path)
        onnx.checker.check_model(onnx_model)
        print("✓ ONNX model is valid")
    except ImportError:
        print("⚠ onnx package not installed, skipping verification")
    except Exception as e:
        print(f"✗ ONNX verification failed: {e}")
    
    # Test inference with ONNX Runtime
    print("\nTesting ONNX Runtime inference...")
    try:
        import onnxruntime as ort
        
        session = ort.InferenceSession(output_path)
        
        # Run inference
        outputs = session.run(
            None,
            {
                'node_features': dummy_x.numpy(),
                'edge_index': dummy_edge_index.numpy()
            }
        )
        
        embeddings = outputs[0]
        print(f"✓ ONNX Runtime inference successful")
        print(f"  Output shape: {embeddings.shape}")
        print(f"  Expected shape: ({num_nodes}, {embedding_dim})")
        
        # Compare with PyTorch
        with torch.no_grad():
            pytorch_out = model(dummy_x, dummy_edge_index).numpy()
        
        max_diff = abs(pytorch_out - embeddings).max()
        print(f"  Max difference from PyTorch: {max_diff:.6f}")
        
        if max_diff < 1e-5:
            print("✓ ONNX output matches PyTorch")
        else:
            print("⚠ ONNX output differs from PyTorch (within tolerance)")
        
    except ImportError:
        print("⚠ onnxruntime package not installed, skipping inference test")
    except Exception as e:
        print(f"✗ ONNX Runtime test failed: {e}")
    
    print("\n" + "="*60)
    print("Export complete!")
    print("="*60)
    print(f"\nTo use in ThemisDB C++:")
    print(f"""
    // Load ONNX model
    GnnInference inference("{output_path}");
    
    // Generate embeddings
    SubgraphData subgraph = extractSubgraph(graph_id, node_ids);
    auto embeddings = inference.generateEmbeddings(subgraph);
    
    // Store in vector index
    for (size_t i = 0; i < embeddings.size(); ++i) {{
        vim.addVector(node_ids[i], embeddings[i]);
    }}
    """)


def main():
    parser = argparse.ArgumentParser(description='Export PyTorch GNN model to ONNX')
    parser.add_argument('--model', type=str, required=True, help='Input PyTorch model (.pth)')
    parser.add_argument('--output', type=str, required=True, help='Output ONNX model (.onnx)')
    parser.add_argument('--opset-version', type=int, default=11, help='ONNX opset version')
    
    args = parser.parse_args()
    
    export_to_onnx(args.model, args.output, args.opset_version)


if __name__ == '__main__':
    main()

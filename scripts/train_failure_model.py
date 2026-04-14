"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            train_failure_model.py                             ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:59:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     372                                            ║
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
ThemisDB Failure Prediction Model Training Pipeline

This script trains a machine learning model to predict shard failures
based on historical metrics data.

Requirements:
- pandas
- numpy
- scikit-learn
- xgboost or lightgbm
- onnx
- skl2onnx

Usage:
    python train_failure_model.py --data failure_data.csv --output model.onnx
"""

import argparse
import sys
from pathlib import Path

# Feature configuration
NUM_FEATURES = 50  # Number of features extracted from metrics

try:
    import pandas as pd
    import numpy as np
    from sklearn.model_selection import train_test_split
    from sklearn.preprocessing import StandardScaler
    from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score
except ImportError as e:
    print(f"Error: Required package not installed: {e}")
    print("Install with: pip install pandas numpy scikit-learn")
    sys.exit(1)


def load_data(data_path):
    """
    Load historical failure data from CSV.
    
    Expected columns:
    - shard_id: Shard identifier
    - timestamp: Observation time
    - avg_latency_ms: Average latency
    - p95_latency_ms: 95th percentile latency
    - p99_latency_ms: 99th percentile latency
    - throughput_ops: Operations per second
    - read_errors: Number of read errors
    - write_errors: Number of write errors
    - retry_count: Number of retries
    - failed_health_checks: Failed health check count
    - recovery_attempts: Recovery attempt count
    - recovery_success_rate: Success rate of recoveries
    - failed: Target variable (1 = failed within 7 days, 0 = healthy)
    """
    print(f"Loading data from {data_path}...")
    
    if not Path(data_path).exists():
        print(f"Error: Data file not found: {data_path}")
        print("\nTo use this training script, you need historical failure data.")
        print("Expected CSV format:")
        print("  shard_id,timestamp,avg_latency_ms,throughput_ops,...,failed")
        print("\nFor synthetic data generation, see:")
        print("  docs/ml/failure_prediction_data_collection.md")
        sys.exit(1)
    
    df = pd.read_csv(data_path)
    print(f"Loaded {len(df)} samples")
    
    return df


def extract_features(df):
    """
    Extract and engineer features from raw metrics.
    
    Features include:
    - Statistical features (mean, std, trend)
    - Recent values
    - Ratios and derived metrics
    """
    print("Extracting features...")
    
    features = []
    labels = []
    
    # Group by shard to compute temporal features
    for shard_id, group in df.groupby('shard_id'):
        if len(group) < 10:
            continue  # Need minimum history
        
        # Sort by timestamp
        group = group.sort_values('timestamp')
        
        # Extract last observation for target
        label = group.iloc[-1]['failed']
        
        # Compute statistical features
        feature_vector = []
        
        # Latency features
        feature_vector.append(group['avg_latency_ms'].mean())
        feature_vector.append(group['avg_latency_ms'].std())
        feature_vector.append(compute_trend(group['avg_latency_ms'].values))
        feature_vector.append(group['p95_latency_ms'].iloc[-1])
        feature_vector.append(group['p99_latency_ms'].iloc[-1])
        
        # Throughput features
        feature_vector.append(group['throughput_ops'].mean())
        feature_vector.append(group['throughput_ops'].std())
        feature_vector.append(compute_trend(group['throughput_ops'].values))
        feature_vector.append(group['throughput_ops'].iloc[-1])
        
        # Error rate features
        total_errors = group['read_errors'] + group['write_errors']
        feature_vector.append(total_errors.mean())
        feature_vector.append(total_errors.std())
        feature_vector.append(compute_trend(total_errors.values))
        feature_vector.append(group['read_errors'].iloc[-1])
        feature_vector.append(group['write_errors'].iloc[-1])
        
        # Health features
        feature_vector.append(group['failed_health_checks'].iloc[-1])
        feature_vector.append(group['recovery_attempts'].iloc[-1])
        feature_vector.append(group['recovery_success_rate'].iloc[-1])
        feature_vector.append(group['retry_count'].iloc[-1])
        
        # Pad to NUM_FEATURES
        while len(feature_vector) < NUM_FEATURES:
            feature_vector.append(0.0)
        
        features.append(feature_vector[:NUM_FEATURES])
        labels.append(label)
    
    print(f"Extracted {len(features)} feature vectors")
    
    return np.array(features), np.array(labels)


def compute_trend(values):
    """Compute linear regression slope as trend indicator."""
    if len(values) < 2:
        return 0.0
    
    x = np.arange(len(values))
    coeffs = np.polyfit(x, values, 1)
    return coeffs[0]


def train_model(X_train, y_train, model_type='sklearn'):
    """
    Train failure prediction model.
    
    Args:
        X_train: Training features
        y_train: Training labels
        model_type: 'sklearn' (simple) or 'xgboost' (advanced)
    
    Returns:
        Trained model
    """
    print(f"Training {model_type} model...")
    
    if model_type == 'xgboost':
        try:
            import xgboost as xgb
            
            model = xgb.XGBClassifier(
                n_estimators=100,
                max_depth=6,
                learning_rate=0.1,
                random_state=42
            )
        except ImportError:
            print("Warning: xgboost not installed, falling back to sklearn")
            model_type = 'sklearn'
    
    if model_type == 'sklearn':
        from sklearn.ensemble import RandomForestClassifier
        
        model = RandomForestClassifier(
            n_estimators=100,
            max_depth=10,
            random_state=42
        )
    
    model.fit(X_train, y_train)
    print("Training complete")
    
    return model


def evaluate_model(model, X_test, y_test):
    """Evaluate model performance."""
    print("\nEvaluating model...")
    
    y_pred = model.predict(X_test)
    
    accuracy = accuracy_score(y_test, y_pred)
    precision = precision_score(y_test, y_pred, zero_division=0)
    recall = recall_score(y_test, y_pred, zero_division=0)
    f1 = f1_score(y_test, y_pred, zero_division=0)
    
    print(f"Accuracy:  {accuracy:.2%}")
    print(f"Precision: {precision:.2%} (true positive rate)")
    print(f"Recall:    {recall:.2%} (sensitivity)")
    print(f"F1 Score:  {f1:.2%}")
    
    if precision < 0.7:
        print("\nWarning: Precision below 70% target")
    if recall < 0.7:
        print("\nWarning: Recall below 70% target")
    
    return {
        'accuracy': accuracy,
        'precision': precision,
        'recall': recall,
        'f1': f1
    }


def export_to_onnx(model, output_path, num_features=NUM_FEATURES):
    """
    Export trained model to ONNX format.
    
    Args:
        model: Trained sklearn model
        output_path: Path to save ONNX model
        num_features: Number of input features
    """
    print(f"\nExporting model to ONNX: {output_path}")
    
    try:
        from skl2onnx import convert_sklearn
        from skl2onnx.common.data_types import FloatTensorType
        
        initial_type = [('float_input', FloatTensorType([None, num_features]))]
        
        onnx_model = convert_sklearn(
            model,
            initial_types=initial_type,
            target_opset=12
        )
        
        with open(output_path, 'wb') as f:
            f.write(onnx_model.SerializeToString())
        
        print(f"Model exported successfully: {output_path}")
        
    except ImportError as e:
        print(f"Error: Cannot export to ONNX: {e}")
        print("Install with: pip install onnx skl2onnx")
        print("\nModel trained but not exported.")
        return False
    
    return True


def main():
    parser = argparse.ArgumentParser(
        description='Train ThemisDB failure prediction model'
    )
    parser.add_argument(
        '--data',
        type=str,
        default='failure_data.csv',
        help='Path to training data CSV'
    )
    parser.add_argument(
        '--output',
        type=str,
        default='../models/failure_prediction.onnx',
        help='Output path for ONNX model'
    )
    parser.add_argument(
        '--model-type',
        type=str,
        choices=['sklearn', 'xgboost'],
        default='sklearn',
        help='Model type to train'
    )
    parser.add_argument(
        '--test-size',
        type=float,
        default=0.2,
        help='Test set size (0.0-1.0)'
    )
    
    args = parser.parse_args()
    
    print("=" * 70)
    print("ThemisDB Failure Prediction Model Training")
    print("=" * 70)
    
    # Load and prepare data
    df = load_data(args.data)
    X, y = extract_features(df)
    
    if len(X) == 0:
        print("Error: No features extracted from data")
        sys.exit(1)
    
    print(f"\nDataset summary:")
    print(f"  Total samples: {len(X)}")
    print(f"  Features: {X.shape[1]}")
    print(f"  Failures: {np.sum(y)} ({np.mean(y)*100:.1f}%)")
    print(f"  Healthy: {len(y) - np.sum(y)} ({(1-np.mean(y))*100:.1f}%)")
    
    # Split data
    X_train, X_test, y_train, y_test = train_test_split(
        X, y,
        test_size=args.test_size,
        random_state=42,
        stratify=y
    )
    
    print(f"\nTrain set: {len(X_train)} samples")
    print(f"Test set: {len(X_test)} samples")
    
    # Normalize features
    scaler = StandardScaler()
    X_train = scaler.fit_transform(X_train)
    X_test = scaler.transform(X_test)
    
    # Train model
    model = train_model(X_train, y_train, args.model_type)
    
    # Evaluate
    metrics = evaluate_model(model, X_test, y_test)
    
    # Export to ONNX
    output_dir = Path(args.output).parent
    output_dir.mkdir(parents=True, exist_ok=True)
    
    success = export_to_onnx(model, args.output)
    
    print("\n" + "=" * 70)
    if success:
        print("Training complete! Model ready for deployment.")
    else:
        print("Training complete, but model export failed.")
    print("=" * 70)
    
    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())

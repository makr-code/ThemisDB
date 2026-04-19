> **Hinweis:** Inhalt ist konzeptuell/referenziell. Code-Bezüge mit `<!-- TODO: verify against source -->` markiert.

# ML Models - Anomalie-Erkennung

## 📋 Übersicht

Machine Learning Modelle für Anomalie-Erkennung in IoT-Sensor-Daten, einschließlich Training, Evaluierung und Online-Learning.

## 🎯 Anomalie-Typen

### 1. Point Anomalies
Einzelne Datenpunkte weichen stark ab.

**Beispiel:** Temperatursensor zeigt plötzlich 100°C

### 2. Contextual Anomalies
Werte sind normal, aber im Kontext ungewöhnlich.

**Beispiel:** 25°C ist normal, aber nicht um 3 Uhr nachts im Winter

### 3. Collective Anomalies
Sequenz von Werten ist ungewöhnlich.

**Beispiel:** Kontinuierlicher Anstieg über mehrere Stunden

## 🔧 Feature Engineering

### Time-Series Features

```python
import numpy as np
import pandas as pd
from scipy import stats

class TimeSeriesFeatureExtractor:
    """Extrahiert Features aus Time-Series Daten."""
    
    @staticmethod
    def extract_features(values: np.array, window_size: int = 10) -> dict:
        """Extrahiert statistische Features."""
        features = {}
        
        # Basis-Statistiken
        features['mean'] = np.mean(values)
        features['std'] = np.std(values)
        features['min'] = np.min(values)
        features['max'] = np.max(values)
        features['median'] = np.median(values)
        
        # Perzentile
        features['q25'] = np.percentile(values, 25)
        features['q75'] = np.percentile(values, 75)
        features['iqr'] = features['q75'] - features['q25']
        
        # Trend
        if len(values) > 1:
            x = np.arange(len(values))
            slope, _, _, _, _ = stats.linregress(x, values)
            features['trend'] = slope
        else:
            features['trend'] = 0
        
        # Variabilität
        if len(values) > 1:
            features['cv'] = features['std'] / features['mean'] if features['mean'] != 0 else 0
        else:
            features['cv'] = 0
        
        # Rate of Change
        if len(values) > 1:
            changes = np.diff(values)
            features['mean_change'] = np.mean(changes)
            features['std_change'] = np.std(changes)
            features['max_change'] = np.max(np.abs(changes))
        
        # Moving Average Abweichung
        if len(values) >= window_size:
            ma = np.convolve(values, np.ones(window_size)/window_size, mode='valid')
            features['ma_deviation'] = np.mean(np.abs(values[-len(ma):] - ma))
        
        return features
    
    @staticmethod
    def extract_temporal_features(timestamp: pd.Timestamp) -> dict:
        """Extrahiert zeitliche Features."""
        return {
            'hour': timestamp.hour,
            'day_of_week': timestamp.dayofweek,
            'day_of_month': timestamp.day,
            'month': timestamp.month,
            'is_weekend': 1 if timestamp.dayofweek >= 5 else 0,
            'is_business_hours': 1 if 9 <= timestamp.hour <= 17 else 0
        }
    
    @staticmethod
    def extract_lag_features(values: np.array, lags: list = [1, 2, 3, 6, 12, 24]) -> dict:
        """Extrahiert Lag-Features."""
        features = {}
        for lag in lags:
            if len(values) > lag:
                features[f'lag_{lag}'] = values[-lag]
                features[f'diff_lag_{lag}'] = values[-1] - values[-lag]
        return features

# Verwendung
extractor = TimeSeriesFeatureExtractor()
values = np.array([20, 21, 22, 21, 23, 24, 23, 25, 26, 25])
features = extractor.extract_features(values)
```

### Sensor Context Features

```python
class SensorContextFeatures:
    """Extrahiert Kontext-Features von Sensoren."""
    
    @staticmethod
    def extract_location_features(location: dict) -> dict:
        """Extrahiert Standort-Features."""
        return {
            'latitude': location['lat'],
            'longitude': location['lon'],
            'altitude': location.get('altitude', 0),
            'location_cluster': SensorContextFeatures._get_location_cluster(location)
        }
    
    @staticmethod
    def _get_location_cluster(location: dict) -> int:
        """Bestimmt Standort-Cluster (z.B. Indoor/Outdoor, Region)."""
        # Vereinfachte Implementierung
        lat_bucket = int(location['lat'] * 10) % 10
        lon_bucket = int(location['lon'] * 10) % 10
        return lat_bucket * 10 + lon_bucket
    
    @staticmethod
    def extract_sensor_metadata_features(metadata: dict) -> dict:
        """Extrahiert Sensor-Metadaten Features."""
        return {
            'battery_level': metadata.get('battery_level', 100) / 100.0,
            'signal_strength': (metadata.get('signal_strength', -50) + 100) / 100.0,
            'age_days': metadata.get('age_days', 0) / 365.0
        }
```

## 🤖 Anomalie-Erkennungs-Modelle

### 1. Isolation Forest

Gut für hochdimensionale Daten, schnell:

```python
from sklearn.ensemble import IsolationForest
from sklearn.preprocessing import StandardScaler
import joblib

class IsolationForestDetector:
    """Anomalie-Erkennung mit Isolation Forest."""
    
    def __init__(self, contamination: float = 0.1, n_estimators: int = 100):
        self.model = IsolationForest(
            contamination=contamination,
            n_estimators=n_estimators,
            random_state=42,
            n_jobs=-1
        )
        self.scaler = StandardScaler()
        self.feature_names = None
        self.is_fitted = False
    
    def fit(self, X: np.array, feature_names: list = None):
        """Trainiert Modell."""
        X_scaled = self.scaler.fit_transform(X)
        self.model.fit(X_scaled)
        self.feature_names = feature_names
        self.is_fitted = True
    
    def predict(self, X: np.array) -> np.array:
        """Prediziert Anomalien (-1 = Anomalie, 1 = Normal)."""
        if not self.is_fitted:
            raise ValueError("Model not fitted yet")
        
        X_scaled = self.scaler.transform(X)
        return self.model.predict(X_scaled)
    
    def score(self, X: np.array) -> np.array:
        """Gibt Anomalie-Scores zurück (negativer = anomaler)."""
        if not self.is_fitted:
            raise ValueError("Model not fitted yet")
        
        X_scaled = self.scaler.transform(X)
        return self.model.score_samples(X_scaled)
    
    def save(self, path: str):
        """Speichert Modell."""
        joblib.dump({
            'model': self.model,
            'scaler': self.scaler,
            'feature_names': self.feature_names
        }, path)
    
    def load(self, path: str):
        """Lädt Modell."""
        data = joblib.load(path)
        self.model = data['model']
        self.scaler = data['scaler']
        self.feature_names = data['feature_names']
        self.is_fitted = True

# Verwendung
detector = IsolationForestDetector(contamination=0.05)

# Training data
X_train = np.random.randn(1000, 10)  # 1000 samples, 10 features
detector.fit(X_train)

# Predict
X_test = np.random.randn(100, 10)
predictions = detector.predict(X_test)
scores = detector.score(X_test)

anomalies = X_test[predictions == -1]
print(f"Found {len(anomalies)} anomalies")
```

### 2. LSTM Autoencoder

Gut für sequentielle Daten:

```python
import torch
import torch.nn as nn

class LSTMAutoencoder(nn.Module):
    """LSTM Autoencoder für Time-Series Anomalie-Erkennung."""
    
    def __init__(
        self,
        input_dim: int,
        hidden_dim: int = 64,
        num_layers: int = 2,
        dropout: float = 0.2
    ):
        super(LSTMAutoencoder, self).__init__()
        
        # Encoder
        self.encoder = nn.LSTM(
            input_dim,
            hidden_dim,
            num_layers,
            batch_first=True,
            dropout=dropout
        )
        
        # Decoder
        self.decoder = nn.LSTM(
            hidden_dim,
            input_dim,
            num_layers,
            batch_first=True,
            dropout=dropout
        )
    
    def forward(self, x):
        """Forward pass."""
        # Encode
        encoded, (hidden, cell) = self.encoder(x)
        
        # Decode
        decoded, _ = self.decoder(encoded, (hidden, cell))
        
        return decoded

class LSTMDetector:
    """Wrapper für LSTM Autoencoder Anomalie-Erkennung."""
    
    def __init__(
        self,
        input_dim: int,
        hidden_dim: int = 64,
        learning_rate: float = 0.001,
        device: str = 'cpu'
    ):
        self.device = device
        self.model = LSTMAutoencoder(input_dim, hidden_dim).to(device)
        self.criterion = nn.MSELoss()
        self.optimizer = torch.optim.Adam(self.model.parameters(), lr=learning_rate)
        self.threshold = None
    
    def train(
        self,
        X_train: np.array,
        epochs: int = 100,
        batch_size: int = 32
    ):
        """Trainiert Modell."""
        self.model.train()
        
        X_tensor = torch.FloatTensor(X_train).to(self.device)
        dataset = torch.utils.data.TensorDataset(X_tensor)
        dataloader = torch.utils.data.DataLoader(
            dataset,
            batch_size=batch_size,
            shuffle=True
        )
        
        for epoch in range(epochs):
            epoch_loss = 0
            for batch in dataloader:
                x = batch[0]
                
                self.optimizer.zero_grad()
                reconstructed = self.model(x)
                loss = self.criterion(reconstructed, x)
                loss.backward()
                self.optimizer.step()
                
                epoch_loss += loss.item()
            
            if (epoch + 1) % 10 == 0:
                print(f"Epoch {epoch+1}/{epochs}, Loss: {epoch_loss/len(dataloader):.6f}")
        
        # Berechne Threshold basierend auf Training-Daten
        self.model.eval()
        with torch.no_grad():
            reconstructed = self.model(X_tensor)
            reconstruction_errors = torch.mean((X_tensor - reconstructed) ** 2, dim=(1, 2))
            self.threshold = torch.quantile(reconstruction_errors, 0.95).item()
    
    def predict(self, X: np.array) -> tuple:
        """Prediziert Anomalien."""
        self.model.eval()
        
        X_tensor = torch.FloatTensor(X).to(self.device)
        
        with torch.no_grad():
            reconstructed = self.model(X_tensor)
            reconstruction_errors = torch.mean(
                (X_tensor - reconstructed) ** 2,
                dim=(1, 2)
            ).cpu().numpy()
        
        predictions = (reconstruction_errors > self.threshold).astype(int)
        predictions = np.where(predictions == 1, -1, 1)  # -1 = anomaly, 1 = normal
        
        return predictions, reconstruction_errors

# Verwendung
detector = LSTMDetector(input_dim=1, hidden_dim=32)

# Training data (sequences of length 10)
X_train = np.random.randn(1000, 10, 1)
detector.train(X_train, epochs=50)

# Predict
X_test = np.random.randn(100, 10, 1)
predictions, scores = detector.predict(X_test)
```

### 3. One-Class SVM

Gut für kleine Datasets:

```python
from sklearn.svm import OneClassSVM

class OneClassSVMDetector:
    """Anomalie-Erkennung mit One-Class SVM."""
    
    def __init__(self, kernel: str = 'rbf', nu: float = 0.1):
        self.model = OneClassSVM(kernel=kernel, nu=nu, gamma='auto')
        self.scaler = StandardScaler()
        self.is_fitted = False
    
    def fit(self, X: np.array):
        """Trainiert Modell."""
        X_scaled = self.scaler.fit_transform(X)
        self.model.fit(X_scaled)
        self.is_fitted = True
    
    def predict(self, X: np.array) -> np.array:
        """Prediziert Anomalien."""
        if not self.is_fitted:
            raise ValueError("Model not fitted yet")
        
        X_scaled = self.scaler.transform(X)
        return self.model.predict(X_scaled)
    
    def decision_function(self, X: np.array) -> np.array:
        """Gibt Decision-Scores zurück."""
        if not self.is_fitted:
            raise ValueError("Model not fitted yet")
        
        X_scaled = self.scaler.transform(X)
        return self.model.decision_function(X_scaled)
```

## 📊 Model Evaluation

### Metriken

```python
from sklearn.metrics import (
    precision_score,
    recall_score,
    f1_score,
    confusion_matrix,
    roc_auc_score,
    classification_report
)

class AnomalyDetectionEvaluator:
    """Evaluiert Anomalie-Erkennungs-Modelle."""
    
    @staticmethod
    def evaluate(y_true: np.array, y_pred: np.array, y_scores: np.array = None) -> dict:
        """Berechnet Evaluation-Metriken."""
        # Konvertiere zu binär (1 = normal, 0 = anomaly)
        y_true_binary = np.where(y_true == -1, 0, 1)
        y_pred_binary = np.where(y_pred == -1, 0, 1)
        
        metrics = {
            'precision': precision_score(y_true_binary, y_pred_binary, zero_division=0),
            'recall': recall_score(y_true_binary, y_pred_binary, zero_division=0),
            'f1': f1_score(y_true_binary, y_pred_binary, zero_division=0),
        }
        
        # Confusion Matrix
        cm = confusion_matrix(y_true_binary, y_pred_binary)
        metrics['confusion_matrix'] = cm.tolist()
        
        # True Negatives, False Positives, False Negatives, True Positives
        if cm.shape == (2, 2):
            tn, fp, fn, tp = cm.ravel()
            metrics['true_negatives'] = int(tn)
            metrics['false_positives'] = int(fp)
            metrics['false_negatives'] = int(fn)
            metrics['true_positives'] = int(tp)
        
        # ROC AUC (falls Scores vorhanden)
        if y_scores is not None:
            try:
                metrics['roc_auc'] = roc_auc_score(y_true_binary, -y_scores)
            except:
                pass
        
        return metrics
    
    @staticmethod
    def print_report(y_true: np.array, y_pred: np.array):
        """Druckt detaillierten Report."""
        y_true_binary = np.where(y_true == -1, 0, 1)
        y_pred_binary = np.where(y_pred == -1, 0, 1)
        
        print(classification_report(
            y_true_binary,
            y_pred_binary,
            target_names=['Anomaly', 'Normal']
        ))

# Verwendung
evaluator = AnomalyDetectionEvaluator()

# Simuliere Predictions
y_true = np.array([1, 1, 1, -1, 1, -1, 1, 1, 1, -1])
y_pred = np.array([1, 1, -1, -1, 1, 1, 1, 1, 1, -1])
y_scores = np.random.randn(10)

metrics = evaluator.evaluate(y_true, y_pred, y_scores)
print(f"Precision: {metrics['precision']:.2f}")
print(f"Recall: {metrics['recall']:.2f}")
print(f"F1: {metrics['f1']:.2f}")
```

### Cross-Validation

```python
from sklearn.model_selection import TimeSeriesSplit

class TimeSeriesCrossValidator:
    """Cross-Validation für Time-Series."""
    
    @staticmethod
    def validate(
        model_class,
        X: np.array,
        y: np.array,
        n_splits: int = 5,
        **model_kwargs
    ) -> dict:
        """Führt Time-Series CV aus."""
        tscv = TimeSeriesSplit(n_splits=n_splits)
        
        scores = {
            'precision': [],
            'recall': [],
            'f1': []
        }
        
        for train_idx, test_idx in tscv.split(X):
            X_train, X_test = X[train_idx], X[test_idx]
            y_train, y_test = y[train_idx], y[test_idx]
            
            # Train model
            model = model_class(**model_kwargs)
            model.fit(X_train)
            
            # Predict
            y_pred = model.predict(X_test)
            
            # Evaluate
            evaluator = AnomalyDetectionEvaluator()
            metrics = evaluator.evaluate(y_test, y_pred)
            
            scores['precision'].append(metrics['precision'])
            scores['recall'].append(metrics['recall'])
            scores['f1'].append(metrics['f1'])
        
        # Durchschnitt
        return {
            'precision_mean': np.mean(scores['precision']),
            'precision_std': np.std(scores['precision']),
            'recall_mean': np.mean(scores['recall']),
            'recall_std': np.std(scores['recall']),
            'f1_mean': np.mean(scores['f1']),
            'f1_std': np.std(scores['f1'])
        }
```

## 🔄 Online Learning

### Incremental Learning

```python
class IncrementalAnomalyDetector:
    """Inkrementelles Lernen für Anomalie-Erkennung."""
    
    def __init__(self, base_model, update_frequency: int = 100):
        self.model = base_model
        self.update_frequency = update_frequency
        self.samples_seen = 0
        self.buffer = []
        self.buffer_size = 1000
    
    def predict_and_update(self, X: np.array) -> np.array:
        """Prediziert und updated Modell inkrementell."""
        # Predict
        predictions = self.model.predict(X)
        
        # Buffer für Re-Training
        self.buffer.extend(X)
        if len(self.buffer) > self.buffer_size:
            self.buffer = self.buffer[-self.buffer_size:]
        
        self.samples_seen += len(X)
        
        # Update Model periodisch
        if self.samples_seen % self.update_frequency == 0:
            self._retrain()
        
        return predictions
    
    def _retrain(self):
        """Re-trainiert Modell mit Buffer."""
        if len(self.buffer) >= 100:
            X_retrain = np.array(self.buffer)
            self.model.fit(X_retrain)
            print(f"Model retrained with {len(self.buffer)} samples")

# Verwendung
base_model = IsolationForestDetector(contamination=0.05)
base_model.fit(np.random.randn(1000, 10))

incremental_detector = IncrementalAnomalyDetector(base_model, update_frequency=100)

# Stream processing
for i in range(10):
    X_batch = np.random.randn(50, 10)
    predictions = incremental_detector.predict_and_update(X_batch)
```

### Adaptive Threshold

```python
class AdaptiveThresholdDetector:
    """Adaptiver Threshold basierend auf Rolling Statistics."""
    
    def __init__(self, window_size: int = 100, n_std: float = 3.0):
        self.window_size = window_size
        self.n_std = n_std
        self.value_buffer = deque(maxlen=window_size)
    
    def predict(self, value: float) -> int:
        """Prediziert ob Wert anomal ist."""
        if len(self.value_buffer) < 10:
            # Zu wenig Daten
            self.value_buffer.append(value)
            return 1  # Normal
        
        # Berechne Rolling Statistics
        mean = np.mean(self.value_buffer)
        std = np.std(self.value_buffer)
        
        # Check if value is outside n_std
        is_anomaly = abs(value - mean) > (self.n_std * std)
        
        # Update buffer (nur wenn nicht anomal)
        if not is_anomaly:
            self.value_buffer.append(value)
        
        return -1 if is_anomaly else 1

# Verwendung
detector = AdaptiveThresholdDetector(window_size=50, n_std=3.0)

for value in [20, 21, 22, 100, 21, 22, 23]:  # 100 ist Anomalie
    prediction = detector.predict(value)
    if prediction == -1:
        print(f"Anomaly detected: {value}")
```

## 🎯 Model Selection Guide

| Modell | Pros | Cons | Use Case |
|--------|------|------|----------|
| **Isolation Forest** | Schnell, skalierbar, gut für hochdim. Daten | Keine sequentielle Information | Real-time, viele Features |
| **LSTM Autoencoder** | Erfasst zeitliche Abhängigkeiten | Langsamer, braucht viel Daten | Time-Series, Sequenzen |
| **One-Class SVM** | Robust, flexibel | Langsam bei großen Daten | Kleine Datasets |
| **Adaptive Threshold** | Einfach, schnell | Nur univariat | Simple Sensor-Monitoring |

## 🎓 Best Practices

1. **Feature Engineering**
   - Nutze Domain-Wissen
   - Kombiniere statistische + temporale Features
   - Normalisiere Features

2. **Training**
   - Verwende nur normale Daten für Training
   - Validiere mit realistischen Anomalien
   - Re-trainiere regelmäßig

3. **Threshold-Tuning**
   - Balance zwischen False Positives und False Negatives
   - Verwende Business-Kontext für Threshold
   - Adaptiere Threshold dynamisch

4. **Production**
   - Monitore Model-Performance kontinuierlich
   - Implementiere A/B-Testing für neue Modelle
   - Logge Predictions für Analyse

## 📚 Weitere Dokumentation

- [SENSOR_SIMULATION.md](SENSOR_SIMULATION.md) - Sensor-Setup
- [CEP_PATTERNS.md](CEP_PATTERNS.md) - Event Processing
- [SCALING_GUIDE.md](SCALING_GUIDE.md) - Skalierung

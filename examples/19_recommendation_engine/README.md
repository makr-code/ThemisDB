> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Recommendation Engine - ML-basierte Empfehlungen mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-complex-red)
![Duration](https://img.shields.io/badge/duration-90--120%20min-blue)

## 📝 Übersicht

Die Recommendation Engine demonstriert ML-Integration mit ThemisDB. Sie lernen:
- User Behavior Tracking
- Collaborative Filtering
- Content-Based Filtering
- Hybrid Recommendations
- A/B Testing
- Real-Time Personalization

## ✨ Features

- ✅ **Collaborative Filtering** - User-Item Matrix
- ✅ **Content-Based** - Feature Similarity
- ✅ **Hybrid Approach** - Beste aus beiden
- ✅ **Behavior Tracking** - Clicks, Views, Purchases
- ✅ **A/B Testing** - Algorithmus-Vergleich
- ✅ **Real-Time Updates** - Live-Personalisierung
- ✅ **Evaluation Metrics** - Precision, Recall
- ✅ **Cold Start** - Für neue User/Items

## 📊 Datenmodell

### Interaction

```json
{
  "id": "int_uuid",
  "user_id": "user_uuid",
  "item_id": "item_uuid",
  "type": "purchase",
  "rating": 5,
  "timestamp": "2025-12-22T15:00:00Z",
  "context": {"device": "mobile", "location": "home"}
}
```

### User

```json
{
  "id": "user_uuid",
  "preferences": ["sci-fi", "action", "drama"],
  "demographics": {"age": 28, "gender": "f"},
  "behavior_vector": [0.8, 0.3, 0.6, ...]
}
```

### Item

```json
{
  "id": "item_uuid",
  "title": "Product Name",
  "category": "Electronics",
  "features": ["feature1", "feature2"],
  "embedding": [0.2, 0.7, 0.4, ...]
}
```

## 🛠️ ThemisDB Features

- **Graph** für User-Item Relationships
- **Vector Search** für Content-Based
- **Time-Series** für Behavior Tracking
- **ML Integration** für Training

## 🔗 Navigation

- ⬅️ [18 - Real-Time Chat](../18_realtime_chat/)
- ➡️ [20 - Smart Home](../20_smart_home/)
- 🏠 [Übersicht](../README.md)

---

**Status**: Ready | **Letzte Aktualisierung**: 2025-12-22

> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Real-Time Chat Application - Echtzeit-Kommunikation mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-complex-red)
![Duration](https://img.shields.io/badge/duration-90--120%20min-blue)

## 📝 Übersicht

Die Real-Time Chat Application zeigt Echtzeit-Features von ThemisDB. Sie lernen:
- Multi-User Chat Rooms
- Direct Messages
- Message History
- Typing Indicators
- File Sharing
- Message Search
- Online/Offline Status

## ✨ Features

- ✅ **Chat Rooms** - Öffentliche und private Räume
- ✅ **Direct Messages** - 1:1 Kommunikation
- ✅ **Message History** - Vollständige Historie
- ✅ **Typing Indicators** - "User is typing..."
- ✅ **File Sharing** - Bilder und Dokumente
- ✅ **Message Search** - Full-Text Suche
- ✅ **Online Status** - Presence Tracking
- ✅ **Read Receipts** - Lesebestätigungen
- ✅ **Reactions** - Emoji-Reaktionen
- ✅ **Threading** - Antworten auf Messages

## 📊 Datenmodell

### Message

```json
{
  "id": "msg_uuid",
  "room_id": "room_uuid",
  "sender_id": "user_uuid",
  "content": "Hello everyone!",
  "type": "text",
  "timestamp": "2025-12-22T14:30:00Z",
  "edited": false,
  "reactions": {"👍": 3, "❤️": 1},
  "thread_id": null,
  "attachments": []
}
```

### Room

```json
{
  "id": "room_uuid",
  "name": "General",
  "type": "public",
  "members": ["user1", "user2"],
  "created_by": "user1",
  "created_at": "2025-12-01",
  "last_activity": "2025-12-22T14:30:00Z"
}
```

## 🛠️ ThemisDB Features

- **Pub/Sub** für Real-Time Messages
- **Time-Series** für Message History
- **Full-Text Search** für Messages
- **Real-Time Updates** für Presence

## 🔗 Navigation

- ⬅️ [17 - CRM](../17_crm/)
- ➡️ [19 - Recommendation Engine](../19_recommendation_engine/)
- 🏠 [Übersicht](../README.md)

---

**Status**: Ready | **Letzte Aktualisierung**: 2025-12-22

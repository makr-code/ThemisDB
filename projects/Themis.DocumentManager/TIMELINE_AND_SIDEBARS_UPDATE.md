# Timeline und Sidebar UI-Update

## Übersicht
Das Themis Document Manager UI wurde mit modernen, professionellen Komponenten erweitert, um eine bessere Benutzerführung und Datenvisualisierung zu unterstützen.

## 🎯 Neue Komponenten

### 1. **Dynamische Timeline (unter Ribbon-Toolbar)**

#### Features:
- **Weiße Hintergrund-Zone** mit visueller Zeitskala
- **Skalierbare Zeitzonen** (1 Woche, 1 Monat, 3 Monate, 1 Jahr)
- **Farbcodierte Zeitbadges** für verschiedene Objekttypen:
  - 📥 **Blau**: Inbox-Elemente
  - ⏰ **Orange**: Fristen/Deadlines
  - ⚙️ **Lila**: Prozesse
  - ✅ **Grün**: Genehmigungen

#### Layout:
```
┌─────────────────────────────────────────────────────┐
│  📅 Timeline    [Zeitskala ▼]   Mo  Di  Mi  Do ...  │
├─────────────────────────────────────────────────────┤
│  09:30 📧 E-Mail eintreffen              von ABC... │
│  11:15 ⏰ Frist: Genehmigung              in 5 T... │
│  14:45 ⚙️ Prozess gestartet                        │
│  16:20 ✅ Genehmigt von Max Mustermann             │
└─────────────────────────────────────────────────────┘
```

#### Funktionalität:
- **Horizontal scrollbar** für lange Zeiträume
- **Hover-Effekte** für Details
- **Farb-Kodierung** für schnelle Erkennung von Elementtypen
- **Zeitangaben** mit Uhrzeit und Kontext

---

### 2. **Erweiterte Rechte Sidebar mit Tabs**

#### Tab 1: **🔗 Graph - 3D Verbindungen**

Zeigt Beziehungen zwischen Dokumenten, Personen und Prozessen:

```
┌─────────────────┐
│ Graph Rendering │
│  (3D Visualizer)│
│  ThreeJS/Babylon│
│  (Placeholder)  │
└─────────────────┘

Verbindungs-Info:
├ Knoten: 15
├ Kanten: 28
└ Zentral: Dokument_001

Verbundene Elemente:
├ 📄 Document_002
├ 👤 Max Mustermann
├ ⚙️ Process_ABC
└ 📅 2025-01-15
```

**Geplante Implementierung:**
- ThreeJS oder Babylon.js für interaktive 3D-Graphen-Visualisierung
- Zoom/Pan-Funktionalität
- Knoten-Auswahl mit Kontextmenü
- Real-time Update bei Dokumentenänderungen

---

#### Tab 2: **💬 AI-Chat - VSCode MCP/SSE Style**

Chatbot-Interface für Fragen und kontextuelle Hilfe:

```
┌──────────────────────────────┐
│  Chat Messages (Scrollable)   │
├──────────────────────────────┤
│ Was sind nächsten Schritte?  │ (User - Right)
│                              │
│ Basierend auf Prozess:       │ (AI - Left)
│ 1. Dokumentation prüfen      │
│ 2. Genehmigung einholen      │
│ 3. Archivierung              │
│                              │
│ 💡 Empfohlene Aktionen:      │ (MCP Actions)
│ [📋 Checkliste] [✉️ Notify] │
├──────────────────────────────┤
│ [Frage stellen...        ] 📤│
└──────────────────────────────┘
```

**Features:**
- **VSCode MCP/SSE Streaming** für echtzeitliche Antworten
- **Kontextbewusst**: Prozess, Dokument und Benutzer-bezogene Antworten
- **Recommended Actions**: Direkte Buttons für häufige Aktionen
- **Message History**: Alle Konversationen speichern

**Geplante Integrationen:**
- OpenAI GPT-4 / Claude
- Prozess-Kontext automatisch einbinden
- Dokumenten-Kontext aus Datenbank
- Actions via MCP (Model Context Protocol)

---

## 🏗️ Technische Architektur

### Layout-Struktur:
```
Main Window
├── Menu Bar (Datei, Ansicht)
├── Ribbon Toolbar (Start, Einfügen, Ansicht, Module)
├── TIMELINE ROW (Full Width)
│   └── Dynamic Badges (Color-coded, Scrollable)
├── CONTENT ROW (3 Columns)
│   ├── Left Sidebar (250px, Navigation)
│   ├── Center (*, Main Content Tabs)
│   └── Right Sidebar (350px, Graph + Chat Tabs)
└── Status Bar (Version, Status Message)
```

### XAML-Komponenten:
- **Timeline Section**: Border + Grid + StackPanel (ScrollViewer für Badges)
- **Right Sidebar Tabs**: RadioButton Group für Tab-Navigation
- **Graph Tab**: Border mit ScrollViewer für verbundene Elemente
- **Chat Tab**: Grid mit Message-Area und Input-Bereich

### Styling:
- **Modern WPF**: Keine WinUI-spezifischen Attribute (kein CornerRadius)
- **Farben**: 
  - Hintergrund: System DynamicResources
  - Badges: Branding-Farben (#2196f3, #ff9800, #9c27b0, #4caf50)
  - Text: Hoher Kontrast für Lesbarkeit

---

## 📊 TimelineModels Integration

Bestehende Models aus `TimelineModels.cs`:
```csharp
// Bereits implementiert und verfügbar:
public class TimelineItem
{
    public string ObjectId { get; set; }
    public TimelineObjectType ObjectType { get; set; }
    public DateTime Date { get; set; }
    public TimelinePriority Priority { get; set; }
    public string Color { get; set; }
    // ... weitere Eigenschaften
}

public enum TimelineObjectType
{
    Inbox, Deadline, Task, Approval, Document, 
    Process, Workflow, Comment, Milestone, Alert
}
```

Die UI nutzt diese Models zur automatischen Farbcodierung und Visualisierung.

---

## 💬 AIChatModels Integration

Bestehende Models aus `AIAssistantModels.cs`:
```csharp
public class AIChatSession
{
    public List<AIChatMessage> Messages { get; set; }
    public string? ProcessId { get; set; }
    public string? DocumentId { get; set; }
    // ... weitere Eigenschaften
}

public class AIChatMessage
{
    public ChatMessageRole Role { get; set; }  // System, User, Assistant, Tool
    public string Content { get; set; }
    public List<ChatAction> Actions { get; set; }  // MCP Actions
}
```

---

## 🔄 Event Handler (Code-Behind)

### `RightSidebarTab_Click()`
```csharp
private void RightSidebarTab_Click(object sender, RoutedEventArgs e)
{
    // Hide all right sidebar content panels
    RightSidebarGraphContent.Visibility = Visibility.Collapsed;
    RightSidebarChatContent.Visibility = Visibility.Collapsed;

    // Show the selected tab content
    if (sender == RightTabGraph)
        RightSidebarGraphContent.Visibility = Visibility.Visible;
    else if (sender == RightTabChat)
        RightSidebarChatContent.Visibility = Visibility.Visible;
}
```

---

## 📋 Nächste Schritte (Geplant)

### Phase 1: Datenbindung
- [ ] TimelineViewModel mit TimelineItems laden
- [ ] GraphViewModel für 3D-Knoten initialisieren
- [ ] AIChatViewModel für Konversationen

### Phase 2: 3D Graph Rendering
- [ ] ThreeJS-Integration für 3D-Visualization
- [ ] Knoten-Positionen berechnen (Force-directed Graph)
- [ ] Interaktive Steuerung (Zoom, Rotation, Pan)

### Phase 3: AI-Chat Integration
- [ ] OpenAI API / Claude Integration
- [ ] SSE/Streaming für Live-Responses
- [ ] MCP Action-Handler
- [ ] Kontext-Injection (Dokument, Prozess, Benutzer)

### Phase 4: Erweiterte Timeline-Features
- [ ] Echtzeit-Updates
- [ ] Filter und Suche
- [ ] Timeline-Selektion für Content-Synchronisation
- [ ] Prozess-aware Highlighting

---

## 🎨 Farb-Schema

| Komponente | Farbe | Bedeutung |
|-----------|-------|----------|
| Timeline Badge - Inbox | #e3f2fd / #2196f3 | Eingehende Elemente |
| Timeline Badge - Deadline | #fff3e0 / #ff9800 | Zeitkritisch |
| Timeline Badge - Process | #f3e5f5 / #9c27b0 | Workflows |
| Timeline Badge - Approval | #e8f5e9 / #4caf50 | Genehmigt/Autorisiert |
| Chat - User Message | #2196f3 | Benutzer-Input |
| Chat - AI Message | #f5f5f5 | Assistent-Output |
| MCP Actions Box | #fffacd | Recommended Actions |

---

## 📱 Responsive Design

- **Timeline**: Vollständig responsiv mit ScrollViewer
- **Sidebars**: Toggelbar über Menu > Ansicht > {Linke/Rechte} Seitenleiste
- **Minimum Window Size**: 1000px breit x 600px hoch
- **Default Window State**: Maximiert

---

## 🔧 Bekannte Limitationen (WPF Standard)

- ❌ `CornerRadius`: Nicht nativ in WPF (WinUI-Feature) → Entfernt
- ❌ `PlaceholderText`: TextBox verwendet `Text` stattdessen
- ⚠️ 3D-Rendering: Benötigt externe Library (ThreeJS/Babylon.js über WebView)

---

## 📚 Referenzen

- **TimelineView.xaml**: Vollständige Timeline-Komponenten
- **TimelineViewModel.cs**: ViewModel mit laden/filtern Logik
- **AIAssistantModels.cs**: Datenstrukturen für Chat
- **MainWindow.xaml**: Neue Layout-Struktur
- **MainWindow.xaml.cs**: Event-Handler für Tab-Navigation

---

**Version**: 1.0.0  
**Datum**: Dezember 2025  
**Status**: ✅ UI-Implementierung abgeschlossen, Datenbindung ausstehend

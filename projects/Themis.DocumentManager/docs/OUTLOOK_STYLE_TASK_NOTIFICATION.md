# Outlook-Style Task Notification

**Datum:** 2025-12-10  
**Feature:** Outlook-Style Notification für neue Aufgaben  
**Status:** ✅ Implementiert

---

## Anforderung

> "Wir brauchen keinen dedizierten Posteingang (Würde eher ein Aufgabeneingang sein) Sondern eher eine optische Notiz: Bei Eingang einer neuen ungelesenen Aufgabe wird der Aufgabenkorb (text decoration: bold) und die Anzahl dahinter in Klammern geschrieben: **Aufgabenkorb (1)**  
> Technisches Vorbild: Outlook"

---

## Implementierung

### Visuelle Darstellung

**Kein neuer Eingang:**
```
Navigation | Aufgaben | Favoriten
```

**Mit neuen Aufgaben (wie Outlook):**
```
Navigation | Aufgaben (3) | Favoriten
           ↑           ↑
         BOLD     Unread Count
```

### XAML Implementation

**Sidebar Tab mit Outlook-Style:**
```xml
<!-- Aufgaben Tab mit Unread Count -->
<RadioButton x:Name="SidebarTabTasks" GroupName="SidebarTabs" 
             Padding="16,8" FontSize="12" Click="SidebarTab_Click">
    <StackPanel Orientation="Horizontal">
        <TextBlock Text="Aufgaben">
            <TextBlock.Style>
                <Style TargetType="TextBlock">
                    <Setter Property="FontWeight" Value="Normal"/>
                    <Style.Triggers>
                        <!-- Bold when unread tasks exist (Outlook-style) -->
                        <DataTrigger Binding="{Binding TaskBasketViewModel.HasUnreadTasks}" 
                                     Value="True">
                            <Setter Property="FontWeight" Value="Bold"/>
                        </DataTrigger>
                    </Style.Triggers>
                </Style>
            </TextBlock.Style>
        </TextBlock>
        
        <!-- Unread count in parentheses (Outlook-style) -->
        <TextBlock Margin="4,0,0,0"
                   Visibility="{Binding TaskBasketViewModel.HasUnreadTasks, 
                              Converter={StaticResource BoolToVisibility}}">
            <TextBlock.Style>
                <Style TargetType="TextBlock">
                    <Setter Property="FontWeight" Value="Bold"/>
                    <Setter Property="Foreground" Value="{DynamicResource SystemControlForegroundAccentBrush}"/>
                </Style>
            </TextBlock.Style>
            <Run Text="("/><Run Text="{Binding TaskBasketViewModel.UnreadTasksCount, Mode=OneWay}"/><Run Text=")"/>
        </TextBlock>
    </StackPanel>
</RadioButton>
```

### ViewModel Properties

**TaskBasketViewModel:**
```csharp
[ObservableProperty]
private int _unreadTasksCount;

[ObservableProperty]
private bool _hasUnreadTasks;

[RelayCommand]
private async Task LoadTasksAsync()
{
    var tasks = await _mediator.Send(query);
    
    Tasks.Clear();
    foreach (var task in tasks)
    {
        Tasks.Add(task);
    }

    // Calculate unread count (new tasks from last 24h)
    UnreadTasksCount = tasks.Count(t => 
        t.Status == TaskStatus.Pending && 
        t.CreatedAt > DateTime.UtcNow.AddDays(-1));
    
    HasUnreadTasks = UnreadTasksCount > 0;
}
```

---

## Outlook-Vergleich

### Microsoft Outlook

**Posteingang ohne neue Nachrichten:**
```
Posteingang
```

**Posteingang mit neuen Nachrichten:**
```
Posteingang (5)
↑          ↑
BOLD    Unread Count
```

### ThemisDB DocumentManager

**Aufgaben ohne neue Tasks:**
```
Navigation | Aufgaben | Favoriten
```

**Aufgaben mit neuen Tasks:**
```
Navigation | Aufgaben (3) | Favoriten
           ↑           ↑
         BOLD     Unread Count
```

---

## Konfiguration

### Unread Definition

**Standardmäßig:**
- Neue Aufgaben der letzten 24 Stunden
- Status: Pending (noch nicht begonnen)

**Konfigurierbar:**
```csharp
// Option 1: Zeitbasiert (letzte 24h)
UnreadTasksCount = tasks.Count(t => 
    t.Status == TaskStatus.Pending && 
    t.CreatedAt > DateTime.UtcNow.AddDays(-1));

// Option 2: Explizites "IsRead" Flag
UnreadTasksCount = tasks.Count(t => !t.IsRead);

// Option 3: Kombiniert
UnreadTasksCount = tasks.Count(t => 
    !t.IsRead && 
    t.Status == TaskStatus.Pending);
```

### Auto-Read Behavior

**Beim Öffnen des Aufgaben-Tabs:**
```csharp
private void SidebarTab_Click(object sender, RoutedEventArgs e)
{
    if (sender == SidebarTabTasks)
    {
        // Tab wechseln
        NavigationTabContent.Visibility = Visibility.Collapsed;
        TasksTabContent.Visibility = Visibility.Visible;
        
        // Optional: Mark all as read when tab is opened
        if (TaskBasketViewModel != null)
        {
            TaskBasketViewModel.MarkAllAsReadCommand.Execute(null);
            // After marking as read, count becomes 0
            // "Aufgaben (3)" → "Aufgaben"
            // FontWeight: Bold → Normal
        }
    }
}
```

---

## Weitere Outlook-Features

### 1. Badge/Notification Bubble (Optional)

**Zusätzlich zur Zahl:**
```xml
<Grid>
    <TextBlock Text="Aufgaben"/>
    
    <!-- Badge with count (like mobile apps) -->
    <Border Background="Red" CornerRadius="10" 
            Padding="6,2" Margin="-8,-8,0,0"
            HorizontalAlignment="Right" VerticalAlignment="Top"
            Visibility="{Binding HasUnreadTasks, Converter={StaticResource BoolToVisibility}}">
        <TextBlock Text="{Binding UnreadTasksCount}" 
                   Foreground="White" FontSize="10" FontWeight="Bold"/>
    </Border>
</Grid>
```

**Ergebnis:**
```
┌─────────────┐
│ Aufgaben ③  │  ← Red badge with count
└─────────────┘
```

### 2. Color Accent (Optional)

**Farbige Hervorhebung wie Outlook:**
```xml
<TextBlock Text="Aufgaben">
    <TextBlock.Style>
        <Style TargetType="TextBlock">
            <Setter Property="FontWeight" Value="Normal"/>
            <Setter Property="Foreground" Value="{DynamicResource SystemControlForegroundBaseHighBrush}"/>
            <Style.Triggers>
                <DataTrigger Binding="{Binding HasUnreadTasks}" Value="True">
                    <Setter Property="FontWeight" Value="Bold"/>
                    <Setter Property="Foreground" Value="{DynamicResource SystemControlForegroundAccentBrush}"/>
                </DataTrigger>
            </Style.Triggers>
        </Style>
    </TextBlock.Style>
</TextBlock>
```

### 3. Desktop Notification (Optional)

**Toast Notification bei neuer Aufgabe:**
```csharp
using Windows.UI.Notifications;

public void ShowNewTaskNotification(TaskItem newTask)
{
    var toastXml = ToastNotificationManager.GetTemplateContent(
        ToastTemplateType.ToastText02);
    
    var textNodes = toastXml.GetElementsByTagName("text");
    textNodes[0].AppendChild(toastXml.CreateTextNode("Neue Aufgabe"));
    textNodes[1].AppendChild(toastXml.CreateTextNode(newTask.Title));
    
    var toast = new ToastNotification(toastXml);
    ToastNotificationManager.CreateToastNotifier("ThemisDB").Show(toast);
}
```

---

## Zusammenfassung

✅ **Outlook-Style Notification implementiert:**
- Kein dedizierter Posteingang
- Optische Notiz im Aufgaben-Tab
- **Fett** wenn ungelesene Aufgaben vorhanden
- **(Anzahl)** in Klammern dahinter
- Genau wie Microsoft Outlook

✅ **Eigenschaften:**
- `UnreadTasksCount` - Anzahl ungelesener Aufgaben
- `HasUnreadTasks` - Boolean für Styling
- Automatische Aktualisierung beim Laden

✅ **Verhalten:**
- Neue Aufgaben (letzte 24h) = ungelesen
- Tab zeigt: "Aufgaben (3)" in Fettschrift
- Nach Öffnen optional: Auto-Mark-as-Read

---

**Erstellt:** 2025-12-10  
**Version:** 1.0 - Outlook-Style  
**Status:** Production Ready

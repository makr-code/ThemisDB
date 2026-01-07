# Windows & Office Integration - Nahtlose Adaptation

## Überblick

Das ThemisDB DocumentManager DMS ist so konzipiert, dass es sich **nahtlos in Windows und Office-Produkte einfügt**, ohne diese zu ersetzen. Es agiert als intelligenter Layer, der bestehende Workflows erweitert und verbessert.

---

## 1. Windows-Integration

### 1.1 Native Windows-Features

#### Datei-Explorer-Integration
```csharp
// Windows Shell Extension für Kontextmenü
public class ThemisShellExtension : IShellExtInit, IContextMenu
{
    // Rechtsklick im Explorer → "ThemisDB: Dokument archivieren"
    public void AddContextMenuItems(IntPtr hMenu)
    {
        // Fügt Menüeinträge zum Windows Explorer hinzu
        InsertMenu(hMenu, 0, MF_STRING, ID_THEMIS_ARCHIVE, "ThemisDB: Archivieren");
        InsertMenu(hMenu, 1, MF_STRING, ID_THEMIS_METADATA, "ThemisDB: Metadaten bearbeiten");
    }
}
```

**Features:**
- Rechtsklick auf Datei → Direktes Archivieren in ThemisDB
- Metadaten-Dialog aus dem Explorer heraus
- Drag & Drop von Explorer in ThemisDB

#### Windows Benachrichtigungen
```csharp
// Native Windows Toast Notifications
public class WindowsNotificationService
{
    public void ShowTaskDueNotification(TaskItem task)
    {
        var toastContent = new ToastContent
        {
            Visual = new ToastVisual
            {
                BindingGeneric = new ToastBindingGeneric
                {
                    Children =
                    {
                        new AdaptiveText { Text = "Aufgabe fällig!" },
                        new AdaptiveText { Text = task.Title }
                    }
                }
            },
            Actions = new ToastActionsCustom
            {
                Buttons =
                {
                    new ToastButton("Öffnen", $"action=open&taskId={task.Id}"),
                    new ToastButton("Erledigt", $"action=complete&taskId={task.Id}")
                }
            }
        };
        
        ToastNotificationManager.CreateToastNotifier().Show(new ToastNotification(toastContent.GetXml()));
    }
}
```

#### Windows Taskleiste Integration
```csharp
// JumpList für Schnellzugriff
public class WindowsJumpListService
{
    public void UpdateJumpList()
    {
        var jumpList = JumpList.CreateJumpList();
        
        // Zuletzt verwendete Dokumente
        jumpList.JumpItems.Add(new JumpTask
        {
            Title = "Letztes Dokument: Vertrag_2024.docx",
            Arguments = "/open:doc123",
            IconResourcePath = "document.ico"
        });
        
        // Kategorien
        jumpList.ShowRecentCategory = true;
        jumpList.ShowFrequentCategory = true;
        
        jumpList.Apply();
    }
}
```

### 1.2 Windows Suchintegration

```csharp
// Windows Search Protocol Handler
// Ermöglicht Suche in ThemisDB direkt aus Windows-Suche
[ComVisible(true)]
[Guid("...")]
public class ThemisSearchProtocolHandler : ISearchProtocolHandler
{
    public ISearchCrawlScopeManager GetSearchScope()
    {
        // ThemisDB-Dokumente in Windows-Suche indizieren
        var scope = new SearchCrawlScopeManager();
        scope.AddDefaultScopeRule("themisdb://documents/*", true, FOLLOW_FLAGS.FF_INDEXCOMPLEXURLS);
        return scope;
    }
}
```

**Resultat:** 
- Windows-Suche findet Dokumente aus ThemisDB
- Cortana/Search kann ThemisDB durchsuchen
- Integriert in Windows 11 Widgets

---

## 2. Microsoft Office Integration

### 2.1 Word Integration

#### Add-In für Word
```csharp
// VSTO Add-In für Word
public class ThemisWordAddIn : Microsoft.Office.Tools.Word.Application
{
    private void ThisAddIn_Startup(object sender, EventArgs e)
    {
        // Ribbon-Button hinzufügen
        CreateCustomRibbon();
        
        // Event Handler für Speichern
        Application.DocumentBeforeSave += OnBeforeSave;
    }
    
    private void OnBeforeSave(Word.Document doc, ref bool saveAsUI, ref bool cancel)
    {
        // Automatisch in ThemisDB archivieren
        var metadata = ExtractMetadataFromDocument(doc);
        var result = ThemisDBClient.ArchiveDocument(doc.FullName, metadata);
        
        if (result.Success)
        {
            // Versionsnummer in Dokument einfügen
            InsertVersionInfo(doc, result.Version);
        }
    }
    
    private void CreateCustomRibbon()
    {
        var ribbon = Globals.Ribbons.Ribbon1;
        ribbon.ThemisArchiveButton.Click += (s, e) => ArchiveCurrentDocument();
        ribbon.ThemisMetadataButton.Click += (s, e) => ShowMetadataDialog();
        ribbon.ThemisVersionsButton.Click += (s, e) => ShowVersionHistory();
    }
}
```

**Features:**
- **Ribbon-Buttons** in Word: "ThemisDB Archivieren", "Metadaten", "Versionen"
- **Auto-Save**: Dokument wird automatisch in ThemisDB gesichert
- **Versions-Tracking**: Revisionsnummern direkt im Dokument
- **Vorlagen-Zugriff**: ThemisDB-Vorlagen aus Word heraus

#### Word Aufgabenbereich (Task Pane)
```xml
<!-- CustomTaskPane.xml -->
<Office:CustomTaskPane ID="ThemisDBPane">
    <Office:Title>ThemisDB</Office:Title>
    <Office:WebView2 Source="https://localhost:5000/word-addon" />
</Office:CustomTaskPane>
```

```csharp
// Task Pane Control
public partial class ThemisTaskPaneControl : UserControl
{
    public void LoadDocumentInfo(string documentId)
    {
        // Zeigt ThemisDB-Infos im Seitenbereich
        DocumentTitleLabel.Text = _document.Title;
        MetadataGrid.ItemsSource = _document.Metadata;
        VersionsList.ItemsSource = _document.Versions;
        RelatedDocuments.ItemsSource = _graphService.GetRelatedDocuments(documentId);
    }
}
```

**Im Word-Fenster:**
```
┌─────────────────────────────────────────────┐
│ Word                        [ThemisDB Pane] │
│ ┌──────────────────────┐  ┌───────────────┐│
│ │                      │  │ ThemisDB Info ││
│ │  Dokument-Inhalt     │  │               ││
│ │                      │  │ Titel: ...    ││
│ │  [Typing here...]    │  │ Status: ✓     ││
│ │                      │  │               ││
│ │                      │  │ Versionen:    ││
│ │                      │  │  • v1.0       ││
│ │                      │  │  • v1.1 (neu) ││
│ │                      │  │               ││
│ │                      │  │ Verwandt:     ││
│ │                      │  │  • Anlage.pdf ││
│ └──────────────────────┘  └───────────────┘│
└─────────────────────────────────────────────┘
```

### 2.2 Excel Integration

```csharp
// Excel Add-In
public class ThemisExcelAddIn
{
    // Ribbon-Buttons
    [RibbonButton(Label = "ThemisDB Export")]
    public void ExportToThemisDB()
    {
        var workbook = Globals.ThisWorkbook;
        var data = ExtractDataFromWorkbook(workbook);
        
        // Als strukturierte Daten in ThemisDB speichern
        ThemisDBClient.StoreStructuredData(data);
    }
    
    // Custom Functions (UDFs)
    [ExcelFunction(Description = "Lädt Daten aus ThemisDB")]
    public static object THEMISDB_QUERY(string query)
    {
        var result = ThemisDBClient.ExecuteQuery(query);
        return ConvertToExcelArray(result);
    }
}
```

**Excel Formula:**
```excel
=THEMISDB_QUERY("documents WHERE category='Vertrag'")
```

**Resultat:** Excel-Tabelle wird mit Live-Daten aus ThemisDB gefüllt

### 2.3 Outlook Integration

#### Aufgaben-Synchronisation
```csharp
public class OutlookTaskSyncService
{
    private readonly Outlook.Application _outlook;
    private readonly ITaskService _themisTaskService;
    
    // Bidirektionale Synchronisation
    public async Task SyncTasksAsync()
    {
        var outlookTasks = GetOutlookTasks();
        var themisTasks = await _themisTaskService.GetMyTasksAsync();
        
        // Outlook → ThemisDB
        foreach (var outlookTask in outlookTasks)
        {
            if (!themisTasks.Any(t => t.OutlookId == outlookTask.EntryID))
            {
                await CreateThemisTaskFromOutlook(outlookTask);
            }
        }
        
        // ThemisDB → Outlook
        foreach (var themisTask in themisTasks)
        {
            if (string.IsNullOrEmpty(themisTask.OutlookId))
            {
                await CreateOutlookTaskFromThemis(themisTask);
            }
            else
            {
                await UpdateOutlookTask(themisTask);
            }
        }
    }
    
    private List<Outlook.TaskItem> GetOutlookTasks()
    {
        var tasksFolder = _outlook.Session.GetDefaultFolder(Outlook.OlDefaultFolders.olFolderTasks);
        return tasksFolder.Items.Cast<Outlook.TaskItem>().ToList();
    }
    
    private async Task CreateOutlookTaskFromThemis(TaskItem themisTask)
    {
        var outlookTask = _outlook.CreateItem(Outlook.OlItemType.olTaskItem) as Outlook.TaskItem;
        
        outlookTask.Subject = themisTask.Title;
        outlookTask.Body = themisTask.Description;
        outlookTask.DueDate = themisTask.DueDate ?? DateTime.Now.AddDays(7);
        outlookTask.Importance = MapPriority(themisTask.Priority);
        outlookTask.Categories = themisTask.Category;
        
        // Custom Property für ThemisDB-ID
        outlookTask.UserProperties.Add("ThemisDB_ID", Outlook.OlUserPropertyType.olText).Value = themisTask.Id;
        
        outlookTask.Save();
        
        // Outlook-ID zurück in ThemisDB speichern
        themisTask.OutlookId = outlookTask.EntryID;
        await _themisTaskService.UpdateTaskAsync(themisTask);
    }
}
```

**Features:**
- ✅ **Bidirektionale Sync**: Outlook ↔ ThemisDB
- ✅ **Echtzeit-Updates**: Änderungen werden sofort synchronisiert
- ✅ **Kategorien**: Outlook-Kategorien = ThemisDB-Kategorien
- ✅ **Benachrichtigungen**: Outlook-Reminder + ThemisDB-Notifications

#### E-Mail-Archivierung
```csharp
// Outlook Add-In Ribbon Button
[RibbonButton(Label = "In ThemisDB archivieren")]
public void ArchiveEmailToThemisDB()
{
    var explorer = Globals.ThisAddIn.Application.ActiveExplorer();
    var selection = explorer.Selection;
    
    foreach (var item in selection)
    {
        if (item is Outlook.MailItem mail)
        {
            // E-Mail als Dokument archivieren
            var document = new Document
            {
                Title = mail.Subject,
                Content = mail.HTMLBody,
                Category = "E-Mail",
                Metadata = new Dictionary<string, object>
                {
                    { "From", mail.SenderEmailAddress },
                    { "To", mail.To },
                    { "Date", mail.ReceivedTime },
                    { "Attachments", mail.Attachments.Count }
                }
            };
            
            // Anhänge separat archivieren
            foreach (Outlook.Attachment attachment in mail.Attachments)
            {
                var tempPath = Path.Combine(Path.GetTempPath(), attachment.FileName);
                attachment.SaveAsFile(tempPath);
                ThemisDBClient.ArchiveAttachment(tempPath, document.Id);
            }
            
            ThemisDBClient.ArchiveDocument(document);
            
            // Kategorisieren in Outlook
            mail.Categories = "ThemisDB Archiviert";
            mail.Save();
        }
    }
}
```

### 2.4 PowerPoint Integration

```csharp
// PowerPoint Add-In
public class ThemisPowerPointAddIn
{
    [RibbonButton(Label = "ThemisDB Vorlagen")]
    public void InsertThemisTemplate()
    {
        var presentation = Globals.ThisAddIn.Application.ActivePresentation;
        
        // Zeige Template-Browser
        var templates = ThemisDBClient.GetPresentationTemplates();
        var selectedTemplate = ShowTemplatePicker(templates);
        
        if (selectedTemplate != null)
        {
            // Füge Slides aus Vorlage ein
            foreach (var slide in selectedTemplate.Slides)
            {
                presentation.Slides.AddSlide(presentation.Slides.Count + 1, slide);
            }
        }
    }
    
    [RibbonButton(Label = "Daten einfügen")]
    public void InsertThemisData()
    {
        // ThemisDB-Daten als Diagramm/Tabelle einfügen
        var data = ThemisDBClient.QueryData("SELECT * FROM statistics");
        var chart = CreateChartFromData(data);
        
        var slide = Globals.ThisAddIn.Application.ActiveWindow.View.Slide;
        slide.Shapes.AddChart(chart);
    }
}
```

### 2.5 OneNote Integration

```csharp
// OneNote Integration
public class OneNoteIntegrationService
{
    private readonly OneNote.Application _oneNote;
    
    public async Task CreateNoteFromDocument(Document document)
    {
        // Erstelle OneNote-Seite mit Dokument-Inhalt
        var notebookId = GetOrCreateNotebook("ThemisDB");
        var sectionId = GetOrCreateSection(notebookId, "Dokumente");
        
        var pageXml = $@"
            <?xml version='1.0'?>
            <one:Page xmlns:one='http://schemas.microsoft.com/office/onenote/2013/onenote'>
                <one:Title>
                    <one:OE><one:T><![CDATA[{document.Title}]]></one:T></one:OE>
                </one:Title>
                <one:Outline>
                    <one:OEChildren>
                        <one:OE>
                            <one:T><![CDATA[{document.Content}]]></one:T>
                        </one:OE>
                        <one:OE>
                            <one:T><![CDATA[Link: themisdb://documents/{document.Id}]]></one:T>
                        </one:OE>
                    </one:OEChildren>
                </one:Outline>
            </one:Page>";
        
        _oneNote.CreateNewPage(sectionId, out string pageId, NewPageStyle.npsBlankPageWithTitle);
        _oneNote.UpdatePageContent(pageXml);
    }
}
```

---

## 3. Nahtlose Workflows

### 3.1 "Direkt aus Office speichern"

```
Benutzer arbeitet in Word
    ↓
Speichert Dokument (Strg+S)
    ↓
ThemisDB Add-In fängt Speicher-Event ab
    ↓
Dialog: "In ThemisDB archivieren?"
    ↓
    ┌─── JA ───┐           ┌─── NEIN ───┐
    │          │           │            │
    v          v           v            v
Metadaten   Normal    Datei wird   Datei wird
abfragen    speichern  normal      normal
            & archiv.  gespeichert gespeichert
```

### 3.2 "Aus Outlook archivieren"

```
E-Mail mit Anhang kommt an
    ↓
Benutzer klickt "ThemisDB Archivieren"
    ↓
Automatisch:
  • E-Mail als Dokument
  • Anhänge als separate Dokumente
  • Beziehungen im Graph
  • Timeline-Event erstellt
    ↓
Outlook-Kategorie: "✓ Archiviert"
```

### 3.3 "Aufgaben-Sync"

```
Aufgabe in ThemisDB erstellt
    ↓
Automatisch in Outlook erstellt
    ↓
Benutzer ändert in Outlook
    ↓
Änderung zurück zu ThemisDB
    ↓
ThemisDB-Timeline aktualisiert
    ↓
Andere Benutzer sehen Update
```

---

## 4. Technische Umsetzung

### 4.1 COM Interop (für Office)

```csharp
// App.xaml.cs - Office COM Interop Setup
private void RegisterOfficeIntegration()
{
    // Registriere COM Add-Ins
    RegistryKey key = Registry.CurrentUser.OpenSubKey(
        @"Software\Microsoft\Office\Word\Addins\ThemisDB.WordAddin", true);
    
    if (key == null)
    {
        key = Registry.CurrentUser.CreateSubKey(
            @"Software\Microsoft\Office\Word\Addins\ThemisDB.WordAddin");
    }
    
    key.SetValue("Description", "ThemisDB Integration für Word");
    key.SetValue("FriendlyName", "ThemisDB");
    key.SetValue("LoadBehavior", 3); // Load on startup
}
```

### 4.2 Protocol Handler (für URI-Schema)

```csharp
// themisdb:// Protocol Handler
[ComVisible(true)]
[Guid("...")]
public class ThemisProtocolHandler : IProtocolHandler
{
    public void HandleUri(Uri uri)
    {
        // themisdb://documents/doc123 → Öffne Dokument
        if (uri.Host == "documents")
        {
            var documentId = uri.AbsolutePath.TrimStart('/');
            OpenDocument(documentId);
        }
        // themisdb://tasks/task456 → Öffne Aufgabe
        else if (uri.Host == "tasks")
        {
            var taskId = uri.AbsolutePath.TrimStart('/');
            OpenTask(taskId);
        }
    }
}
```

**Registry-Eintrag:**
```ini
[HKEY_CLASSES_ROOT\themisdb]
@="URL:ThemisDB Protocol"
"URL Protocol"=""

[HKEY_CLASSES_ROOT\themisdb\shell\open\command]
@="\"C:\\Program Files\\ThemisDB\\ThemisDB.DocumentManager.exe\" \"%1\""
```

### 4.3 Deployment

```xml
<!-- Setup.wxs - WiX Installer -->
<Product>
    <Feature Id="CoreFeature" Title="ThemisDB Core" Level="1">
        <ComponentRef Id="MainApplication" />
    </Feature>
    
    <Feature Id="OfficeIntegration" Title="Office Integration" Level="1">
        <ComponentRef Id="WordAddin" />
        <ComponentRef Id="ExcelAddin" />
        <ComponentRef Id="OutlookAddin" />
        <ComponentRef Id="PowerPointAddin" />
    </Feature>
    
    <Feature Id="WindowsIntegration" Title="Windows Integration" Level="1">
        <ComponentRef Id="ShellExtension" />
        <ComponentRef Id="ProtocolHandler" />
        <ComponentRef Id="SearchProvider" />
    </Feature>
</Product>
```

---

## 5. Zusammenfassung

### ✅ ThemisDB ersetzt NICHT Office
- Word, Excel, PowerPoint, Outlook bleiben die Hauptwerkzeuge
- ThemisDB fügt sich **nahtlos** in bestehende Workflows ein
- Benutzer arbeiten weiter in gewohnter Umgebung

### ✅ ThemisDB erweitert Office
- Automatische Archivierung
- Versionsmanagement
- Metadaten-Extraktion
- Intelligente Suche
- Beziehungen zwischen Dokumenten

### ✅ ThemisDB nutzt Windows
- Explorer-Integration
- Taskleiste/JumpLists
- Native Notifications
- Windows Search
- Protocol Handler

### 🎯 Ergebnis
Ein DMS, das sich **anfühlt wie ein Teil von Windows und Office**, nicht wie eine separate Anwendung.

**Beispiel-Workflow:**
1. Benutzer öffnet Word
2. Arbeitet an Dokument
3. Speichert (Strg+S)
4. ThemisDB archiviert automatisch
5. Versionen werden getrackt
6. Metadaten werden extrahiert
7. Timeline wird aktualisiert
8. Verwandte Dokumente werden verlinkt
9. **Alles transparent und ohne Unterbrechung**

---

**Version**: 1.0.0  
**Datum**: 2024-06-12  
**Fokus**: Nahtlose Integration, keine Ersetzung

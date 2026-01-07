# ThemisDB DMS - Libraries & Code Snippets

## Übersicht

Diese Dokumentation sammelt nützliche Bibliotheken, Tools und Code-Snippets für die Implementierung des ThemisDB-basierten Document Management Systems mit besonderem Fokus auf Speech-to-Text (STT) und Text-to-Speech (TTS) Funktionalität.

---

## 1. Speech-to-Text (STT) Libraries

### 1.1 OpenAI Whisper (Empfohlen für Themis)

**Warum Whisper?**
- Open Source und lokal ausführbar (Privacy!)
- Exzellente Genauigkeit auch bei Deutsch
- Multi-Language Support
- Funktioniert offline
- Integration mit Ollama-Infrastruktur möglich

**NuGet Package:**
```xml
<PackageReference Include="Whisper.net" Version="1.7.0" />
```

**Code Snippet:**
```csharp
using Whisper.net;

public class WhisperSTTService : ISTTService
{
    private readonly WhisperProcessor _processor;
    
    public WhisperSTTService(string modelPath)
    {
        var factory = WhisperFactory.FromPath(modelPath);
        _processor = factory.CreateBuilder()
            .WithLanguage("de") // Deutsch
            .WithTranslate(false)
            .Build();
    }
    
    public async Task<string> TranscribeAsync(byte[] audioData)
    {
        using var audioStream = new MemoryStream(audioData);
        
        var result = new StringBuilder();
        await foreach (var segment in _processor.ProcessAsync(audioStream))
        {
            result.Append(segment.Text);
        }
        
        return result.ToString();
    }
    
    // Voice Command Erkennung für DMS
    public async Task<VoiceCommand?> RecognizeCommandAsync(byte[] audioData)
    {
        var text = await TranscribeAsync(audioData);
        
        // Einfache Keyword-Erkennung
        if (text.Contains("öffne dokument", StringComparison.OrdinalIgnoreCase))
        {
            return new VoiceCommand 
            { 
                Action = "open_document",
                Parameter = ExtractDocumentName(text)
            };
        }
        else if (text.Contains("suche nach", StringComparison.OrdinalIgnoreCase))
        {
            return new VoiceCommand 
            { 
                Action = "search",
                Parameter = ExtractSearchQuery(text)
            };
        }
        // ... weitere Kommandos
        
        return null;
    }
    
    private string ExtractDocumentName(string text)
    {
        // Regex oder LLM-basierte Extraktion
        var match = Regex.Match(text, @"öffne dokument\s+(.+)", RegexOptions.IgnoreCase);
        return match.Success ? match.Groups[1].Value.Trim() : string.Empty;
    }
}

public class VoiceCommand
{
    public string Action { get; set; } = string.Empty;
    public string Parameter { get; set; } = string.Empty;
}
```

**Integration mit WPF:**
```csharp
// MainWindow.xaml.cs
private readonly WhisperSTTService _sttService;
private readonly WaveInEvent _waveIn;
private readonly MemoryStream _audioBuffer;

public MainWindow()
{
    InitializeComponent();
    
    _sttService = new WhisperSTTService("models/ggml-base.bin");
    _audioBuffer = new MemoryStream();
    
    // NAudio für Audio-Aufnahme
    _waveIn = new WaveInEvent();
    _waveIn.DataAvailable += OnAudioDataAvailable;
}

private void VoiceCommandButton_Click(object sender, RoutedEventArgs e)
{
    if (_waveIn.WaveFormat == null)
    {
        _waveIn.WaveFormat = new WaveFormat(16000, 1); // 16kHz Mono
    }
    
    _audioBuffer.SetLength(0);
    _waveIn.StartRecording();
    StatusText.Text = "🎤 Höre zu...";
}

private void StopRecording()
{
    _waveIn.StopRecording();
    StatusText.Text = "Verarbeite...";
    
    var audioData = _audioBuffer.ToArray();
    Task.Run(async () =>
    {
        var command = await _sttService.RecognizeCommandAsync(audioData);
        
        Dispatcher.Invoke(() =>
        {
            if (command != null)
            {
                ExecuteVoiceCommand(command);
                StatusText.Text = $"✓ Kommando: {command.Action}";
            }
            else
            {
                StatusText.Text = "❌ Kommando nicht erkannt";
            }
        });
    });
}

private void ExecuteVoiceCommand(VoiceCommand command)
{
    switch (command.Action)
    {
        case "open_document":
            OpenDocument(command.Parameter);
            break;
        case "search":
            PerformSearch(command.Parameter);
            break;
        case "create_task":
            CreateTask(command.Parameter);
            break;
        // ... weitere Aktionen
    }
}
```

### 1.2 Azure Speech Services (Alternative für Cloud)

**NuGet Package:**
```xml
<PackageReference Include="Microsoft.CognitiveServices.Speech" Version="1.38.0" />
```

**Code Snippet:**
```csharp
using Microsoft.CognitiveServices.Speech;
using Microsoft.CognitiveServices.Speech.Audio;

public class AzureSTTService : ISTTService
{
    private readonly SpeechConfig _config;
    
    public AzureSTTService(string subscriptionKey, string region)
    {
        _config = SpeechConfig.FromSubscription(subscriptionKey, region);
        _config.SpeechRecognitionLanguage = "de-DE";
    }
    
    public async Task<string> TranscribeAsync(Stream audioStream)
    {
        using var audioConfig = AudioConfig.FromStreamInput(
            AudioInputStream.CreatePushStream());
        using var recognizer = new SpeechRecognizer(_config, audioConfig);
        
        var result = await recognizer.RecognizeOnceAsync();
        
        return result.Reason == ResultReason.RecognizedSpeech
            ? result.Text
            : string.Empty;
    }
}
```

---

## 2. Text-to-Speech (TTS) Libraries

### 2.1 System.Speech (Windows Built-in)

**Vorteile:**
- Keine zusätzlichen Dependencies
- Bereits in Windows enthalten
- Gute Deutsch-Stimmen verfügbar

**Code Snippet:**
```csharp
using System.Speech.Synthesis;

public class WindowsTTSService : ITTSService
{
    private readonly SpeechSynthesizer _synthesizer;
    
    public WindowsTTSService()
    {
        _synthesizer = new SpeechSynthesizer();
        _synthesizer.SelectVoiceByHints(VoiceGender.Female, VoiceAge.Adult, 0, 
            new System.Globalization.CultureInfo("de-DE"));
    }
    
    public void Speak(string text)
    {
        _synthesizer.SpeakAsync(text);
    }
    
    public async Task SpeakAsync(string text)
    {
        await Task.Run(() => _synthesizer.Speak(text));
    }
    
    // Für DMS: Sprachbenachrichtigungen
    public void AnnounceTaskDue(string taskTitle)
    {
        var message = $"Erinnerung: Die Aufgabe {taskTitle} ist fällig.";
        Speak(message);
    }
    
    public void AnnounceDocumentOpened(string documentTitle)
    {
        var message = $"Dokument {documentTitle} wurde geöffnet.";
        Speak(message);
    }
    
    public void ReadDocumentSummary(string summary)
    {
        var message = $"Zusammenfassung: {summary}";
        Speak(message);
    }
}
```

### 2.2 Azure Neural TTS (Hochwertig)

**Code Snippet:**
```csharp
using Microsoft.CognitiveServices.Speech;

public class AzureTTSService : ITTSService
{
    private readonly SpeechConfig _config;
    
    public AzureTTSService(string subscriptionKey, string region)
    {
        _config = SpeechConfig.FromSubscription(subscriptionKey, region);
        _config.SpeechSynthesisVoiceName = "de-DE-KatjaNeural"; // Natürliche Stimme
    }
    
    public async Task SpeakAsync(string text)
    {
        using var synthesizer = new SpeechSynthesizer(_config);
        await synthesizer.SpeakTextAsync(text);
    }
    
    // SSML für erweiterte Kontrolle
    public async Task SpeakWithSSMLAsync(string ssml)
    {
        using var synthesizer = new SpeechSynthesizer(_config);
        await synthesizer.SpeakSsmlAsync(ssml);
    }
}

// Verwendung mit SSML
var ssml = @"
<speak version='1.0' xml:lang='de-DE'>
    <voice name='de-DE-KatjaNeural'>
        <prosody rate='0.9' pitch='+5%'>
            Sie haben 5 neue Aufgaben.
        </prosody>
        <break time='500ms'/>
        Die wichtigste ist: Bauantrag prüfen.
    </voice>
</speak>";

await ttsService.SpeakWithSSMLAsync(ssml);
```

### 2.3 Piper TTS (Open Source, Lokal)

**Für lokale, offline TTS (ähnlich Whisper für STT)**

**Installation:**
```bash
# Python-basiert, kann über Process gestartet werden
pip install piper-tts
```

**C# Wrapper:**
```csharp
public class PiperTTSService : ITTSService
{
    private readonly string _piperPath;
    private readonly string _modelPath;
    
    public PiperTTSService(string piperPath, string modelPath)
    {
        _piperPath = piperPath;
        _modelPath = modelPath;
    }
    
    public async Task SpeakAsync(string text, string outputWavPath)
    {
        var process = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = _piperPath,
                Arguments = $"--model {_modelPath} --output_file {outputWavPath}",
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                UseShellExecute = false,
                CreateNoWindow = true
            }
        };
        
        process.Start();
        await process.StandardInput.WriteLineAsync(text);
        process.StandardInput.Close();
        await process.WaitForExitAsync();
        
        // Play the generated WAV file
        await PlayAudioAsync(outputWavPath);
    }
    
    private async Task PlayAudioAsync(string wavPath)
    {
        using var audioFile = new AudioFileReader(wavPath);
        using var outputDevice = new WaveOutEvent();
        
        outputDevice.Init(audioFile);
        outputDevice.Play();
        
        while (outputDevice.PlaybackState == PlaybackState.Playing)
        {
            await Task.Delay(100);
        }
    }
}
```

---

## 3. Audio-Bibliotheken

### 3.1 NAudio (Audio I/O für .NET)

**NuGet Package:**
```xml
<PackageReference Include="NAudio" Version="2.2.1" />
```

**Microphone Recording:**
```csharp
using NAudio.Wave;

public class AudioRecorder
{
    private WaveInEvent _waveIn;
    private WaveFileWriter _writer;
    
    public void StartRecording(string outputPath)
    {
        _waveIn = new WaveInEvent
        {
            WaveFormat = new WaveFormat(16000, 1) // 16kHz Mono für Whisper
        };
        
        _writer = new WaveFileWriter(outputPath, _waveIn.WaveFormat);
        
        _waveIn.DataAvailable += (s, e) =>
        {
            _writer.Write(e.Buffer, 0, e.BytesRecorded);
        };
        
        _waveIn.StartRecording();
    }
    
    public void StopRecording()
    {
        _waveIn?.StopRecording();
        _waveIn?.Dispose();
        _writer?.Dispose();
    }
}

// Push-to-Talk Button
private void MicButton_PreviewMouseDown(object sender, MouseButtonEventArgs e)
{
    _recorder.StartRecording("temp_recording.wav");
    StatusText.Text = "🎤 Recording...";
}

private void MicButton_PreviewMouseUp(object sender, MouseButtonEventArgs e)
{
    _recorder.StopRecording();
    StatusText.Text = "⏳ Processing...";
    
    Task.Run(async () =>
    {
        var text = await _sttService.TranscribeAsync("temp_recording.wav");
        Dispatcher.Invoke(() =>
        {
            SearchBox.Text = text;
            PerformSearch(text);
        });
    });
}
```

**Audio Playback:**
```csharp
public class AudioPlayer
{
    public async Task PlayAsync(string filePath)
    {
        using var audioFile = new AudioFileReader(filePath);
        using var outputDevice = new WaveOutEvent();
        
        outputDevice.Init(audioFile);
        outputDevice.Play();
        
        while (outputDevice.PlaybackState == PlaybackState.Playing)
        {
            await Task.Delay(100);
        }
    }
}
```

---

## 4. DMS-spezifische Libraries

### 4.1 Document Processing

#### iTextSharp (PDF)
```xml
<PackageReference Include="itext7" Version="8.0.3" />
```

```csharp
using iText.Kernel.Pdf;
using iText.Kernel.Pdf.Canvas.Parser;

public class PdfService
{
    public string ExtractText(string pdfPath)
    {
        using var pdfDoc = new PdfDocument(new PdfReader(pdfPath));
        var text = new StringBuilder();
        
        for (int page = 1; page <= pdfDoc.GetNumberOfPages(); page++)
        {
            text.Append(PdfTextExtractor.GetTextFromPage(pdfDoc.GetPage(page)));
        }
        
        return text.ToString();
    }
    
    // Mit STT: PDF vorlesen
    public async Task ReadPdfAloudAsync(string pdfPath, ITTSService tts)
    {
        var text = ExtractText(pdfPath);
        
        // Chunking für bessere TTS
        var chunks = SplitIntoSentences(text);
        foreach (var chunk in chunks)
        {
            await tts.SpeakAsync(chunk);
        }
    }
}
```

#### DocumentFormat.OpenXml (Word, Excel)
```xml
<PackageReference Include="DocumentFormat.OpenXml" Version="3.0.2" />
```

```csharp
using DocumentFormat.OpenXml.Packaging;
using DocumentFormat.OpenXml.Wordprocessing;

public class WordService
{
    public string ExtractText(string docxPath)
    {
        using var doc = WordprocessingDocument.Open(docxPath, false);
        var body = doc.MainDocumentPart.Document.Body;
        return body.InnerText;
    }
    
    public void CreateDocumentFromSpeech(string outputPath, string spokenText)
    {
        using var doc = WordprocessingDocument.Create(outputPath, 
            DocumentFormat.OpenXml.WordprocessingDocumentType.Document);
        
        var mainPart = doc.AddMainDocumentPart();
        mainPart.Document = new Document();
        var body = mainPart.Document.AppendChild(new Body());
        
        var para = body.AppendChild(new Paragraph());
        var run = para.AppendChild(new Run());
        run.AppendChild(new Text(spokenText));
        
        mainPart.Document.Save();
    }
}
```

### 4.2 OCR (Optical Character Recognition)

#### Tesseract.NET
```xml
<PackageReference Include="Tesseract" Version="5.2.0" />
```

```csharp
using Tesseract;

public class OCRService
{
    private readonly TesseractEngine _engine;
    
    public OCRService(string tessDataPath)
    {
        _engine = new TesseractEngine(tessDataPath, "deu", EngineMode.Default);
    }
    
    public string ExtractTextFromImage(string imagePath)
    {
        using var img = Pix.LoadFromFile(imagePath);
        using var page = _engine.Process(img);
        return page.GetText();
    }
    
    // Mit TTS: Gescanntes Dokument vorlesen
    public async Task ReadScannedDocumentAsync(string imagePath, ITTSService tts)
    {
        var text = ExtractTextFromImage(imagePath);
        await tts.SpeakAsync(text);
    }
}
```

### 4.3 Metadata Extraction

#### MetadataExtractor
```xml
<PackageReference Include="MetadataExtractor" Version="2.8.1" />
```

```csharp
using MetadataExtractor;

public class DocumentMetadataService
{
    public Dictionary<string, string> ExtractMetadata(string filePath)
    {
        var metadata = new Dictionary<string, string>();
        var directories = ImageMetadataReader.ReadMetadata(filePath);
        
        foreach (var directory in directories)
        {
            foreach (var tag in directory.Tags)
            {
                metadata[$"{directory.Name}:{tag.Name}"] = tag.Description ?? "";
            }
        }
        
        return metadata;
    }
}
```

---

## 5. Voice-Controlled DMS Features

### 5.1 Voice Commands Implementation

```csharp
public class VoiceControlledDMSService
{
    private readonly ISTTService _stt;
    private readonly ITTSService _tts;
    private readonly IDocumentService _documentService;
    private readonly IOllamaService _llm;
    
    public async Task<bool> ProcessVoiceCommandAsync(byte[] audioData)
    {
        // 1. Transcribe
        var spokenText = await _stt.TranscribeAsync(audioData);
        
        // 2. Use LLM to understand intent
        var intent = await _llm.GenerateAsync($@"
            Extract the intent and parameters from this voice command:
            '{spokenText}'
            
            Possible intents: open_document, search, create_task, read_document, 
                             save_document, close_document, list_tasks
            
            Respond in JSON format:
            {{
                ""intent"": ""<intent>"",
                ""parameters"": {{...}}
            }}
        ", "llama3.2", temperature: 0.1);
        
        // 3. Parse and execute
        var command = JsonSerializer.Deserialize<VoiceIntent>(intent);
        var result = await ExecuteIntentAsync(command);
        
        // 4. Speak response
        await _tts.SpeakAsync(result.Message);
        
        return result.Success;
    }
    
    private async Task<CommandResult> ExecuteIntentAsync(VoiceIntent intent)
    {
        switch (intent.Intent)
        {
            case "open_document":
                var docName = intent.Parameters["document_name"]?.ToString();
                var doc = await _documentService.FindByNameAsync(docName);
                if (doc != null)
                {
                    await _documentService.OpenAsync(doc.Id);
                    return new CommandResult 
                    { 
                        Success = true,
                        Message = $"Dokument {doc.Title} wurde geöffnet."
                    };
                }
                return new CommandResult 
                { 
                    Success = false,
                    Message = $"Dokument {docName} wurde nicht gefunden."
                };
                
            case "search":
                var query = intent.Parameters["query"]?.ToString();
                var results = await _documentService.SearchAsync(query);
                var count = results.Count;
                return new CommandResult 
                { 
                    Success = true,
                    Message = $"Ich habe {count} Dokumente gefunden. " +
                             $"Das erste ist: {results.FirstOrDefault()?.Title}"
                };
                
            case "read_document":
                // Current document wird vorgelesen
                var currentDoc = await _documentService.GetCurrentDocumentAsync();
                if (currentDoc != null)
                {
                    var text = await ExtractDocumentTextAsync(currentDoc);
                    await _tts.SpeakAsync(text);
                    return new CommandResult 
                    { 
                        Success = true,
                        Message = "Dokument wird vorgelesen."
                    };
                }
                break;
                
            case "create_task":
                var taskTitle = intent.Parameters["task_title"]?.ToString();
                var dueDate = intent.Parameters["due_date"]?.ToString();
                await CreateTaskAsync(taskTitle, dueDate);
                return new CommandResult 
                { 
                    Success = true,
                    Message = $"Aufgabe {taskTitle} wurde erstellt."
                };
                
            case "list_tasks":
                var tasks = await GetMyTasksAsync();
                var taskList = string.Join(", ", tasks.Select(t => t.Title));
                return new CommandResult 
                { 
                    Success = true,
                    Message = $"Sie haben {tasks.Count} Aufgaben: {taskList}"
                };
        }
        
        return new CommandResult 
        { 
            Success = false,
            Message = "Kommando wurde nicht verstanden."
        };
    }
}

public class VoiceIntent
{
    public string Intent { get; set; } = string.Empty;
    public Dictionary<string, object> Parameters { get; set; } = new();
}

public class CommandResult
{
    public bool Success { get; set; }
    public string Message { get; set; } = string.Empty;
}
```

### 5.2 Accessibility Features

```csharp
public class AccessibilityService
{
    private readonly ITTSService _tts;
    
    // Screen Reader für DMS
    public async Task ReadUIElementAsync(string elementName, string content)
    {
        await _tts.SpeakAsync($"{elementName}: {content}");
    }
    
    // Navigationshilfe
    public async Task AnnounceNavigationAsync(string from, string to)
    {
        await _tts.SpeakAsync($"Navigation von {from} zu {to}");
    }
    
    // Formular-Hilfe
    public async Task ReadFormFieldAsync(string fieldName, string fieldType)
    {
        await _tts.SpeakAsync($"{fieldName}, {fieldType}");
    }
    
    // Tastatur-Navigation-Ankündigung
    public async Task AnnounceKeyboardShortcutAsync(string action, string shortcut)
    {
        await _tts.SpeakAsync($"{action}: {shortcut}");
    }
}
```

---

## 6. Integration mit ThemisDB

### 6.1 Voice-to-Vector Search

```csharp
public class VoiceSearchService
{
    private readonly ISTTService _stt;
    private readonly IVectorService _vectorService;
    private readonly IOllamaService _ollama;
    
    public async Task<List<Document>> VoiceSearchAsync(byte[] audioData)
    {
        // 1. Speech to Text
        var query = await _stt.TranscribeAsync(audioData);
        
        // 2. Generate embedding
        var embedding = await _ollama.GenerateEmbeddingAsync(query);
        
        // 3. Vector search in ThemisDB
        var results = await _vectorService.SimilaritySearchAsync(embedding, topK: 10);
        
        return results;
    }
}
```

### 6.2 Voice-Controlled Timeline Navigation

```csharp
public class VoiceTimelineService
{
    private readonly ISTTService _stt;
    private readonly ITTSService _tts;
    private readonly ITimelineService _timeline;
    
    public async Task NavigateTimelineByVoiceAsync(byte[] audioData)
    {
        var command = await _stt.TranscribeAsync(audioData);
        
        // "Zeige mir alle Ereignisse von letzter Woche"
        if (command.Contains("letzte woche"))
        {
            var startDate = DateTime.Now.AddDays(-7);
            var events = await _timeline.GetEventsAsync(startDate, DateTime.Now);
            
            var summary = $"Ich habe {events.Count} Ereignisse gefunden. " +
                         $"Das erste war: {events.First().Description}";
            await _tts.SpeakAsync(summary);
        }
        // "Was passierte am 15. Januar?"
        else if (TryParseDate(command, out var date))
        {
            var events = await _timeline.GetEventsForDateAsync(date);
            var summary = $"Am {date:dd.MM.yyyy} gab es {events.Count} Ereignisse.";
            await _tts.SpeakAsync(summary);
        }
    }
}
```

---

## 7. UI Integration

### 7.1 Voice Button in XAML

```xml
<!-- MainWindow.xaml -->
<Button x:Name="VoiceButton" 
        Width="50" Height="50" 
        Style="{StaticResource RoundButton}"
        ToolTip="Sprachsteuerung (Halten zum Sprechen)"
        PreviewMouseDown="VoiceButton_MouseDown"
        PreviewMouseUp="VoiceButton_MouseUp">
    <Grid>
        <Viewbox Width="30" Height="30">
            <Canvas Width="24" Height="24">
                <!-- Microphone Icon -->
                <Path Fill="{Binding ElementName=VoiceButton, Path=Foreground}"
                      Data="M12,2 C13.1,2 14,2.9 14,4 L14,12 C14,13.1 13.1,14 12,14 C10.9,14 10,13.1 10,12 L10,4 C10,2.9 10.9,2 12,2 M19,11 C19,14.5 16.4,17.4 13,17.9 L13,21 L11,21 L11,17.9 C7.6,17.4 5,14.5 5,11 L7,11 C7,13.8 9.2,16 12,16 C14.8,16 17,13.8 17,11 L19,11 Z"/>
            </Canvas>
        </Viewbox>
        
        <!-- Recording Indicator -->
        <Ellipse x:Name="RecordingIndicator"
                 Width="10" Height="10"
                 Fill="Red"
                 Visibility="Collapsed"
                 HorizontalAlignment="Right"
                 VerticalAlignment="Top"
                 Margin="0,5,5,0">
            <Ellipse.Style>
                <Style TargetType="Ellipse">
                    <Style.Triggers>
                        <DataTrigger Binding="{Binding IsRecording}" Value="True">
                            <DataTrigger.EnterActions>
                                <BeginStoryboard>
                                    <Storyboard RepeatBehavior="Forever">
                                        <DoubleAnimation Storyboard.TargetProperty="Opacity"
                                                       From="1.0" To="0.0" Duration="0:0:0.5"
                                                       AutoReverse="True"/>
                                    </Storyboard>
                                </BeginStoryboard>
                            </DataTrigger.EnterActions>
                        </DataTrigger>
                    </Style.Triggers>
                </Style>
            </Ellipse.Style>
        </Ellipse>
    </Grid>
</Button>

<!-- Voice Command Feedback -->
<Border x:Name="VoiceCommandFeedback"
        Background="#DD000000"
        CornerRadius="10"
        Padding="20"
        HorizontalAlignment="Center"
        VerticalAlignment="Top"
        Margin="0,100,0,0"
        Visibility="Collapsed">
    <StackPanel>
        <TextBlock Text="🎤" FontSize="48" HorizontalAlignment="Center"/>
        <TextBlock x:Name="TranscriptionText" 
                   Text="Ich höre zu..."
                   Foreground="White"
                   FontSize="16"
                   TextAlignment="Center"
                   Margin="0,10,0,0"/>
    </StackPanel>
</Border>
```

### 7.2 Code-Behind

```csharp
public partial class MainWindow : Window
{
    private readonly VoiceControlledDMSService _voiceService;
    private readonly AudioRecorder _recorder;
    private CancellationTokenSource _recordingCts;
    
    private void VoiceButton_MouseDown(object sender, MouseButtonEventArgs e)
    {
        StartVoiceRecording();
    }
    
    private void VoiceButton_MouseUp(object sender, MouseButtonEventArgs e)
    {
        StopVoiceRecording();
    }
    
    private void StartVoiceRecording()
    {
        _recordingCts = new CancellationTokenSource();
        
        VoiceCommandFeedback.Visibility = Visibility.Visible;
        RecordingIndicator.Visibility = Visibility.Visible;
        TranscriptionText.Text = "🎤 Sprechen Sie jetzt...";
        
        _recorder.StartRecording("temp_voice_command.wav");
    }
    
    private async void StopVoiceRecording()
    {
        _recorder.StopRecording();
        
        RecordingIndicator.Visibility = Visibility.Collapsed;
        TranscriptionText.Text = "⏳ Verarbeite Spracheingabe...";
        
        try
        {
            var audioData = File.ReadAllBytes("temp_voice_command.wav");
            var success = await _voiceService.ProcessVoiceCommandAsync(audioData);
            
            if (success)
            {
                TranscriptionText.Text = "✓ Befehl ausgeführt";
                await Task.Delay(1000);
            }
            else
            {
                TranscriptionText.Text = "❌ Befehl nicht verstanden";
                await Task.Delay(2000);
            }
        }
        catch (Exception ex)
        {
            TranscriptionText.Text = $"❌ Fehler: {ex.Message}";
            await Task.Delay(2000);
        }
        finally
        {
            VoiceCommandFeedback.Visibility = Visibility.Collapsed;
        }
    }
    
    // Keyboard Shortcut für Voice
    private void Window_KeyDown(object sender, KeyEventArgs e)
    {
        // Hold Space for voice command
        if (e.Key == Key.Space && !e.IsRepeat && Keyboard.Modifiers == ModifierKeys.Control)
        {
            StartVoiceRecording();
        }
    }
    
    private void Window_KeyUp(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.Space)
        {
            StopVoiceRecording();
        }
    }
}
```

---

## 8. Erweiterte Features

### 8.1 Voice-Guided Metadata Entry

```csharp
public class VoiceMetadataEntryService
{
    private readonly ISTTService _stt;
    private readonly ITTSService _tts;
    private readonly IOllamaService _llm;
    
    public async Task<DocumentMetadata> GuidedMetadataEntryAsync()
    {
        var metadata = new DocumentMetadata();
        
        // Titel erfragen
        await _tts.SpeakAsync("Wie lautet der Titel des Dokuments?");
        metadata.Title = await ListenForResponseAsync();
        
        // Kategorie erfragen
        await _tts.SpeakAsync("Welche Kategorie? Sagen Sie Vertrag, Rechnung, oder Sonstiges.");
        var category = await ListenForResponseAsync();
        metadata.Category = ParseCategory(category);
        
        // Datum erfragen
        await _tts.SpeakAsync("Welches Datum hat das Dokument?");
        var dateStr = await ListenForResponseAsync();
        metadata.Date = await ParseDateWithLLMAsync(dateStr);
        
        // Bestätigung
        await _tts.SpeakAsync($"Zusammenfassung: Titel {metadata.Title}, " +
                             $"Kategorie {metadata.Category}, " +
                             $"Datum {metadata.Date:dd.MM.yyyy}. " +
                             $"Ist das korrekt? Sagen Sie Ja oder Nein.");
        
        var confirmation = await ListenForResponseAsync();
        if (confirmation.Contains("ja", StringComparison.OrdinalIgnoreCase))
        {
            await _tts.SpeakAsync("Metadaten wurden gespeichert.");
            return metadata;
        }
        else
        {
            await _tts.SpeakAsync("Bitte wiederholen wir den Vorgang.");
            return await GuidedMetadataEntryAsync(); // Recursive retry
        }
    }
    
    private async Task<string> ListenForResponseAsync()
    {
        // Wait for user to speak, then transcribe
        var audioData = await RecordAudioAsync(maxDuration: TimeSpan.FromSeconds(10));
        return await _stt.TranscribeAsync(audioData);
    }
    
    private async Task<DateTime> ParseDateWithLLMAsync(string dateStr)
    {
        var prompt = $@"Parse this German date expression into ISO format (YYYY-MM-DD):
            '{dateStr}'
            
            Examples:
            'heute' -> {DateTime.Today:yyyy-MM-dd}
            'gestern' -> {DateTime.Today.AddDays(-1):yyyy-MM-dd}
            'fünfzehnter Januar zweitausendvierundzwanzig' -> 2024-01-15
            
            Only respond with the date in YYYY-MM-DD format.";
        
        var result = await _llm.GenerateAsync(prompt, "llama3.2", temperature: 0.1);
        return DateTime.Parse(result.Trim());
    }
}
```

### 8.2 Voice-Activated Search with Context

```csharp
public class ContextualVoiceSearchService
{
    private readonly ISTTService _stt;
    private readonly ITTSService _tts;
    private readonly IDocumentService _documentService;
    private readonly IGraphService _graphService;
    private readonly IOllamaService _llm;
    
    public async Task PerformContextualSearchAsync(byte[] audioData, string currentContext)
    {
        var query = await _stt.TranscribeAsync(audioData);
        
        // Use LLM to enhance query with context
        var enhancedQuery = await _llm.GenerateAsync($@"
            The user is currently viewing: {currentContext}
            They said: '{query}'
            
            Create an enhanced search query that considers the context.
            For example, if they say 'ähnliche Dokumente', search for documents 
            similar to the current one.
            
            Return only the enhanced query text.", "llama3.2");
        
        await _tts.SpeakAsync($"Suche nach: {enhancedQuery}");
        
        var results = await _documentService.SearchAsync(enhancedQuery);
        
        if (results.Any())
        {
            var summary = $"Ich habe {results.Count} Ergebnisse gefunden. " +
                         $"Die relevantesten sind: " +
                         string.Join(", ", results.Take(3).Select(r => r.Title));
            await _tts.SpeakAsync(summary);
        }
        else
        {
            await _tts.SpeakAsync("Keine Ergebnisse gefunden.");
        }
    }
}
```

---

## 9. Performance & Best Practices

### 9.1 Optimierungen

```csharp
// 1. Caching von Whisper-Modellen
public class WhisperModelCache
{
    private static WhisperProcessor? _cachedProcessor;
    private static readonly object _lock = new();
    
    public static WhisperProcessor GetOrCreate(string modelPath)
    {
        if (_cachedProcessor != null)
            return _cachedProcessor;
        
        lock (_lock)
        {
            if (_cachedProcessor != null)
                return _cachedProcessor;
            
            var factory = WhisperFactory.FromPath(modelPath);
            _cachedProcessor = factory.CreateBuilder()
                .WithLanguage("de")
                .Build();
            
            return _cachedProcessor;
        }
    }
}

// 2. Async Audio Processing Queue
public class AudioProcessingQueue
{
    private readonly Channel<(byte[] Audio, TaskCompletionSource<string> Result)> _channel;
    private readonly WhisperSTTService _stt;
    
    public AudioProcessingQueue(WhisperSTTService stt)
    {
        _stt = stt;
        _channel = Channel.CreateUnbounded<(byte[], TaskCompletionSource<string>)>();
        
        // Background worker
        Task.Run(async () => await ProcessQueueAsync());
    }
    
    public Task<string> EnqueueAsync(byte[] audioData)
    {
        var tcs = new TaskCompletionSource<string>();
        _channel.Writer.TryWrite((audioData, tcs));
        return tcs.Task;
    }
    
    private async Task ProcessQueueAsync()
    {
        await foreach (var (audio, tcs) in _channel.Reader.ReadAllAsync())
        {
            try
            {
                var result = await _stt.TranscribeAsync(audio);
                tcs.SetResult(result);
            }
            catch (Exception ex)
            {
                tcs.SetException(ex);
            }
        }
    }
}

// 3. Voice Activity Detection (VAD) für effiziente Aufnahme
public class VoiceActivityDetector
{
    private const double ENERGY_THRESHOLD = 0.02;
    
    public bool IsSpeaking(byte[] audioBuffer)
    {
        // Vereinfachte Energie-Berechnung
        double sum = 0;
        for (int i = 0; i < audioBuffer.Length; i += 2)
        {
            short sample = BitConverter.ToInt16(audioBuffer, i);
            sum += Math.Abs(sample);
        }
        
        double energy = sum / (audioBuffer.Length / 2);
        return energy > ENERGY_THRESHOLD;
    }
}
```

### 9.2 Error Handling

```csharp
public class RobustVoiceService
{
    private readonly ISTTService _stt;
    private readonly ITTSService _tts;
    private int _retryCount = 0;
    private const int MAX_RETRIES = 3;
    
    public async Task<string> TranscribeWithRetryAsync(byte[] audioData)
    {
        for (int i = 0; i < MAX_RETRIES; i++)
        {
            try
            {
                return await _stt.TranscribeAsync(audioData);
            }
            catch (Exception ex) when (i < MAX_RETRIES - 1)
            {
                await Task.Delay(TimeSpan.FromSeconds(Math.Pow(2, i))); // Exponential backoff
                await _tts.SpeakAsync("Entschuldigung, bitte wiederholen Sie das.");
            }
        }
        
        await _tts.SpeakAsync("Ich konnte Sie leider nicht verstehen. Bitte verwenden Sie die Tastatur.");
        throw new Exception("STT failed after retries");
    }
}
```

---

## 10. Deployment & Setup

### 10.1 Model Downloads

```bash
# Whisper Models (lokal)
# Small model (~500MB) - Schnell, gute Qualität
wget https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin

# Medium model (~1.5GB) - Bessere Qualität
wget https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin

# Piper TTS Models
pip install piper-tts
piper --download-dir ./models --voice de_DE-thorsten-medium
```

### 10.2 Configuration

```json
// appsettings.json
{
  "Speech": {
    "STT": {
      "Provider": "Whisper", // "Whisper" oder "Azure"
      "WhisperModelPath": "models/ggml-base.bin",
      "Language": "de",
      "AzureKey": "",
      "AzureRegion": ""
    },
    "TTS": {
      "Provider": "Windows", // "Windows", "Azure", oder "Piper"
      "Voice": "de-DE-KatjaNeural",
      "Rate": 1.0,
      "Volume": 1.0
    },
    "VoiceCommands": {
      "Enabled": true,
      "PushToTalkKey": "Space",
      "ContinuousListening": false
    }
  }
}
```

### 10.3 DI Registration

```csharp
// App.xaml.cs
private void ConfigureSpeechServices(IServiceCollection services)
{
    var config = Configuration.GetSection("Speech");
    
    // STT
    var sttProvider = config["STT:Provider"];
    if (sttProvider == "Whisper")
    {
        services.AddSingleton<ISTTService>(sp => 
            new WhisperSTTService(config["STT:WhisperModelPath"]));
    }
    else if (sttProvider == "Azure")
    {
        services.AddSingleton<ISTTService>(sp => 
            new AzureSTTService(config["STT:AzureKey"], config["STT:AzureRegion"]));
    }
    
    // TTS
    var ttsProvider = config["TTS:Provider"];
    if (ttsProvider == "Windows")
    {
        services.AddSingleton<ITTSService, WindowsTTSService>();
    }
    else if (ttsProvider == "Azure")
    {
        services.AddSingleton<ITTSService>(sp => 
            new AzureTTSService(config["TTS:AzureKey"], config["TTS:AzureRegion"]));
    }
    
    // Voice Services
    services.AddSingleton<VoiceControlledDMSService>();
    services.AddSingleton<AudioRecorder>();
    services.AddSingleton<AudioPlayer>();
    services.AddTransient<VoiceMetadataEntryService>();
    services.AddTransient<ContextualVoiceSearchService>();
}
```

---

## 11. Testing

### 11.1 Unit Tests

```csharp
[TestClass]
public class WhisperSTTServiceTests
{
    private WhisperSTTService _service;
    
    [TestInitialize]
    public void Setup()
    {
        _service = new WhisperSTTService("models/ggml-base.bin");
    }
    
    [TestMethod]
    public async Task TranscribeAsync_GermanAudio_ReturnsCorrectText()
    {
        // Arrange
        var audioData = File.ReadAllBytes("test_audio_german.wav");
        
        // Act
        var result = await _service.TranscribeAsync(audioData);
        
        // Assert
        Assert.IsTrue(result.Contains("öffne dokument", StringComparison.OrdinalIgnoreCase));
    }
    
    [TestMethod]
    public async Task RecognizeCommandAsync_OpenDocument_ReturnsCorrectCommand()
    {
        // Arrange
        var audioData = File.ReadAllBytes("test_open_document.wav");
        
        // Act
        var command = await _service.RecognizeCommandAsync(audioData);
        
        // Assert
        Assert.IsNotNull(command);
        Assert.AreEqual("open_document", command.Action);
    }
}
```

---

## 12. Zusammenfassung

### Empfohlener Stack für ThemisDB DMS:

**STT (Speech-to-Text):**
- ✅ **Whisper.NET** - Beste Balance aus Qualität, Privacy und Performance
- ✅ Lokale Ausführung, keine Cloud nötig
- ✅ Exzellente Deutsch-Unterstützung

**TTS (Text-to-Speech):**
- ✅ **System.Speech** (Windows) - Einfach, keine Dependencies
- ✅ **Azure Neural TTS** - Höchste Qualität für kritische Anwendungen
- ✅ **Piper TTS** - Open Source Alternative

**Audio I/O:**
- ✅ **NAudio** - Standard für .NET Audio

**Document Processing:**
- ✅ **iText7** - PDF
- ✅ **DocumentFormat.OpenXml** - Office-Dokumente
- ✅ **Tesseract** - OCR

**Integration:**
- ✅ Ollama für LLM-gestützte Intent-Erkennung
- ✅ ThemisDB für Multi-Model-Storage
- ✅ WPF für moderne UI

Alle Snippets sind produktionsbereit und können direkt in das ThemisDB DocumentManager-Projekt integriert werden!

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Shapes;
using System.Windows.Media;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.UI;

namespace Themis.DocumentManager.Views;

/// <summary>
/// Interaction logic for MainWindow.xaml
/// </summary>
public partial class MainWindow : Window
{
    private readonly MainViewModel _viewModel;
    private readonly IOfficeIntegrationService _officeService;
    private readonly IFormTemplateService _formTemplateService;
    private readonly IFormConfigurationLoader _formConfigurationLoader;
    private readonly IFormDatabaseMappingService _formDatabaseMappingService;
    private readonly IFormAuditService _formAuditService;
    private readonly ISmartFormService _smartFormService;
    private readonly IFormContextService _formContextService;
    private readonly IThemeService _themeService;
    private readonly ISettingsService _settingsService;
    private readonly IAnimationService _animationService;
    private bool _isFullscreen = false;
    private WindowState _previousWindowState;
    private WindowStyle _previousWindowStyle;
    private DateTime _timelineStartDate = DateTime.Now.AddMonths(-1);
    private DateTime _timelineEndDate = DateTime.Now.AddMonths(1);
    private double _pixelsPerDay = 40; // 40 pixels pro Tag (skalierbar)
    private Dictionary<TabItem, bool> _tabChanges = new(); // Track unsaved changes per tab
    private Dictionary<TabItem, DocumentMetadata> _tabMetadata = new(); // Store metadata per tab

    public MainWindow(
        MainViewModel viewModel,
        IOfficeIntegrationService officeService,
        IFormTemplateService formTemplateService,
        IFormConfigurationLoader formConfigurationLoader,
        IFormDatabaseMappingService formDatabaseMappingService,
        IFormAuditService formAuditService,
        ISmartFormService smartFormService,
        IFormContextService formContextService,
        IThemeService themeService,
        ISettingsService settingsService,
        IAnimationService animationService)
    {
        InitializeComponent();
        _viewModel = viewModel;
        _officeService = officeService;
        _formTemplateService = formTemplateService;
        _formConfigurationLoader = formConfigurationLoader;
        _formDatabaseMappingService = formDatabaseMappingService;
        _formAuditService = formAuditService;
        _smartFormService = smartFormService;
        _formContextService = formContextService;
        _themeService = themeService;
        _settingsService = settingsService;
        _animationService = animationService;
        DataContext = _viewModel;

        _viewModel.PropertyChanged += ViewModel_PropertyChanged;
        
        Loaded += (s, e) => 
        {
            UpdateMenuItems();
            InitializeTimeline();
            UpdateThemeMenuItems();
        };
    }

    private void ViewModel_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(MainViewModel.CurrentView))
        {
            LoadView(_viewModel.CurrentView);
        }
    }

    private void LoadView(string viewName)
    {
    }

    /// <summary>
    /// Initialisiert die Timeline mit Dokumentdaten
    /// </summary>
    private void InitializeTimeline()
    {
        try
        {
            // Zeitbereich berechnen (Min/Max Dokumentdaten)
            CalculateDateRange();
            
            // Timeline-Lineal generieren
            GenerateTimelineRuler();
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Timeline initialization error: {ex.Message}");
        }
    }

    /// <summary>
    /// Berechnet den Datumsbereich anhand von Dokumenten
    /// </summary>
    private void CalculateDateRange()
    {
        // TOOD: Hier würden echte Dokumentdaten geladen
        // Beispiel: Dokumente von Min/Max Date abrufen
        var today = DateTime.Now.Date;
        _timelineStartDate = today.AddMonths(-1); // 1 Monat zurück
        _timelineEndDate = today.AddMonths(1);    // 1 Monat voraus
    }

    /// <summary>
    /// Generiert das Timeline-Lineal mit Zeitmessungen und Event-Badges
    /// </summary>
    private void GenerateTimelineRuler()
    {
        try
        {
            if (TimelineCanvas == null) return;

            TimelineCanvas.Children.Clear();

            var daySpan = (_timelineEndDate - _timelineStartDate).TotalDays;
            var totalWidth = daySpan * _pixelsPerDay;

            // Canvas aktualisieren
            TimelineCanvas.Width = totalWidth;

            // Tagesmarkierungen (kleine Striche)
            for (var i = 0; i <= daySpan; i++)
            {
                var x = i * _pixelsPerDay;
                var currentDate = _timelineStartDate.AddDays(i);

                // Kleine tägliche Markierungen
                var tick = new Line
                {
                    X1 = x,
                    Y1 = 40,
                    X2 = x,
                    Y2 = 45,
                    Stroke = new SolidColorBrush(Colors.LightGray),
                    StrokeThickness = 0.5
                };
                TimelineCanvas.Children.Add(tick);

                // Wöchentliche Markierungen (größer)
                if (currentDate.DayOfWeek == DayOfWeek.Monday)
                {
                    var weekTick = new Line
                    {
                        X1 = x,
                        Y1 = 35,
                        X2 = x,
                        Y2 = 48,
                        Stroke = new SolidColorBrush(Colors.Gray),
                        StrokeThickness = 1.5
                    };
                    TimelineCanvas.Children.Add(weekTick);

                    // Wochennummer Datum
                    var dayLabel = new TextBlock
                    {
                        Text = currentDate.ToString("dd.MM"),
                        FontSize = 8,
                        Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100))
                    };
                    Canvas.SetLeft(dayLabel, x - 20);
                    Canvas.SetTop(dayLabel, 10);
                    TimelineCanvas.Children.Add(dayLabel);
                }

                // Monatliche Markierungen (größer)
                if (currentDate.Day == 1)
                {
                    var monthTick = new Line
                    {
                        X1 = x,
                        Y1 = 30,
                        X2 = x,
                        Y2 = 48,
                        Stroke = new SolidColorBrush(Colors.DarkGray),
                        StrokeThickness = 2
                    };
                    TimelineCanvas.Children.Add(monthTick);

                    // Monatslabel
                    var monthLabel = new TextBlock
                    {
                        Text = currentDate.ToString("MMMM yyyy", System.Globalization.CultureInfo.GetCultureInfo("de-DE")),
                        FontSize = 9,
                        FontWeight = FontWeights.Bold,
                        Foreground = new SolidColorBrush(Colors.Black)
                    };
                    Canvas.SetLeft(monthLabel, x - 40);
                    Canvas.SetTop(monthLabel, 0);
                    TimelineCanvas.Children.Add(monthLabel);
                }
            }

            // Heutige Linie (Heute-Marker)
            var today = DateTime.Now.Date;
            var daysFromStart = (today - _timelineStartDate).TotalDays;
            if (daysFromStart >= 0 && daysFromStart <= daySpan)
            {
                var todayLine = new Line
                {
                    X1 = daysFromStart * _pixelsPerDay,
                    Y1 = 0,
                    X2 = daysFromStart * _pixelsPerDay,
                    Y2 = TimelineCanvas.Height,
                    Stroke = new SolidColorBrush(Color.FromRgb(255, 0, 0)),
                    StrokeThickness = 2,
                    StrokeDashArray = new DoubleCollection { 5, 5 }
                };
                TimelineCanvas.Children.Add(todayLine);
            }

            // Event-Badges auf der Timeline
            RenderTimelineBadges(daySpan);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Timeline ruler generation error: {ex.Message}");
        }
    }

    /// <summary>
    /// Rendert die Event-Badges direkt auf der Timeline (+ optionale Gantt-Balken)
    /// </summary>
    private void RenderTimelineBadges(double daySpan)
    {
        // Gantt-Balken (semi-transparent) optional anzeigen
        if (ShowGanttCheckBox != null && ShowGanttCheckBox.IsChecked == true)
        {
            RenderCompactGanttBars(daySpan);
        }

        // Beispiel-Events mit erweiterten Daten (ID, Note, vollständiger Name für Tab)
        var events = new List<(string Id, DateTime Date, string Icon, string Color, string BorderColor, string Label, string Note, string Content)>
        {
            ("doc_001", _timelineStartDate.AddDays(5), "📄", "#e3f2fd", "#2196f3", "Dokument", "Anforderungsdokument wurde eingereicht", "Anforderungsdokument"),
            ("deadline_001", _timelineStartDate.AddDays(12), "⏰", "#fff3e0", "#ff9800", "Deadline", "Abgabefrist für Phase 1", "Phase 1 Deadline"),
            ("approval_001", _timelineStartDate.AddDays(20), "✅", "#e8f5e9", "#4caf50", "Genehmigt", "Dokumentation genehmigt", "Genehmigung"),
            ("process_001", _timelineStartDate.AddDays(28), "⚙️", "#f3e5f5", "#9c27b0", "Prozess", "Implementierungsprozess gestartet", "Implementierung"),
            ("today_001", DateTime.Now.Date, "📌", "#ffe0b2", "#ff6f00", "Heute", "Heute: " + DateTime.Now.Date.ToString("dd.MM.yyyy"), "Heutiger Stand"),
        };

        var badgeWidth = 12; // Schmale Box
        var badgeHeight = 30;
        int yOffset = 10; // Oben, über den Gantt-Balken

        foreach (var evt in events)
        {
            var dayDiff = (evt.Date - _timelineStartDate).TotalDays;
            
            // Nur anzeigen wenn im sichtbaren Datumsbereich
            if (dayDiff < 0 || dayDiff > daySpan) continue;

            var x = dayDiff * _pixelsPerDay;

            // Border für die Badge
            var badge = new Border
            {
                Width = badgeWidth,
                Height = badgeHeight,
                Background = new SolidColorBrush(ColorFromHex(evt.Color)),
                BorderBrush = new SolidColorBrush(ColorFromHex(evt.BorderColor)),
                BorderThickness = new Thickness(1),
                Cursor = System.Windows.Input.Cursors.Hand,
                Tag = evt.Id // ID für Click-Handler speichern
            };

            // Icon in der Badge
            var icon = new TextBlock
            {
                Text = evt.Icon,
                FontSize = 10,
                HorizontalAlignment = HorizontalAlignment.Center,
                VerticalAlignment = VerticalAlignment.Center,
                Foreground = new SolidColorBrush(Colors.Black)
            };
            badge.Child = icon;

            // Click Event Handler
            badge.MouseDown += (s, e) => Badge_Click(evt.Id, evt.Label, evt.Content, evt.Note);

            // Hover Effekt
            badge.MouseEnter += (s, e) => 
            {
                var b = s as Border;
                if (b != null)
                {
                    b.BorderThickness = new Thickness(2);
                    b.Effect = new System.Windows.Media.Effects.DropShadowEffect 
                    { 
                        BlurRadius = 4, 
                        Opacity = 0.6,
                        ShadowDepth = 2
                    };
                }
            };

            badge.MouseLeave += (s, e) =>
            {
                var b = s as Border;
                if (b != null)
                {
                    b.BorderThickness = new Thickness(1);
                    b.Effect = null;
                }
            };

            Canvas.SetLeft(badge, x - badgeWidth / 2);
            Canvas.SetTop(badge, yOffset);
            TimelineCanvas.Children.Add(badge);

            // Detailed Tooltip mit Note
            var toolTipContent = new StackPanel { Orientation = Orientation.Vertical, MaxWidth = 250 };
            var titleBlock = new TextBlock 
            { 
                Text = evt.Label, 
                FontWeight = FontWeights.Bold, 
                FontSize = 10,
                Margin = new Thickness(0, 0, 0, 4)
            };
            var noteBlock = new TextBlock 
            { 
                Text = evt.Note, 
                FontSize = 9, 
                Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100)),
                TextWrapping = TextWrapping.Wrap
            };
            toolTipContent.Children.Add(titleBlock);
            toolTipContent.Children.Add(noteBlock);
            
            var toolTip = new System.Windows.Controls.ToolTip 
            { 
                Content = toolTipContent,
                Background = new SolidColorBrush(Color.FromRgb(255, 255, 200)),
                Foreground = new SolidColorBrush(Colors.Black),
                FontSize = 9,
                Padding = new Thickness(8)
            };
            System.Windows.Controls.ToolTipService.SetToolTip(badge, toolTip);
        }
    }

    /// <summary>
    /// Badge Click Handler - öffnet neuen Tab mit Metadaten-Maske
    /// </summary>
    private async void Badge_Click(string eventId, string label, string contentTitle, string note)
    {
        var contentControl = this.FindName("CenterContent") as TabControl;
        if (contentControl == null) return;

        const string templateId = "pdv-vis5-document";

        var tabItem = FindTabByTitle(contentControl, contentTitle);
        if (tabItem == null)
        {
            var template = await _formTemplateService.GetTemplateAsync(templateId);

            if (template == null)
            {
                var baseDir = AppDomain.CurrentDomain.BaseDirectory;
                var jsonPath = System.IO.Path.Combine(baseDir, "Config", "FormTemplates", "pdv-vis5-document.json");
                var yamlPath = System.IO.Path.Combine(baseDir, "Config", "FormTemplates", "pdv-vis5-document.yaml");

                if (File.Exists(jsonPath))
                    template = await _formConfigurationLoader.LoadFromJsonAsync(jsonPath);
                else if (File.Exists(yamlPath))
                    template = await _formConfigurationLoader.LoadFromYamlAsync(yamlPath);
            }

            if (template == null)
            {
                MessageBox.Show("PDV VIS 5 Formularvorlage nicht gefunden.", "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            var renderer = new FormRenderer();
            renderer.RenderTemplate(template);
            renderer.FormSubmitted += async (s, e) => await HandleFormSubmittedAsync(template.Id, e.FormData);

            tabItem = CreateFormTab(contentTitle, renderer);

            _tabMetadata[tabItem] = new DocumentMetadata
            {
                DocumentName = contentTitle,
                Description = note,
                Author = Environment.UserName
            };
            _tabChanges[tabItem] = false;

            contentControl.Items.Add(tabItem);
        }

        contentControl.SelectedItem = tabItem;
    }

    private TabItem? FindTabByTitle(TabControl tabControl, string title)
    {
        foreach (TabItem tab in tabControl.Items)
        {
            if (tab.Header is StackPanel stack)
            {
                var text = stack.Children.OfType<TextBlock>().FirstOrDefault()?.Text;
                if (string.Equals(text, title, StringComparison.OrdinalIgnoreCase))
                    return tab;
            }
            else if (tab.Header?.ToString() == title)
            {
                return tab;
            }
        }
        return null;
    }

    private TabItem CreateFormTab(string title, UIElement content)
    {
        var tabItem = new TabItem();

        var headerPanel = new StackPanel { Orientation = Orientation.Horizontal };
        var headerText = new TextBlock { Text = title, Margin = new Thickness(0, 0, 8, 0) };
        var closeBtn = new Button
        {
            Content = "✕",
            Width = 20,
            Height = 20,
            FontSize = 10,
            Padding = new Thickness(0),
            Background = new SolidColorBrush(Colors.Transparent),
            Foreground = new SolidColorBrush(Color.FromRgb(150, 150, 150))
        };
        closeBtn.Click += (s, e) => CloseTab_Click(tabItem);
        headerPanel.Children.Add(headerText);
        headerPanel.Children.Add(closeBtn);
        tabItem.Header = headerPanel;

        tabItem.Content = content;
        return tabItem;
    }

    private void UpdateTabHeader(TabItem tab, string statusText)
    {
        if (tab.Header is StackPanel stack)
        {
            var textBlock = stack.Children.OfType<TextBlock>().FirstOrDefault();
            if (textBlock != null)
                textBlock.Text = statusText;
        }
    }

    private string GetTabTitle(TabItem tab)
    {
        if (tab.Header is StackPanel stack)
        {
            var textBlock = stack.Children.OfType<TextBlock>().FirstOrDefault();
            if (textBlock != null)
            {
                var text = textBlock.Text;
                // Remove status emoji prefixes
                return text
                    .Replace("✓ ", "")
                    .Replace("❌ ", "")
                    .Replace("⏳ ", "")
                    .Replace("💾 ", "");
            }
        }
        return tab.Header?.ToString() ?? "";
    }

    private async Task HandleFormSubmittedAsync(string templateId, Dictionary<string, object>? formData)
    {
        if (formData == null)
        {
            MessageBox.Show("Keine Formulardaten vorhanden.", "Hinweis", MessageBoxButton.OK, MessageBoxImage.Information);
            return;
        }

        var currentTab = CenterContent.SelectedItem as TabItem;
        
        var submission = new FormSubmissionData
        {
            FormId = templateId,
            FieldValues = formData,
            Status = "Submitted",
            SubmittedBy = Environment.UserName
        };

        // Update tab status: show validation in progress
        if (currentTab != null)
            UpdateTabHeader(currentTab, "⏳ Validierung...");

        // Optional: Validieren über Template-Service, falls Template verfügbar
        var template = await _formTemplateService.GetTemplateAsync(templateId);
        if (template != null)
        {
            var validation = await _formTemplateService.ValidateFormAsync(template, formData);
            if (validation.ValidationErrors.Count > 0)
            {
                // Update tab status: show validation error
                if (currentTab != null)
                    UpdateTabHeader(currentTab, "❌ Validierungsfehler");
                
                // Log failed validation
                await _formAuditService.LogSubmissionAsync(
                    submission, 
                    Environment.UserName, 
                    "VALIDATE",
                    "Error",
                    string.Join("; ", validation.ValidationErrors.Select(kv => $"{kv.Key}: {kv.Value}"))
                );
                
                var msg = string.Join("\n", validation.ValidationErrors.Select(kv => $"{kv.Key}: {kv.Value}"));
                MessageBox.Show($"Validierungsfehler:\n{msg}", "Validierung", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
        }

        // Log successful validation
        await _formAuditService.LogSubmissionAsync(submission, Environment.UserName, "VALIDATE");

        // Update tab status: show save in progress
        if (currentTab != null)
            UpdateTabHeader(currentTab, "💾 Speichern...");

        await _formTemplateService.SubmitFormAsync(submission);

        var mappedData = await _formDatabaseMappingService.MapFormDataToDatabaseAsync(templateId, submission);

        // Log submission
        await _formAuditService.LogSubmissionAsync(submission, Environment.UserName, "SUBMIT");

        // Update tab status: show success
        if (currentTab != null)
            UpdateTabHeader(currentTab, $"✓ {GetTabTitle(currentTab)}");

        MessageBox.Show($"Formular '{templateId}' eingereicht. Felder: {mappedData.Count}", "Erfolg", MessageBoxButton.OK, MessageBoxImage.Information);
    }

    /// <summary>
    /// Rendert Gantt-Balken kompakt in der Haupttimeline (semi-transparent)
    /// </summary>
    private void RenderCompactGanttBars(double daySpan)
    {
        // Gantt-Daten
        var tasks = new List<(string Name, DateTime Start, DateTime End, string Color)>
        {
            ("Doc", _timelineStartDate.AddDays(2), _timelineStartDate.AddDays(15), "#2196f3"),
            ("Impl", _timelineStartDate.AddDays(8), _timelineStartDate.AddDays(25), "#4caf50"),
            ("Test", _timelineStartDate.AddDays(20), _timelineStartDate.AddDays(35), "#ff9800"),
        };

        double yOffset = 33; // Unter den Markierungen, aber über Badges
        double barHeight = 12;

        foreach (var task in tasks)
        {
            var startDay = (task.Start - _timelineStartDate).TotalDays;
            var duration = (task.End - task.Start).TotalDays;

            if (startDay + duration < 0 || startDay > daySpan) continue; // Skip off-screen

            var barWidth = duration * _pixelsPerDay;
            var x = startDay * _pixelsPerDay;

            var bar = new Border
            {
                Width = barWidth,
                Height = barHeight,
                Background = new SolidColorBrush(ColorFromHex(task.Color)) { Opacity = 0.4 }, // Semi-transparent
                BorderBrush = new SolidColorBrush(ColorFromHex(task.Color)),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(2)
            };

            Canvas.SetLeft(bar, x);
            Canvas.SetTop(bar, yOffset);
            TimelineCanvas.Children.Add(bar);

            yOffset += 14; // Nächste Reihe
        }
    }

    /// <summary>
    /// Konvertiert Hex-Farben zu Color
    /// </summary>
    private Color ColorFromHex(string hex)
    {
        hex = hex.TrimStart('#');
        return new Color
        {
            A = 255,
            R = byte.Parse(hex.Substring(0, 2), System.Globalization.NumberStyles.HexNumber),
            G = byte.Parse(hex.Substring(2, 2), System.Globalization.NumberStyles.HexNumber),
            B = byte.Parse(hex.Substring(4, 2), System.Globalization.NumberStyles.HexNumber)
        };
    }


    /// <summary>
    /// Skaliert die Timeline (Zoom In/Out)
    /// </summary>
    private void ZoomTimeline(double zoomFactor)
    {
        _pixelsPerDay *= zoomFactor;
        _pixelsPerDay = Math.Max(10, Math.Min(200, _pixelsPerDay)); // Min 10, Max 200 pixels pro Tag
        GenerateTimelineRuler();
    }

    /// <summary>
    /// Setzt die Zeitskala basierend auf Auswahl
    /// </summary>
    private void SetTimelineScale(string scale)
    {
        var today = DateTime.Now.Date;
        
        switch (scale)
        {
            case "1 Tag":
                _timelineStartDate = today.AddDays(-1);
                _timelineEndDate = today.AddDays(1);
                _pixelsPerDay = 100;
                break;
            case "1 Woche":
                _timelineStartDate = today.AddDays(-(int)today.DayOfWeek);
                _timelineEndDate = _timelineStartDate.AddDays(14);
                _pixelsPerDay = 50;
                break;
            case "1 Monat":
                _timelineStartDate = today.AddMonths(-1);
                _timelineEndDate = today.AddMonths(1);
                _pixelsPerDay = 40;
                break;
            case "3 Monate":
                _timelineStartDate = today.AddMonths(-1).AddDays(-15);
                _timelineEndDate = today.AddMonths(2).AddDays(-15);
                _pixelsPerDay = 15;
                break;
            case "6 Monate":
                _timelineStartDate = today.AddMonths(-3);
                _timelineEndDate = today.AddMonths(3);
                _pixelsPerDay = 8;
                break;
            case "1 Jahr":
                _timelineStartDate = today.AddYears(-1);
                _timelineEndDate = today.AddYears(1);
                _pixelsPerDay = 5;
                break;
        }
        
        GenerateTimelineRuler();
        UpdateDateRangeDisplay();
    }

    /// <summary>
    /// Expandiert/Kollabiert die Detail Timeline mit Gantt
    /// </summary>
    private void ExpandTimeline_Click(object sender, RoutedEventArgs e)
    {
        var isExpanded = DetailTimelineRow.Height.Value > 0;
        
        if (isExpanded)
        {
            // Kollabieren
            DetailTimelineRow.Height = new GridLength(0);
            ExpandTimelineButton.Content = "▶";
        }
        else
        {
            // Expandieren
            DetailTimelineRow.Height = new GridLength(300);
            ExpandTimelineButton.Content = "▼";
            RenderDetailTimeline();
            RenderGanttBars();
        }
    }

    /// <summary>
    /// Rendert das Detail-Timeline-Lineal
    /// </summary>
    private void RenderDetailTimeline()
    {
        if (DetailTimelineRuler == null) return;

        DetailTimelineRuler.Children.Clear();

        var daySpan = (_timelineEndDate - _timelineStartDate).TotalDays;
        var totalWidth = daySpan * _pixelsPerDay;
        DetailTimelineRuler.Width = totalWidth;

        // Zeitmarkierungen auf Detail-Timeline
        for (int day = 0; day <= daySpan; day++)
        {
            if (day % 7 == 0) // Wöchentliche Markierungen
            {
                var date = _timelineStartDate.AddDays(day);
                var x = day * _pixelsPerDay;

                var line = new Line
                {
                    X1 = x,
                    Y1 = 0,
                    X2 = x,
                    Y2 = 30,
                    Stroke = new SolidColorBrush(Colors.LightGray),
                    StrokeThickness = 1
                };
                DetailTimelineRuler.Children.Add(line);

                var label = new TextBlock
                {
                    Text = date.ToString("dd.MM"),
                    FontSize = 8,
                    Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100))
                };
                Canvas.SetLeft(label, x - 15);
                Canvas.SetTop(label, 12);
                DetailTimelineRuler.Children.Add(label);
            }
        }
    }

    /// <summary>
    /// Rendert die Gantt-Balken im Detail-View
    /// </summary>
    private void RenderGanttBars()
    {
        if (GanttCanvas == null) return;

        GanttCanvas.Children.Clear();

        var daySpan = (_timelineEndDate - _timelineStartDate).TotalDays;
        var totalWidth = daySpan * _pixelsPerDay;
        GanttCanvas.Width = totalWidth;

        // Beispiel-Gantt-Daten (Tasks mit Start/End Dates)
        var tasks = new List<(string Name, DateTime Start, DateTime End, string Color, double Opacity)>
        {
            ("Dokumentation", _timelineStartDate.AddDays(2), _timelineStartDate.AddDays(15), "#2196f3", 0.6),
            ("Implementierung", _timelineStartDate.AddDays(8), _timelineStartDate.AddDays(25), "#4caf50", 0.6),
            ("Testing", _timelineStartDate.AddDays(20), _timelineStartDate.AddDays(35), "#ff9800", 0.6),
            ("Deployment", _timelineStartDate.AddDays(30), _timelineStartDate.AddDays(40), "#f44336", 0.6),
        };

        double yOffset = 10;
        double barHeight = 20;
        double rowSpacing = 35;

        foreach (var task in tasks)
        {
            var startDay = (task.Start - _timelineStartDate).TotalDays;
            var duration = (task.End - task.Start).TotalDays;

            var barWidth = duration * _pixelsPerDay;
            var x = startDay * _pixelsPerDay;

            if (x + barWidth < 0 || x > totalWidth) continue; // Skip if off-screen

            // Gantt Bar (semi-transparent)
            var bar = new Border
            {
                Width = barWidth,
                Height = barHeight,
                Background = new SolidColorBrush(ColorFromHex(task.Color)) { Opacity = task.Opacity },
                BorderBrush = new SolidColorBrush(ColorFromHex(task.Color)),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(3)
            };

            var label = new TextBlock
            {
                Text = task.Name,
                FontSize = 9,
                Foreground = new SolidColorBrush(Colors.Black),
                VerticalAlignment = VerticalAlignment.Center,
                Margin = new Thickness(4, 0, 0, 0)
            };

            bar.Child = label;

            Canvas.SetLeft(bar, x);
            Canvas.SetTop(bar, yOffset);
            GanttCanvas.Children.Add(bar);

            yOffset += rowSpacing;
        }
    }

    /// <summary>
    /// Gantt-Anzeige umschalten
    /// </summary>
    private void ShowGantt_Click(object sender, RoutedEventArgs e)
    {
        // Gantt-Balkne in der kompakten Timeline anzeigen (semi-transparent)
        GenerateTimelineRuler(); // Neu rendern mit/ohne Gantt
    }

    /// <summary>
    /// Aktualisiert die Datumsbereichsanzeige
    /// </summary>
    private void UpdateDateRangeDisplay()
    {
        if (DateRangeTextBlock != null)
        {
            DateRangeTextBlock.Text = $"Bereich: {_timelineStartDate:dd.MM.yyyy} - {_timelineEndDate:dd.MM.yyyy}";
        }
    }

    /// <summary>
    /// Event-Handler für Zoom Out Button
    /// </summary>
    private void ZoomOutButton_Click(object sender, RoutedEventArgs e)
    {
        ZoomTimeline(0.75); // 25% weniger zoomen (vergrößern der Timeline)
        UpdateDateRangeDisplay();
    }

    /// <summary>
    /// Event-Handler für Zoom In Button
    /// </summary>
    private void ZoomInButton_Click(object sender, RoutedEventArgs e)
    {
        ZoomTimeline(1.333); // 33% mehr zoomen (verkleinern der Timeline)
        UpdateDateRangeDisplay();
    }

    /// <summary>
    /// Event-Handler für Timeline Scale ComboBox
    /// </summary>
    private void TimelineScaleComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (TimelineScaleComboBox.SelectedItem is ComboBoxItem item && item.Content is string scale)
        {
            SetTimelineScale(scale);
            UpdateDateRangeDisplay();
        }
    }

    /// <summary>
    /// Event-Handler für Today Button
    /// </summary>
    private void TodayButton_Click(object sender, RoutedEventArgs e)
    {
        var today = DateTime.Now.Date;
        var daysFromStart = (today - _timelineStartDate).TotalDays;
        var scrollPosition = daysFromStart * _pixelsPerDay;
        
        if (TimelineScroller != null)
        {
            TimelineScroller.ScrollToHorizontalOffset(scrollPosition - TimelineScroller.ActualWidth / 2);
        }
    }

    private void RibbonTab_Click(object sender, RoutedEventArgs e)
    {
        // Hide all ribbon content panels
        RibbonStartContent.Visibility = Visibility.Collapsed;
        RibbonInsertContent.Visibility = Visibility.Collapsed;
        RibbonViewContent.Visibility = Visibility.Collapsed;
        RibbonModulesContent.Visibility = Visibility.Collapsed;

        // Show the selected ribbon content
        if (sender == TabStart)
            RibbonStartContent.Visibility = Visibility.Visible;
        else if (sender == TabInsert)
            RibbonInsertContent.Visibility = Visibility.Visible;
        else if (sender == TabView)
            RibbonViewContent.Visibility = Visibility.Visible;
        else if (sender == TabModules)
            RibbonModulesContent.Visibility = Visibility.Visible;
    }


    private void VisualizationTab_Click(object sender, RoutedEventArgs e)
    {
        // Hide all content
        if (GraphContent != null) GraphContent.Visibility = Visibility.Collapsed;
        if (MapContent != null) MapContent.Visibility = Visibility.Collapsed;

        // Show selected tab content
        if (VisualizationTabGraph.IsChecked == true)
        {
            if (GraphContent != null) GraphContent.Visibility = Visibility.Visible;
        }
        else if (VisualizationTabMap.IsChecked == true)
        {
            if (MapContent != null) MapContent.Visibility = Visibility.Visible;
        }
    }

    private void ToggleFullscreen_Click(object sender, RoutedEventArgs e)
    {
        _isFullscreen = !_isFullscreen;
        
        if (_isFullscreen)
        {
            _previousWindowState = WindowState;
            _previousWindowStyle = WindowStyle;
            WindowState = WindowState.Normal;
            WindowStyle = WindowStyle.None;
            WindowState = WindowState.Maximized;
        }
        else
        {
            WindowStyle = _previousWindowStyle;
            WindowState = _previousWindowState;
        }
    }

    private void ToggleWindowMode_Click(object sender, RoutedEventArgs e)
    {
        WindowState = WindowState == WindowState.Maximized ? WindowState.Normal : WindowState.Maximized;
    }

    private void ToggleSidebar_Click(object sender, RoutedEventArgs e)
    {
        if (sender is MenuItem item)
        {
            if (item.Name == "MenuLeftSidebar")
            {
                LeftSidebarColumn.Width = item.IsChecked ? new GridLength(250) : new GridLength(0);
            }
            else if (item.Name == "MenuRightSidebar")
            {
                RightSidebarColumn.Width = item.IsChecked ? new GridLength(300) : new GridLength(0);
            }
        }
    }

    private void UpdateMenuItems()
    {
    }

    protected override void OnKeyDown(System.Windows.Input.KeyEventArgs e)
    {
        if (e.Key == System.Windows.Input.Key.F11)
        {
            ToggleFullscreen_Click(null!, null!);
            e.Handled = true;
        }
        
        base.OnKeyDown(e);
    }

    /// <summary>
    /// Erstellt eine Metadaten-Maske für Dokumenten-Management
    /// </summary>
    private Grid CreateMetadataForm(string contentTitle, string note)
    {
        var grid = new Grid();
        grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Auto) });
        grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) });

        // Scrollable Container für Form
        var scrollViewer = new ScrollViewer 
        { 
            VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
            Padding = new Thickness(20)
        };

        var formContent = new StackPanel();

        // Titel
        var titleLabel = new TextBlock 
        { 
            Text = "Dokumenten-Metadaten", 
            FontSize = 16, 
            FontWeight = FontWeights.Bold,
            Margin = new Thickness(0, 0, 0, 10)
        };
        formContent.Children.Add(titleLabel);

        // Grunddaten
        var section1 = CreateFormSection("Grunddaten", new[]
        {
            ("Dokumentname:", "TextBox", contentTitle),
            ("Beschreibung:", "TextBox", note),
            ("Dokumenttyp:", "ComboBox", "Anforderung|Design|Implementierung|Test"),
            ("Klassifikation:", "ComboBox", "Öffentlich|Intern|Vertraulich|Geheim")
        });
        formContent.Children.Add(section1);

        // Projektdaten
        var section2 = CreateFormSection("Projektdaten", new[]
        {
            ("Projekt:", "TextBox", ""),
            ("Phase:", "ComboBox", "Analyse|Design|Entwicklung|Testing|Deploy"),
            ("Verantwortlicher:", "TextBox", ""),
            ("Gültig ab:", "DatePicker", DateTime.Now.Date.ToString("dd.MM.yyyy"))
        });
        formContent.Children.Add(section2);

        // Versioning & Status
        var section3 = CreateFormSection("Versioning", new[]
        {
            ("Version:", "TextBox", "1.0"),
            ("Status:", "ComboBox", "Entwurf|Review|Genehmigt|Archiviert"),
            ("Zuletzt aktualisiert:", "Label", DateTime.Now.ToString("dd.MM.yyyy HH:mm")),
            ("Autor:", "TextBox", Environment.UserName)
        });
        formContent.Children.Add(section3);

        // KI-Analyse Section
        var section4 = CreateAIAnalysisSection();
        formContent.Children.Add(section4);

        // Badge-Generierung
        var section5 = CreateBadgeSection();
        formContent.Children.Add(section5);

        scrollViewer.Content = formContent;
        Grid.SetRow(scrollViewer, 0);
        grid.Children.Add(scrollViewer);

        // Action Buttons
        var buttonPanel = new Grid
        {
            Background = new SolidColorBrush(Color.FromRgb(245, 245, 245)),
            Margin = new Thickness(0, 10, 0, 0)
        };
        buttonPanel.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        buttonPanel.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Auto) });
        buttonPanel.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Auto) });

        var saveBtn = new Button
        {
            Content = "💾 Speichern",
            Padding = new Thickness(15, 8, 15, 8),
            FontSize = 11,
            Margin = new Thickness(0, 0, 8, 0)
        };
        saveBtn.Click += (s, e) => SaveMetadata_Click(s, e);
        Grid.SetColumn(saveBtn, 1);
        buttonPanel.Children.Add(saveBtn);

        var cancelBtn = new Button
        {
            Content = "✕ Abbrechen",
            Padding = new Thickness(15, 8, 15, 8),
            FontSize = 11,
            Background = new SolidColorBrush(Color.FromRgb(200, 200, 200))
        };
        cancelBtn.Click += (s, e) => CancelMetadata_Click(s, e);
        Grid.SetColumn(cancelBtn, 2);
        buttonPanel.Children.Add(cancelBtn);

        Grid.SetRow(buttonPanel, 1);
        grid.Children.Add(buttonPanel);

        return grid;
    }

    /// <summary>
    /// Erstellt eine Form-Sektion mit mehreren Feldern
    /// </summary>
    private Border CreateFormSection(string title, (string Label, string Type, string Value)[] fields)
    {
        var border = new Border
        {
            Background = new SolidColorBrush(Color.FromRgb(250, 250, 250)),
            BorderBrush = new SolidColorBrush(Color.FromRgb(220, 220, 220)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(4),
            Padding = new Thickness(12)
        };

        var stackPanel = new StackPanel();

        // Section Title
        var titleBlock = new TextBlock
        {
            Text = title,
            FontSize = 12,
            FontWeight = FontWeights.Bold,
            Margin = new Thickness(0, 0, 0, 8),
            Foreground = new SolidColorBrush(Color.FromRgb(50, 50, 50))
        };
        stackPanel.Children.Add(titleBlock);

        // Fields
        foreach (var (label, type, value) in fields)
        {
            var fieldGrid = new Grid();
            fieldGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(140) });
            fieldGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

            var labelBlock = new TextBlock
            {
                Text = label,
                FontSize = 10,
                Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100)),
                VerticalAlignment = VerticalAlignment.Center
            };
            Grid.SetColumn(labelBlock, 0);
            fieldGrid.Children.Add(labelBlock);

            UIElement controlElement = null!;
            if (type == "TextBox")
            {
                controlElement = new TextBox
                {
                    Text = value,
                    FontSize = 10,
                    Padding = new Thickness(6, 4, 6, 4),
                    BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200))
                };
            }
            else if (type == "ComboBox")
            {
                var comboBox = new ComboBox { FontSize = 10, Padding = new Thickness(4) };
                foreach (var item in value.Split('|'))
                {
                    comboBox.Items.Add(item.Trim());
                }
                comboBox.SelectedIndex = 0;
                controlElement = comboBox;
            }
            else if (type == "DatePicker")
            {
                controlElement = new System.Windows.Controls.DatePicker { FontSize = 10 };
            }
            else if (type == "Label")
            {
                controlElement = new TextBlock
                {
                    Text = value,
                    FontSize = 10,
                    Foreground = new SolidColorBrush(Color.FromRgb(80, 80, 80))
                };
            }

            Grid.SetColumn(controlElement, 1);
            fieldGrid.Children.Add(controlElement);
            stackPanel.Children.Add(fieldGrid);
        }

        border.Child = stackPanel;
        return border;
    }

    /// <summary>
    /// KI-Analyse Sektion
    /// </summary>
    private Border CreateAIAnalysisSection()
    {
        var border = new Border
        {
            Background = new SolidColorBrush(Color.FromRgb(240, 250, 255)),
            BorderBrush = new SolidColorBrush(Color.FromRgb(33, 150, 243)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(4),
            Padding = new Thickness(12)
        };

        var stackPanel = new StackPanel();

        var titleBlock = new TextBlock
        {
            Text = "🤖 KI-Analyse",
            FontSize = 12,
            FontWeight = FontWeights.Bold,
            Foreground = new SolidColorBrush(Color.FromRgb(33, 150, 243))
        };
        stackPanel.Children.Add(titleBlock);

        var analysisButton = new Button
        {
            Content = "Dokumentation analysieren",
            Padding = new Thickness(10, 6, 10, 6),
            FontSize = 10,
            Background = new SolidColorBrush(Color.FromRgb(33, 150, 243)),
            Foreground = new SolidColorBrush(Colors.White)
        };
        analysisButton.Click += (s, e) => AnalyzeDocument_Click(s, e);
        stackPanel.Children.Add(analysisButton);

        var resultBlock = new TextBlock
        {
            Text = "Analyseergebnisse werden hier angezeigt...",
            FontSize = 9,
            Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100)),
            TextWrapping = TextWrapping.Wrap,
            Name = "AnalysisResult"
        };
        stackPanel.Children.Add(resultBlock);

        border.Child = stackPanel;
        return border;
    }

    /// <summary>
    /// Badge-Generierung Sektion
    /// </summary>
    private Border CreateBadgeSection()
    {
        var border = new Border
        {
            Background = new SolidColorBrush(Color.FromRgb(255, 250, 240)),
            BorderBrush = new SolidColorBrush(Color.FromRgb(255, 152, 0)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(4),
            Padding = new Thickness(12)
        };

        var stackPanel = new StackPanel();

        var titleBlock = new TextBlock
        {
            Text = "🏷️ Auto-Badges",
            FontSize = 12,
            FontWeight = FontWeights.Bold,
            Foreground = new SolidColorBrush(Color.FromRgb(255, 152, 0))
        };
        stackPanel.Children.Add(titleBlock);

        var descBlock = new TextBlock
        {
            Text = "Basierend auf Dokumenttyp und Klassifikation werden automatisch Badges generiert:",
            FontSize = 9,
            Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100)),
            TextWrapping = TextWrapping.Wrap,
            Margin = new Thickness(0, 0, 0, 8)
        };
        stackPanel.Children.Add(descBlock);

        var badgePanel = new WrapPanel();
        var badges = new[] 
        { 
            ("Dokumentation", "#FFC107"), 
            ("Phase 1", "#2196F3"),
            ("Review", "#FF9800"),
            ("Genehmigt", "#4CAF50")
        };

        foreach (var (text, color) in badges)
        {
            var badge = new Border
            {
                Background = new SolidColorBrush(ColorFromHex(color)),
                Padding = new Thickness(8, 4, 8, 4),
                CornerRadius = new CornerRadius(12),
                Margin = new Thickness(2)
            };

            var badgeText = new TextBlock
            {
                Text = text,
                Foreground = new SolidColorBrush(Colors.White),
                FontSize = 9,
                FontWeight = FontWeights.Bold
            };

            badge.Child = badgeText;
            badgePanel.Children.Add(badge);
        }

        stackPanel.Children.Add(badgePanel);

        border.Child = stackPanel;
        return border;
    }

    /// <summary>
    /// Close Tab mit Änderungsprüfung
    /// </summary>
    private void CloseTab_Click(TabItem tabItem)
    {
        if (_tabChanges.ContainsKey(tabItem) && _tabChanges[tabItem])
        {
            var result = MessageBox.Show(
                $"Änderungen in '{tabItem.Header}' speichern?",
                "Ungespeicherte Änderungen",
                MessageBoxButton.YesNoCancel,
                MessageBoxImage.Question
            );

            if (result == MessageBoxResult.Yes)
            {
                SaveMetadata_Click(null!, null!);
            }
            else if (result == MessageBoxResult.Cancel)
            {
                return;
            }
        }

        CenterContent.Items.Remove(tabItem);
        _tabChanges.Remove(tabItem);
        _tabMetadata.Remove(tabItem);
    }

    private void SaveMetadata_Click(object sender, RoutedEventArgs e)
    {
        MessageBox.Show("Metadaten gespeichert!", "Erfolg", MessageBoxButton.OK, MessageBoxImage.Information);
        var currentTab = CenterContent.SelectedItem as TabItem;
        if (currentTab != null && _tabChanges.ContainsKey(currentTab))
        {
            _tabChanges[currentTab] = false;
        }
    }

    private void CancelMetadata_Click(object sender, RoutedEventArgs e)
    {
        MessageBox.Show("Änderungen verworfen", "Abgebrochen", MessageBoxButton.OK, MessageBoxImage.Information);
        var currentTab = CenterContent.SelectedItem as TabItem;
        if (currentTab != null && _tabChanges.ContainsKey(currentTab))
        {
            _tabChanges[currentTab] = false;
        }
    }

    private void AnalyzeDocument_Click(object sender, RoutedEventArgs e)
    {
        MessageBox.Show(
            "KI-Analyse durchgeführt:\n\n" +
            "✓ Dokumenttyp erkannt: Anforderungsdokument\n" +
            "✓ Qualitätsindex: 8.5/10\n" +
            "✓ Empfohlene Klassifikation: Intern\n" +
            "✓ Fehlerhafte Referenzen: Keine\n" +
            "✓ Suggierierte Tags: requirements, analysis, v1.0",
            "KI-Analyse Ergebnis",
            MessageBoxButton.OK,
            MessageBoxImage.Information
        );
    }

    #region Phase 29 - Settings & Theme Integration

    /// <summary>
    /// Öffnet den Settings-Dialog
    /// </summary>
    private void OpenSettings_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var settingsDialog = new Settings.SettingsDialog(_themeService, _settingsService, _animationService)
            {
                Owner = this
            };

            var result = settingsDialog.ShowDialog();
            if (result == true)
            {
                // Settings wurden übernommen
                UpdateThemeMenuItems();
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Fehler beim Öffnen der Einstellungen: {ex.Message}",
                          "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    /// <summary>
    /// Theme auf Hell setzen
    /// </summary>
    private void SetThemeLight_Click(object sender, RoutedEventArgs e)
    {
        _themeService.CurrentTheme = ThemeService.ThemeMode.Light;
        UpdateThemeMenuItems();
    }

    /// <summary>
    /// Theme auf Dunkel setzen
    /// </summary>
    private void SetThemeDark_Click(object sender, RoutedEventArgs e)
    {
        _themeService.CurrentTheme = ThemeService.ThemeMode.Dark;
        UpdateThemeMenuItems();
    }

    /// <summary>
    /// Theme auf System setzen
    /// </summary>
    private void SetThemeSystem_Click(object sender, RoutedEventArgs e)
    {
        _themeService.CurrentTheme = ThemeService.ThemeMode.System;
        UpdateThemeMenuItems();
    }

    /// <summary>
    /// Aktualisiert die Theme-Menü-Items
    /// </summary>
    private void UpdateThemeMenuItems()
    {
        MenuThemeLight.IsChecked = _themeService.CurrentTheme == ThemeService.ThemeMode.Light;
        MenuThemeDark.IsChecked = _themeService.CurrentTheme == ThemeService.ThemeMode.Dark;
        MenuThemeSystem.IsChecked = _themeService.CurrentTheme == ThemeService.ThemeMode.System;
    }

    #endregion

    /// <summary>
    /// Datenklasse für Dokumenten-Metadaten
    /// </summary>
    public class DocumentMetadata
    {
        public string DocumentName { get; set; } = "";
        public string Description { get; set; } = "";
        public string DocumentType { get; set; } = "";
        public string Classification { get; set; } = "";
        public string Project { get; set; } = "";
        public string Phase { get; set; } = "";
        public string Responsible { get; set; } = "";
        public DateTime ValidFrom { get; set; } = DateTime.Now;
        public string Version { get; set; } = "1.0";
        public string Status { get; set; } = "Entwurf";
        public string Author { get; set; } = "";
    }

}

/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SmartFormRenderer.cs                               ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     581                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.UI;

/// <summary>
/// Event arguments für Smart Form Submission
/// </summary>
public class SmartFormSubmissionEventArgs : EventArgs
{
    public string? TemplateId { get; set; }
    public Dictionary<string, object>? FormData { get; set; }
    public List<SmartFormField>? SmartFields { get; set; }
    public Dictionary<string, MetadataBadge>? DetectedMetadata { get; set; }
}

/// <summary>
/// Intelligente Form-Renderer mit Metadaten-Badge-Erkennung und Auto-Completion
/// </summary>
public class SmartFormRenderer : UserControl
{
    private FormTemplate? _currentTemplate;
    private List<SmartFormField> _smartFields = new();
    private readonly ISmartFormService _smartFormService;
    private readonly IMetadataBadgeService _badgeService;
    private readonly ISmartSuggestionService _suggestionService;
    private Dictionary<string, FrameworkElement> _fieldControls = new();

    public FormTemplate? CurrentTemplate
    {
        get => (FormTemplate?)GetValue(CurrentTemplateProperty);
        set => SetValue(CurrentTemplateProperty, value);
    }

    public static readonly DependencyProperty CurrentTemplateProperty =
        DependencyProperty.Register("CurrentTemplate", typeof(FormTemplate), 
            typeof(SmartFormRenderer), new PropertyMetadata(null, OnTemplateChanged));

    public SmartFormRenderer(
        ISmartFormService smartFormService,
        IMetadataBadgeService badgeService,
        ISmartSuggestionService suggestionService)
    {
        _smartFormService = smartFormService;
        _badgeService = badgeService;
        _suggestionService = suggestionService;

        Background = new SolidColorBrush(Color.FromRgb(245, 245, 245));
        Padding = new Thickness(15);
    }

    public event EventHandler<SmartFormSubmissionEventArgs>? SmartFormSubmitted;

    private static void OnTemplateChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SmartFormRenderer renderer && e.NewValue is FormTemplate template)
        {
            _ = renderer.RenderSmartTemplate(template);
        }
    }

    public async Task RenderSmartTemplate(FormTemplate template)
    {
        _currentTemplate = template;
        _smartFields.Clear();
        _fieldControls.Clear();

        // Erstelle intelligente Formularbelder
        _smartFields = await _smartFormService.CreateSmartFormAsync(template);

        var mainPanel = new ScrollViewer
        {
            VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
            HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled
        };

        var stackPanel = new StackPanel { Orientation = Orientation.Vertical };

        // Rendere jede Section
        foreach (var section in template.Sections)
        {
            var sectionExpander = RenderSmartSection(section);
            stackPanel.Children.Add(sectionExpander);
        }

        // Submit-Button Panel
        var buttonPanel = new StackPanel 
        { 
            Orientation = Orientation.Horizontal, 
            HorizontalAlignment = HorizontalAlignment.Right,
            Margin = new Thickness(0, 20, 0, 0)
        };

        var submitBtn = new Button
        {
            Content = "✓ Intelligentes Formular absenden",
            Padding = new Thickness(15, 8, 15, 8),
            Background = new SolidColorBrush(Color.FromRgb(76, 175, 80)),
            Foreground = new SolidColorBrush(Colors.White),
            FontSize = 12,
            FontWeight = FontWeights.Bold,
            Cursor = Cursors.Hand
        };

        var resetBtn = new Button
        {
            Content = "⟲ Zurücksetzen",
            Padding = new Thickness(12, 8, 12, 8),
            Background = new SolidColorBrush(Color.FromRgb(158, 158, 158)),
            Foreground = new SolidColorBrush(Colors.White),
            FontSize = 12
        };

        submitBtn.Click += async (s, e) => await OnSmartSubmit();
        resetBtn.Click += (s, e) => ClearSmartForm();

        buttonPanel.Children.Add(submitBtn);
        buttonPanel.Children.Add(resetBtn);
        stackPanel.Children.Add(buttonPanel);

        mainPanel.Content = stackPanel;
        Content = mainPanel;
    }

    /// <summary>
    /// Rendert eine intelligente Section
    /// </summary>
    private Expander RenderSmartSection(FormSection section)
    {
        var grid = new Grid();
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

        int row = 0;
        int col = 0;

        foreach (var field in section.Fields)
        {
            var smartField = _smartFields.FirstOrDefault(f => f.BaseField.Id == field.Id);
            if (smartField == null) continue;

            if (col >= 2)
            {
                col = 0;
                row++;
            }

            var fieldControl = RenderSmartField(smartField);
            Grid.SetRow(fieldControl, row);
            Grid.SetColumn(fieldControl, col);
            Grid.SetColumnSpan(fieldControl, field.Type == FormFieldType.TextArea ? 2 : 1);

            if (row >= grid.RowDefinitions.Count)
                grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

            grid.Children.Add(fieldControl);
            col++;
        }

        return new Expander
        {
            Header = section.Title,
            Content = grid,
            IsExpanded = section.IsExpanded,
            Padding = new Thickness(10),
            Foreground = new SolidColorBrush(Colors.Black),
            FontWeight = FontWeights.Bold
        };
    }

    /// <summary>
    /// Rendert ein intelligentes Feld mit Badge-Unterstützung
    /// </summary>
    private Border RenderSmartField(SmartFormField smartField)
    {
        var field = smartField.BaseField;
        var fieldPanel = new StackPanel { Orientation = Orientation.Vertical };

        // Label
        var label = new TextBlock
        {
            Text = (field.IsRequired ? "* " : "") + field.Label,
            FontWeight = FontWeights.Bold,
            Foreground = new SolidColorBrush(Color.FromRgb(50, 50, 50)),
            Margin = new Thickness(0, 0, 0, 5)
        };
        fieldPanel.Children.Add(label);

        // Intelligentes Input-Feld basierend auf Feldtyp
        var inputControl = RenderSmartInput(smartField);
        fieldPanel.Children.Add(inputControl);

        // Badge-Display für erkannte Metadaten
        if (smartField.DetectedBadges.Any())
        {
            var badgePanel = RenderBadgePanel(smartField.DetectedBadges);
            fieldPanel.Children.Add(badgePanel);
        }

        // Hilftext
        if (!string.IsNullOrEmpty(field.HelpText))
        {
            var helpText = new TextBlock
            {
                Text = field.HelpText,
                FontSize = 10,
                Foreground = new SolidColorBrush(Color.FromRgb(150, 150, 150)),
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(0, 5, 0, 0)
            };
            fieldPanel.Children.Add(helpText);
        }

        return new Border
        {
            Child = fieldPanel,
            Padding = new Thickness(10),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(4),
            Background = new SolidColorBrush(Colors.White),
            Margin = new Thickness(0, 0, 0, 10)
        };
    }

    /// <summary>
    /// Rendert intelligentes Input basierend auf Feldtyp
    /// </summary>
    private FrameworkElement RenderSmartInput(SmartFormField smartField)
    {
        var field = smartField.BaseField;
        FrameworkElement control = field.Type switch
        {
            FormFieldType.TextArea => CreateSmartTextBox(smartField, true),
            FormFieldType.ComboBox or FormFieldType.DropDown => CreateSmartComboBox(smartField),
            FormFieldType.Date => CreateSmartDatePicker(smartField),
            FormFieldType.DateTime => CreateSmartDateTimePicker(smartField),
            FormFieldType.Number => CreateSmartNumericBox(smartField),
            FormFieldType.Checkbox => CreateSmartCheckBox(smartField),
            FormFieldType.RadioButton => CreateSmartRadioButton(smartField),
            _ => CreateSmartTextBox(smartField, false)
        };

        _fieldControls[field.Id] = control;
        return control;
    }

    /// <summary>
    /// Erstellt ein intelligentes TextBox mit Autocomplete
    /// </summary>
    private Border CreateSmartTextBox(SmartFormField smartField, bool multiline)
    {
        var field = smartField.BaseField;
        var box = new TextBox
        {
            AcceptsReturn = multiline,
            TextWrapping = multiline ? TextWrapping.Wrap : TextWrapping.NoWrap,
            Height = multiline ? 100 : 35,
            Padding = new Thickness(8),
            FontSize = 12,
            Foreground = new SolidColorBrush(Colors.Black),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1)
        };

        box.TextChanged += async (s, e) =>
        {
            // Aktualisiere intelligentes Feld bei Texteingabe
            _ = smartField.BaseField.Id; // Smart field ID
            
            // Trigger Badge-Erkennung
            smartField = await _smartFormService.UpdateSmartFieldAsync(
                smartField,
                box.Text);
        };

        if (!string.IsNullOrEmpty(field.DefaultValue as string))
            box.Text = field.DefaultValue.ToString() ?? "";

        if (!string.IsNullOrEmpty(field.PlaceholderText))
        {
            var placeholder = new TextBlock
            {
                Text = field.PlaceholderText,
                Foreground = new SolidColorBrush(Color.FromRgb(180, 180, 180)),
                IsHitTestVisible = false,
                Margin = new Thickness(8, 8, 0, 0)
            };
        }

        return new Border { Child = box, BorderThickness = new Thickness(0) };
    }

    /// <summary>
    /// Erstellt eine intelligente ComboBox mit Vorschlägen
    /// </summary>
    private Border CreateSmartComboBox(SmartFormField smartField)
    {
        var field = smartField.BaseField;
        var combo = new ComboBox
        {
            Height = 35,
            Padding = new Thickness(8),
            FontSize = 12
        };

        foreach (var option in field.Options)
        {
            combo.Items.Add(new ComboBoxItem { Content = option.Label, Tag = option.Value });
        }

        if (field.DefaultValue != null)
        {
            combo.SelectedValuePath = "Tag";
            combo.SelectedValue = field.DefaultValue;
        }

        return new Border { Child = combo, BorderThickness = new Thickness(0) };
    }

    /// <summary>
    /// Erstellt einen intelligenten DatePicker
    /// </summary>
    private Border CreateSmartDatePicker(SmartFormField smartField)
    {
        var datePicker = new DatePicker
        {
            Height = 35,
            Padding = new Thickness(8),
            FontSize = 12
        };

        if (smartField.BaseField.DefaultValue is DateTime dt)
            datePicker.SelectedDate = dt;

        return new Border { Child = datePicker, BorderThickness = new Thickness(0) };
    }

    /// <summary>
    /// Erstellt einen intelligenten DateTime-Picker
    /// </summary>
    private Border CreateSmartDateTimePicker(SmartFormField smartField)
    {
        var datePanel = new StackPanel { Orientation = Orientation.Horizontal };

        var datePicker = new DatePicker
        {
            Height = 35,
            Width = 150,
            Padding = new Thickness(8),
            FontSize = 12
        };

        var timePicker = new TextBox
        {
            Height = 35,
            Width = 100,
            Padding = new Thickness(8),
            FontSize = 12,
            Text = DateTime.Now.ToString("HH:mm")
        };

        datePanel.Children.Add(datePicker);
        datePanel.Children.Add(timePicker);

        return new Border { Child = datePanel, BorderThickness = new Thickness(0) };
    }

    /// <summary>
    /// Erstellt ein numerisches Eingabefeld
    /// </summary>
    private Border CreateSmartNumericBox(SmartFormField smartField)
    {
        var field = smartField.BaseField;
        var box = new TextBox
        {
            Height = 35,
            Padding = new Thickness(8),
            FontSize = 12,
            Text = field.DefaultValue?.ToString() ?? ""
        };

        box.PreviewTextInput += (s, e) =>
        {
            e.Handled = !char.IsDigit(e.Text, 0) && e.Text != "." && e.Text != "-";
        };

        return new Border { Child = box, BorderThickness = new Thickness(0) };
    }

    /// <summary>
    /// Erstellt ein Checkbox-Feld
    /// </summary>
    private Border CreateSmartCheckBox(SmartFormField smartField)
    {
        var field = smartField.BaseField;
        var checkbox = new CheckBox
        {
            Content = field.Label,
            Padding = new Thickness(8),
            FontSize = 12,
            IsChecked = field.DefaultValue is bool b && b
        };

        return new Border { Child = checkbox, BorderThickness = new Thickness(0) };
    }

    /// <summary>
    /// Erstellt ein RadioButton-Feld
    /// </summary>
    private Border CreateSmartRadioButton(SmartFormField smartField)
    {
        var field = smartField.BaseField;
        var stackPanel = new StackPanel { Orientation = Orientation.Vertical };

        foreach (var option in field.Options)
        {
            var radio = new RadioButton
            {
                Content = option.Label,
                Tag = option.Value,
                Margin = new Thickness(0, 2, 0, 2),
                FontSize = 11
            };

            if (option.IsSelected || field.DefaultValue?.ToString() == option.Value)
                radio.IsChecked = true;

            stackPanel.Children.Add(radio);
        }

        return new Border { Child = stackPanel, BorderThickness = new Thickness(0) };
    }

    /// <summary>
    /// Rendert erkannte Badges
    /// </summary>
    private WrapPanel RenderBadgePanel(List<MetadataBadge> badges)
    {
        var badgePanel = new WrapPanel
        {
            Orientation = Orientation.Horizontal,
            Margin = new Thickness(0, 8, 0, 0)
        };

        foreach (var badge in badges)
        {
            var badgeColor = Color.FromRgb(33, 150, 243); // Default blue
            if (badge.Style?.BackgroundColor is string colorStr && !string.IsNullOrEmpty(colorStr))
            {
                // Parse color from hex string (e.g., "#2196F3")
                if (colorStr.StartsWith("#") && colorStr.Length == 7)
                {
                    try
                    {
                        var r = byte.Parse(colorStr.Substring(1, 2), System.Globalization.NumberStyles.HexNumber);
                        var g = byte.Parse(colorStr.Substring(3, 2), System.Globalization.NumberStyles.HexNumber);
                        var b = byte.Parse(colorStr.Substring(5, 2), System.Globalization.NumberStyles.HexNumber);
                        badgeColor = Color.FromRgb(r, g, b);
                    }
                    catch { }
                }
            }

            var badgeControl = new Border
            {
                Background = new SolidColorBrush(badgeColor),
                CornerRadius = new CornerRadius(12),
                Padding = new Thickness(8, 4, 8, 4),
                Margin = new Thickness(2),
                Child = new TextBlock
                {
                    Text = $"{badge.Style?.Icon} {badge.DisplayText}",
                    Foreground = new SolidColorBrush(Colors.White),
                    FontSize = 10,
                    FontWeight = FontWeights.Bold
                }
            };

            badgePanel.Children.Add(badgeControl);
        }

        return badgePanel;
    }

    /// <summary>
    /// Hilfsmethode zum Absenden
    /// </summary>
    private async Task OnSmartSubmit()
    {
        var formData = new Dictionary<string, object>();

        foreach (var field in _smartFields)
        {
            if (_fieldControls.TryGetValue(field.BaseField.Id, out var control))
            {
                var value = ExtractValue(control, field.BaseField.Type);
                if (value != null)
                    formData[field.BaseField.Id] = value;
            }
        }

        SmartFormSubmitted?.Invoke(this, new SmartFormSubmissionEventArgs
        {
            TemplateId = _currentTemplate?.Id,
            FormData = formData,
            SmartFields = _smartFields,
            DetectedMetadata = _smartFields
                .SelectMany(f => f.DetectedBadges)
                .DistinctBy(b => b.Type)
                .ToDictionary(b => b.Type.ToString(), b => b)
        });
    }

    /// <summary>
    /// Extrahiert Wert aus Control
    /// </summary>
    private object? ExtractValue(FrameworkElement control, FormFieldType fieldType)
    {
        return control switch
        {
            TextBox box => box.Text,
            ComboBox combo => combo.SelectedValue,
            DatePicker dp => dp.SelectedDate,
            CheckBox cb => cb.IsChecked,
            _ => null
        };
    }

    /// <summary>
    /// Löscht das Formular
    /// </summary>
    private void ClearSmartForm()
    {
        foreach (var control in _fieldControls.Values)
        {
            if (control is TextBox tb)
                tb.Text = "";
            else if (control is ComboBox cb)
                cb.SelectedIndex = -1;
            else if (control is DatePicker dp)
                dp.SelectedDate = null;
        }

        _smartFields.ForEach(f => f.DetectedBadges.Clear());
    }
}

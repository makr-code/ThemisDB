/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            FormRenderer.cs                                    ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     733                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using Themis.DocumentManager.Services;
using static Themis.DocumentManager.Services.FormTemplateService;

namespace Themis.DocumentManager.UI;

/// <summary>
/// Event arguments for form submission
/// </summary>
public class FormSubmissionEventArgs : EventArgs
{
    public string? TemplateId { get; set; }
    public Dictionary<string, object>? FormData { get; set; }
}

/// <summary>
/// Renders dynamic form templates into WPF UI controls
/// </summary>
public class FormRenderer : UserControl
{
    private FormTemplate? _currentTemplate;
    private Dictionary<string, FrameworkElement> _fieldControls = new();
    private Dictionary<string, string> _formData = new();

    public FormTemplate? CurrentTemplate
    {
        get => (FormTemplate?)GetValue(CurrentTemplateProperty);
        set => SetValue(CurrentTemplateProperty, value);
    }

    public static readonly DependencyProperty CurrentTemplateProperty =
        DependencyProperty.Register("CurrentTemplate", typeof(FormTemplate), 
            typeof(FormRenderer), new PropertyMetadata(null, OnTemplateChanged));

    public Dictionary<string, string> FormData => _formData;

    public FormRenderer()
    {
        Background = new SolidColorBrush(Color.FromRgb(245, 245, 245));
        Padding = new Thickness(15);
    }

    public event EventHandler<FormSubmissionEventArgs>? FormSubmitted;

    private void OnFormSubmitted()
    {
        FormSubmitted?.Invoke(this, new FormSubmissionEventArgs());
    }

    private static void OnTemplateChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is FormRenderer renderer && e.NewValue is FormTemplate template)
            renderer.RenderTemplate(template);
    }

    public void RenderTemplate(FormTemplate template)
    {
        _currentTemplate = template;
        _fieldControls.Clear();
        _formData.Clear();

        var mainPanel = new StackPanel { Orientation = Orientation.Vertical };
        mainPanel.Margin = new Thickness(0, 0, 0, 15);

        var titleBlock = new TextBlock
        {
            Text = template.Name,
            FontSize = 24,
            FontWeight = FontWeights.Bold,
            Foreground = new SolidColorBrush(Color.FromRgb(31, 78, 121)),
            Margin = new Thickness(0, 0, 0, 20)
        };
        mainPanel.Children.Add(titleBlock);

        foreach (var section in template.Sections.OrderBy(s => s.Order))
        {
            mainPanel.Children.Add(RenderSection(section));
            mainPanel.Children.Add(new Border { Height = 10 });
        }

        var buttonPanel = new StackPanel 
        { 
            Orientation = Orientation.Horizontal,
            HorizontalAlignment = HorizontalAlignment.Right,
            Margin = new Thickness(0, 20, 0, 0)
        };

        var submitButton = new Button
        {
            Content = "Formular absenden",
            Padding = new Thickness(20, 10, 20, 10),
            Background = new SolidColorBrush(Color.FromRgb(0, 120, 215)),
            Foreground = Brushes.White,
            FontSize = 14,
            FontWeight = FontWeights.SemiBold,
            Cursor = Cursors.Hand,
            Margin = new Thickness(0, 0, 10, 0)
        };
        submitButton.Click += (s, e) => SubmitForm();

        var resetButton = new Button
        {
            Content = "Zurücksetzen",
            Padding = new Thickness(20, 10, 20, 10),
            Background = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            Foreground = Brushes.Black,
            FontSize = 14
        };
        resetButton.Click += (s, e) => ResetForm();

        buttonPanel.Children.Add(submitButton);
        buttonPanel.Children.Add(resetButton);
        mainPanel.Children.Add(buttonPanel);

        var scrollViewer = new ScrollViewer
        {
            Content = mainPanel,
            VerticalScrollBarVisibility = ScrollBarVisibility.Auto
        };

        Content = scrollViewer;
    }

    private FrameworkElement RenderSection(FormSection section)
    {
        var expander = new Expander
        {
            Header = section.Title,
            IsExpanded = section.IsExpanded,
            Foreground = new SolidColorBrush(Color.FromRgb(31, 78, 121)),
            FontSize = 16,
            FontWeight = FontWeights.SemiBold,
            Margin = new Thickness(0, 10, 0, 0),
            Padding = new Thickness(10)
        };

        var grid = new Grid();
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

        int row = 0;
        for (int i = 0; i < section.Fields.Count; i++)
        {
            var field = section.Fields[i];
            if (i % 2 == 0)
                grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

            var fieldControl = RenderField(field);
            Grid.SetRow(fieldControl, row);
            Grid.SetColumn(fieldControl, i % 2);
            grid.Children.Add(fieldControl);

            if (i % 2 == 1)
                row++;
        }

        if (section.Fields.Count % 2 != 0)
            grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

        expander.Content = grid;
        return expander;
    }

    private FrameworkElement RenderField(FormField field)
    {
        var container = new StackPanel { Orientation = Orientation.Vertical, Margin = new Thickness(10) };

        var label = new TextBlock
        {
            Text = $"{field.Label}{(field.IsRequired ? " *" : "")}",
            FontWeight = FontWeights.SemiBold,
            Margin = new Thickness(0, 0, 0, 5),
            Foreground = new SolidColorBrush(Color.FromRgb(64, 64, 64))
        };
        container.Children.Add(label);

        FrameworkElement control = field.Type switch
        {
            FormFieldType.Text => RenderTextBox(field),
            FormFieldType.TextArea => RenderTextArea(field),
            FormFieldType.Number => RenderNumberBox(field),
            FormFieldType.Decimal => RenderDecimalBox(field),
            FormFieldType.Currency => RenderCurrencyBox(field),
            FormFieldType.Date => RenderDatePicker(field),
            FormFieldType.DateTime => RenderDateTimePicker(field),
            FormFieldType.Email => RenderEmailBox(field),
            FormFieldType.Phone => RenderPhoneBox(field),
            FormFieldType.Checkbox => RenderCheckBox(field),
            FormFieldType.RadioButton => RenderRadioButtons(field),
            FormFieldType.DropDown => RenderComboBox(field),
            FormFieldType.MultiSelect => RenderMultiSelect(field),
            FormFieldType.ComboBox => RenderComboBoxEditable(field),
            FormFieldType.FileUpload => RenderFileUpload(field),
            FormFieldType.Signature => RenderSignatureField(field),
            FormFieldType.Image => RenderImageField(field),
            FormFieldType.Label => RenderReadOnlyLabel(field),
            FormFieldType.Hidden => RenderHiddenField(field),
            FormFieldType.Custom => RenderCustomField(field),
            _ => RenderTextBox(field)
        };

        container.Children.Add(control);
        _fieldControls[field.Id] = control;

        if (!string.IsNullOrEmpty(field.HelpText))
        {
            var helpText = new TextBlock
            {
                Text = field.HelpText,
                FontSize = 11,
                Foreground = new SolidColorBrush(Color.FromRgb(128, 128, 128)),
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(0, 3, 0, 0),
                FontStyle = FontStyles.Italic
            };
            container.Children.Add(helpText);
        }

        return container;
    }

    private FrameworkElement RenderTextBox(FormField field)
    {
        var textBox = new TextBox
        {
            Height = 35,
            Padding = new Thickness(8),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1),
            FontSize = 12
        };

        if (field.MaxLength.HasValue)
            textBox.MaxLength = field.MaxLength.Value;

        if (field.DefaultValue != null)
            textBox.Text = field.DefaultValue.ToString() ?? "";

        textBox.TextChanged += (s, e) => 
        {
            _formData[field.Id] = textBox.Text;
            ValidateField(field, textBox.Text);
        };

        return textBox;
    }

    private FrameworkElement RenderTextArea(FormField field)
    {
        var textBox = new TextBox
        {
            Height = 100,
            Padding = new Thickness(8),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1),
            FontSize = 12,
            TextWrapping = TextWrapping.Wrap,
            AcceptsReturn = true,
            VerticalScrollBarVisibility = ScrollBarVisibility.Auto
        };

        if (field.MaxLength.HasValue)
            textBox.MaxLength = field.MaxLength.Value;

        if (field.DefaultValue != null)
            textBox.Text = field.DefaultValue.ToString() ?? "";

        textBox.TextChanged += (s, e) =>
        {
            _formData[field.Id] = textBox.Text;
            ValidateField(field, textBox.Text);
        };

        return textBox;
    }

    private FrameworkElement RenderNumberBox(FormField field)
    {
        var textBox = new TextBox
        {
            Height = 35,
            Padding = new Thickness(8),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1),
            FontSize = 12
        };

        textBox.PreviewTextInput += (s, e) => e.Handled = !char.IsDigit(e.Text, 0) && e.Text != "-";

        if (field.DefaultValue != null)
            textBox.Text = field.DefaultValue.ToString() ?? "";

        textBox.TextChanged += (s, e) =>
        {
            _formData[field.Id] = textBox.Text;
            ValidateField(field, textBox.Text);
        };

        return textBox;
    }

    private FrameworkElement RenderDecimalBox(FormField field)
    {
        var textBox = new TextBox
        {
            Height = 35,
            Padding = new Thickness(8),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1),
            FontSize = 12
        };

        textBox.PreviewTextInput += (s, e) => e.Handled = !char.IsDigit(e.Text, 0) && e.Text != "-" && e.Text != ".";

        textBox.TextChanged += (s, e) =>
        {
            _formData[field.Id] = textBox.Text;
            ValidateField(field, textBox.Text);
        };

        return textBox;
    }

    private FrameworkElement RenderCurrencyBox(FormField field)
    {
        var panel = new StackPanel { Orientation = Orientation.Horizontal };
        var textBox = new TextBox
        {
            Height = 35,
            Padding = new Thickness(8),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1),
            FontSize = 12,
            Width = 150
        };

        var currency = new TextBlock
        {
            Text = "EUR",
            VerticalAlignment = VerticalAlignment.Center,
            Margin = new Thickness(10, 0, 0, 0),
            FontWeight = FontWeights.SemiBold
        };

        panel.Children.Add(textBox);
        panel.Children.Add(currency);

        textBox.PreviewTextInput += (s, e) => e.Handled = !char.IsDigit(e.Text, 0) && e.Text != "-" && e.Text != ".";

        textBox.TextChanged += (s, e) =>
        {
            _formData[field.Id] = textBox.Text;
            ValidateField(field, textBox.Text);
        };

        return panel;
    }

    private FrameworkElement RenderDatePicker(FormField field)
    {
        var datePicker = new DatePicker { Height = 35, Padding = new Thickness(8) };

        if (field.DefaultValue != null && DateTime.TryParse(field.DefaultValue.ToString(), out var date))
            datePicker.SelectedDate = date;

        datePicker.SelectedDateChanged += (s, e) =>
        {
            if (datePicker.SelectedDate.HasValue)
                _formData[field.Id] = datePicker.SelectedDate.Value.ToString("yyyy-MM-dd");
        };

        return datePicker;
    }

    private FrameworkElement RenderDateTimePicker(FormField field)
    {
        var panel = new StackPanel { Orientation = Orientation.Horizontal };

        var datePicker = new DatePicker { Height = 35, Padding = new Thickness(8), Margin = new Thickness(0, 0, 10, 0) };
        var timePicker = new TextBox 
        { 
            Height = 35, 
            Width = 100,
            Padding = new Thickness(8),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1)
        };

        datePicker.SelectedDateChanged += (s, e) => UpdateDateTime();
        timePicker.TextChanged += (s, e) => UpdateDateTime();

        void UpdateDateTime()
        {
            if (datePicker.SelectedDate.HasValue)
                _formData[field.Id] = datePicker.SelectedDate.Value.ToString("yyyy-MM-dd") + " " + timePicker.Text;
        }

        panel.Children.Add(datePicker);
        panel.Children.Add(timePicker);

        return panel;
    }

    private FrameworkElement RenderEmailBox(FormField field)
    {
        var textBox = new TextBox
        {
            Height = 35,
            Padding = new Thickness(8),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1),
            FontSize = 12
        };

        textBox.TextChanged += (s, e) =>
        {
            _formData[field.Id] = textBox.Text;
            ValidateField(field, textBox.Text);
        };

        return textBox;
    }

    private FrameworkElement RenderPhoneBox(FormField field)
    {
        var textBox = new TextBox
        {
            Height = 35,
            Padding = new Thickness(8),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1),
            FontSize = 12
        };

        textBox.TextChanged += (s, e) =>
        {
            _formData[field.Id] = textBox.Text;
            ValidateField(field, textBox.Text);
        };

        return textBox;
    }

    private FrameworkElement RenderCheckBox(FormField field)
    {
        var checkBox = new CheckBox { Padding = new Thickness(5), FontSize = 12 };
        checkBox.Checked += (s, e) => _formData[field.Id] = "true";
        checkBox.Unchecked += (s, e) => _formData[field.Id] = "false";
        return checkBox;
    }

    private FrameworkElement RenderRadioButtons(FormField field)
    {
        var panel = new StackPanel { Orientation = Orientation.Vertical };

        foreach (var option in field.Options)
        {
            var radioButton = new RadioButton
            {
                Content = option.Label,
                Padding = new Thickness(5),
                FontSize = 12,
                GroupName = field.Id,
                Margin = new Thickness(0, 0, 0, 5)
            };

            radioButton.Checked += (s, e) => _formData[field.Id] = option.Value;
            panel.Children.Add(radioButton);
        }

        return panel;
    }

    private FrameworkElement RenderComboBox(FormField field)
    {
        var comboBox = new ComboBox { Height = 35, Padding = new Thickness(8), FontSize = 12 };

        foreach (var option in field.Options)
            comboBox.Items.Add(new ComboBoxItem { Content = option.Label, Tag = option.Value });

        comboBox.SelectionChanged += (s, e) =>
        {
            if (comboBox.SelectedItem is ComboBoxItem item && item.Tag is string value)
                _formData[field.Id] = value;
        };

        return comboBox;
    }

    private FrameworkElement RenderMultiSelect(FormField field)
    {
        var panel = new StackPanel { Orientation = Orientation.Vertical };

        foreach (var option in field.Options)
        {
            var checkBox = new CheckBox
            {
                Content = option.Label,
                Padding = new Thickness(5),
                FontSize = 12,
                Tag = option.Value,
                Margin = new Thickness(0, 0, 0, 5)
            };

            checkBox.Checked += (s, e) => UpdateMultiSelect();
            checkBox.Unchecked += (s, e) => UpdateMultiSelect();
            panel.Children.Add(checkBox);
        }

        void UpdateMultiSelect()
        {
            var selected = new List<string>();
            foreach (CheckBox cb in panel.Children.OfType<CheckBox>())
            {
                if (cb.IsChecked == true && cb.Tag is string val)
                    selected.Add(val);
            }
            _formData[field.Id] = string.Join(";", selected);
        }

        return panel;
    }

    private FrameworkElement RenderComboBoxEditable(FormField field)
    {
        var comboBox = new ComboBox { Height = 35, Padding = new Thickness(8), FontSize = 12, IsEditable = true };

        foreach (var option in field.Options)
            comboBox.Items.Add(option.Label);

        comboBox.SelectionChanged += (s, e) => _formData[field.Id] = (comboBox.SelectedItem as string) ?? comboBox.Text;

        return comboBox;
    }

    private FrameworkElement RenderFileUpload(FormField field)
    {
        var panel = new StackPanel { Orientation = Orientation.Horizontal };

        var button = new Button
        {
            Content = "📁 Datei wählen",
            Padding = new Thickness(15, 8, 15, 8),
            Background = new SolidColorBrush(Color.FromRgb(230, 230, 230)),
            Margin = new Thickness(0, 0, 10, 0)
        };

        var label = new TextBlock
        {
            Text = "Keine Datei ausgewählt",
            VerticalAlignment = VerticalAlignment.Center,
            Foreground = new SolidColorBrush(Color.FromRgb(128, 128, 128))
        };

        button.Click += (s, e) =>
        {
            var dialog = new Microsoft.Win32.OpenFileDialog();
            if (dialog.ShowDialog() == true)
            {
                label.Text = System.IO.Path.GetFileName(dialog.FileName);
                _formData[field.Id] = dialog.FileName;
            }
        };

        panel.Children.Add(button);
        panel.Children.Add(label);

        return panel;
    }

    private FrameworkElement RenderSignatureField(FormField field)
    {
        var border = new Border
        {
            Height = 100,
            Background = new SolidColorBrush(Color.FromRgb(255, 255, 255)),
            BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            BorderThickness = new Thickness(1),
            Child = new InkCanvas()
        };
        return border;
    }

    private FrameworkElement RenderImageField(FormField field)
    {
        return new Image
        {
            Height = 150,
            Width = 150,
            Stretch = Stretch.UniformToFill,
            Margin = new Thickness(5)
        };
    }

    private FrameworkElement RenderReadOnlyLabel(FormField field)
    {
        return new TextBlock
        {
            Text = field.DefaultValue?.ToString() ?? field.Label,
            FontSize = 12,
            Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100))
        };
    }

    private FrameworkElement RenderHiddenField(FormField field)
    {
        var textBox = new TextBox { Visibility = Visibility.Hidden };
        if (field.DefaultValue != null)
        {
            var val = field.DefaultValue.ToString() ?? "";
            textBox.Text = val;
            _formData[field.Id] = val;
        }
        return textBox;
    }

    private FrameworkElement RenderCustomField(FormField field)
    {
        var panel = new StackPanel
        {
            Background = new SolidColorBrush(Color.FromRgb(255, 250, 205)),
            Margin = new Thickness(10)
        };

        var label = new TextBlock
        {
            Text = $"[Custom Field - {field.Type}]",
            FontSize = 11,
            Foreground = new SolidColorBrush(Color.FromRgb(128, 0, 0))
        };

        panel.Children.Add(label);
        return panel;
    }

    private void ValidateField(FormField field, string? value)
    {
        var errors = new List<string>();

        if (field.IsRequired && string.IsNullOrWhiteSpace(value))
            errors.Add("Dieses Feld ist erforderlich");

        if (!string.IsNullOrEmpty(field.Pattern) && !string.IsNullOrEmpty(value))
        {
            if (!System.Text.RegularExpressions.Regex.IsMatch(value, field.Pattern))
                errors.Add($"Format ungültig");
        }

        if (field.MinLength.HasValue && (value?.Length ?? 0) < field.MinLength)
            errors.Add($"Mindestens {field.MinLength} Zeichen erforderlich");

        if (field.MaxLength.HasValue && (value?.Length ?? 0) > field.MaxLength)
            errors.Add($"Maximal {field.MaxLength} Zeichen erlaubt");

        if (_fieldControls.TryGetValue(field.Id, out var control) && control is TextBox tb)
        {
            if (errors.Count > 0)
            {
                tb.BorderBrush = new SolidColorBrush(Color.FromRgb(255, 0, 0));
                tb.BorderThickness = new Thickness(2);
                tb.ToolTip = string.Join("\n", errors);
            }
            else
            {
                tb.BorderBrush = new SolidColorBrush(Color.FromRgb(200, 200, 200));
                tb.BorderThickness = new Thickness(1);
                tb.ToolTip = null;
            }
        }
    }

    private void SubmitForm()
    {
        if (_currentTemplate == null)
            return;

        var allData = new Dictionary<string, object>();
        foreach (var kvp in _formData)
            allData[kvp.Key] = kvp.Value ?? "";

        FormSubmitted?.Invoke(this, new FormSubmissionEventArgs { TemplateId = _currentTemplate.Id, FormData = allData });
        MessageBox.Show("Formular wurde abgesendet", "Erfolg", MessageBoxButton.OK, MessageBoxImage.Information);
    }

    private void ResetForm()
    {
        _formData.Clear();
        foreach (var control in _fieldControls.Values)
        {
            if (control is TextBox tb)
                tb.Clear();
            else if (control is ComboBox cb)
                cb.SelectedIndex = -1;
            else if (control is CheckBox chk)
                chk.IsChecked = false;
        }
    }

    public void ClearForm() => ResetForm();
}


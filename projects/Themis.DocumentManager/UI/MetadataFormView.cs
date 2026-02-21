/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataFormView.cs                                ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     506                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.UI;

/// <summary>
/// Moderne Form-Ansicht für Metadaten mit editierbaren, validierbaren Feldern
/// </summary>
public class MetadataFormView : UserControl
{
    private Grid _mainGrid = null!;
    private ScrollViewer _scrollViewer = null!;
    private StackPanel _formPanel = null!;
    private List<MetadataFieldGroup> _groups = new();
    private Dictionary<string, FrameworkElement> _fieldControls = new();

    public event EventHandler<FieldEditEventArgs>? FieldValueChanged;

    public MetadataFormView()
    {
        InitializeComponent();
    }

    private void InitializeComponent()
    {
        _mainGrid = new Grid();
        _scrollViewer = new ScrollViewer
        {
            VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
            HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled
        };

        _formPanel = new StackPanel
        {
            Margin = new Thickness(0)
        };

        _scrollViewer.Content = _formPanel;
        _mainGrid.Children.Add(_scrollViewer);
        Content = _mainGrid;
    }

    public void LoadMetadata(List<MetadataFieldGroup> groups)
    {
        _groups = groups ?? new List<MetadataFieldGroup>();
        _fieldControls.Clear();
        RenderForm();
    }

    private void RenderForm()
    {
        _formPanel.Children.Clear();

        foreach (var group in _groups.OrderBy(g => g.DisplayOrder))
        {
            var groupPanel = CreateGroupPanel(group);
            _formPanel.Children.Add(groupPanel);
        }
    }

    private Border CreateGroupPanel(MetadataFieldGroup group)
    {
        var border = new Border
        {
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Color.FromRgb(226, 232, 240)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(6),
            Margin = new Thickness(0, 0, 0, 12),
            Padding = new Thickness(16)
        };

        var mainStack = new StackPanel();

        // Group Header
        var headerPanel = new StackPanel
        {
            Orientation = Orientation.Horizontal,
            Margin = new Thickness(0, 0, 0, 12)
        };

        var iconText = new TextBlock
        {
            Text = group.Icon ?? "📋",
            FontSize = 18,
            Margin = new Thickness(0, 0, 8, 0),
            VerticalAlignment = VerticalAlignment.Center
        };

        var titleText = new TextBlock
        {
            Text = group.Title,
            FontSize = 14,
            FontWeight = FontWeights.SemiBold,
            Foreground = new SolidColorBrush(Color.FromRgb(30, 41, 59)),
            VerticalAlignment = VerticalAlignment.Center
        };

        headerPanel.Children.Add(iconText);
        headerPanel.Children.Add(titleText);
        mainStack.Children.Add(headerPanel);

        // Fields Grid
        var fieldsGrid = new Grid
        {
            Margin = new Thickness(0)
        };

        fieldsGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(140) }); // Label
        fieldsGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) }); // Input

        int row = 0;
        foreach (var field in group.Fields)
        {
            fieldsGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

            // Label
            var label = CreateLabel(field);
            Grid.SetRow(label, row);
            Grid.SetColumn(label, 0);
            fieldsGrid.Children.Add(label);

            // Input Control
            var input = CreateInputControl(field, group);
            Grid.SetRow(input, row);
            Grid.SetColumn(input, 1);
            fieldsGrid.Children.Add(input);

            _fieldControls[$"{group.Id}_{field.FieldName}"] = input;

            row++;
        }

        mainStack.Children.Add(fieldsGrid);
        border.Child = mainStack;

        return border;
    }

    private FrameworkElement CreateLabel(MetadataField field)
    {
        var stackPanel = new StackPanel
        {
            Orientation = Orientation.Horizontal,
            Margin = new Thickness(0, 0, 12, 12),
            VerticalAlignment = VerticalAlignment.Top
        };

        if (field.IsRequired)
        {
            var asterisk = new TextBlock
            {
                Text = "*",
                Foreground = new SolidColorBrush(Color.FromRgb(239, 68, 68)),
                FontWeight = FontWeights.Bold,
                Margin = new Thickness(0, 0, 4, 0),
                VerticalAlignment = VerticalAlignment.Center
            };
            stackPanel.Children.Add(asterisk);
        }

        var labelText = new TextBlock
        {
            Text = field.FieldName,
            FontSize = 12,
            FontWeight = FontWeights.Medium,
            Foreground = new SolidColorBrush(Color.FromRgb(71, 85, 105)),
            VerticalAlignment = VerticalAlignment.Center,
            TextWrapping = TextWrapping.Wrap
        };

        stackPanel.Children.Add(labelText);
        return stackPanel;
    }

    private FrameworkElement CreateInputControl(MetadataField field, MetadataFieldGroup group)
    {
        var container = new Border
        {
            Margin = new Thickness(0, 0, 0, 12)
        };

        FrameworkElement control;

        switch (field.Type)
        {
            case FieldType.Date:
                control = CreateDatePicker(field);
                break;

            case FieldType.Boolean:
                control = CreateCheckBox(field);
                break;

            case FieldType.Dropdown:
                control = CreateComboBox(field);
                break;

            case FieldType.Number:
                control = CreateNumberInput(field);
                break;

            case FieldType.RichText:
                control = CreateTextArea(field, multiline: true);
                break;

            default:
                control = CreateTextBox(field);
                break;
        }

        container.Child = control;
        return container;
    }

    private DatePicker CreateDatePicker(MetadataField field)
    {
        var datePicker = new DatePicker
        {
            SelectedDate = string.IsNullOrEmpty(field.CurrentValue)
                ? null
                : DateTime.TryParse(field.CurrentValue, out var date) ? date : null,
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Color.FromRgb(203, 213, 225)),
            BorderThickness = new Thickness(1),
            Padding = new Thickness(10, 6, 10, 6),
            FontSize = 13
        };

        datePicker.SelectedDateChanged += (s, e) =>
        {
            if (datePicker.SelectedDate.HasValue)
            {
                field.CurrentValue = datePicker.SelectedDate.Value.ToString("dd.MM.yyyy");
                FieldValueChanged?.Invoke(this, new FieldEditEventArgs
                {
                    Field = field,
                    NewValue = field.CurrentValue
                });
            }
        };

        return datePicker;
    }

    private CheckBox CreateCheckBox(MetadataField field)
    {
        var checkBox = new CheckBox
        {
            IsChecked = field.CurrentValue?.ToLower() == "true",
            VerticalAlignment = VerticalAlignment.Center,
            FontSize = 13
        };

        checkBox.Checked += (s, e) =>
        {
            field.CurrentValue = "true";
            FieldValueChanged?.Invoke(this, new FieldEditEventArgs
            {
                Field = field,
                NewValue = "true"
            });
        };

        checkBox.Unchecked += (s, e) =>
        {
            field.CurrentValue = "false";
            FieldValueChanged?.Invoke(this, new FieldEditEventArgs
            {
                Field = field,
                NewValue = "false"
            });
        };

        return checkBox;
    }

    private ComboBox CreateComboBox(MetadataField field)
    {
        var comboBox = new ComboBox
        {
            ItemsSource = field.Options ?? new List<string>(),
            SelectedItem = field.CurrentValue,
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Color.FromRgb(203, 213, 225)),
            BorderThickness = new Thickness(1),
            Padding = new Thickness(10, 6, 10, 6),
            FontSize = 13,
            IsEditable = false
        };

        comboBox.SelectionChanged += (s, e) =>
        {
            if (comboBox.SelectedItem != null)
            {
                field.CurrentValue = comboBox.SelectedItem.ToString();
                FieldValueChanged?.Invoke(this, new FieldEditEventArgs
                {
                    Field = field,
                    NewValue = field.CurrentValue
                });
            }
        };

        return comboBox;
    }

    private TextBox CreateNumberInput(MetadataField field)
    {
        var textBox = new TextBox
        {
            Text = field.CurrentValue ?? string.Empty,
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Color.FromRgb(203, 213, 225)),
            BorderThickness = new Thickness(1),
            Padding = new Thickness(10, 6, 10, 6),
            FontSize = 13
        };

        textBox.PreviewTextInput += (s, e) =>
        {
            e.Handled = !IsNumeric(e.Text);
        };

        textBox.TextChanged += (s, e) =>
        {
            field.CurrentValue = textBox.Text;
            ValidateField(textBox, field);
            FieldValueChanged?.Invoke(this, new FieldEditEventArgs
            {
                Field = field,
                NewValue = textBox.Text
            });
        };

        return textBox;
    }

    private TextBox CreateTextBox(MetadataField field)
    {
        var textBox = new TextBox
        {
            Text = field.CurrentValue ?? string.Empty,
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Color.FromRgb(203, 213, 225)),
            BorderThickness = new Thickness(1),
            Padding = new Thickness(10, 6, 10, 6),
            FontSize = 13
        };

        textBox.TextChanged += (s, e) =>
        {
            field.CurrentValue = textBox.Text;
            ValidateField(textBox, field);
            FieldValueChanged?.Invoke(this, new FieldEditEventArgs
            {
                Field = field,
                NewValue = textBox.Text
            });
        };

        textBox.GotFocus += (s, e) =>
        {
            textBox.BorderBrush = new SolidColorBrush(Color.FromRgb(59, 130, 246)); // Blue focus
            textBox.BorderThickness = new Thickness(2);
        };

        textBox.LostFocus += (s, e) =>
        {
            textBox.BorderBrush = new SolidColorBrush(Color.FromRgb(203, 213, 225));
            textBox.BorderThickness = new Thickness(1);
            ValidateField(textBox, field);
        };

        return textBox;
    }

    private TextBox CreateTextArea(MetadataField field, bool multiline)
    {
        var textBox = new TextBox
        {
            Text = field.CurrentValue ?? string.Empty,
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Color.FromRgb(203, 213, 225)),
            BorderThickness = new Thickness(1),
            Padding = new Thickness(10, 8, 10, 8),
            FontSize = 13,
            MinHeight = 80,
            AcceptsReturn = multiline,
            TextWrapping = TextWrapping.Wrap,
            VerticalScrollBarVisibility = ScrollBarVisibility.Auto
        };

        textBox.TextChanged += (s, e) =>
        {
            field.CurrentValue = textBox.Text;
            FieldValueChanged?.Invoke(this, new FieldEditEventArgs
            {
                Field = field,
                NewValue = textBox.Text
            });
        };

        textBox.GotFocus += (s, e) =>
        {
            textBox.BorderBrush = new SolidColorBrush(Color.FromRgb(59, 130, 246));
            textBox.BorderThickness = new Thickness(2);
        };

        textBox.LostFocus += (s, e) =>
        {
            textBox.BorderBrush = new SolidColorBrush(Color.FromRgb(203, 213, 225));
            textBox.BorderThickness = new Thickness(1);
        };

        return textBox;
    }

    private void ValidateField(TextBox textBox, MetadataField field)
    {
        if (field.IsRequired && string.IsNullOrWhiteSpace(field.CurrentValue))
        {
            // Mark as invalid
            textBox.BorderBrush = new SolidColorBrush(Color.FromRgb(239, 68, 68)); // Red
            textBox.BorderThickness = new Thickness(2);
        }
        else
        {
            // Mark as valid
            textBox.BorderBrush = new SolidColorBrush(Color.FromRgb(203, 213, 225));
            textBox.BorderThickness = new Thickness(1);
        }
    }

    private bool IsNumeric(string text)
    {
        return text.All(c => char.IsDigit(c) || c == '.' || c == ',' || c == '-');
    }

    public (bool IsValid, List<string> Errors) ValidateAllFields()
    {
        var errors = new List<string>();

        foreach (var group in _groups)
        {
            foreach (var field in group.Fields.Where(f => f.IsRequired))
            {
                if (string.IsNullOrWhiteSpace(field.CurrentValue))
                {
                    errors.Add($"{group.Title}: {field.FieldName} ist erforderlich");
                }
            }
        }

        return (errors.Count == 0, errors);
    }

    public Dictionary<string, string> ExportMetadata()
    {
        var result = new Dictionary<string, string>();

        foreach (var group in _groups)
        {
            foreach (var field in group.Fields)
            {
                if (!string.IsNullOrEmpty(field.CurrentValue))
                {
                    result[field.ThemisPath] = field.CurrentValue;
                }
            }
        }

        return result;
    }
}

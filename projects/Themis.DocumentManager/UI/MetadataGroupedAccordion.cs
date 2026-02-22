/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataGroupedAccordion.cs                        ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     611                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
/// Gruppierte Accordion-Ansicht für Metadaten
/// Intelligentes Auto-Collapse für leere Sektionen
/// </summary>
public class MetadataGroupedAccordion : UserControl
{
    private StackPanel? _sectionsPanel;

    public static readonly DependencyProperty FieldGroupsProperty =
        DependencyProperty.Register(nameof(FieldGroups), typeof(List<MetadataFieldGroup>),
            typeof(MetadataGroupedAccordion), new PropertyMetadata(null, OnFieldGroupsChanged));

    public static readonly DependencyProperty CollapseStrategyProperty =
        DependencyProperty.Register(nameof(Strategy), typeof(CollapseStrategy),
            typeof(MetadataGroupedAccordion), new PropertyMetadata(CollapseStrategy.HideEmptySections));

    public List<MetadataFieldGroup>? FieldGroups
    {
        get => (List<MetadataFieldGroup>?)GetValue(FieldGroupsProperty);
        set => SetValue(FieldGroupsProperty, value);
    }

    public CollapseStrategy Strategy
    {
        get => (CollapseStrategy)GetValue(CollapseStrategyProperty);
        set => SetValue(CollapseStrategyProperty, value);
    }

    public event EventHandler<FieldGroupExpandedEventArgs>? GroupExpanded;
    public event EventHandler<FieldEditEventArgs>? FieldValueChanged;

    public MetadataGroupedAccordion()
    {
        InitializeUI();
    }

    private void InitializeUI()
    {
        var scrollViewer = new ScrollViewer
        {
            VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
            HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled
        };

        _sectionsPanel = new StackPanel
        {
            Orientation = Orientation.Vertical,
            Margin = new Thickness(0)
        };

        scrollViewer.Content = _sectionsPanel;

        Content = new Border
        {
            Child = scrollViewer,
            Background = new SolidColorBrush(Color.FromRgb(250, 250, 250)),
            BorderBrush = new SolidColorBrush(Color.FromRgb(224, 224, 224)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(4),
            Padding = new Thickness(5)
        };
    }

    private static void OnFieldGroupsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is MetadataGroupedAccordion accordion && e.NewValue is List<MetadataFieldGroup> groups)
        {
            accordion.RenderGroupedView(groups);
        }
    }

    public void RenderGroupedView(List<MetadataFieldGroup> groups)
    {
        if (_sectionsPanel == null) return;
        _sectionsPanel.Children.Clear();

        // Validierungs-Panel hinzufügen (oben)
        var validationPanel = CreateValidationPanel(groups);
        if (validationPanel != null)
        {
            _sectionsPanel.Children.Add(validationPanel);
        }

        var sortedGroups = groups.OrderBy(g => g.DisplayOrder).ToList();

        foreach (var group in sortedGroups)
        {
            // Skip komplett leere Gruppen wenn Strategy = HideEmptySections
            if (Strategy == CollapseStrategy.HideEmptySections && group.IsEmpty)
                continue;

            var expander = CreateGroupExpander(group);
            _sectionsPanel.Children.Add(expander);
        }
    }

    private Border? CreateValidationPanel(List<MetadataFieldGroup> groups)
    {
        var requiredFields = groups
            .SelectMany(g => g.Fields)
            .Where(f => f.IsRequired && string.IsNullOrEmpty(f.CurrentValue))
            .ToList();

        if (requiredFields.Count == 0)
            return null; // Keine Validierungsfehler

        var stackPanel = new StackPanel
        {
            Orientation = Orientation.Vertical,
            Margin = new Thickness(0, 0, 0, 8)
        };

        var headerText = new TextBlock
        {
            Text = $"⚠️ {requiredFields.Count} erforderliche Feld(er) fehlen",
            FontSize = 11,
            FontWeight = FontWeights.Bold,
            Foreground = new SolidColorBrush(Color.FromRgb(200, 100, 0)),
            Margin = new Thickness(0, 0, 0, 6)
        };
        stackPanel.Children.Add(headerText);

        foreach (var field in requiredFields.Take(5)) // Max. 5 anzeigen
        {
            var fieldLabel = new TextBlock
            {
                Text = $"• {field.FieldName}",
                FontSize = 10,
                Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100)),
                Margin = new Thickness(12, 0, 0, 3)
            };
            stackPanel.Children.Add(fieldLabel);
        }

        if (requiredFields.Count > 5)
        {
            var moreText = new TextBlock
            {
                Text = $"... und {requiredFields.Count - 5} weitere",
                FontSize = 10,
                Foreground = new SolidColorBrush(Color.FromRgb(150, 150, 150))
            };
            stackPanel.Children.Add(moreText);
        }

        return new Border
        {
            Child = stackPanel,
            Background = new SolidColorBrush(Color.FromRgb(255, 250, 240)),
            BorderBrush = new SolidColorBrush(Color.FromRgb(255, 180, 0)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(4),
            Padding = new Thickness(10, 8, 10, 8),
            Margin = new Thickness(0, 0, 0, 0)
        };
    }

    private Expander CreateGroupExpander(MetadataFieldGroup group)
    {
        // Header mit Feld-Count
        var headerPanel = new StackPanel { Orientation = Orientation.Horizontal };

        var iconText = new TextBlock
        {
            Text = group.Icon,
            FontSize = 14,
            Margin = new Thickness(0, 0, 5, 0),
            VerticalAlignment = VerticalAlignment.Center
        };

        var titleText = new TextBlock
        {
            Text = group.Title,
            FontSize = 13,
            FontWeight = FontWeights.Bold,
            VerticalAlignment = VerticalAlignment.Center
        };

        var countBadge = new Border
        {
            Background = new SolidColorBrush(group.FilledFieldCount > 0
                ? Color.FromRgb(76, 175, 80)
                : Color.FromRgb(158, 158, 158)),
            CornerRadius = new CornerRadius(10),
            Padding = new Thickness(6, 2, 6, 2),
            Margin = new Thickness(8, 0, 0, 0),
            Child = new TextBlock
            {
                Text = $"{group.FilledFieldCount} ausgefüllt",
                Foreground = new SolidColorBrush(Colors.White),
                FontSize = 10,
                FontWeight = FontWeights.SemiBold
            }
        };

        headerPanel.Children.Add(iconText);
        headerPanel.Children.Add(titleText);
        headerPanel.Children.Add(countBadge);

        // Content Grid
        var contentGrid = CreateFieldGrid(group);

        var expander = new Expander
        {
            Header = headerPanel,
            Content = contentGrid,
            IsExpanded = ShouldExpandGroup(group),
            Margin = new Thickness(0, 0, 0, 5),
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Color.FromRgb(224, 224, 224)),
            BorderThickness = new Thickness(1),
            Padding = new Thickness(10)
        };

        expander.Expanded += (s, e) =>
        {
            GroupExpanded?.Invoke(this, new FieldGroupExpandedEventArgs { Group = group });
        };

        return expander;
    }

    private Grid CreateFieldGrid(MetadataFieldGroup group)
    {
        var grid = new Grid();
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

        var visibleFields = GetVisibleFields(group);
        int row = 0;
        int col = 0;

        foreach (var field in visibleFields)
        {
            if (col >= 2)
            {
                col = 0;
                row++;
            }

            var fieldControl = CreateFieldControl(field, group);
            Grid.SetRow(fieldControl, row);
            Grid.SetColumn(fieldControl, col);

            if (row >= grid.RowDefinitions.Count)
                grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

            grid.Children.Add(fieldControl);
            col++;
        }

        // "Alle Felder anzeigen" Button wenn Felder ausgeblendet
        if (Strategy == CollapseStrategy.HideEmptyFields &&
            group.Fields.Count > visibleFields.Count)
        {
            var showAllBtn = new Button
            {
                Content = $"+ {group.Fields.Count - visibleFields.Count} leere Felder anzeigen",
                Padding = new Thickness(8, 4, 8, 4),
                Background = new SolidColorBrush(Color.FromRgb(240, 240, 240)),
                BorderThickness = new Thickness(0),
                FontSize = 11,
                HorizontalAlignment = HorizontalAlignment.Left,
                Margin = new Thickness(0, 10, 0, 0),
                Cursor = System.Windows.Input.Cursors.Hand
            };

            showAllBtn.Click += (s, e) =>
            {
                // Re-render mit allen Feldern
                var allFields = group.Fields;
                grid.Children.Clear();
                grid.RowDefinitions.Clear();
                RenderAllFields(grid, allFields, group);
                showAllBtn.Visibility = Visibility.Collapsed;
            };

            Grid.SetRow(showAllBtn, row + 1);
            Grid.SetColumn(showAllBtn, 0);
            Grid.SetColumnSpan(showAllBtn, 2);
            grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            grid.Children.Add(showAllBtn);
        }

        return grid;
    }

    private void RenderAllFields(Grid grid, List<MetadataField> fields, MetadataFieldGroup group)
    {
        int row = 0;
        int col = 0;

        foreach (var field in fields)
        {
            if (col >= 2)
            {
                col = 0;
                row++;
            }

            var fieldControl = CreateFieldControl(field, group);
            Grid.SetRow(fieldControl, row);
            Grid.SetColumn(fieldControl, col);

            if (row >= grid.RowDefinitions.Count)
                grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

            grid.Children.Add(fieldControl);
            col++;
        }
    }

    private Border CreateFieldControl(MetadataField field, MetadataFieldGroup group)
    {
        var stackPanel = new StackPanel
        {
            Orientation = Orientation.Vertical,
            Margin = new Thickness(5)
        };

        // Label mit Required-Indikator
        var labelText = new TextBlock
        {
            Text = (field.IsRequired ? "* " : "") + field.FieldName,
            FontSize = 11,
            FontWeight = FontWeights.SemiBold,
            Foreground = new SolidColorBrush(Color.FromRgb(80, 80, 80)),
            Margin = new Thickness(0, 0, 0, 3)
        };
        stackPanel.Children.Add(labelText);

        // Editierbar: TextBox für Text-Felder oder DatePicker für Dates
        UIElement inputControl;
        if (field.Type == FieldType.Date)
        {
            var datePicker = new System.Windows.Controls.DatePicker
            {
                SelectedDate = string.IsNullOrEmpty(field.CurrentValue) 
                    ? DateTime.Now 
                    : DateTime.Parse(field.CurrentValue),
                Padding = new Thickness(8, 6, 8, 6),
                FontSize = 12,
                Background = new SolidColorBrush(Colors.White)
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

            inputControl = datePicker;
        }
        else if (field.Type == FieldType.Number)
        {
            var textBox = new TextBox
            {
                Text = field.CurrentValue ?? string.Empty,
                Padding = new Thickness(8, 6, 8, 6),
                FontSize = 12,
                Background = new SolidColorBrush(Colors.White),
                TextWrapping = TextWrapping.Wrap
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

            inputControl = textBox;
        }
        else if (field.Type == FieldType.Dropdown && field.Options != null && field.Options.Count > 0)
        {
            var comboBox = new ComboBox
            {
                ItemsSource = field.Options,
                SelectedItem = field.CurrentValue,
                Padding = new Thickness(8, 6, 8, 6),
                FontSize = 12,
                Background = new SolidColorBrush(Colors.White),
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

            inputControl = comboBox;
        }
        else if (field.Type == FieldType.Boolean)
        {
            var checkBox = new CheckBox
            {
                IsChecked = field.CurrentValue?.ToLower() == "true",
                Margin = new Thickness(8, 6, 8, 6),
                FontSize = 12
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

            inputControl = checkBox;
        }
        else
        {
            // Default: Multiline TextBox
            var textBox = new TextBox
            {
                Text = field.CurrentValue ?? string.Empty,
                Padding = new Thickness(8, 6, 8, 6),
                FontSize = 12,
                MinHeight = 40,
                TextWrapping = TextWrapping.Wrap,
                VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                Background = new SolidColorBrush(Colors.White),
                Foreground = new SolidColorBrush(Colors.Black)
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

            inputControl = textBox;
        }

        stackPanel.Children.Add(inputControl);

        // ThemisDB Path Indicator
        if (!string.IsNullOrEmpty(field.ThemisPath))
        {
            var pathText = new TextBlock
            {
                Text = $"🔗 {field.ThemisPath}",
                FontSize = 9,
                Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100)),
                Margin = new Thickness(0, 2, 0, 0)
            };
            stackPanel.Children.Add(pathText);
        }

        return new Border
        {
            Child = stackPanel,
            BorderBrush = new SolidColorBrush(Color.FromRgb(224, 224, 224)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(3),
            Background = new SolidColorBrush(Colors.White),
            Margin = new Thickness(2)
        };
    }

    private List<MetadataField> GetVisibleFields(MetadataFieldGroup group)
    {
        return Strategy switch
        {
            CollapseStrategy.HideEmptyFields => group.Fields.Where(f => !string.IsNullOrEmpty(f.CurrentValue)).ToList(),
            CollapseStrategy.ShowAllExpanded or CollapseStrategy.ShowAllCollapsed => group.Fields,
            _ => group.Fields
        };
    }

    private bool ShouldExpandGroup(MetadataFieldGroup group)
    {
        return Strategy switch
        {
            CollapseStrategy.HideEmptySections => group.FilledFieldCount > 0,
            CollapseStrategy.HideEmptyFields => group.FilledFieldCount > 0,
            CollapseStrategy.ShowAllExpanded => true,
            CollapseStrategy.ShowAllCollapsed => false,
            _ => group.IsExpanded
        };
    }

    /// <summary>
    /// Validiere alle erforderlichen Felder
    /// </summary>
    public (bool IsValid, List<string> ErrorMessages) ValidateRequiredFields()
    {
        var errors = new List<string>();

        if (FieldGroups == null)
            return (true, errors);

        foreach (var group in FieldGroups)
        {
            foreach (var field in group.Fields)
            {
                if (field.IsRequired && string.IsNullOrWhiteSpace(field.CurrentValue))
                {
                    errors.Add($"{group.Title} > {field.FieldName}");
                }
            }
        }

        return (errors.Count == 0, errors);
    }

    /// <summary>
    /// Exportiere alle Metadaten als Key-Value Dictionary
    /// </summary>
    public Dictionary<string, string> ExportMetadata()
    {
        var data = new Dictionary<string, string>();

        if (FieldGroups == null)
            return data;

        foreach (var group in FieldGroups)
        {
            foreach (var field in group.Fields)
            {
                var key = $"{group.Title}.{field.FieldName}".Replace(" ", "_");
                data[key] = field.CurrentValue ?? string.Empty;
            }
        }

        return data;
    }
}

public enum CollapseStrategy
{
    HideEmptySections,      // Sektionen mit 0 Feldern komplett ausblenden
    HideEmptyFields,        // Leere Felder innerhalb Sektionen ausblenden
    ShowAllCollapsed,       // Alle Sektionen initial collapsed
    ShowAllExpanded         // Alle Sektionen initial expanded
}

public class FieldGroupExpandedEventArgs : EventArgs
{
    public MetadataFieldGroup? Group { get; set; }
}

public class FieldEditEventArgs : EventArgs
{
    public MetadataField? Field { get; set; }
    public string? NewValue { get; set; }
}

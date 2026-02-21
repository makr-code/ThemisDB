/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataSummaryPanel.cs                            ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     337                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
/// Zusammenfassungs-View für Metadaten mit statistischen Informationen
/// </summary>
public class MetadataSummaryPanel : UserControl
{
    public static readonly DependencyProperty BindingProperty =
        DependencyProperty.Register(nameof(Binding), typeof(DocumentMetadataBinding),
            typeof(MetadataSummaryPanel), new PropertyMetadata(null, OnBindingChanged));

    public DocumentMetadataBinding? Binding
    {
        get => (DocumentMetadataBinding?)GetValue(BindingProperty);
        set => SetValue(BindingProperty, value);
    }

    private StackPanel? _contentPanel;

    public MetadataSummaryPanel()
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

        _contentPanel = new StackPanel
        {
            Orientation = Orientation.Vertical,
            Margin = new Thickness(12)
        };

        scrollViewer.Content = _contentPanel;

        Content = new Border
        {
            Child = scrollViewer,
            Background = new SolidColorBrush(Color.FromRgb(250, 250, 250)),
            BorderBrush = new SolidColorBrush(Color.FromRgb(224, 224, 224)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(4)
        };
    }

    private static void OnBindingChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is MetadataSummaryPanel panel)
        {
            panel.RenderSummary();
        }
    }

    private void RenderSummary()
    {
        if (_contentPanel == null)
            return;

        _contentPanel.Children.Clear();

        if (Binding == null)
        {
            var emptyText = new TextBlock
            {
                Text = "Keine Metadaten vorhanden",
                FontSize = 12,
                Foreground = new SolidColorBrush(Color.FromRgb(150, 150, 150)),
                TextAlignment = TextAlignment.Center,
                Margin = new Thickness(0, 40, 0, 0)
            };
            _contentPanel.Children.Add(emptyText);
            return;
        }

        // Header
        var titleBlock = new TextBlock
        {
            Text = "Metadaten-Übersicht",
            FontSize = 16,
            FontWeight = FontWeights.Bold,
            Margin = new Thickness(0, 0, 0, 16)
        };
        _contentPanel.Children.Add(titleBlock);

        // Binding-Informationen
        _contentPanel.Children.Add(CreateInfoSection("Dokument-Information",
            new[]
            {
                ("Dokument-ID:", Binding.DocumentId),
                ("Prozess-ID:", Binding.ProcessId),
                ("Status:", Binding.Status.ToString()),
                ("Version:", Binding.Version.ToString())
            }
        ));

        // Zeitstempel
        var timeSection = CreateInfoSection("Zeitstempel",
            new[]
            {
                ("Erstellt am:", Binding.CreatedAt.ToString("dd.MM.yyyy HH:mm:ss")),
                ("Erstellt von:", Binding.CreatedBy),
                ("Finalisiert am:", Binding.FinalizedAt?.ToString("dd.MM.yyyy HH:mm:ss") ?? "—"),
                ("Finalisiert von:", Binding.FinalizedBy ?? "—")
            }
        );
        timeSection.Margin = new Thickness(0, 12, 0, 0);
        _contentPanel.Children.Add(timeSection);

        // Feld-Statistiken
        var filledCount = Binding.BoundFields.Count(f => !string.IsNullOrEmpty(f.CurrentValue));
        var requiredCount = Binding.BoundFields.Count(f => f.IsRequired);
        var filledRequired = Binding.BoundFields.Count(f => f.IsRequired && !string.IsNullOrEmpty(f.CurrentValue));

        var statsSection = CreateInfoSection("Feld-Statistiken",
            new[]
            {
                ("Gesamtfelder:", Binding.BoundFields.Count.ToString()),
                ("Ausgefüllt:", $"{filledCount} ({(Binding.BoundFields.Count > 0 ? (filledCount * 100 / Binding.BoundFields.Count) : 0)}%)"),
                ("Erforderlich:", requiredCount.ToString()),
                ("Erforderlich ausgefüllt:", $"{filledRequired}/{requiredCount}")
            }
        );
        statsSection.Margin = new Thickness(0, 12, 0, 0);
        _contentPanel.Children.Add(statsSection);

        // Feld-Liste (Top 10)
        if (Binding.BoundFields.Count > 0)
        {
            var fieldsToShow = Binding.BoundFields
                .OrderBy(f => string.IsNullOrEmpty(f.CurrentValue))
                .ThenBy(f => f.FieldName)
                .Take(10)
                .ToList();

            var fieldListSection = CreateFieldList("Top-Felder", fieldsToShow);
            fieldListSection.Margin = new Thickness(0, 12, 0, 0);
            _contentPanel.Children.Add(fieldListSection);
        }

        // Sicherheits-Info
        if (Binding.Status == BindingStatus.Finalized)
        {
            var securityBorder = new Border
            {
                Background = new SolidColorBrush(Color.FromRgb(240, 255, 240)),
                BorderBrush = new SolidColorBrush(Color.FromRgb(76, 175, 80)),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(4),
                Padding = new Thickness(10, 8, 10, 8),
                Margin = new Thickness(0, 8, 0, 0)
            };

            var securityStack = new StackPanel();

            var securityTitle = new TextBlock
            {
                Text = "🔒 Finalisiertes Dokument",
                FontSize = 12,
                FontWeight = FontWeights.Bold,
                Foreground = new SolidColorBrush(Color.FromRgb(76, 175, 80)),
                Margin = new Thickness(0, 0, 0, 4)
            };
            securityStack.Children.Add(securityTitle);

            var securityMsg = new TextBlock
            {
                Text = "Dieses Dokument ist finalisiert und kann nicht mehr bearbeitet werden.",
                FontSize = 10,
                Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100)),
                TextWrapping = TextWrapping.Wrap
            };
            securityStack.Children.Add(securityMsg);

            securityBorder.Child = securityStack;
            _contentPanel.Children.Add(securityBorder);
        }
    }

    private Border CreateInfoSection(string title, (string Label, string Value)[] items)
    {
        var stack = new StackPanel();

        var titleBlock = new TextBlock
        {
            Text = title,
            FontSize = 12,
            FontWeight = FontWeights.Bold,
            Margin = new Thickness(0, 0, 0, 8),
            Foreground = new SolidColorBrush(Color.FromRgb(50, 50, 50))
        };
        stack.Children.Add(titleBlock);

        foreach (var (label, value) in items)
        {
            var grid = new Grid();
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(140) });
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

            var labelBlock = new TextBlock
            {
                Text = label,
                FontSize = 10,
                FontWeight = FontWeights.SemiBold,
                Foreground = new SolidColorBrush(Color.FromRgb(100, 100, 100))
            };
            Grid.SetColumn(labelBlock, 0);
            grid.Children.Add(labelBlock);

            var valueBlock = new TextBlock
            {
                Text = value,
                FontSize = 10,
                Foreground = new SolidColorBrush(Colors.Black),
                TextWrapping = TextWrapping.Wrap
            };
            Grid.SetColumn(valueBlock, 1);
            grid.Children.Add(valueBlock);

            stack.Children.Add(grid);
        }

        return new Border
        {
            Child = stack,
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Color.FromRgb(224, 224, 224)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(4),
            Padding = new Thickness(10, 8, 10, 8)
        };
    }

    private Border CreateFieldList(string title, List<MetadataField> fields)
    {
        var stack = new StackPanel();

        var titleBlock = new TextBlock
        {
            Text = title,
            FontSize = 12,
            FontWeight = FontWeights.Bold,
            Margin = new Thickness(0, 0, 0, 8),
            Foreground = new SolidColorBrush(Color.FromRgb(50, 50, 50))
        };
        stack.Children.Add(titleBlock);

        foreach (var field in fields)
        {
            var fieldBorder = new Border
            {
                Background = new SolidColorBrush(string.IsNullOrEmpty(field.CurrentValue) 
                    ? Color.FromRgb(245, 245, 245) 
                    : Color.FromRgb(255, 255, 255)),
                BorderBrush = new SolidColorBrush(Color.FromRgb(240, 240, 240)),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(3),
                Padding = new Thickness(8, 6, 8, 6),
                Margin = new Thickness(0, 0, 0, 4)
            };

            var fieldStack = new StackPanel { Orientation = Orientation.Vertical };

            var fieldName = new TextBlock
            {
                Text = (field.IsRequired ? "* " : "") + field.FieldName,
                FontSize = 10,
                FontWeight = FontWeights.SemiBold,
                Foreground = new SolidColorBrush(Color.FromRgb(50, 50, 50)),
                Margin = new Thickness(0, 0, 0, 2)
            };
            fieldStack.Children.Add(fieldName);

            var fieldValue = new TextBlock
            {
                Text = string.IsNullOrEmpty(field.CurrentValue) ? "(leer)" : field.CurrentValue,
                FontSize = 9,
                Foreground = string.IsNullOrEmpty(field.CurrentValue)
                    ? new SolidColorBrush(Color.FromRgb(180, 180, 180))
                    : new SolidColorBrush(Colors.Black),
                TextWrapping = TextWrapping.Wrap
            };
            fieldStack.Children.Add(fieldValue);

            fieldBorder.Child = fieldStack;
            stack.Children.Add(fieldBorder);
        }

        return new Border
        {
            Child = stack,
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Color.FromRgb(224, 224, 224)),
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(4),
            Padding = new Thickness(10, 8, 10, 8)
        };
    }
}

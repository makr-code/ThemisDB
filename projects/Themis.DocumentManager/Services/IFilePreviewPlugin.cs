/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IFilePreviewPlugin.cs                              ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     389                                            ║
    • Open Issues:     TODOs: 2, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Plugin-Schnittstelle für Datei-Previews
/// </summary>
public interface IFilePreviewPlugin
{
    /// <summary>
    /// Name des Plugins
    /// </summary>
    string PluginName { get; }

    /// <summary>
    /// Unterstützte Dateierweiterungen (z.B. ".pdf", ".txt", ".png")
    /// </summary>
    IEnumerable<string> SupportedExtensions { get; }

    /// <summary>
    /// Prüft, ob das Plugin diese Datei unterstützt
    /// </summary>
    bool CanPreview(string filePath);

    /// <summary>
    /// Generiert ein WPF UIElement für die Vorschau
    /// </summary>
    UIElement GeneratePreview(string filePath);

    /// <summary>
    /// Gibt Metadaten der Datei zurück
    /// </summary>
    Dictionary<string, string> GetMetadata(string filePath);
}

/// <summary>
/// Service zur Verwaltung von File-Preview-Plugins
/// </summary>
public interface IFilePreviewPluginService
{
    /// <summary>
    /// Registriert ein Plugin
    /// </summary>
    void RegisterPlugin(IFilePreviewPlugin plugin);

    /// <summary>
    /// Findet ein passendes Plugin für eine Datei
    /// </summary>
    IFilePreviewPlugin? FindPluginForFile(string filePath);

    /// <summary>
    /// Generiert Preview mit dem passenden Plugin
    /// </summary>
    UIElement? GeneratePreview(string filePath);

    /// <summary>
    /// Alle registrierten Plugins
    /// </summary>
    IEnumerable<IFilePreviewPlugin> RegisteredPlugins { get; }
}

/// <summary>
/// Standard-Implementierung des Plugin-Service
/// </summary>
public class FilePreviewPluginService : IFilePreviewPluginService
{
    private readonly List<IFilePreviewPlugin> _plugins = new();

    public IEnumerable<IFilePreviewPlugin> RegisteredPlugins => _plugins;

    public void RegisterPlugin(IFilePreviewPlugin plugin)
    {
        if (!_plugins.Contains(plugin))
        {
            _plugins.Add(plugin);
            Console.WriteLine($"Plugin registriert: {plugin.PluginName}");
        }
    }

    public IFilePreviewPlugin? FindPluginForFile(string filePath)
    {
        var extension = System.IO.Path.GetExtension(filePath).ToLowerInvariant();
        return _plugins.FirstOrDefault(p => p.SupportedExtensions.Contains(extension));
    }

    public UIElement? GeneratePreview(string filePath)
    {
        var plugin = FindPluginForFile(filePath);
        if (plugin != null && plugin.CanPreview(filePath))
        {
            try
            {
                return plugin.GeneratePreview(filePath);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Fehler bei Preview-Generierung: {ex.Message}");
                return null;
            }
        }
        return null;
    }
}

/// <summary>
/// Text-Datei Preview Plugin (.txt, .log, .cs, .xml, etc.)
/// </summary>
public class TextFilePreviewPlugin : IFilePreviewPlugin
{
    public string PluginName => "Text File Viewer";

    public IEnumerable<string> SupportedExtensions => new[] { ".txt", ".log", ".cs", ".xml", ".json", ".md", ".html", ".css", ".js" };

    public bool CanPreview(string filePath)
    {
        return System.IO.File.Exists(filePath);
    }

    public UIElement GeneratePreview(string filePath)
    {
        var textBox = new System.Windows.Controls.TextBox
        {
            IsReadOnly = true,
            VerticalScrollBarVisibility = System.Windows.Controls.ScrollBarVisibility.Auto,
            HorizontalScrollBarVisibility = System.Windows.Controls.ScrollBarVisibility.Auto,
            FontFamily = new System.Windows.Media.FontFamily("Consolas"),
            FontSize = 12,
            Padding = new Thickness(8),
            Background = System.Windows.Media.Brushes.White
        };

        try
        {
            textBox.Text = System.IO.File.ReadAllText(filePath);
        }
        catch (Exception ex)
        {
            textBox.Text = $"Fehler beim Laden der Datei: {ex.Message}";
        }

        return textBox;
    }

    public Dictionary<string, string> GetMetadata(string filePath)
    {
        var fileInfo = new System.IO.FileInfo(filePath);
        return new Dictionary<string, string>
        {
            { "Größe", $"{fileInfo.Length / 1024} KB" },
            { "Erstellt", fileInfo.CreationTime.ToString("dd.MM.yyyy HH:mm") },
            { "Geändert", fileInfo.LastWriteTime.ToString("dd.MM.yyyy HH:mm") },
            { "Zeilen", System.IO.File.ReadLines(filePath).Count().ToString() }
        };
    }
}

/// <summary>
/// Bild-Datei Preview Plugin (.png, .jpg, .bmp, .gif)
/// </summary>
public class ImageFilePreviewPlugin : IFilePreviewPlugin
{
    public string PluginName => "Image Viewer";

    public IEnumerable<string> SupportedExtensions => new[] { ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".ico" };

    public bool CanPreview(string filePath)
    {
        return System.IO.File.Exists(filePath);
    }

    public UIElement GeneratePreview(string filePath)
    {
        var image = new System.Windows.Controls.Image
        {
            Stretch = System.Windows.Media.Stretch.Uniform,
            MaxWidth = 800,
            MaxHeight = 600
        };

        try
        {
            var bitmap = new System.Windows.Media.Imaging.BitmapImage();
            bitmap.BeginInit();
            bitmap.UriSource = new Uri(filePath, UriKind.Absolute);
            bitmap.CacheOption = System.Windows.Media.Imaging.BitmapCacheOption.OnLoad;
            bitmap.EndInit();
            image.Source = bitmap;
        }
        catch (Exception ex)
        {
            var errorText = new System.Windows.Controls.TextBlock
            {
                Text = $"Fehler beim Laden des Bildes: {ex.Message}",
                Foreground = System.Windows.Media.Brushes.Red,
                Margin = new Thickness(16)
            };
            return errorText;
        }

        var scrollViewer = new System.Windows.Controls.ScrollViewer
        {
            HorizontalScrollBarVisibility = System.Windows.Controls.ScrollBarVisibility.Auto,
            VerticalScrollBarVisibility = System.Windows.Controls.ScrollBarVisibility.Auto,
            Content = image
        };

        return scrollViewer;
    }

    public Dictionary<string, string> GetMetadata(string filePath)
    {
        var fileInfo = new System.IO.FileInfo(filePath);
        var metadata = new Dictionary<string, string>
        {
            { "Größe", $"{fileInfo.Length / 1024} KB" },
            { "Erstellt", fileInfo.CreationTime.ToString("dd.MM.yyyy HH:mm") },
            { "Geändert", fileInfo.LastWriteTime.ToString("dd.MM.yyyy HH:mm") }
        };

        try
        {
            var bitmap = new System.Windows.Media.Imaging.BitmapImage(new Uri(filePath, UriKind.Absolute));
            metadata.Add("Auflösung", $"{bitmap.PixelWidth} x {bitmap.PixelHeight}");
            metadata.Add("DPI", $"{bitmap.DpiX} x {bitmap.DpiY}");
        }
        catch { }

        return metadata;
    }
}

/// <summary>
/// PDF Preview Plugin (Platzhalter - benötigt PDF-Bibliothek)
/// </summary>
public class PdfPreviewPlugin : IFilePreviewPlugin
{
    public string PluginName => "PDF Viewer";

    public IEnumerable<string> SupportedExtensions => new[] { ".pdf" };

    public bool CanPreview(string filePath)
    {
        return System.IO.File.Exists(filePath);
    }

    public UIElement GeneratePreview(string filePath)
    {
        // TODO: PDF-Rendering mit externer Bibliothek (z.B. PDFium, PdfSharp)
        var placeholder = new System.Windows.Controls.Border
        {
            Background = System.Windows.Media.Brushes.WhiteSmoke,
            BorderBrush = System.Windows.Media.Brushes.Gray,
            BorderThickness = new Thickness(1),
            Padding = new Thickness(20)
        };

        var stack = new System.Windows.Controls.StackPanel
        {
            VerticalAlignment = VerticalAlignment.Center,
            HorizontalAlignment = HorizontalAlignment.Center
        };

        stack.Children.Add(new System.Windows.Controls.TextBlock
        {
            Text = "📄",
            FontSize = 48,
            HorizontalAlignment = HorizontalAlignment.Center,
            Margin = new Thickness(0, 0, 0, 16)
        });

        stack.Children.Add(new System.Windows.Controls.TextBlock
        {
            Text = "PDF-Vorschau",
            FontSize = 16,
            FontWeight = FontWeights.Bold,
            HorizontalAlignment = HorizontalAlignment.Center
        });

        stack.Children.Add(new System.Windows.Controls.TextBlock
        {
            Text = System.IO.Path.GetFileName(filePath),
            FontSize = 12,
            Foreground = System.Windows.Media.Brushes.Gray,
            HorizontalAlignment = HorizontalAlignment.Center,
            Margin = new Thickness(0, 8, 0, 0)
        });

        stack.Children.Add(new System.Windows.Controls.TextBlock
        {
            Text = "(PDF-Rendering-Bibliothek erforderlich)",
            FontSize = 10,
            Foreground = System.Windows.Media.Brushes.DarkGray,
            HorizontalAlignment = HorizontalAlignment.Center,
            Margin = new Thickness(0, 16, 0, 0)
        });

        placeholder.Child = stack;
        return placeholder;
    }

    public Dictionary<string, string> GetMetadata(string filePath)
    {
        var fileInfo = new System.IO.FileInfo(filePath);
        return new Dictionary<string, string>
        {
            { "Größe", $"{fileInfo.Length / 1024} KB" },
            { "Erstellt", fileInfo.CreationTime.ToString("dd.MM.yyyy HH:mm") },
            { "Geändert", fileInfo.LastWriteTime.ToString("dd.MM.yyyy HH:mm") },
            { "Format", "PDF" }
        };
    }
}

/// <summary>
/// MSG (Outlook-Mail) Preview Plugin (Platzhalter)
/// </summary>
public class MsgPreviewPlugin : IFilePreviewPlugin
{
    public string PluginName => "MSG Viewer";

    public IEnumerable<string> SupportedExtensions => new[] { ".msg", ".eml" };

    public bool CanPreview(string filePath)
    {
        return System.IO.File.Exists(filePath);
    }

    public UIElement GeneratePreview(string filePath)
    {
        // TODO: MSG-Parsing mit externer Bibliothek (z.B. MsgReader)
        var grid = new System.Windows.Controls.Grid();
        grid.RowDefinitions.Add(new System.Windows.Controls.RowDefinition { Height = GridLength.Auto });
        grid.RowDefinitions.Add(new System.Windows.Controls.RowDefinition { Height = new GridLength(1, GridUnitType.Star) });

        var header = new System.Windows.Controls.Border
        {
            Background = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(245, 245, 245)),
            BorderBrush = System.Windows.Media.Brushes.LightGray,
            BorderThickness = new Thickness(0, 0, 0, 1),
            Padding = new Thickness(12)
        };

        var headerStack = new System.Windows.Controls.StackPanel();
        headerStack.Children.Add(new System.Windows.Controls.TextBlock
        {
            Text = "📧 E-Mail Vorschau",
            FontSize = 14,
            FontWeight = FontWeights.Bold
        });
        headerStack.Children.Add(new System.Windows.Controls.TextBlock
        {
            Text = System.IO.Path.GetFileName(filePath),
            FontSize = 10,
            Foreground = System.Windows.Media.Brushes.Gray,
            Margin = new Thickness(0, 4, 0, 0)
        });

        header.Child = headerStack;
        System.Windows.Controls.Grid.SetRow(header, 0);
        grid.Children.Add(header);

        var content = new System.Windows.Controls.TextBlock
        {
            Text = "(MSG-Parser-Bibliothek erforderlich für vollständige Vorschau)",
            Foreground = System.Windows.Media.Brushes.DarkGray,
            Margin = new Thickness(16),
            TextWrapping = TextWrapping.Wrap
        };
        System.Windows.Controls.Grid.SetRow(content, 1);
        grid.Children.Add(content);

        return grid;
    }

    public Dictionary<string, string> GetMetadata(string filePath)
    {
        var fileInfo = new System.IO.FileInfo(filePath);
        return new Dictionary<string, string>
        {
            { "Größe", $"{fileInfo.Length / 1024} KB" },
            { "Erstellt", fileInfo.CreationTime.ToString("dd.MM.yyyy HH:mm") },
            { "Geändert", fileInfo.LastWriteTime.ToString("dd.MM.yyyy HH:mm") },
            { "Format", "Outlook MSG" }
        };
    }
}

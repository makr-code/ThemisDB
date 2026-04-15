/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ValueConverters.cs                                 ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:19:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   85.0/100                                       ║
    • Total Lines:     172                                            ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;
using Themis.AqlQueryBuilder.Models;

namespace Themis.AqlQueryBuilder;

/// <summary>
/// Converts CollectionType to icon emoji
/// </summary>
public class CollectionTypeToIconConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is CollectionType type)
        {
            return type switch
            {
                CollectionType.Relational => "📊",
                CollectionType.Graph => "🕸️",
                CollectionType.Vector => "🔢",
                CollectionType.Geo => "📍",
                CollectionType.Hybrid => "🔀",
                _ => "📄"
            };
        }
        return "📄";
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}

/// <summary>
/// Converts FieldDataType to color for visual indication
/// </summary>
public class FieldTypeToColorConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is FieldDataType type)
        {
            return type switch
            {
                FieldDataType.String => new SolidColorBrush(Color.FromRgb(0, 122, 204)),      // Blue
                FieldDataType.Integer => new SolidColorBrush(Color.FromRgb(139, 92, 246)),    // Purple
                FieldDataType.Float => new SolidColorBrush(Color.FromRgb(249, 115, 22)),      // Orange
                FieldDataType.Boolean => new SolidColorBrush(Color.FromRgb(34, 197, 94)),     // Green
                FieldDataType.Date => new SolidColorBrush(Color.FromRgb(236, 72, 153)),       // Pink
                FieldDataType.DateTime => new SolidColorBrush(Color.FromRgb(236, 72, 153)),   // Pink
                FieldDataType.Vector => new SolidColorBrush(Color.FromRgb(249, 115, 22)),     // Orange
                FieldDataType.GeoPoint => new SolidColorBrush(Color.FromRgb(16, 185, 129)),   // Green
                FieldDataType.GeoPolygon => new SolidColorBrush(Color.FromRgb(16, 185, 129)), // Green
                FieldDataType.GeoLineString => new SolidColorBrush(Color.FromRgb(16, 185, 129)), // Green
                _ => new SolidColorBrush(Colors.Gray)
            };
        }
        return new SolidColorBrush(Colors.Gray);
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}

/// <summary>
/// Converts ConnectionStatus to color for visual indication
/// </summary>
public class ConnectionStatusToColorConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is ConnectionStatus status)
        {
            return status switch
            {
                ConnectionStatus.Connected => new SolidColorBrush(Color.FromRgb(34, 197, 94)),      // Green
                ConnectionStatus.Connecting => new SolidColorBrush(Color.FromRgb(249, 115, 22)),    // Orange
                ConnectionStatus.Disconnected => new SolidColorBrush(Colors.Gray),                   // Gray
                ConnectionStatus.Error => new SolidColorBrush(Color.FromRgb(239, 68, 68)),          // Red
                _ => new SolidColorBrush(Colors.Gray)
            };
        }
        return new SolidColorBrush(Colors.Gray);
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}

/// <summary>
/// Converts ConnectionStatus to icon/text
/// </summary>
public class ConnectionStatusToIconConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is ConnectionStatus status)
        {
            return status switch
            {
                ConnectionStatus.Connected => "✅",
                ConnectionStatus.Connecting => "⏳",
                ConnectionStatus.Disconnected => "⭕",
                ConnectionStatus.Error => "❌",
                _ => "⭕"
            };
        }
        return "⭕";
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}

/// <summary>
/// Converts Enum to string for ComboBox binding
/// </summary>
public class EnumToStringConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value == null) return string.Empty;
        return value.ToString();
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is string str && !string.IsNullOrEmpty(str))
        {
            try
            {
                return Enum.Parse(targetType, str);
            }
            catch
            {
                return Enum.GetValues(targetType).GetValue(0);
            }
        }
        return Enum.GetValues(targetType).GetValue(0);
    }
}


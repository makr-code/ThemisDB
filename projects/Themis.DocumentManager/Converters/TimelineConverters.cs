/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineConverters.cs                              ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   73.0/100                                       ║
    • Total Lines:     147                                            ║
    • Open Issues:     TODOs: 1, Stubs: 6                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Converters;

/// <summary>
/// Converts TimelineScale enum to readable German string
/// </summary>
public class ScaleToStringConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is not TimelineScale scale) return "Unbekannt";

        return scale switch
        {
            TimelineScale.OneDay => "1 Tag",
            TimelineScale.ThreeDays => "3 Tage",
            TimelineScale.OneWeek => "1 Woche",
            TimelineScale.TwoWeeks => "2 Wochen",
            TimelineScale.OneMonth => "1 Monat",
            TimelineScale.ThreeMonths => "3 Monate",
            TimelineScale.SixMonths => "6 Monate",
            TimelineScale.OneYear => "1 Jahr",
            TimelineScale.FiveYears => "5 Jahre",
            _ => "Unbekannt"
        };
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}

/// <summary>
/// Converts DateTime to X position on canvas (requires TimelineRange context)
/// </summary>
public class DateToXConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is not DateTime date) return 0.0;
        
        // TODO: Get actual timeline range from DataContext
        // For now, simple calculation
        var baseDate = new DateTime(2025, 12, 1);
        var daysDiff = (date - baseDate).TotalDays;
        return daysDiff * 40; // 40px per day (adjust based on scale)
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}

/// <summary>
/// Converts swimlane index to Y position
/// </summary>
public class SwimlaneToYConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is not int index) return 0.0;
        return index * 60.0; // 60px per swimlane
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}

/// <summary>
/// Converts TimeSpan duration to width in pixels
/// </summary>
public class DurationToWidthConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is TimeSpan duration)
        {
            return duration.TotalDays * 40; // 40px per day
        }
        
        // Handle TimelineGanttBar duration calculation
        if (value is TimelineGanttBar bar)
        {
            var duration2 = bar.EndDate - bar.StartDate;
            return duration2.TotalDays * 40;
        }
        
        return 100.0;
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}

/// <summary>
/// Converts progress (0-1) to width percentage
/// </summary>
public class ProgressToWidthConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is not double progress) return 0.0;
        return progress * 100.0; // Returns percentage
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}

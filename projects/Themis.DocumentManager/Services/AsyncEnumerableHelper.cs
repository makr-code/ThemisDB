/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AsyncEnumerableHelper.cs                           ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     84                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Helper methods for converting IAsyncEnumerable to synchronous results
/// </summary>
public static class AsyncEnumerableHelper
{
    /// <summary>
    /// Converts IAsyncEnumerable to List synchronously (for compatibility)
    /// </summary>
    public static async Task<List<T>> ToListAsyncSafe<T>(IAsyncEnumerable<T>? enumerable, CancellationToken cancellationToken = default)
    {
        if (enumerable == null)
            return new List<T>();

        var list = new List<T>();
        await foreach (var item in enumerable.ConfigureAwait(false).WithCancellation(cancellationToken))
        {
            list.Add(item);
        }
        return list;
    }

    /// <summary>
    /// Gets first or default from IAsyncEnumerable (for compatibility)
    /// </summary>
    public static async Task<T?> FirstOrDefaultAsyncSafe<T>(IAsyncEnumerable<T>? enumerable, CancellationToken cancellationToken = default) where T : class
    {
        if (enumerable == null)
            return null;

        await foreach (var item in enumerable.ConfigureAwait(false).WithCancellation(cancellationToken))
        {
            return item;
        }
        return null;
    }

    /// <summary>
    /// Gets first or default from IAsyncEnumerable (for value types)
    /// </summary>
    public static async Task<T?> FirstOrDefaultAsyncSafe<T>(IAsyncEnumerable<T>? enumerable, T defaultValue, CancellationToken cancellationToken = default)
    {
        if (enumerable == null)
            return defaultValue;

        await foreach (var item in enumerable.ConfigureAwait(false).WithCancellation(cancellationToken))
        {
            return item;
        }
        return defaultValue;
    }
}

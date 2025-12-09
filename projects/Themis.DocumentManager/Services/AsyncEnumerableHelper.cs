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

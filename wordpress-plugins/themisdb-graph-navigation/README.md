# ThemisDB Graph Navigation

Standalone WordPress plugin for the interactive graph navigation overlay.

## Features

- Force-directed graph overlay navigation
- Uses WordPress content (posts, pages, categories, tags)
- Enqueued independently from the theme
- Automatic update support via ThemisDB updater

## Installation

1. Copy `themisdb-graph-navigation` into `wp-content/plugins/`.
2. Activate **ThemisDB Graph Navigation** in WordPress admin.
3. Ensure no duplicate graph script is enqueued by the active theme.

## Notes

- Provides backward-compatible function `themisdb_get_graph_data()`.
- Main data function is `themisdb_graph_navigation_get_graph_data()`.

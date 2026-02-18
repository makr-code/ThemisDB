# Persistent Podcast Player

A modern WordPress plugin that provides a persistent podcast player with episode thumbnails, progress bar, time display, and links to related posts.

## Features

- **Custom post type** `pod_episode` for managing podcast episodes
- **Thumbnail support** - Set featured images for episodes
- **REST API endpoint** for fetching episodes with thumbnail URLs
- **Modern sticky player bar** with gradient background
- **Progress bar** - Visual playback progress with seek functionality
- **Time display** - Shows current time and total duration (MM:SS or HH:MM:SS format)
- **Episode info** - Title and excerpt from related posts
- **"Zum Artikel" link** - Links to related WordPress posts
- **Enhanced playlist view** with:
  - Episode thumbnails (or placeholder icons)
  - Episode titles and descriptions
  - Play button overlays on hover
  - Grid layout with cards
  - Active episode highlighting
- **Responsive design** for mobile, tablet, and desktop
- **Optional localStorage persistence** for playback state

## Installation

1. Upload the `persistent-podcast-player` folder to the `/wp-content/plugins/` directory
2. Activate the plugin through the 'Plugins' menu in WordPress
3. Add podcast episodes through the 'Podcast Episodes' menu in the admin area

## Usage

### Adding Episodes

1. Go to "Podcast Episodes" in the WordPress admin
2. Click "Add New Episode"
3. Enter the episode title and description
4. Set a **featured image** (this will be used as the episode thumbnail)
5. Add custom fields:
   - `audio_url`: URL to the audio file (MP3, etc.)
   - `related_post_id`: ID of the related WordPress post (optional)
6. Publish the episode

### Custom Fields

- **audio_url**: The URL to the audio file for this episode
- **related_post_id**: The ID of a WordPress post that is related to this episode. If provided, the player will show the post's excerpt and a link to the post.

### Featured Images

Each episode can have a featured image that will be displayed in the playlist. The plugin automatically generates multiple sizes (thumbnail, medium, full) for optimal performance.

### REST API

The plugin provides a REST endpoint at `/wp-json/persistent-player/v1/episodes` that returns up to 50 published episodes with the following fields:

- `id`: Episode ID
- `title`: Episode title
- `audio`: Audio file URL
- `desc`: Episode description (stripped HTML)
- `excerpt`: Excerpt from related post (if available)
- `permalink`: Link to related post (if available)
- `thumbnail`: Object with URLs for different image sizes
  - `full`: Full-size image URL
  - `medium`: Medium-size image URL
  - `thumbnail`: Thumbnail-size image URL

## Player Features

### Main Player Bar
- **Sticky position**: Remains visible at the bottom while scrolling
- **Modern gradient design**: Purple gradient background with glassmorphism effects
- **Controls**: Previous, Play/Pause, Next buttons
- **Progress bar**: Visual representation of playback progress
  - Click anywhere on the bar to seek
  - Hover effect for better visibility
- **Time display**: Shows current time and total duration
- **Episode info**: Current episode title and related post excerpt
- **Link button**: "Zum Artikel" button linking to related post (hidden if no related post)
- **Playlist toggle**: Button to show/hide the playlist

### Playlist
- **Grid layout**: Responsive grid showing episode cards
- **Thumbnails**: Episode featured images or placeholder icons
- **Episode info**: Title and description preview
- **Play button overlay**: Appears on hover for quick playback
- **Active indicator**: Highlights the currently playing episode
- **Smooth animations**: Hover effects and transitions

### Playback Features
- **Auto-play**: Selecting an episode from the playlist starts playback automatically
- **Next episode**: Automatically plays the next episode when current one ends
- **Seek support**: Click on progress bar to jump to specific time
- **Keyboard-friendly**: All controls are keyboard accessible

## Technical Details

- **PHP**: Plugin uses WordPress hooks, REST API, and post thumbnails
- **JavaScript**: jQuery-based player with AJAX for fetching episodes
- **CSS**: Modern responsive styling with gradients, shadows, and animations
- **Persistence**: Optional localStorage for saving current episode and playback position

## Requirements

- WordPress 5.0 or higher
- PHP 7.4 or higher

## Customization

### Excerpt Length
You can customize the excerpt length using a filter:

```php
add_filter('ppp_excerpt_length', function() {
    return 50; // Number of words
});
```

### Styling
The player uses CSS custom properties that can be overridden in your theme:
- Modern gradient background
- Glassmorphism effects
- Smooth animations
- Responsive breakpoints

## License

MIT License - see LICENSE file for details

## Author

ThemisDB Team

# Persistent Podcast Player

A **state-of-the-art** WordPress plugin that provides a modern, feature-rich podcast player with all the features users expect in 2026.

## ✨ Key Features

### 🎵 Player Controls
- **Play/Pause/Previous/Next** - Full playback control
- **Skip buttons** - Skip backward 15s and forward 30s
- **Progress bar** - Visual playback progress with seek functionality (click to jump)
- **Time display** - Shows current time and total duration (MM:SS or HH:MM:SS format)
- **Volume control** - Volume slider with mute button
- **Playback speed** - Adjustable speed (0.5x, 0.75x, 1x, 1.25x, 1.5x, 1.75x, 2x)
- **Continuous play** - Toggle to auto-play next episode
- **Loading states** - Visual feedback during audio loading
- **Error handling** - User-friendly error messages with retry button
- **Buffer indicator** - Shows buffered audio progress

### ⌨️ Keyboard Shortcuts (Best Practice)
- **Space** - Play/Pause
- **Arrow Left** - Skip backward 15 seconds
- **Arrow Right** - Skip forward 30 seconds
- **Arrow Up** - Increase volume
- **Arrow Down** - Decrease volume
- **M** - Mute/Unmute
- **N** - Next episode
- **P** - Previous episode
- **1-9** - Seek to percentage (10%-90%)

### 🎨 Modern UI Design
- **Gradient background** - Beautiful purple gradient (#667eea → #764ba2)
- **Glassmorphism effects** - Modern backdrop-filter styling
- **Smooth animations** - All interactions have smooth transitions
- **Visual feedback** - Skip feedback overlay, hover states
- **Responsive design** - Optimized for desktop, tablet, and mobile
- **Accessibility** - Full ARIA labels and screen reader support

### 📝 Enhanced Playlist
- **Thumbnail support** - Episode featured images from WordPress media
- **Card-based design** - Modern grid layout with episode cards
- **Episode information** - Title, description, and thumbnail
- **Play button overlay** - Quick play on hover
- **Download buttons** - Download episodes directly
- **Active indicator** - Highlights currently playing episode
- **Search/Filter** - Easy episode navigation

### 💾 Smart Persistence
- **Volume preference** - Remembers your volume setting
- **Playback speed** - Saves your preferred speed
- **Continuous play setting** - Remembers auto-play preference
- **Current episode** - Resumes where you left off
- **Playback position** - Returns to last position in episode

### 🔗 WordPress Integration
- **Custom post type** - `pod_episode` for managing episodes
- **Featured images** - Full thumbnail support
- **REST API** - `/wp-json/persistent-player/v1/episodes` endpoint
- **Related posts** - Link episodes to blog posts with excerpts
- **Meta fields** - `audio_url` and `related_post_id` custom fields

## Installation

1. Upload the `persistent-podcast-player` folder to `/wp-content/plugins/`
2. Activate the plugin through the 'Plugins' menu in WordPress
3. Add podcast episodes through the 'Podcast Episodes' menu

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

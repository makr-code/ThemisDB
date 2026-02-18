# Persistent Podcast Player

A WordPress plugin that provides a persistent podcast player with episode excerpts and links to related posts.

## Features

- Custom post type `pod_episode` for managing podcast episodes
- REST API endpoint for fetching episodes
- Sticky player bar with play/pause/previous/next controls
- Display episode title, excerpt from related posts, and "Zum Artikel" link
- Playlist view with all available episodes
- Responsive design for mobile and desktop
- Optional localStorage persistence for playback state

## Installation

1. Upload the `persistent-podcast-player` folder to the `/wp-content/plugins/` directory
2. Activate the plugin through the 'Plugins' menu in WordPress
3. Add podcast episodes through the 'Podcast Episodes' menu in the admin area

## Usage

### Adding Episodes

1. Go to "Podcast Episodes" in the WordPress admin
2. Click "Add New Episode"
3. Enter the episode title and description
4. Add custom fields:
   - `audio_url`: URL to the audio file (MP3, etc.)
   - `related_post_id`: ID of the related WordPress post (optional)
5. Publish the episode

### Custom Fields

- **audio_url**: The URL to the audio file for this episode
- **related_post_id**: The ID of a WordPress post that is related to this episode. If provided, the player will show the post's excerpt and a link to the post.

### REST API

The plugin provides a REST endpoint at `/wp-json/persistent-player/v1/episodes` that returns up to 50 published episodes with the following fields:

- `id`: Episode ID
- `title`: Episode title
- `audio`: Audio file URL
- `desc`: Episode description (stripped HTML)
- `excerpt`: Excerpt from related post (if available)
- `permalink`: Link to related post (if available)

## Player Features

- **Sticky Bar**: The player appears at the bottom of the page and remains visible while scrolling
- **Episode Title**: Shows the currently playing episode title
- **Excerpt**: Shows an excerpt from the related WordPress post
- **Zum Artikel Link**: Links to the related WordPress post (hidden if no related post is set)
- **Playlist**: Toggle the playlist to see all available episodes and select one to play
- **Controls**: Play/pause, previous, and next buttons
- **Auto-play**: When selecting an episode from the playlist, it automatically starts playing

## Technical Details

- **PHP**: Plugin uses WordPress hooks and REST API
- **JavaScript**: jQuery-based player with AJAX for fetching episodes
- **CSS**: Responsive styling with mobile support
- **Persistence**: Optional localStorage for saving current episode and playback position

## Requirements

- WordPress 5.0 or higher
- PHP 7.4 or higher

## License

MIT License - see LICENSE file for details

## Author

ThemisDB Team

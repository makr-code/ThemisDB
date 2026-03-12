# ThemisDB WordPress Theme

A modern, professional WordPress theme designed specifically for ThemisDB - High-Performance Multi-Model Database with Native AI/LLM Integration.

## Description

ThemisDB is a sophisticated WordPress theme that combines clean design with powerful functionality. Built with best practices and modern web standards, it features the Themis brand colors and is optimized for technical content and database documentation.

### Key Features

- **Modern Design**: Clean, professional layout inspired by midnight-blogger theme
- **Featured Slider**: Showcase important posts with an animated slider (NEW!)
- **Custom Widgets**: 4 specialized widgets for enhanced content presentation (NEW!)
- **Themis Brand Colors**: 
  - Primary: #2c3e50 (dark blue-gray)
  - Secondary: #3498db (bright blue)
  - Accent: #7c4dff (purple)
  - Success: #27ae60 (green)
- **Fully Responsive**: Mobile-first approach with beautiful layouts on all devices
- **Customizable**: Theme Customizer support for colors and logo
- **SEO Optimized**: Clean, semantic HTML5 markup
- **Accessibility Ready**: WCAG 2.1 Level AA compliant
- **Translation Ready**: Full i18n support
- **Widget Areas**: Sidebar + 3 footer widget areas
- **Custom Templates**: Full-width page template included
- **Post Formats**: Support for standard, gallery, video, and more
- **Navigation Menus**: Primary and footer menu locations
- **Featured Images**: Beautiful post thumbnails
- **Code Highlighting**: Optimized styles for code blocks (perfect for technical content)

## New Widget Features 🎉

### ThemisDB: Featured Slider
- Displays sticky posts in an animated slider
- Autoplay with pause on hover
- Touch gestures for mobile devices
- Keyboard navigation support
- Perfect for highlighting important articles

### ThemisDB: Recent Posts
- Shows recent posts with optional thumbnails
- Customizable number of posts
- Clean, compact design

### ThemisDB: Category Highlights
- Showcase posts from specific categories
- Featured images and excerpts
- Great for organizing content by topic

### ThemisDB: Call to Action
- Eye-catching CTA boxes with gradient backgrounds
- 4 color styles (Primary, Secondary, Accent, Success)
- Perfect for downloads, links, or announcements

For detailed widget documentation, see `/wordpress-theme/WIDGETS_GUIDE.md`

## Installation

### Standard Installation

1. Download the theme files
2. Go to WordPress Admin > Appearance > Themes
3. Click "Add New" then "Upload Theme"
4. Choose the theme ZIP file and click "Install Now"
5. Activate the theme

### Manual Installation

1. Upload the `themisdb` folder to `/wp-content/themes/`
2. Go to WordPress Admin > Appearance > Themes
3. Activate the ThemisDB theme

## Theme Setup

### Recommended Plugins

- **Gutenberg/Block Editor**: Fully supported with custom color palette
- **Contact Form 7**: For contact forms
- **Yoast SEO**: Enhanced SEO features
- **WP Super Cache**: Performance optimization

### Initial Configuration

1. **Set up menus**: 
   - Go to Appearance > Menus
   - Create menus for "Primary Menu" and "Footer Menu"

2. **Configure widgets**:
   - Go to Appearance > Widgets
   - Add widgets to Sidebar and Footer areas

3. **Customize colors**:
   - Go to Appearance > Customize > Theme Colors
   - Adjust primary, secondary, and accent colors

4. **Add a logo**:
   - Go to Appearance > Customize > Site Identity
   - Upload your custom logo

5. **Set featured images**:
   - Add featured images to your posts for beautiful thumbnails

## Theme Structure

```
themisdb/
├── css/
│   ├── style.css              # Main stylesheet
│   └── widgets.css            # Widget styles (NEW!)
├── js/
│   ├── navigation.js          # Navigation and mobile menu scripts
│   ├── slider.js              # Featured slider functionality (NEW!)
│   └── enhancements.js        # Modern UI enhancements
├── inc/
│   └── widgets.php            # Custom widget classes (NEW!)
├── template-parts/
│   ├── content.php            # Default post template
│   ├── content-single.php     # Single post template
│   ├── content-page.php       # Page template
│   ├── content-search.php     # Search results template
│   └── content-none.php       # No results template
├── archive.php                # Archive page template
├── comments.php               # Comments template
├── footer.php                 # Footer template
├── functions.php              # Theme functions
├── header.php                 # Header template
├── index.php                  # Main template file
├── page.php                   # Page template
├── search.php                 # Search results page

Graph navigation was moved to the standalone plugin:
- ../themisdb-graph-navigation/
├── searchform.php             # Search form template
├── sidebar.php                # Sidebar template
├── single.php                 # Single post template
├── template-full-width.php    # Full-width page template
└── style.css                  # Theme stylesheet (with metadata)
```

## Customization

### Colors

The theme uses CSS custom properties (variables) for easy color customization:

```css
:root {
    --primary-color: #2c3e50;
    --secondary-color: #3498db;
    --accent-purple: #7c4dff;
    --success-color: #27ae60;
    /* ... more variables */
}
```

You can customize these through the WordPress Customizer or by creating a child theme.

### Child Theme

To create a child theme:

1. Create a new folder in `wp-content/themes/`, e.g., `themisdb-child`
2. Create a `style.css` file:

```css
/*
 Theme Name:   ThemisDB Child
 Template:     themisdb
*/
```

3. Create a `functions.php` file:

```php
<?php
function themisdb_child_enqueue_styles() {
    wp_enqueue_style( 'parent-style', get_template_directory_uri() . '/style.css' );
}
add_action( 'wp_enqueue_scripts', 'themisdb_child_enqueue_styles' );
```

## Browser Support

- Chrome (latest 2 versions)
- Firefox (latest 2 versions)
- Safari (latest 2 versions)
- Edge (latest 2 versions)
- Mobile Safari
- Chrome for Android

## Performance

The theme is optimized for performance:
- Minimal CSS and JavaScript
- No jQuery dependencies
- Optimized images support
- Clean, semantic HTML
- Efficient database queries

## Accessibility

ThemisDB theme follows WordPress accessibility standards:
- Keyboard navigation support
- Screen reader friendly
- ARIA landmarks
- Skip to content link
- Proper heading hierarchy
- Color contrast compliance

## Support

For support, please visit:
- **GitHub**: https://github.com/makr-code/wordpressPlugins
- **Documentation**: See repository docs folder

## Changelog

### Version 1.0.0
- Initial release
- Modern, responsive design
- Themis brand colors integration
- Full WordPress theme features
- Accessibility ready
- Translation ready

## Credits

- **Theme Author**: ThemisDB Team
- **Inspiration**: midnight-blogger WordPress theme
- **Framework**: WordPress Theme Standards
- **Icons**: Bootstrap Icons (embedded inline)

## License

This theme is licensed under the MIT License.

Copyright (c) 2024 ThemisDB Team

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

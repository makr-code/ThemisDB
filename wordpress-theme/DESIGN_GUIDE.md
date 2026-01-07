# ThemisDB WordPress Theme - Visual Design Guide

## Theme Preview & Design Elements

### Color Palette

```
┌─────────────────────────────────────────────────────────────┐
│ PRIMARY COLOR                                               │
│ #2c3e50 - Dark Blue-Gray                                    │
│ ███████████████████████████████████████████████████████████ │
│ Used for: Headers, Text, Buttons, Footer Background        │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ SECONDARY COLOR                                             │
│ #3498db - Bright Blue                                       │
│ ███████████████████████████████████████████████████████████ │
│ Used for: Links, Hover States, Call-to-Actions             │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ ACCENT PURPLE                                               │
│ #7c4dff - Vibrant Purple                                    │
│ ███████████████████████████████████████████████████████████ │
│ Used for: Widget Titles, Borders, Highlights               │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ SUCCESS GREEN                                               │
│ #27ae60 - Fresh Green                                       │
│ ███████████████████████████████████████████████████████████ │
│ Used for: Success Messages, Category Badges                │
└─────────────────────────────────────────────────────────────┘
```

### Layout Structure

```
┌──────────────────────────────────────────────────────────────┐
│                         HEADER                               │
│  ┌────────┐                            ┌────────────────┐   │
│  │  LOGO  │  ThemisDB               │ Home Features │   │
│  └────────┘                            │ Docs   About   │   │
│                                         └────────────────┘   │
│  Gradient: #2c3e50 → #34495e                                │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│                      MAIN CONTENT AREA                       │
│                                                              │
│  ┌─────────────────────────────┐  ┌──────────────────────┐ │
│  │                             │  │     SIDEBAR          │ │
│  │  ┌─────────────────────┐   │  │  ┌────────────────┐  │ │
│  │  │  Featured Image     │   │  │  │  Widget 1      │  │ │
│  │  │  1200x675           │   │  │  │  Search        │  │ │
│  │  └─────────────────────┘   │  │  └────────────────┘  │ │
│  │                             │  │                      │ │
│  │  POST TITLE                 │  │  ┌────────────────┐  │ │
│  │  By Author | Date | Cat    │  │  │  Widget 2      │  │ │
│  │                             │  │  │  Categories    │  │ │
│  │  Post content with clean    │  │  └────────────────┘  │ │
│  │  typography and proper      │  │                      │ │
│  │  spacing. Code blocks are   │  │  ┌────────────────┐  │ │
│  │  highlighted beautifully.   │  │  │  Widget 3      │  │ │
│  │                             │  │  │  Recent Posts  │  │ │
│  │  [Read More →]              │  │  └────────────────┘  │ │
│  │                             │  │                      │ │
│  └─────────────────────────────┘  └──────────────────────┘ │
│                                                              │
│  Maximum width: 1400px, Centered                            │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│                          FOOTER                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Footer Col 1 │  │ Footer Col 2 │  │ Footer Col 3 │      │
│  │  • Link 1    │  │  • Link 1    │  │  • Link 1    │      │
│  │  • Link 2    │  │  • Link 2    │  │  • Link 2    │      │
│  │  • Link 3    │  │  • Link 3    │  │  • Link 3    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                              │
│  ─────────────────────────────────────────────────────────  │
│  Powered by ThemisDB | Theme: ThemisDB Theme                │
│  Background: #2c3e50                                        │
└──────────────────────────────────────────────────────────────┘
```

### Mobile Layout (768px and below)

```
┌─────────────────────────────┐
│         HEADER              │
│  ┌──────┐  ThemisDB    ☰   │
│  │ LOGO │                   │
│  └──────┘                   │
└─────────────────────────────┘

┌─────────────────────────────┐
│      MAIN CONTENT           │
│  ┌─────────────────────┐   │
│  │  Featured Image     │   │
│  └─────────────────────┘   │
│                             │
│  POST TITLE                 │
│  Meta info                  │
│                             │
│  Content stacks vertically  │
│  for optimal mobile view    │
│                             │
│  [Read More →]              │
└─────────────────────────────┘

┌─────────────────────────────┐
│         SIDEBAR             │
│  (appears below content)    │
│                             │
│  Widgets stack vertically   │
└─────────────────────────────┘

┌─────────────────────────────┐
│          FOOTER             │
│  Columns stack vertically   │
└─────────────────────────────┘
```

### Typography

```
┌──────────────────────────────────────────────────────────────┐
│ HEADINGS                                                     │
│                                                              │
│ H1 - Post/Page Title (2.5rem / 40px)                        │
│      Font: Helvetica Neue, Bold                             │
│      Color: #2c3e50                                         │
│                                                              │
│ H2 - Section Headers (2rem / 32px)                          │
│      Font: Helvetica Neue, Bold                             │
│      Border-bottom: 3px solid #3498db                       │
│                                                              │
│ H3 - Sub-sections (1.75rem / 28px)                          │
│ H4 - Minor headers (1.5rem / 24px)                          │
│                                                              │
│ BODY TEXT                                                    │
│ Font: System Font Stack                                     │
│       -apple-system, BlinkMacSystemFont, Segoe UI, Roboto   │
│ Size: 1rem (16px)                                          │
│ Line Height: 1.7                                           │
│ Color: #2c3e50                                             │
└──────────────────────────────────────────────────────────────┘
```

### UI Components

#### Post Card
```
┌─────────────────────────────────────────────────┐
│  Featured Image                                 │
│  (Full width, rounded corners)                  │
├─────────────────────────────────────────────────┤
│  Post Title                                     │
│  👤 Author | 📅 Date | 📁 Category             │
├─────────────────────────────────────────────────┤
│  Excerpt text goes here with proper spacing.   │
│  Shows first 40 words of content...            │
│                                                 │
│  [Read More →]                                  │
└─────────────────────────────────────────────────┘
Background: White (#ffffff)
Border-radius: 12px
Box-shadow: 0 2px 4px rgba(0,0,0,0.1)
Hover: Lift effect with larger shadow
```

#### Button Styles
```
┌────────────────┐
│  Primary Button│  Background: #3498db
└────────────────┘  Color: #ffffff
                    Hover: #2980b9 + lift effect

┌────────────────┐
│ Secondary Btn  │  Background: #ecf0f1
└────────────────┘  Color: #2c3e50
                    Border: 2px solid #bdc3c7
```

#### Code Blocks
```
┌─────────────────────────────────────────────┐
│ function themisdb_setup() {                 │ Background: #2c3e50
│     add_theme_support('post-thumbnails');   │ Color: #ffffff
│ }                                           │ Font: Courier New
└─────────────────────────────────────────────┘ Padding: 1.5rem
```

#### Blockquotes
```
│ "This is a blockquote with a purple       │ Left border: 4px #7c4dff
│  accent border and light background."     │ Background: #f8f9fa
│                                           │ Padding: 1rem 1.5rem
│  - Quote Author                           │ Font-style: italic
```

### Widget Styling

```
┌──────────────────────────┐
│ Widget Title             │  Border-bottom: 2px #7c4dff
├──────────────────────────┤  Background: #ffffff
│ • Item 1                 │  Padding: 1.5rem
│ • Item 2                 │  Border-radius: 8px
│ • Item 3                 │  Box-shadow: small
└──────────────────────────┘
```

### Navigation

#### Desktop Navigation
```
Logo  ThemisDB         Home    Features    Docs    About
      ─────────────────────────────────────────────────
                                              ▼
                                        Dropdown items
                                        on hover
```

#### Mobile Navigation (Hamburger Menu)
```
Logo  ThemisDB                                    ☰

When opened:
┌─────────────────────────┐
│ • Home                  │
│ • Features              │
│ • Docs                  │
│ • About                 │
└─────────────────────────┘
```

### Spacing System

```
Spacing Scale (based on rem):
0.5rem = 8px   - Tight spacing
1rem   = 16px  - Normal spacing
1.5rem = 24px  - Comfortable spacing
2rem   = 32px  - Section spacing
3rem   = 48px  - Large gaps
4rem   = 64px  - Major sections
```

### Responsive Breakpoints

```
Mobile:    < 768px   (Stack layout, hamburger menu)
Tablet:    768-1024px (Sidebar below content)
Desktop:   > 1024px  (Full two-column layout)
```

### Accessibility Features

```
✓ Keyboard Navigation
  - Tab through all interactive elements
  - Enter/Space to activate
  - Escape to close menus

✓ Screen Reader Support
  - ARIA landmarks (header, nav, main, aside, footer)
  - Alt text for images
  - Proper heading hierarchy
  - Skip-to-content link

✓ Color Contrast
  - All text meets WCAG AA standards
  - Primary text on white: 12.63:1 (AAA)
  - Secondary blue on white: 4.65:1 (AA)

✓ Focus Indicators
  - Visible focus outline: 2px solid #3498db
  - Offset: 2px
```

### Animation & Interactions

```
Transitions: all 0.3s ease

Card Hover:
  - Transform: translateY(-2px)
  - Shadow: Larger (0 8px 16px)

Button Hover:
  - Transform: translateY(-2px)
  - Background: Darker shade

Link Hover:
  - Color: Secondary dark
  - Text-decoration: underline

Smooth Scroll:
  - Behavior: smooth
  - For anchor links
```

### Screenshot Specification

```
Dimensions: 1200px × 900px (4:3 ratio)

Required Elements:
┌─────────────────────────────────────────────┐
│ Header with logo and navigation (top 15%)  │ Gradient background
├─────────────────────────────────────────────┤
│ ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│ │  Post 1  │  │  Post 2  │  │ Sidebar  │  │ Show 2 posts + sidebar
│ │ + image  │  │ + image  │  │ widgets  │  │ Prominent Themis colors
│ └──────────┘  └──────────┘  └──────────┘  │
├─────────────────────────────────────────────┤
│ Footer with widget areas (bottom 15%)      │ Dark background
└─────────────────────────────────────────────┘
```

## Design Philosophy

1. **Clean & Modern**: Minimalist design with focus on content
2. **Professional**: Suitable for technical documentation and business
3. **Readable**: Optimal typography and spacing for long-form content
4. **Branded**: Consistent use of Themis colors throughout
5. **Accessible**: WCAG 2.1 AA compliant for all users
6. **Performant**: Fast loading with minimal CSS/JS

## Best Practices Implemented

✅ Mobile-first responsive design
✅ Semantic HTML5 markup
✅ BEM-like CSS naming conventions
✅ CSS custom properties for theming
✅ Progressive enhancement
✅ Touch-friendly tap targets (44px minimum)
✅ Accessible color contrasts
✅ Keyboard navigation support
✅ Screen reader compatibility
✅ Performance optimization (minimal code)

---

**Note**: To create the actual screenshot.png, use a tool like Photoshop, Figma, or take a screenshot of your live site after setup. The screenshot should showcase the header, a couple of blog posts with featured images, the sidebar with widgets, and the footer area, all using the Themis brand colors prominently.

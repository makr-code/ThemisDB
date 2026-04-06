"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            colors.py                                          ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:04:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     234                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Color-Blind Friendly Palette Module

Provides scientifically validated color palettes for vendor-neutral visualization
that are accessible to individuals with color vision deficiencies.

References:
- Paul Tol (2021). "Colour Schemes." Personal webpage.
- Okabe & Ito (2008). "Color Universal Design." J*FLY.
"""

from typing import List, Dict, Tuple
from dataclasses import dataclass


@dataclass
class ColorBlindPalette:
    """
    Color-blind friendly palettes for scientific visualization.
    
    Uses the Okabe-Ito and Paul Tol color schemes which are proven
    to be distinguishable for all forms of color blindness including
    deuteranopia, protanopia, and tritanopia.
    """
    
    # Okabe-Ito palette (optimized for color blindness)
    OKABE_ITO = {
        'blue': '#0072B2',
        'vermillion': '#D55E00',
        'bluish_green': '#009E73',
        'yellow': '#F0E442',
        'sky_blue': '#56B4E9',
        'reddish_purple': '#CC79A7',
        'orange': '#E69F00',
        'black': '#000000'
    }
    
    # Paul Tol's bright scheme
    TOL_BRIGHT = {
        'blue': '#4477AA',
        'cyan': '#66CCEE',
        'green': '#228833',
        'yellow': '#CCBB44',
        'red': '#EE6677',
        'purple': '#AA3377',
        'grey': '#BBBBBB'
    }
    
    # Paul Tol's muted scheme (softer, professional)
    TOL_MUTED = {
        'indigo': '#332288',
        'cyan': '#88CCEE',
        'teal': '#44AA99',
        'green': '#117733',
        'olive': '#999933',
        'sand': '#DDCC77',
        'rose': '#CC6677',
        'wine': '#882255',
        'purple': '#AA4499',
        'grey': '#DDDDDD'
    }
    
    @classmethod
    def get_palette(cls, name: str = 'okabe_ito') -> List[str]:
        """
        Get a color palette by name.
        
        Args:
            name: Palette name ('okabe_ito', 'tol_bright', 'tol_muted')
            
        Returns:
            List of hex color codes
        """
        palettes = {
            'okabe_ito': list(cls.OKABE_ITO.values()),
            'tol_bright': list(cls.TOL_BRIGHT.values()),
            'tol_muted': list(cls.TOL_MUTED.values())
        }
        return palettes.get(name.lower(), list(cls.OKABE_ITO.values()))
    
    @classmethod
    def get_sequential_palette(cls, n: int, palette: str = 'tol_muted') -> List[str]:
        """
        Get n colors from a palette, cycling if necessary.
        
        Args:
            n: Number of colors needed
            palette: Base palette name
            
        Returns:
            List of n hex color codes
        """
        base_colors = cls.get_palette(palette)
        colors = []
        for i in range(n):
            colors.append(base_colors[i % len(base_colors)])
        return colors
    
    @classmethod
    def get_diverging_palette(cls) -> Dict[str, str]:
        """
        Get a diverging color palette for comparison visualization.
        
        Returns:
            Dictionary with 'negative', 'neutral', 'positive' colors
        """
        return {
            'negative': '#D55E00',  # Vermillion (warm)
            'neutral': '#BBBBBB',   # Grey
            'positive': '#0072B2'   # Blue (cool)
        }
    
    @classmethod
    def get_matplotlib_colors(cls, n: int, palette: str = 'tol_muted') -> List[Tuple[float, float, float]]:
        """
        Get colors in matplotlib-compatible RGB format.
        
        Args:
            n: Number of colors needed
            palette: Base palette name
            
        Returns:
            List of RGB tuples (values 0-1)
        """
        hex_colors = cls.get_sequential_palette(n, palette)
        rgb_colors = []
        for hex_color in hex_colors:
            # Convert hex to RGB (0-1 scale)
            hex_color = hex_color.lstrip('#')
            r, g, b = tuple(int(hex_color[i:i+2], 16) / 255.0 for i in (0, 2, 4))
            rgb_colors.append((r, g, b))
        return rgb_colors


class VendorNeutralStyle:
    """
    Vendor-neutral styling guidelines for reports and visualizations.
    """
    
    @staticmethod
    def get_css_theme() -> str:
        """
        Get CSS theme for HTML reports with vendor-neutral styling.
        
        Returns:
            CSS string for embedding in HTML reports
        """
        return """
        :root {
            --color-primary: #332288;
            --color-secondary: #88CCEE;
            --color-success: #117733;
            --color-warning: #DDCC77;
            --color-danger: #CC6677;
            --color-info: #44AA99;
            --color-neutral: #DDDDDD;
            --color-text: #2C3E50;
            --color-background: #FFFFFF;
            --font-family: 'Segoe UI', 'Helvetica Neue', Arial, sans-serif;
        }
        
        body {
            font-family: var(--font-family);
            color: var(--color-text);
            background-color: var(--color-background);
            line-height: 1.6;
        }
        
        .neutrality-seal {
            background-color: #F8F9FA;
            border-left: 4px solid var(--color-info);
            padding: 1rem;
            margin: 1rem 0;
        }
        
        .statistical-note {
            background-color: #FFF9E6;
            border-left: 4px solid var(--color-warning);
            padding: 0.75rem;
            margin: 0.5rem 0;
            font-size: 0.9em;
        }
        
        .citation {
            font-size: 0.85em;
            color: #666;
            margin: 0.5rem 0;
        }
        """
    
    @staticmethod
    def get_chart_config() -> Dict:
        """
        Get chart configuration for consistent, vendor-neutral visualizations.
        
        Returns:
            Dictionary of chart configuration options
        """
        return {
            'font_family': 'sans-serif',
            'title_size': 14,
            'label_size': 11,
            'tick_size': 9,
            'grid_alpha': 0.3,
            'grid_color': '#CCCCCC',
            'line_width': 2,
            'marker_size': 6,
            'figure_dpi': 150,
            'background_color': '#FFFFFF',
            'axes_color': '#2C3E50'
        }

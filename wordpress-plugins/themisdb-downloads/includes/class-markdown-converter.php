<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-markdown-converter.php                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     338                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */


/**
 * Markdown Converter
 * Shared markdown to HTML conversion with Mermaid support
 */

if (!defined('ABSPATH')) {
    exit;
}

if (!class_exists('ThemisDB_Markdown_Converter')) {

class ThemisDB_Markdown_Converter {
    
    /**
     * Enhanced markdown to HTML conversion with Mermaid support
     * 
     * @param string $markdown Markdown text
     * @return string HTML
     */
    public static function convert($markdown) {
        // Escape HTML entities first for security
        $html = htmlspecialchars($markdown, ENT_QUOTES, 'UTF-8');
        
        // Process in the correct order to avoid conflicts
        
        // 1. Extract and protect code blocks (including Mermaid) before other processing
        $code_blocks = array();
        $html = preg_replace_callback('/```([a-zA-Z0-9+\-]*)\n(.*?)\n```/s', function($matches) use (&$code_blocks) {
            $language = trim($matches[1]);
            $code = $matches[2];
            $placeholder = '___CODE_BLOCK_' . count($code_blocks) . '___';
            
            // Check if it's a Mermaid diagram
            if ($language === 'mermaid') {
                $code_blocks[$placeholder] = '<div class="mermaid">' . "\n" . $code . "\n" . '</div>';
            } else {
                // Regular code block with optional language class
                $lang_class = $language ? ' class="language-' . esc_attr($language) . '"' : '';
                $code_blocks[$placeholder] = '<pre><code' . $lang_class . '>' . $code . '</code></pre>';
            }
            
            return $placeholder;
        }, $html);
        
        // 2. Extract and protect inline code
        $inline_codes = array();
        $html = preg_replace_callback('/`([^`]+)`/', function($matches) use (&$inline_codes) {
            $placeholder = '___INLINE_CODE_' . count($inline_codes) . '___';
            $inline_codes[$placeholder] = '<code>' . $matches[1] . '</code>';
            return $placeholder;
        }, $html);
        
        // 3. Horizontal rules (--- or ***)
        $html = preg_replace('/^(\*{3,}|-{3,})$/m', '<hr>', $html);
        
        // 4. Headers (must process from h6 to h1 to avoid conflicts)
        $html = preg_replace('/^###### (.+)$/m', '<h6>$1</h6>', $html);
        $html = preg_replace('/^##### (.+)$/m', '<h5>$1</h5>', $html);
        $html = preg_replace('/^#### (.+)$/m', '<h4>$1</h4>', $html);
        $html = preg_replace('/^### (.+)$/m', '<h3>$1</h3>', $html);
        $html = preg_replace('/^## (.+)$/m', '<h2>$1</h2>', $html);
        $html = preg_replace('/^# (.+)$/m', '<h1>$1</h1>', $html);
        
        // 5. Bold, italic, strikethrough (process in correct order)
        // Bold and italic combined (***text*** or ___text___)
        $html = preg_replace('/\*\*\*(.+?)\*\*\*/', '<strong><em>$1</em></strong>', $html);
        $html = preg_replace('/___(.+?)___/', '<strong><em>$1</em></strong>', $html);
        // Bold (**text** or __text__)
        $html = preg_replace('/\*\*(.+?)\*\*/', '<strong>$1</strong>', $html);
        $html = preg_replace('/__(.+?)__/', '<strong>$1</strong>', $html);
        // Italic (*text* or _text_)
        $html = preg_replace('/\*(.+?)\*/', '<em>$1</em>', $html);
        $html = preg_replace('/_([^_\s](?:.*?[^_\s])?)_/', '<em>$1</em>', $html);
        // Strikethrough (~~text~~)
        $html = preg_replace('/~~(.+?)~~/', '<del>$1</del>', $html);
        
        // 6. Links and images
        // Images: ![alt](url "title")
        $html = preg_replace_callback('/!\[([^\]]*)\]\(([^\s\)]+)(?:\s+"([^"]*)")?\)/', function($matches) {
            $alt = esc_attr($matches[1]);
            $url = esc_url($matches[2]);
            $title = isset($matches[3]) ? ' title="' . esc_attr($matches[3]) . '"' : '';
            return '<img src="' . $url . '" alt="' . $alt . '"' . $title . ' class="markdown-image">';
        }, $html);
        
        // Links: [text](url "title")
        $html = preg_replace_callback('/\[([^\]]+)\]\(([^\s\)]+)(?:\s+"([^"]*)")?\)/', function($matches) {
            $text = esc_html($matches[1]);
            $url = esc_url($matches[2]);
            $title = isset($matches[3]) ? ' title="' . esc_attr($matches[3]) . '"' : '';
            return '<a href="' . $url . '"' . $title . ' target="_blank" rel="noopener noreferrer">' . $text . '</a>';
        }, $html);
        
        // 7. Tables
        $html = self::parse_tables($html);
        
        // 8. Lists (must handle nested lists properly)
        $html = self::parse_lists($html);
        
        // 9. Blockquotes
        $html = self::parse_blockquotes($html);
        
        // 10. Restore code blocks
        foreach ($code_blocks as $placeholder => $code_html) {
            $html = str_replace($placeholder, $code_html, $html);
        }
        
        // 11. Restore inline code
        foreach ($inline_codes as $placeholder => $code_html) {
            $html = str_replace($placeholder, $code_html, $html);
        }
        
        // 12. Paragraphs - split by double newlines but avoid wrapping block elements
        $html = self::wrap_paragraphs($html);
        
        // 13. Line breaks - convert single newlines to <br> within paragraphs
        $html = preg_replace('/\n(?![<\n])/', '<br>', $html);
        
        // Clean up excessive whitespace
        $html = preg_replace('/\n{3,}/', "\n\n", $html);
        
        return $html;
    }
    
    /**
     * Parse markdown tables
     * 
     * @param string $html HTML content
     * @return string Processed HTML
     */
    private static function parse_tables($html) {
        // Match markdown tables
        $html = preg_replace_callback('/^(\|.+\|)\n(\|[\s\-\:]+\|)\n((?:\|.+\|\n?)+)/m', function($matches) {
            $header_line = trim($matches[1]);
            $separator_line = trim($matches[2]);
            $body_lines = trim($matches[3]);
            
            // Parse header
            $headers = array_map('trim', explode('|', trim($header_line, '|')));
            
            // Parse alignment from separator line
            $alignments = array();
            $separators = explode('|', trim($separator_line, '|'));
            foreach ($separators as $sep) {
                $sep = trim($sep);
                if (preg_match('/^:.*:$/', $sep)) {
                    $alignments[] = 'center';
                } elseif (preg_match('/^:/', $sep)) {
                    $alignments[] = 'left';
                } elseif (preg_match('/:$/', $sep)) {
                    $alignments[] = 'right';
                } else {
                    $alignments[] = '';
                }
            }
            
            // Build table HTML
            $table = '<table class="markdown-table">';
            
            // Header
            $table .= '<thead><tr>';
            foreach ($headers as $i => $header) {
                $align = isset($alignments[$i]) && $alignments[$i] ? ' style="text-align:' . $alignments[$i] . '"' : '';
                $table .= '<th' . $align . '>' . $header . '</th>';
            }
            $table .= '</tr></thead>';
            
            // Body
            $table .= '<tbody>';
            $body_rows = explode("\n", $body_lines);
            foreach ($body_rows as $row) {
                if (empty(trim($row))) continue;
                $cells = array_map('trim', explode('|', trim($row, '|')));
                $table .= '<tr>';
                foreach ($cells as $i => $cell) {
                    $align = isset($alignments[$i]) && $alignments[$i] ? ' style="text-align:' . $alignments[$i] . '"' : '';
                    $table .= '<td' . $align . '>' . $cell . '</td>';
                }
                $table .= '</tr>';
            }
            $table .= '</tbody>';
            
            $table .= '</table>';
            return $table;
        }, $html);
        
        return $html;
    }
    
    /**
     * Parse markdown lists (ordered and unordered, with nesting)
     * 
     * @param string $html HTML content
     * @return string Processed HTML
     */
    private static function parse_lists($html) {
        $lines = explode("\n", $html);
        $result = array();
        $list_stack = array(); // Stack to track nested lists
        
        foreach ($lines as $line) {
            // Check for list items (unordered: -, *, + | ordered: 1., 2., etc.)
            if (preg_match('/^(\s*)([\-\*\+]|\d+\.)\s+(.*)$/', $line, $matches)) {
                $indent = strlen($matches[1]);
                $marker = $matches[2];
                $content = $matches[3];
                $is_ordered = preg_match('/^\d+\.$/', $marker);
                $list_type = $is_ordered ? 'ol' : 'ul';
                
                // Determine nesting level (every 2-4 spaces is one level)
                $level = intval($indent / 2);
                
                // Close lists if we've decreased indentation
                while (count($list_stack) > $level + 1) {
                    $closed_type = array_pop($list_stack);
                    $result[] = '</' . $closed_type . '>';
                }
                
                // Open new list if needed
                if (count($list_stack) === $level) {
                    $result[] = '<' . $list_type . '>';
                    $list_stack[] = $list_type;
                } elseif (count($list_stack) > 0 && $list_stack[count($list_stack) - 1] !== $list_type && count($list_stack) === $level + 1) {
                    // Different list type at same level - close previous and open new
                    $closed_type = array_pop($list_stack);
                    $result[] = '</' . $closed_type . '>';
                    $result[] = '<' . $list_type . '>';
                    $list_stack[] = $list_type;
                }
                
                $result[] = '<li>' . $content . '</li>';
            } else {
                // Close all open lists
                while (!empty($list_stack)) {
                    $closed_type = array_pop($list_stack);
                    $result[] = '</' . $closed_type . '>';
                }
                $result[] = $line;
            }
        }
        
        // Close any remaining open lists
        while (!empty($list_stack)) {
            $closed_type = array_pop($list_stack);
            $result[] = '</' . $closed_type . '>';
        }
        
        return implode("\n", $result);
    }
    
    /**
     * Parse markdown blockquotes
     * 
     * @param string $html HTML content
     * @return string Processed HTML
     */
    private static function parse_blockquotes($html) {
        $lines = explode("\n", $html);
        $result = array();
        $in_blockquote = false;
        $blockquote_lines = array();
        
        foreach ($lines as $line) {
            if (preg_match('/^>\s?(.*)$/', $line, $matches)) {
                if (!$in_blockquote) {
                    $in_blockquote = true;
                }
                $blockquote_lines[] = $matches[1];
            } else {
                if ($in_blockquote) {
                    $result[] = '<blockquote>' . implode("\n", $blockquote_lines) . '</blockquote>';
                    $blockquote_lines = array();
                    $in_blockquote = false;
                }
                $result[] = $line;
            }
        }
        
        // Close blockquote if still open
        if ($in_blockquote) {
            $result[] = '<blockquote>' . implode("\n", $blockquote_lines) . '</blockquote>';
        }
        
        return implode("\n", $result);
    }
    
    /**
     * Wrap content in paragraphs, but skip block-level elements
     * 
     * @param string $html HTML content
     * @return string Processed HTML
     */
    private static function wrap_paragraphs($html) {
        // Split by double newlines
        $blocks = preg_split('/\n\n+/', $html);
        $result = array();
        
        foreach ($blocks as $block) {
            $block = trim($block);
            if (empty($block)) continue;
            
            // Check if block starts with a block-level element
            if (preg_match('/^<(h[1-6]|ul|ol|pre|blockquote|table|div|hr)[\s>]/', $block)) {
                $result[] = $block;
            } else {
                // Wrap in paragraph
                $result[] = '<p>' . $block . '</p>';
            }
        }
        
        return implode("\n\n", $result);
    }
}

}

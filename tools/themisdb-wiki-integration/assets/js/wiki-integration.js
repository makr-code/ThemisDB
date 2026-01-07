/**
 * ThemisDB Wiki Integration - JavaScript
 * Version: 1.0.0
 */

(function($) {
    'use strict';
    
    // Initialize when document is ready
    $(document).ready(function() {
        ThemisDBWikiIntegration.init();
    });
    
    /**
     * Main object
     */
    var ThemisDBWikiIntegration = {
        
        /**
         * Initialize
         */
        init: function() {
            this.initSmoothScroll();
            this.initCodeCopy();
            this.initTOCHighlight();
        },
        
        /**
         * Smooth scroll for TOC links
         */
        initSmoothScroll: function() {
            $('.themisdb-wiki-toc a').on('click', function(e) {
                var target = $(this).attr('href');
                
                if (target.indexOf('#') === 0) {
                    e.preventDefault();
                    
                    var $target = $(target);
                    
                    if ($target.length) {
                        $('html, body').animate({
                            scrollTop: $target.offset().top - 100
                        }, 500);
                    }
                }
            });
        },
        
        /**
         * Add copy button to code blocks
         */
        initCodeCopy: function() {
            $('.themisdb-wiki-content pre code').each(function() {
                var $code = $(this);
                var $pre = $code.parent();
                
                // Create copy button
                var $button = $('<button>')
                    .addClass('themisdb-copy-code')
                    .html('📋 Copy')
                    .css({
                        'position': 'absolute',
                        'top': '0.5rem',
                        'right': '0.5rem',
                        'padding': '0.25rem 0.5rem',
                        'font-size': '0.875rem',
                        'background': '#0969da',
                        'color': '#ffffff',
                        'border': 'none',
                        'border-radius': '3px',
                        'cursor': 'pointer',
                        'opacity': '0',
                        'transition': 'opacity 0.2s ease'
                    });
                
                // Make pre relative for absolute positioning
                $pre.css('position', 'relative');
                
                // Append button
                $pre.append($button);
                
                // Show button on hover
                $pre.hover(
                    function() {
                        $button.css('opacity', '1');
                    },
                    function() {
                        $button.css('opacity', '0');
                    }
                );
                
                // Copy on click
                $button.on('click', function(e) {
                    e.preventDefault();
                    
                    var text = $code.text();
                    
                    // Try modern Clipboard API first
                    if (navigator.clipboard && navigator.clipboard.writeText) {
                        navigator.clipboard.writeText(text).then(function() {
                            // Update button text
                            $button.html('✅ Copied!');
                            
                            setTimeout(function() {
                                $button.html('📋 Copy');
                            }, 2000);
                        }).catch(function() {
                            // Fallback to legacy method
                            fallbackCopy(text);
                        });
                    } else {
                        // Fallback for older browsers
                        fallbackCopy(text);
                    }
                    
                    function fallbackCopy(text) {
                        // Create temporary textarea
                        var $temp = $('<textarea>')
                            .val(text)
                            .css({
                                'position': 'absolute',
                                'left': '-9999px'
                            })
                            .appendTo('body');
                        
                        // Select and copy
                        $temp.select();
                        try {
                            document.execCommand('copy');
                            // Update button text
                            $button.html('✅ Copied!');
                        } catch (err) {
                            $button.html('❌ Failed');
                        }
                        $temp.remove();
                        
                        setTimeout(function() {
                            $button.html('📋 Copy');
                        }, 2000);
                    }
                });
            });
        },
        
        /**
         * Highlight current section in TOC
         */
        initTOCHighlight: function() {
            var $toc = $('.themisdb-wiki-toc');
            
            if (!$toc.length) {
                return;
            }
            
            var $links = $toc.find('a');
            var $sections = $('.themisdb-wiki-content h2, .themisdb-wiki-content h3');
            
            if (!$sections.length) {
                return;
            }
            
            // Highlight on scroll
            $(window).on('scroll', function() {
                var scrollPos = $(window).scrollTop() + 150;
                
                $sections.each(function() {
                    var $section = $(this);
                    var sectionTop = $section.offset().top;
                    var sectionBottom = sectionTop + $section.outerHeight();
                    var sectionId = '#' + $section.attr('id');
                    
                    if (scrollPos >= sectionTop && scrollPos < sectionBottom) {
                        $links.removeClass('active');
                        $links.filter('[href="' + sectionId + '"]').addClass('active');
                    }
                });
            });
            
            // Add CSS for active state
            $('<style>')
                .html('.themisdb-wiki-toc a.active { font-weight: bold; color: #0550ae; }')
                .appendTo('head');
        }
    };
    
})(jQuery);

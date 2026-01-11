/**
 * ThemisDB Formula Renderer JavaScript
 * Version: 1.0.0
 */

(function($) {
    'use strict';
    
    /**
     * Initialize formula rendering
     */
    function initFormulaRendering() {
        // Wait for KaTeX to be available
        if (typeof renderMathInElement === 'undefined') {
            console.warn('KaTeX auto-render not loaded yet, retrying...');
            setTimeout(initFormulaRendering, 100);
            return;
        }
        
        // Get settings from localized script
        var autoRender = typeof themisdbFormula !== 'undefined' && typeof themisdbFormula.autoRender !== 'undefined' 
            ? themisdbFormula.autoRender 
            : true;
        var inlineDelim = typeof themisdbFormula !== 'undefined' ? themisdbFormula.inlineDelimiter : '$';
        var blockDelim = typeof themisdbFormula !== 'undefined' ? themisdbFormula.blockDelimiter : '$$';
        
        if (!autoRender) {
            console.log('ThemisDB Formula Renderer: Auto-render is disabled');
            return;
        }
        
        // Configure delimiters
        var delimiters = [
            {left: blockDelim, right: blockDelim, display: true},
            {left: inlineDelim, right: inlineDelim, display: false}
        ];
        
        // Find all content containers
        var containers = document.querySelectorAll('.themisdb-formula-content, .themisdb-formula, .themisdb-formula-block, .themisdb-formula-inline');
        
        if (containers.length === 0) {
            // Fallback: render in main content area
            containers = document.querySelectorAll('.entry-content, .post-content, .page-content, article, .comment-content');
        }
        
        // Render formulas in each container
        containers.forEach(function(container) {
            try {
                renderMathInElement(container, {
                    delimiters: delimiters,
                    throwOnError: false,
                    errorColor: '#dc3232',
                    strict: false,
                    trust: false,
                    // Ignore code blocks and pre elements
                    ignoredTags: ['script', 'noscript', 'style', 'textarea', 'pre', 'code'],
                    // Ignore specific classes
                    ignoredClasses: ['no-formula', 'ignore-formula']
                });
                
                // Mark as rendered
                container.classList.add('themisdb-formula-rendered');
                
            } catch (error) {
                console.error('ThemisDB Formula Renderer: Error rendering formulas', error);
                showError(container, error.message);
            }
        });
        
        console.log('ThemisDB Formula Renderer: Initialized and rendered ' + containers.length + ' container(s)');
    }
    
    /**
     * Show error message
     * 
     * @param {HTMLElement} container The container element
     * @param {string} message The error message
     */
    function showError(container, message) {
        var errorDiv = document.createElement('div');
        errorDiv.className = 'themisdb-formula-error';
        errorDiv.innerHTML = '<strong>Formula Rendering Error:</strong> ' + escapeHtml(message);
        container.appendChild(errorDiv);
    }
    
    /**
     * Escape HTML special characters
     * 
     * @param {string} text The text to escape
     * @return {string} The escaped text
     */
    function escapeHtml(text) {
        var map = {
            '&': '&amp;',
            '<': '&lt;',
            '>': '&gt;',
            '"': '&quot;',
            "'": '&#039;'
        };
        return text.replace(/[&<>"']/g, function(m) { return map[m]; });
    }
    
    /**
     * Re-render formulas (useful for AJAX-loaded content)
     */
    window.themisdbRenderFormulas = function() {
        initFormulaRendering();
    };
    
    // Initialize when DOM is ready
    $(document).ready(function() {
        initFormulaRendering();
    });
    
    // Re-render on AJAX complete (for dynamic content)
    // Use a more intelligent approach with configurable timeout
    var ajaxCompleteTimeout = null;
    $(document).ajaxComplete(function() {
        // Clear any pending timeout
        if (ajaxCompleteTimeout) {
            clearTimeout(ajaxCompleteTimeout);
        }
        // Set new timeout with configurable delay
        var delay = typeof themisdbFormula !== 'undefined' && themisdbFormula.ajaxDelay 
            ? themisdbFormula.ajaxDelay 
            : 500;
        ajaxCompleteTimeout = setTimeout(initFormulaRendering, delay);
    });
    
    // Support for Gutenberg editor
    if (window.wp && window.wp.domReady) {
        wp.domReady(function() {
            initFormulaRendering();
        });
    }
    
    // Support for Classic Editor
    if (window.tinymce) {
        tinymce.on('AddEditor', function(event) {
            event.editor.on('init', function() {
                setTimeout(initFormulaRendering, 500);
            });
        });
    }
    
})(jQuery);

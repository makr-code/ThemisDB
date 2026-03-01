/**
 * ThemisDB Formula Renderer JavaScript
 * Version: 1.1.0
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
    
    /**
     * Copy-to-Clipboard functionality
     */
    function addCopyButtons() {
        document.querySelectorAll('.themisdb-formula-block').forEach(container => {
            // Skip if button already exists
            if (container.querySelector('.themisdb-copy-formula')) {
                return;
            }
            
            // Get LaTeX code
            const latex = container.dataset.latex || container.textContent.trim();
            
            // Create copy button
            const copyBtn = document.createElement('button');
            copyBtn.className = 'themisdb-copy-formula';
            copyBtn.setAttribute('aria-label', 'Copy LaTeX code');
            copyBtn.innerHTML = '📋 Copy';
            copyBtn.title = 'Copy LaTeX code to clipboard';
            
            copyBtn.onclick = function(e) {
                e.preventDefault();
                e.stopPropagation();
                
                // Copy to clipboard
                if (navigator.clipboard && navigator.clipboard.writeText) {
                    navigator.clipboard.writeText(latex).then(() => {
                        // Success feedback
                        copyBtn.innerHTML = '✅ Copied!';
                        copyBtn.classList.add('copied');
                        
                        setTimeout(() => {
                            copyBtn.innerHTML = '📋 Copy';
                            copyBtn.classList.remove('copied');
                        }, 2000);
                    }).catch(err => {
                        console.error('Copy failed:', err);
                        fallbackCopy(latex, copyBtn);
                    });
                } else {
                    fallbackCopy(latex, copyBtn);
                }
            };
            
            container.style.position = 'relative';
            container.appendChild(copyBtn);
        });
    }
    
    /**
     * Fallback copy method for older browsers
     */
    function fallbackCopy(text, button) {
        const textarea = document.createElement('textarea');
        textarea.value = text;
        textarea.style.position = 'fixed';
        textarea.style.opacity = '0';
        document.body.appendChild(textarea);
        textarea.select();
        
        try {
            document.execCommand('copy');
            button.innerHTML = '✅ Copied!';
            button.classList.add('copied');
            setTimeout(() => {
                button.innerHTML = '📋 Copy';
                button.classList.remove('copied');
            }, 2000);
        } catch (err) {
            console.error('Fallback copy failed:', err);
            button.innerHTML = '❌ Failed';
        }
        
        document.body.removeChild(textarea);
    }
    
    /**
     * Add MathML for screen readers
     */
    function addMathMLAccessibility() {
        document.querySelectorAll('.themisdb-formula-block, .themisdb-formula-inline').forEach(container => {
            const katexElement = container.querySelector('.katex');
            if (!katexElement || container.querySelector('.mathml-alternative')) {
                return;
            }
            
            // Get LaTeX source
            const latex = container.dataset.latex || container.textContent.trim();
            
            try {
                // Generate MathML using KaTeX
                const mathml = katex.renderToString(latex, {
                    output: 'mathml',
                    throwOnError: false
                });
                
                // Create hidden MathML container for screen readers
                const mathmlDiv = document.createElement('div');
                mathmlDiv.className = 'mathml-alternative sr-only';
                mathmlDiv.setAttribute('aria-label', 'Mathematical formula: ' + latex);
                mathmlDiv.innerHTML = mathml;
                
                container.appendChild(mathmlDiv);
                
                // Hide visual formula from screen readers
                katexElement.setAttribute('aria-hidden', 'true');
                
            } catch (err) {
                console.warn('MathML generation failed:', err);
            }
        });
    }
    
    // Add screen reader only class
    const style = document.createElement('style');
    style.textContent = `
        .sr-only {
            position: absolute;
            width: 1px;
            height: 1px;
            padding: 0;
            margin: -1px;
            overflow: hidden;
            clip: rect(0, 0, 0, 0);
            white-space: nowrap;
            border-width: 0;
        }
    `;
    document.head.appendChild(style);
    
    // Initialize copy buttons and MathML after formula rendering
    $(document).ready(function($) {
        // Use MutationObserver to detect when formulas are rendered
        function observeFormulaRendering() {
            const observer = new MutationObserver(function(mutations) {
                let hasFormulaChanges = false;
                mutations.forEach(function(mutation) {
                    mutation.addedNodes.forEach(function(node) {
                        if (node.nodeType === 1) { // Element node
                            if (node.classList && (node.classList.contains('katex') || 
                                node.classList.contains('themisdb-formula-rendered') ||
                                node.querySelector && node.querySelector('.katex'))) {
                                hasFormulaChanges = true;
                            }
                        }
                    });
                });
                
                if (hasFormulaChanges) {
                    // Small delay to ensure rendering is complete
                    setTimeout(function() {
                        addCopyButtons();
                        addMathMLAccessibility();
                    }, 100);
                }
            });
            
            // Observe the document body for formula additions
            observer.observe(document.body, {
                childList: true,
                subtree: true
            });
        }
        
        // Initial setup with fallback timeout
        setTimeout(function() {
            addCopyButtons();
            addMathMLAccessibility();
            observeFormulaRendering();
        }, 500);
        
        // Also add after AJAX content loads with observer
        $(document).ajaxComplete(function() {
            setTimeout(function() {
                addCopyButtons();
                addMathMLAccessibility();
            }, 500);
        });
    });
    
})(jQuery);

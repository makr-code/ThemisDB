/**
 * Markdown Editor with SimpleMDE
 * Handles the wiki markdown editing interface
 */

(function($) {
    'use strict';
    
    $(document).ready(function() {
        // Initialize SimpleMDE only on wiki edit pages
        if ($('#wiki-markdown-editor').length === 0) {
            return;
        }
        
        // Initialize SimpleMDE
        var simplemde = new SimpleMDE({
            element: document.getElementById('wiki-markdown-editor'),
            spellChecker: false,
            autosave: {
                enabled: true,
                uniqueId: 'themisdb-wiki-' + themisdbWiki.postId,
                delay: 1000
            },
            toolbar: [
                'bold', 'italic', 'strikethrough', '|',
                'heading-1', 'heading-2', 'heading-3', '|',
                'code', 'quote', 'unordered-list', 'ordered-list', '|',
                'link', 'image', 'table', 'horizontal-rule', '|',
                'preview', 'side-by-side', 'fullscreen', '|',
                {
                    name: 'wikilink',
                    action: function customFunction(editor){
                        var cm = editor.codemirror;
                        var selection = cm.getSelection();
                        var placeholderText = 'Page Name';
                        var text = selection || placeholderText;
                        var wikilink = '[[' + text + ']]';
                        cm.replaceSelection(wikilink);
                        
                        // If no selection, select the placeholder text for easy replacement
                        if (!selection) {
                            var cursor = cm.getCursor();
                            var startOffset = text.length + 2; // text length + ']]'
                            var endOffset = 2; // ']]'
                            cm.setSelection(
                                {line: cursor.line, ch: cursor.ch - startOffset},
                                {line: cursor.line, ch: cursor.ch - endOffset}
                            );
                        }
                    },
                    className: 'fa fa-link',
                    title: 'Insert [[WikiLink]]'
                },
                'guide'
            ],
            previewRender: function(plainText) {
                // Convert markdown with WikiLinks
                return convertMarkdownWithWikiLinks(plainText);
            },
            renderingConfig: {
                codeSyntaxHighlighting: true
            },
            placeholder: 'Write your wiki content in Markdown...\n\nUse [[Page Name]] for wiki links\nUse [[Page Name|Display Text]] for custom link text\nUse [[Page Name#Section]] to link to a section',
            lineWrapping: true,
            styleSelectedText: true
        });
        
        // Hide default WordPress editor
        $('#postdivrich').hide();
        
        // Sync SimpleMDE content to hidden textarea before submit
        $('form#post').on('submit', function() {
            var markdown = simplemde.value();
            $('#wiki-markdown-editor').val(markdown);
        });
        
        /**
         * Convert Markdown with WikiLinks to HTML (for preview)
         */
        function convertMarkdownWithWikiLinks(markdown) {
            // Parse WikiLinks first
            markdown = parseWikiLinks(markdown);
            
            // Use marked.js if available, otherwise basic conversion
            if (typeof marked !== 'undefined') {
                return marked.parse(markdown);
            }
            
            // Basic markdown conversion for preview
            return basicMarkdownConversion(markdown);
        }
        
        /**
         * Parse WikiLinks
         */
        function parseWikiLinks(markdown) {
            // [[Page Name]] → link
            // [[Page Name|Display Text]] → link with custom text
            // [[Page Name#Section]] → link with anchor
            
            var pattern = /\[\[([^\|\]]+)(?:\|([^\]]+))?\]\]/g;
            
            return markdown.replace(pattern, function(match, pageName, displayText) {
                var text = displayText || pageName;
                var slug = pageName.toLowerCase().replace(/\s+/g, '-');
                var url = '/wiki/' + slug + '/';
                
                return '<a href="' + url + '" class="wikilink">' + text + '</a>';
            });
        }
        
        /**
         * Basic Markdown Conversion (fallback)
         */
        function basicMarkdownConversion(markdown) {
            var html = markdown;
            
            // Headers
            html = html.replace(/^### (.+)$/gm, '<h3>$1</h3>');
            html = html.replace(/^## (.+)$/gm, '<h2>$1</h2>');
            html = html.replace(/^# (.+)$/gm, '<h1>$1</h1>');
            
            // Bold
            html = html.replace(/\*\*(.+?)\*\*/g, '<strong>$1</strong>');
            html = html.replace(/__(.+?)__/g, '<strong>$1</strong>');
            
            // Italic
            html = html.replace(/\*(.+?)\*/g, '<em>$1</em>');
            html = html.replace(/_(.+?)_/g, '<em>$1</em>');
            
            // Code inline
            html = html.replace(/`([^`]+)`/g, '<code>$1</code>');
            
            // Links
            html = html.replace(/\[([^\]]+)\]\(([^\)]+)\)/g, '<a href="$2">$1</a>');
            
            // Line breaks
            html = html.replace(/\n/g, '<br>');
            
            return html;
        }
        
        // GitHub sync button handler
        $('#sync-to-github').on('click', function(e) {
            e.preventDefault();
            
            var $button = $(this);
            var postId = themisdbWiki.postId;
            
            if (!postId) {
                alert('Please save the post first');
                return;
            }
            
            $button.prop('disabled', true).text('Syncing...');
            
            $.post(ajaxurl, {
                action: 'sync_wiki_to_github',
                post_id: postId,
                nonce: themisdbWiki.nonce
            }, function(response) {
                if (response.success) {
                    alert('✓ ' + response.data.message);
                } else {
                    alert('✗ ' + response.data.message);
                }
            }).fail(function() {
                alert('✗ Sync failed. Please try again.');
            }).always(function() {
                $button.prop('disabled', false).text('Push to GitHub');
            });
        });
        
        // Wiki link autocomplete (future enhancement)
        // Could fetch available wiki pages via AJAX and show suggestions
    });
    
})(jQuery);

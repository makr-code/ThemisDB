/**
 * Version Diff Viewer
 * Handles revision history and diff viewing
 */

(function($) {
    'use strict';
    
    $(document).ready(function() {
        // Setup revision history handlers
        setupRevisionHandlers();
    });
    
    /**
     * Setup Revision Handlers
     */
    function setupRevisionHandlers() {
        // View diff button
        $(document).on('click', '.view-diff', function(e) {
            e.preventDefault();
            
            var $button = $(this);
            var revisionId = $button.data('revision-id');
            var $row = $button.closest('tr');
            var $prevRow = $row.prev('tr');
            
            if ($prevRow.length === 0) {
                alert('No previous revision to compare');
                return;
            }
            
            var prevRevisionId = $prevRow.find('.view-diff').data('revision-id');
            
            showDiff(prevRevisionId, revisionId);
        });
        
        // Restore revision button
        $(document).on('click', '.restore-revision', function(e) {
            e.preventDefault();
            
            if (!confirm('Are you sure you want to restore this revision? Current content will be overwritten.')) {
                return;
            }
            
            var $button = $(this);
            var revisionId = $button.data('revision-id');
            var postId = themisdbWiki.postId;
            
            $button.prop('disabled', true).text('Restoring...');
            
            $.post(ajaxurl, {
                action: 'restore_wiki_revision',
                post_id: postId,
                revision_id: revisionId,
                nonce: themisdbWiki.nonce
            }, function(response) {
                if (response.success) {
                    alert('✓ ' + response.data.message);
                    location.reload();
                } else {
                    alert('✗ ' + response.data.message);
                    $button.prop('disabled', false).text('Restore');
                }
            }).fail(function() {
                alert('✗ Restore failed. Please try again.');
                $button.prop('disabled', false).text('Restore');
            });
        });
    }
    
    /**
     * Show Diff Between Two Revisions
     */
    function showDiff(oldRevisionId, newRevisionId) {
        var $diffViewer = $('#diff-viewer');
        
        if ($diffViewer.length === 0) {
            $diffViewer = $('<div id="diff-viewer" class="wiki-diff"></div>').insertAfter('.wiki-revision-history');
        }
        
        $diffViewer.html('<h4>Loading diff...</h4>').show();
        
        $.post(ajaxurl, {
            action: 'get_wiki_diff',
            old_id: oldRevisionId,
            new_id: newRevisionId,
            nonce: themisdbWiki.nonce
        }, function(response) {
            if (response.success) {
                renderDiff(response.data.old, response.data.new);
            } else {
                $diffViewer.html('<p class="error">Failed to load diff: ' + response.data.message + '</p>');
            }
        }).fail(function() {
            $diffViewer.html('<p class="error">Failed to load diff. Please try again.</p>');
        });
    }
    
    /**
     * Render Diff
     */
    function renderDiff(oldText, newText) {
        var $diffViewer = $('#diff-viewer');
        
        $diffViewer.html('<h4>Changes</h4><div class="diff-container"></div>');
        
        if (typeof Diff === 'undefined') {
            $diffViewer.find('.diff-container').html('<p class="error">Diff library not loaded</p>');
            return;
        }
        
        // Use jsdiff library
        var diff = Diff.diffLines(oldText || '', newText || '');
        var html = '';
        
        diff.forEach(function(part) {
            var color = part.added ? 'diff-added' : 
                       part.removed ? 'diff-removed' : 
                       'diff-unchanged';
            var prefix = part.added ? '+ ' : part.removed ? '- ' : '  ';
            
            var lines = part.value.split('\n');
            
            lines.forEach(function(line, index) {
                // Skip last empty line
                if (index === lines.length - 1 && line === '') {
                    return;
                }
                
                html += '<div class="diff-line ' + color + '">';
                html += '<span class="diff-prefix">' + escapeHtml(prefix) + '</span>';
                html += '<span class="diff-content">' + escapeHtml(line) + '</span>';
                html += '</div>';
            });
        });
        
        $diffViewer.find('.diff-container').html(html);
        
        // Add stats
        var stats = getDiffStats(diff);
        var statsHtml = '<div class="diff-stats">';
        statsHtml += '<span class="diff-stat diff-added-stat">+' + stats.added + '</span> ';
        statsHtml += '<span class="diff-stat diff-removed-stat">-' + stats.removed + '</span> ';
        statsHtml += '<span class="diff-stat diff-unchanged-stat">' + stats.unchanged + ' unchanged</span>';
        statsHtml += '</div>';
        
        $diffViewer.prepend(statsHtml);
    }
    
    /**
     * Get Diff Statistics
     */
    function getDiffStats(diff) {
        var stats = {
            added: 0,
            removed: 0,
            unchanged: 0
        };
        
        diff.forEach(function(part) {
            var lines = part.value.split('\n').length - 1;
            
            if (part.added) {
                stats.added += lines;
            } else if (part.removed) {
                stats.removed += lines;
            } else {
                stats.unchanged += lines;
            }
        });
        
        return stats;
    }
    
    /**
     * Escape HTML
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
     * Side-by-Side Diff View (future enhancement)
     */
    function renderSideBySideDiff(oldText, newText) {
        // TODO: Implement side-by-side diff view
        // This would show old and new versions side by side
    }
    
    /**
     * Inline Diff View with Word-Level Changes (future enhancement)
     */
    function renderInlineDiff(oldText, newText) {
        // TODO: Implement inline diff with word-level highlighting
        // This would highlight specific words/characters that changed
    }
    
})(jQuery);

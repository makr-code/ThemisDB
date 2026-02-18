(function($) {
    'use strict';
    
    let episodes = [];
    let currentIndex = 0;
    let audio = null;
    let playbackSpeed = 1;
    let volume = 1;
    let continuousPlay = true;
    
    // Initialize player on document ready
    $(document).ready(function() {
        audio = document.getElementById('ppp-audio');
        
        if (!audio) {
            console.error('Audio element not found');
            return;
        }
        
        // Fetch episodes from REST API
        fetchEpisodes();
        
        // Set up event listeners
        setupEventListeners();
        
        // Load preferences
        loadPreferences();
        
        // Setup keyboard shortcuts
        setupKeyboardShortcuts();
    });
    
    /**
     * Fetch episodes from REST API
     */
    function fetchEpisodes() {
        if (!pppData || !pppData.restUrl) {
            console.error('REST URL not available');
            return;
        }
        
        $.ajax({
            url: pppData.restUrl,
            method: 'GET',
            success: function(data) {
                episodes = data;
                
                // Load state from localStorage (optional)
                loadState();
                
                renderPlaylist();
                
                // Load episode based on saved state or first episode if available
                if (episodes.length > 0) {
                    updateEpisodeUI(currentIndex, false);
                }
            },
            error: function(xhr, status, error) {
                console.error('Failed to fetch episodes:', error);
            }
        });
    }
    
    /**
     * Render playlist
     */
    function renderPlaylist() {
        const playlistContainer = $('#ppp-playlist-items');
        playlistContainer.empty();
        
        if (episodes.length === 0) {
            playlistContainer.html('<div class="ppp-playlist-empty">No episodes available</div>');
            return;
        }
        
        episodes.forEach(function(episode, index) {
            const item = $('<div>')
                .addClass('ppp-playlist-item')
                .attr('data-index', index);
            
            // Add thumbnail if available
            if (episode.thumbnail && episode.thumbnail.thumbnail) {
                const thumbnail = $('<div>')
                    .addClass('ppp-playlist-thumbnail')
                    .css('background-image', 'url(' + episode.thumbnail.thumbnail + ')');
                item.append(thumbnail);
            } else {
                // Default thumbnail placeholder
                const placeholder = $('<div>')
                    .addClass('ppp-playlist-thumbnail ppp-playlist-thumbnail-placeholder')
                    .html('<span>&#127911;</span>'); // Microphone emoji
                item.append(placeholder);
            }
            
            // Episode info container
            const info = $('<div>').addClass('ppp-playlist-info');
            
            const title = $('<div>')
                .addClass('ppp-playlist-title')
                .text(episode.title);
            
            const desc = $('<div>')
                .addClass('ppp-playlist-desc')
                .text(episode.desc ? episode.desc.substring(0, 80) + '...' : '');
            
            info.append(title);
            if (desc.text()) {
                info.append(desc);
            }
            
            item.append(info);
            
            // Action buttons container
            const actions = $('<div>').addClass('ppp-playlist-actions');
            
            // Play button overlay
            const playBtn = $('<button>')
                .addClass('ppp-playlist-play-btn')
                .html('&#9654;')
                .attr('title', 'Play')
                .attr('aria-label', 'Play episode');
            
            actions.append(playBtn);
            
            // Download button
            if (episode.audio) {
                const downloadBtn = $('<button>')
                    .addClass('ppp-download-btn')
                    .html('&#8595;')
                    .attr('title', 'Download')
                    .attr('aria-label', 'Download episode');
                
                actions.append(downloadBtn);
            }
            
            item.append(actions);
            
            if (index === currentIndex) {
                item.addClass('ppp-active');
            }
            
            playlistContainer.append(item);
        });
    }
    
    /**
     * Set up event listeners
     */
    function setupEventListeners() {
        // Play/Pause button
        $('#ppp-play-pause').on('click', function() {
            if (audio.paused) {
                playAudio();
            } else {
                pauseAudio();
            }
        });
        
        // Previous button
        $('#ppp-prev').on('click', function() {
            playPrevious();
        });
        
        // Next button
        $('#ppp-next').on('click', function() {
            playNext();
        });
        
        // Skip backward button
        $('#ppp-skip-backward').on('click', function() {
            skipBackward(15);
        });
        
        // Skip forward button
        $('#ppp-skip-forward').on('click', function() {
            skipForward(30);
        });
        
        // Volume button
        $('#ppp-volume-btn').on('click', function() {
            toggleMute();
        });
        
        // Volume slider
        $('#ppp-volume-slider').on('input', function() {
            setVolume($(this).val() / 100);
        });
        
        // Speed button
        $('#ppp-speed-btn').on('click', function(e) {
            e.stopPropagation();
            $('#ppp-speed-menu').toggle();
        });
        
        // Speed options
        $('.ppp-speed-option').on('click', function() {
            const speed = parseFloat($(this).attr('data-speed'));
            setPlaybackSpeed(speed);
            $('#ppp-speed-menu').hide();
        });
        
        // Close speed menu when clicking outside
        $(document).on('click', function(e) {
            if (!$(e.target).closest('.ppp-speed-container').length) {
                $('#ppp-speed-menu').hide();
            }
        });
        
        // Continuous play toggle
        $('#ppp-continuous-play').on('change', function() {
            continuousPlay = $(this).is(':checked');
            savePreferences();
        });
        
        // Playlist toggle
        $('#ppp-toggle-playlist').on('click', function() {
            $('#ppp-playlist').toggle();
        });
        
        // Playlist item click (delegated event)
        $('#ppp-playlist-items').on('click', '.ppp-playlist-item, .ppp-playlist-play-btn', function(e) {
            e.stopPropagation();
            const item = $(this).hasClass('ppp-playlist-item') ? $(this) : $(this).closest('.ppp-playlist-item');
            const index = parseInt(item.attr('data-index'));
            selectEpisode(index);
        });
        
        // Download button
        $('#ppp-playlist-items').on('click', '.ppp-download-btn', function(e) {
            e.stopPropagation();
            const index = parseInt($(this).closest('.ppp-playlist-item').attr('data-index'));
            downloadEpisode(index);
        });
        
        // Progress bar click for seeking
        $('#ppp-progress-bar').on('click', function(e) {
            const bar = $(this);
            const clickX = e.pageX - bar.offset().left;
            const width = bar.width();
            const percentage = clickX / width;
            
            if (audio.duration) {
                audio.currentTime = audio.duration * percentage;
            }
        });
        
        // Progress bar keyboard navigation
        $('#ppp-progress-bar').on('keydown', function(e) {
            if (!audio.duration) return;
            
            const step = audio.duration * 0.05; // 5% steps
            
            switch(e.key) {
                case 'ArrowLeft':
                    e.preventDefault();
                    audio.currentTime = Math.max(0, audio.currentTime - step);
                    break;
                case 'ArrowRight':
                    e.preventDefault();
                    audio.currentTime = Math.min(audio.duration, audio.currentTime + step);
                    break;
            }
        });
        
        // Error retry button
        $('#ppp-error-retry').on('click', function() {
            hideError();
            updateEpisodeUI(currentIndex, true);
        });
        
        // Audio events
        $(audio).on('loadstart', function() {
            showLoading();
        });
        
        $(audio).on('canplay', function() {
            hideLoading();
        });
        
        $(audio).on('error', function() {
            hideLoading();
            showError('Failed to load audio. Please try again.');
        });
        
        $(audio).on('ended', function() {
            if (continuousPlay) {
                playNext();
            } else {
                pauseAudio();
            }
        });
        
        // Audio time update (for progress bar and time display)
        $(audio).on('timeupdate', function() {
            updateProgress();
            saveState();
        });
        
        // Audio metadata loaded (for duration)
        $(audio).on('loadedmetadata', function() {
            updateTotalTime();
        });
        
        // Update buffer progress
        $(audio).on('progress', function() {
            updateBuffer();
        });
    }
    
    /**
     * Update progress bar and current time
     */
    function updateProgress() {
        if (!audio.duration) return;
        
        const percentage = (audio.currentTime / audio.duration) * 100;
        $('#ppp-progress-fill').css('width', percentage + '%');
        
        $('#ppp-current-time').text(formatTime(audio.currentTime));
    }
    
    /**
     * Update total time display
     */
    function updateTotalTime() {
        if (audio.duration) {
            $('#ppp-total-time').text(formatTime(audio.duration));
        }
    }
    
    /**
     * Format time in MM:SS or HH:MM:SS
     */
    function formatTime(seconds) {
        if (isNaN(seconds)) return '0:00';
        
        seconds = Math.floor(seconds);
        
        const hours = Math.floor(seconds / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = Math.floor(seconds % 60);
        
        if (hours > 0) {
            return hours + ':' + pad(minutes) + ':' + pad(secs);
        }
        return minutes + ':' + pad(secs);
    }
    
    /**
     * Pad number with leading zero
     */
    function pad(num) {
        return num < 10 ? '0' + num : num;
    }
    
    /**
     * Setup keyboard shortcuts
     */
    function setupKeyboardShortcuts() {
        $(document).on('keydown', function(e) {
            // Don't trigger if user is typing in an input
            if ($(e.target).is('input, textarea')) return;
            
            switch(e.key) {
                case ' ':
                    e.preventDefault();
                    if (audio.paused) {
                        playAudio();
                    } else {
                        pauseAudio();
                    }
                    break;
                case 'ArrowLeft':
                    if (!$(e.target).is('#ppp-progress-bar')) {
                        e.preventDefault();
                        skipBackward(15);
                    }
                    break;
                case 'ArrowRight':
                    if (!$(e.target).is('#ppp-progress-bar')) {
                        e.preventDefault();
                        skipForward(30);
                    }
                    break;
                case 'ArrowUp':
                    e.preventDefault();
                    setVolume(Math.min(1, volume + 0.1));
                    $('#ppp-volume-slider').val(volume * 100);
                    break;
                case 'ArrowDown':
                    e.preventDefault();
                    setVolume(Math.max(0, volume - 0.1));
                    $('#ppp-volume-slider').val(volume * 100);
                    break;
                case 'm':
                case 'M':
                    e.preventDefault();
                    toggleMute();
                    break;
                case 'n':
                case 'N':
                    e.preventDefault();
                    playNext();
                    break;
                case 'p':
                case 'P':
                    e.preventDefault();
                    playPrevious();
                    break;
            }
            
            // Number keys 1-9 for seeking
            if (e.key >= '1' && e.key <= '9') {
                e.preventDefault();
                const percentage = parseInt(e.key) / 10;
                if (audio.duration) {
                    audio.currentTime = audio.duration * percentage;
                }
            }
        });
    }
    
    /**
     * Skip backward by seconds
     */
    function skipBackward(seconds) {
        if (audio.currentTime > 0) {
            audio.currentTime = Math.max(0, audio.currentTime - seconds);
            showSkipFeedback(-seconds);
        }
    }
    
    /**
     * Skip forward by seconds
     */
    function skipForward(seconds) {
        if (audio.duration && audio.currentTime < audio.duration) {
            audio.currentTime = Math.min(audio.duration, audio.currentTime + seconds);
            showSkipFeedback(seconds);
        }
    }
    
    /**
     * Show visual feedback for skip
     */
    function showSkipFeedback(seconds) {
        const feedback = $('<div>')
            .addClass('ppp-skip-feedback')
            .text((seconds > 0 ? '+' : '') + seconds + 's')
            .css({
                position: 'absolute',
                top: '50%',
                left: '50%',
                transform: 'translate(-50%, -50%)',
                background: 'rgba(255, 255, 255, 0.9)',
                color: '#667eea',
                padding: '20px 30px',
                borderRadius: '10px',
                fontSize: '24px',
                fontWeight: 'bold',
                zIndex: 10000,
                pointerEvents: 'none',
                boxShadow: '0 4px 20px rgba(0, 0, 0, 0.2)'
            });
        
        $('#ppp-player').append(feedback);
        
        setTimeout(function() {
            feedback.fadeOut(300, function() {
                $(this).remove();
            });
        }, 800);
    }
    
    /**
     * Set volume (0-1)
     */
    function setVolume(vol) {
        volume = Math.max(0, Math.min(1, vol));
        audio.volume = volume;
        
        updateVolumeUI();
        savePreferences();
    }
    
    /**
     * Toggle mute
     */
    function toggleMute() {
        if (audio.volume > 0) {
            audio.volume = 0;
            $('#ppp-volume-slider').val(0);
            $('#ppp-volume-btn').addClass('ppp-muted');
        } else {
            audio.volume = volume;
            $('#ppp-volume-slider').val(volume * 100);
            $('#ppp-volume-btn').removeClass('ppp-muted');
        }
        updateVolumeUI();
    }
    
    /**
     * Update volume UI
     */
    function updateVolumeUI() {
        if (audio.volume === 0) {
            $('#ppp-volume-btn').addClass('ppp-muted');
        } else {
            $('#ppp-volume-btn').removeClass('ppp-muted');
        }
    }
    
    /**
     * Set playback speed
     */
    function setPlaybackSpeed(speed) {
        playbackSpeed = speed;
        audio.playbackRate = speed;
        
        $('.ppp-speed-option').removeClass('ppp-speed-active');
        $('.ppp-speed-option[data-speed="' + speed + '"]').addClass('ppp-speed-active');
        $('#ppp-speed-label').text(speed + 'x');
        
        savePreferences();
    }
    
    /**
     * Download episode
     */
    function downloadEpisode(index) {
        const episode = episodes[index];
        if (!episode || !episode.audio) return;
        
        // Extract file extension from URL or use generic
        const url = episode.audio;
        const extension = url.substring(url.lastIndexOf('.')) || '.mp3';
        const filename = episode.title.replace(/[^a-z0-9]/gi, '_') + extension;
        
        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    }
    
    /**
     * Show loading indicator
     */
    function showLoading() {
        $('#ppp-loading').fadeIn(200);
    }
    
    /**
     * Hide loading indicator
     */
    function hideLoading() {
        $('#ppp-loading').fadeOut(200);
    }
    
    /**
     * Show error message
     */
    function showError(message) {
        $('#ppp-error-message').text(message);
        $('#ppp-error').fadeIn(200);
    }
    
    /**
     * Hide error message
     */
    function hideError() {
        $('#ppp-error').fadeOut(200);
    }
    
    /**
     * Update buffer progress
     */
    function updateBuffer() {
        if (!audio.duration) return;
        
        if (audio.buffered.length > 0) {
            const buffered = audio.buffered.end(audio.buffered.length - 1);
            const percentage = (buffered / audio.duration) * 100;
            $('#ppp-progress-buffer').css('width', percentage + '%');
        }
    }
    
    /**
     * Save preferences to localStorage
     */
    function savePreferences() {
        if (typeof(Storage) !== 'undefined') {
            try {
                localStorage.setItem('ppp_volume', volume);
                localStorage.setItem('ppp_speed', playbackSpeed);
                localStorage.setItem('ppp_continuous_play', continuousPlay);
            } catch (e) {
                // localStorage might be disabled
            }
        }
    }
    
    /**
     * Load preferences from localStorage
     */
    function loadPreferences() {
        if (typeof(Storage) !== 'undefined') {
            try {
                const savedVolume = localStorage.getItem('ppp_volume');
                const savedSpeed = localStorage.getItem('ppp_speed');
                const savedContinuous = localStorage.getItem('ppp_continuous_play');
                
                if (savedVolume !== null) {
                    setVolume(parseFloat(savedVolume));
                    $('#ppp-volume-slider').val(volume * 100);
                }
                
                if (savedSpeed !== null) {
                    setPlaybackSpeed(parseFloat(savedSpeed));
                }
                
                if (savedContinuous !== null) {
                    continuousPlay = savedContinuous === 'true' || savedContinuous === true;
                    $('#ppp-continuous-play').prop('checked', continuousPlay);
                }
            } catch (e) {
                // localStorage might be disabled
            }
        }
    }
    
    /**
     * Play audio
     */
    function playAudio() {
        if (!episodes[currentIndex] || !episodes[currentIndex].audio) {
            console.error('No audio URL available');
            return;
        }
        
        audio.play().then(function() {
            $('#ppp-play-pause').removeClass('ppp-btn-play').addClass('ppp-btn-pause');
        }).catch(function(error) {
            console.error('Failed to play audio:', error);
        });
    }
    
    /**
     * Pause audio
     */
    function pauseAudio() {
        audio.pause();
        $('#ppp-play-pause').removeClass('ppp-btn-pause').addClass('ppp-btn-play');
    }
    
    /**
     * Play previous episode
     */
    function playPrevious() {
        if (currentIndex > 0) {
            selectEpisode(currentIndex - 1);
        }
    }
    
    /**
     * Play next episode
     */
    function playNext() {
        if (currentIndex < episodes.length - 1) {
            selectEpisode(currentIndex + 1);
        } else {
            // Reached end of playlist
            pauseAudio();
        }
    }
    
    /**
     * Select and play an episode
     */
    function selectEpisode(index) {
        if (index < 0 || index >= episodes.length) {
            return;
        }
        
        currentIndex = index;
        updateEpisodeUI(index, true);
        renderPlaylist(); // Update active state
    }
    
    /**
     * Update episode UI
     */
    function updateEpisodeUI(index, autoplay) {
        const episode = episodes[index];
        
        if (!episode) {
            return;
        }
        
        // Update title
        $('#ppp-title').text(episode.title);
        
        // Update excerpt
        if (episode.excerpt) {
            $('#ppp-excerpt').text(episode.excerpt).show();
        } else {
            $('#ppp-excerpt').text('').hide();
        }
        
        // Update link
        const link = $('#ppp-link');
        if (episode.permalink) {
            link.attr('href', episode.permalink).show();
        } else {
            link.attr('href', '#').hide();
        }
        
        // Load audio
        if (episode.audio) {
            audio.src = episode.audio;
            audio.load();
            
            // Restore saved time for this episode (only once)
            if (!autoplay && typeof(Storage) !== 'undefined') {
                try {
                    const savedTime = localStorage.getItem('ppp_current_time');
                    if (savedTime !== null && !isNaN(savedTime)) {
                        $(audio).one('loadedmetadata', function() {
                            audio.currentTime = parseFloat(savedTime);
                        });
                    }
                } catch (e) {
                    // localStorage might be disabled
                }
            }
            
            if (autoplay) {
                playAudio();
            }
        }
    }
    
    /**
     * Save state to localStorage
     */
    function saveState() {
        if (typeof(Storage) !== 'undefined') {
            try {
                localStorage.setItem('ppp_current_index', currentIndex);
                localStorage.setItem('ppp_current_time', audio.currentTime);
            } catch (e) {
                // localStorage might be disabled
            }
        }
    }
    
    /**
     * Load state from localStorage
     */
    function loadState() {
        if (typeof(Storage) !== 'undefined') {
            try {
                const savedIndex = localStorage.getItem('ppp_current_index');
                
                if (savedIndex !== null) {
                    const index = parseInt(savedIndex);
                    if (index >= 0 && index < episodes.length) {
                        currentIndex = index;
                    }
                }
            } catch (e) {
                // localStorage might be disabled
            }
        }
    }
    
})(jQuery);

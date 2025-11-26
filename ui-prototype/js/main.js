// Main Application Initialization

// Initialize the app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    // OBS Polyemesis UI Prototype loaded

    // Initial render
    renderProfiles();

    // Set up connection status right-click
    const connectionHeader = document.getElementById('connectionHeader');
    if (connectionHeader) {
        connectionHeader.addEventListener('contextmenu', (e) => {
            e.preventDefault();
            contextMenu.show(e.pageX, e.pageY, contextMenuItems.connection(), null, 'connection');
        });
    }

    // Test connection button
    document.getElementById('testConnectionBtn').addEventListener('click', () => {
        const indicator = document.getElementById('connectionIndicator');
        const text = document.getElementById('connectionText');

        indicator.className = 'status-indicator starting';
        text.textContent = 'Testing...';

        setTimeout(() => {
            indicator.className = 'status-indicator active';
            text.textContent = 'restreamer.example.com:8080';
        }, 1500);
    });

    // Simulate real-time updates
    setInterval(() => {
        // Update active stream durations
        mockProfiles.forEach(profile => {
            profile.destinations.forEach(dest => {
                if (dest.status === 'active') {
                    dest.duration++;
                    dest.totalFrames += dest.fps;

                    // Simulate bitrate fluctuation
                    dest.currentBitrate = dest.bitrate * (0.9 + Math.random() * 0.1);

                    // Randomly drop frames
                    if (Math.random() < 0.001) {
                        dest.droppedFrames++;
                    }
                }
            });
        });

        // Update process uptimes
        mockProcesses.forEach(proc => {
            if (proc.state === 'running' || proc.state === 'starting') {
                proc.uptime++;

                // Simulate CPU/memory fluctuation
                proc.cpu = Math.max(5, Math.min(50, proc.cpu + (Math.random() - 0.5) * 5));
                proc.memory = Math.max(128, Math.min(1024, proc.memory + (Math.random() - 0.5) * 20));
            }
        });

        // Update session durations
        mockSessions.forEach(sess => {
            sess.duration++;
            sess.bytesSent += 1024 * 1024 * (0.1 + Math.random() * 0.2); // ~100-300 KB/s
        });

        // Re-render if any profiles are expanded (to show updated stats)
        const expandedProfiles = document.querySelectorAll('.profile-content.expanded');
        if (expandedProfiles.length > 0) {
            renderProfiles();
            // Restore expanded state
            expandedProfiles.forEach(expanded => {
                const profileId = expanded.closest('.profile-widget').getAttribute('data-profile-id');
                const widget = document.querySelector(`[data-profile-id="${profileId}"]`);
                if (widget) {
                    widget.querySelector('.profile-content').classList.add('expanded');
                    widget.querySelector('.profile-header').classList.add('expanded');
                }
            });
        }
    }, 1000);

    // Add keyboard shortcuts
    document.addEventListener('keydown', (e) => {
        // Ctrl/Cmd + S to start all profiles
        if ((e.ctrlKey || e.metaKey) && e.key === 's') {
            e.preventDefault();
            document.getElementById('startAllBtn').click();
        }

        // Ctrl/Cmd + Q to stop all profiles
        if ((e.ctrlKey || e.metaKey) && e.key === 'q') {
            e.preventDefault();
            document.getElementById('stopAllBtn').click();
        }

        // Ctrl/Cmd + N to create new profile
        if ((e.ctrlKey || e.metaKey) && e.key === 'n') {
            e.preventDefault();
            document.getElementById('newProfileBtn').click();
        }

        // Ctrl/Cmd + M to open monitoring
        if ((e.ctrlKey || e.metaKey) && e.key === 'm') {
            e.preventDefault();
            document.getElementById('monitoringBtn').click();
        }
    });

    // Add tooltip support for buttons with title attributes
    const buttons = document.querySelectorAll('button[title]');
    buttons.forEach(btn => {
        btn.addEventListener('mouseenter', (e) => {
            // Could implement tooltip here if desired
        });
    });

    // Initialization complete
    // Keyboard shortcuts available:
    //   Ctrl/Cmd + S: Start all profiles
    //   Ctrl/Cmd + Q: Stop all profiles
    //   Ctrl/Cmd + N: New profile
    //   Ctrl/Cmd + M: Open monitoring
    //   Esc: Close modals/menus
});

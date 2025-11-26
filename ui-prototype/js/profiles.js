// Profile Rendering and Management

function renderProfiles() {
    const container = document.getElementById('profilesContainer');

    if (mockProfiles.length === 0) {
        container.innerHTML = `
            <div class="no-profiles">
                <div class="no-profiles-icon">📺</div>
                <div class="no-profiles-title">No Streaming Profiles</div>
                <div class="no-profiles-text">
                    Create your first profile to start multistreaming to multiple platforms
                </div>
                <button class="btn btn-primary" onclick="openProfileEditModal(null)">
                    <span class="icon">+</span> Create Profile
                </button>
            </div>
        `;
        return;
    }

    container.innerHTML = '';

    mockProfiles.forEach(profile => {
        const profileWidget = createProfileWidget(profile);
        container.appendChild(profileWidget);
    });
}

function createProfileWidget(profile) {
    const widget = document.createElement('div');
    widget.className = 'profile-widget';
    widget.setAttribute('data-profile-id', profile.id);

    // Determine profile aggregate status
    let aggregateStatus = profile.status;
    if (profile.status === 'active') {
        const hasError = profile.destinations.some(d => d.status === 'error');
        const hasStarting = profile.destinations.some(d => d.status === 'starting');
        if (hasError) aggregateStatus = 'error';
        else if (hasStarting) aggregateStatus = 'starting';
    }

    // Create summary text
    const activeCount = profile.destinations.filter(d => d.status === 'active').length;
    const errorCount = profile.destinations.filter(d => d.status === 'error').length;
    const totalCount = profile.destinations.length;

    let summaryText = '';
    if (profile.status === 'inactive') {
        summaryText = `${totalCount} destination${totalCount !== 1 ? 's' : ''}`;
    } else if (profile.status === 'starting') {
        summaryText = `Starting ${totalCount} destination${totalCount !== 1 ? 's' : ''}...`;
    } else {
        const parts = [];
        if (activeCount > 0) parts.push(`${activeCount} active`);
        if (errorCount > 0) parts.push(`${errorCount} error${errorCount !== 1 ? 's' : ''}`);
        summaryText = parts.join(', ') || `${totalCount} destinations`;
    }

    // Profile header
    const header = document.createElement('div');
    header.className = 'profile-header';
    header.innerHTML = `
        <span class="profile-status-indicator ${aggregateStatus}">${getStatusIcon(aggregateStatus)}</span>
        <div class="profile-info">
            <div class="profile-name">${profile.name}</div>
            <div class="profile-summary">${summaryText}</div>
        </div>
        <div class="profile-header-actions">
            ${profile.status === 'active' || profile.status === 'starting'
                ? '<button class="profile-btn stop" onclick="stopProfile(\'' + profile.id + '\', event)">■ Stop</button>'
                : '<button class="profile-btn start" onclick="startProfile(\'' + profile.id + '\', event)">▶ Start</button>'
            }
            <button class="profile-btn" onclick="openProfileEditModal(mockProfiles.find(p => p.id === \'' + profile.id + '\'))">Edit</button>
            <button class="profile-btn menu" onclick="showProfileContextMenu(event, \'' + profile.id + '\')">⋮</button>
        </div>
    `;

    // Toggle expansion on header click (but not on buttons)
    header.addEventListener('click', (e) => {
        if (!e.target.closest('button')) {
            toggleProfileExpansion(profile.id);
        }
    });

    // Right-click context menu on header
    header.addEventListener('contextmenu', (e) => {
        e.preventDefault();
        showProfileContextMenu(e, profile.id);
    });

    widget.appendChild(header);

    // Profile content (destinations)
    const content = document.createElement('div');
    content.className = 'profile-content';

    const destList = document.createElement('div');
    destList.className = 'destinations-list';

    profile.destinations.forEach(dest => {
        const destRow = createDestinationRow(dest, profile);
        destList.appendChild(destRow);
    });

    content.appendChild(destList);
    widget.appendChild(content);

    return widget;
}

function createDestinationRow(dest, profile) {
    const row = document.createElement('div');
    row.className = 'destination-row';
    row.setAttribute('data-destination-id', dest.id);

    // Build stats HTML
    let statsHTML = '';
    if (dest.status === 'active') {
        const droppedPercent = calculateDroppedPercent(dest.droppedFrames, dest.totalFrames);
        const droppedClass = droppedPercent > 5 ? 'error' : droppedPercent > 1 ? 'warning' : 'success';

        statsHTML = `
            <div class="destination-stats">
                <span class="stat-item success">↑ ${formatBitrate(dest.currentBitrate)}</span>
                <span class="stat-item ${droppedClass}">${dest.droppedFrames} dropped (${droppedPercent}%)</span>
                <span class="stat-item">${formatDuration(dest.duration)}</span>
            </div>
        `;
    } else if (dest.status === 'starting') {
        statsHTML = `
            <div class="destination-stats">
                <span class="stat-item warning">Connecting...</span>
            </div>
        `;
    } else if (dest.status === 'error') {
        statsHTML = `
            <div class="destination-stats">
                <span class="stat-item error">${dest.error}</span>
            </div>
        `;
    }

    row.innerHTML = `
        <span class="destination-status ${dest.status}">${getStatusIcon(dest.status)}</span>
        <div class="destination-info">
            <div class="destination-name">${dest.service}</div>
            <div class="destination-details">
                <span class="destination-detail">${dest.resolution}</span>
                <span class="destination-detail">${formatBitrate(dest.bitrate)}</span>
                ${dest.fps > 0 ? '<span class="destination-detail">' + dest.fps + ' FPS</span>' : ''}
            </div>
        </div>
        ${statsHTML}
        <div class="destination-actions">
            ${dest.status === 'active'
                ? '<button class="destination-btn stop" onclick="stopDestination(\'' + profile.id + '\', \'' + dest.id + '\', event)">■</button>'
                : '<button class="destination-btn start" onclick="startDestination(\'' + profile.id + '\', \'' + dest.id + '\', event)">▶</button>'
            }
            <button class="destination-btn" onclick="editDestination(\'' + dest.id + '\')">⚙️</button>
        </div>
    `;

    // Right-click context menu
    row.addEventListener('contextmenu', (e) => {
        e.preventDefault();
        showDestinationContextMenu(e, profile.id, dest.id);
    });

    // Double-click to expand details
    row.addEventListener('dblclick', () => {
        toggleDestinationDetails(row, dest);
    });

    return row;
}

function toggleDestinationDetails(row, dest) {
    const existing = row.querySelector('.destination-expanded');
    if (existing) {
        existing.remove();
        return;
    }

    const details = document.createElement('div');
    details.className = 'destination-expanded';
    details.innerHTML = `
        <div class="destination-expanded-grid">
            <div class="detail-item">
                <span class="detail-label">Server:</span>
                <span class="detail-value">live-${dest.service.toLowerCase()}.tv</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">Resolution:</span>
                <span class="detail-value">${dest.resolution}</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">Bitrate:</span>
                <span class="detail-value">${formatBitrate(dest.currentBitrate)} / ${formatBitrate(dest.bitrate)}</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">FPS:</span>
                <span class="detail-value">${dest.fps} fps</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">Dropped:</span>
                <span class="detail-value">${dest.droppedFrames} (${calculateDroppedPercent(dest.droppedFrames, dest.totalFrames)}%)</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">Duration:</span>
                <span class="detail-value">${formatDuration(dest.duration)}</span>
            </div>
        </div>
        <div class="destination-expanded-actions">
            <button class="btn btn-secondary btn-sm">📊 Stats</button>
            <button class="btn btn-secondary btn-sm">📝 Logs</button>
            <button class="btn btn-secondary btn-sm">🔍 Test Health</button>
        </div>
    `;

    row.appendChild(details);
}

function toggleProfileExpansion(profileId) {
    const widget = document.querySelector(`[data-profile-id="${profileId}"]`);
    if (!widget) return;

    const header = widget.querySelector('.profile-header');
    const content = widget.querySelector('.profile-content');

    if (content.classList.contains('expanded')) {
        content.classList.remove('expanded');
        header.classList.remove('expanded');
    } else {
        // Collapse all others first
        document.querySelectorAll('.profile-content.expanded').forEach(c => {
            c.classList.remove('expanded');
            c.previousElementSibling.classList.remove('expanded');
        });

        content.classList.add('expanded');
        header.classList.add('expanded');
    }
}

// Profile actions
function startProfile(profileId, event) {
    if (event) event.stopPropagation();

    const profile = mockProfiles.find(p => p.id === profileId);
    if (!profile) return;

    profile.status = 'starting';
    profile.destinations.forEach(d => d.status = 'starting');
    renderProfiles();

    setTimeout(() => {
        profile.status = 'active';
        profile.destinations.forEach(d => {
            d.status = 'active';
            d.currentBitrate = d.bitrate * (0.9 + Math.random() * 0.1);
        });
        renderProfiles();
    }, 1500);
}

function stopProfile(profileId, event) {
    if (event) event.stopPropagation();

    const profile = mockProfiles.find(p => p.id === profileId);
    if (!profile) return;

    profile.status = 'inactive';
    profile.destinations.forEach(d => {
        d.status = 'inactive';
        d.currentBitrate = 0;
        d.duration = 0;
    });
    renderProfiles();
}

function startDestination(profileId, destId, event) {
    if (event) event.stopPropagation();

    const profile = mockProfiles.find(p => p.id === profileId);
    if (!profile) return;

    const dest = profile.destinations.find(d => d.id === destId);
    if (!dest) return;

    dest.status = 'starting';
    renderProfiles();

    setTimeout(() => {
        dest.status = 'active';
        dest.currentBitrate = dest.bitrate * (0.9 + Math.random() * 0.1);
        renderProfiles();
    }, 1000);
}

function stopDestination(profileId, destId, event) {
    if (event) event.stopPropagation();

    const profile = mockProfiles.find(p => p.id === profileId);
    if (!profile) return;

    const dest = profile.destinations.find(d => d.id === destId);
    if (!dest) return;

    dest.status = 'inactive';
    dest.currentBitrate = 0;
    dest.duration = 0;
    renderProfiles();
}

// Context menu handlers
function showProfileContextMenu(event, profileId) {
    event.preventDefault();
    event.stopPropagation();

    const profile = mockProfiles.find(p => p.id === profileId);
    if (!profile) return;

    contextMenu.show(event.pageX, event.pageY, contextMenuItems.profile(profile), profile, 'profile');
}

function showDestinationContextMenu(event, profileId, destId) {
    event.preventDefault();
    event.stopPropagation();

    const profile = mockProfiles.find(p => p.id === profileId);
    if (!profile) return;

    const dest = profile.destinations.find(d => d.id === destId);
    if (!dest) return;

    contextMenu.show(event.pageX, event.pageY, contextMenuItems.destination(dest, profile), dest, 'destination');
}

// Bulk actions
document.getElementById('startAllBtn').addEventListener('click', () => {
    mockProfiles.forEach(profile => {
        if (profile.status !== 'active') {
            profile.status = 'starting';
            profile.destinations.forEach(d => d.status = 'starting');
        }
    });
    renderProfiles();

    setTimeout(() => {
        mockProfiles.forEach(profile => {
            if (profile.status === 'starting') {
                profile.status = 'active';
                profile.destinations.forEach(d => {
                    if (d.status === 'starting') {
                        d.status = 'active';
                        d.currentBitrate = d.bitrate * (0.9 + Math.random() * 0.1);
                    }
                });
            }
        });
        renderProfiles();
    }, 1500);
});

document.getElementById('stopAllBtn').addEventListener('click', () => {
    mockProfiles.forEach(profile => {
        profile.status = 'inactive';
        profile.destinations.forEach(d => {
            d.status = 'inactive';
            d.currentBitrate = 0;
            d.duration = 0;
        });
    });
    renderProfiles();
});

document.getElementById('refreshBtn').addEventListener('click', () => {
    renderProfiles();
    alert('Profiles refreshed!');
});

// Profile Rendering and Management

function renderProfiles() {
    const container = document.getElementById('profilesContainer');

    if (mockProfiles.length === 0) {
        // Use DOM methods to prevent XSS
        const noProfilesDiv = document.createElement('div');
        noProfilesDiv.className = 'no-profiles';

        const icon = document.createElement('div');
        icon.className = 'no-profiles-icon';
        icon.textContent = '📺';

        const title = document.createElement('div');
        title.className = 'no-profiles-title';
        title.textContent = 'No Streaming Profiles';

        const text = document.createElement('div');
        text.className = 'no-profiles-text';
        text.textContent = 'Create your first profile to start multistreaming to multiple platforms';

        const btn = document.createElement('button');
        btn.className = 'btn btn-primary';
        btn.onclick = () => openProfileEditModal(null);

        const iconSpan = document.createElement('span');
        iconSpan.className = 'icon';
        iconSpan.textContent = '+';

        btn.appendChild(iconSpan);
        btn.appendChild(document.createTextNode(' Create Profile'));

        noProfilesDiv.appendChild(icon);
        noProfilesDiv.appendChild(title);
        noProfilesDiv.appendChild(text);
        noProfilesDiv.appendChild(btn);

        container.innerHTML = '';
        container.appendChild(noProfilesDiv);
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

    // Profile header - use DOM methods to prevent XSS
    const header = document.createElement('div');
    header.className = 'profile-header';

    const statusIndicator = document.createElement('span');
    statusIndicator.className = `profile-status-indicator ${aggregateStatus}`;
    statusIndicator.textContent = getStatusIcon(aggregateStatus);

    const profileInfo = document.createElement('div');
    profileInfo.className = 'profile-info';

    const profileName = document.createElement('div');
    profileName.className = 'profile-name';
    profileName.textContent = profile.name;

    const profileSummary = document.createElement('div');
    profileSummary.className = 'profile-summary';
    profileSummary.textContent = summaryText;

    profileInfo.appendChild(profileName);
    profileInfo.appendChild(profileSummary);

    const profileActions = document.createElement('div');
    profileActions.className = 'profile-header-actions';

    // Start/Stop button
    const startStopBtn = document.createElement('button');
    if (profile.status === 'active' || profile.status === 'starting') {
        startStopBtn.className = 'profile-btn stop';
        startStopBtn.textContent = '■ Stop';
        startStopBtn.onclick = (event) => stopProfile(profile.id, event);
    } else {
        startStopBtn.className = 'profile-btn start';
        startStopBtn.textContent = '▶ Start';
        startStopBtn.onclick = (event) => startProfile(profile.id, event);
    }

    // Edit button
    const editBtn = document.createElement('button');
    editBtn.className = 'profile-btn';
    editBtn.textContent = 'Edit';
    editBtn.onclick = () => openProfileEditModal(mockProfiles.find(p => p.id === profile.id));

    // Menu button
    const menuBtn = document.createElement('button');
    menuBtn.className = 'profile-btn menu';
    menuBtn.textContent = '⋮';
    menuBtn.onclick = (event) => showProfileContextMenu(event, profile.id);

    profileActions.appendChild(startStopBtn);
    profileActions.appendChild(editBtn);
    profileActions.appendChild(menuBtn);

    header.appendChild(statusIndicator);
    header.appendChild(profileInfo);
    header.appendChild(profileActions);

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

    // Use DOM methods to prevent XSS
    const statusSpan = document.createElement('span');
    statusSpan.className = `destination-status ${dest.status}`;
    statusSpan.textContent = getStatusIcon(dest.status);

    const destInfo = document.createElement('div');
    destInfo.className = 'destination-info';

    const destName = document.createElement('div');
    destName.className = 'destination-name';
    destName.textContent = dest.service;

    const destDetails = document.createElement('div');
    destDetails.className = 'destination-details';

    const resolutionSpan = document.createElement('span');
    resolutionSpan.className = 'destination-detail';
    resolutionSpan.textContent = dest.resolution;

    const bitrateSpan = document.createElement('span');
    bitrateSpan.className = 'destination-detail';
    bitrateSpan.textContent = formatBitrate(dest.bitrate);

    destDetails.appendChild(resolutionSpan);
    destDetails.appendChild(bitrateSpan);

    if (dest.fps > 0) {
        const fpsSpan = document.createElement('span');
        fpsSpan.className = 'destination-detail';
        fpsSpan.textContent = dest.fps + ' FPS';
        destDetails.appendChild(fpsSpan);
    }

    destInfo.appendChild(destName);
    destInfo.appendChild(destDetails);

    row.appendChild(statusSpan);
    row.appendChild(destInfo);

    // Build stats section
    if (dest.status === 'active') {
        const droppedPercent = calculateDroppedPercent(dest.droppedFrames, dest.totalFrames);
        const droppedClass = droppedPercent > 5 ? 'error' : droppedPercent > 1 ? 'warning' : 'success';

        const statsDiv = document.createElement('div');
        statsDiv.className = 'destination-stats';

        const bitrateItem = document.createElement('span');
        bitrateItem.className = 'stat-item success';
        bitrateItem.textContent = '↑ ' + formatBitrate(dest.currentBitrate);

        const droppedItem = document.createElement('span');
        droppedItem.className = `stat-item ${droppedClass}`;
        droppedItem.textContent = `${dest.droppedFrames} dropped (${droppedPercent}%)`;

        const durationItem = document.createElement('span');
        durationItem.className = 'stat-item';
        durationItem.textContent = formatDuration(dest.duration);

        statsDiv.appendChild(bitrateItem);
        statsDiv.appendChild(droppedItem);
        statsDiv.appendChild(durationItem);

        row.appendChild(statsDiv);
    } else if (dest.status === 'starting') {
        const statsDiv = document.createElement('div');
        statsDiv.className = 'destination-stats';

        const connectingItem = document.createElement('span');
        connectingItem.className = 'stat-item warning';
        connectingItem.textContent = 'Connecting...';

        statsDiv.appendChild(connectingItem);
        row.appendChild(statsDiv);
    } else if (dest.status === 'error') {
        const statsDiv = document.createElement('div');
        statsDiv.className = 'destination-stats';

        const errorItem = document.createElement('span');
        errorItem.className = 'stat-item error';
        errorItem.textContent = dest.error;

        statsDiv.appendChild(errorItem);
        row.appendChild(statsDiv);
    }

    // Destination actions
    const actionsDiv = document.createElement('div');
    actionsDiv.className = 'destination-actions';

    const startStopBtn = document.createElement('button');
    if (dest.status === 'active') {
        startStopBtn.className = 'destination-btn stop';
        startStopBtn.textContent = '■';
        startStopBtn.onclick = (event) => stopDestination(profile.id, dest.id, event);
    } else {
        startStopBtn.className = 'destination-btn start';
        startStopBtn.textContent = '▶';
        startStopBtn.onclick = (event) => startDestination(profile.id, dest.id, event);
    }

    const settingsBtn = document.createElement('button');
    settingsBtn.className = 'destination-btn';
    settingsBtn.textContent = '⚙️';
    settingsBtn.onclick = () => editDestination(dest.id);

    actionsDiv.appendChild(startStopBtn);
    actionsDiv.appendChild(settingsBtn);

    row.appendChild(actionsDiv);

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

    // Use DOM methods to prevent XSS
    const details = document.createElement('div');
    details.className = 'destination-expanded';

    const grid = document.createElement('div');
    grid.className = 'destination-expanded-grid';

    // Helper function to create detail items
    function createDetailItem(label, value) {
        const item = document.createElement('div');
        item.className = 'detail-item';

        const labelSpan = document.createElement('span');
        labelSpan.className = 'detail-label';
        labelSpan.textContent = label + ':';

        const valueSpan = document.createElement('span');
        valueSpan.className = 'detail-value';
        valueSpan.textContent = value;

        item.appendChild(labelSpan);
        item.appendChild(valueSpan);

        return item;
    }

    grid.appendChild(createDetailItem('Server', `live-${dest.service.toLowerCase()}.tv`));
    grid.appendChild(createDetailItem('Resolution', dest.resolution));
    grid.appendChild(createDetailItem('Bitrate', `${formatBitrate(dest.currentBitrate)} / ${formatBitrate(dest.bitrate)}`));
    grid.appendChild(createDetailItem('FPS', `${dest.fps} fps`));
    grid.appendChild(createDetailItem('Dropped', `${dest.droppedFrames} (${calculateDroppedPercent(dest.droppedFrames, dest.totalFrames)}%)`));
    grid.appendChild(createDetailItem('Duration', formatDuration(dest.duration)));

    const actions = document.createElement('div');
    actions.className = 'destination-expanded-actions';

    const statsBtn = document.createElement('button');
    statsBtn.className = 'btn btn-secondary btn-sm';
    statsBtn.textContent = '📊 Stats';

    const logsBtn = document.createElement('button');
    logsBtn.className = 'btn btn-secondary btn-sm';
    logsBtn.textContent = '📝 Logs';

    const healthBtn = document.createElement('button');
    healthBtn.className = 'btn btn-secondary btn-sm';
    healthBtn.textContent = '🔍 Test Health';

    actions.appendChild(statsBtn);
    actions.appendChild(logsBtn);
    actions.appendChild(healthBtn);

    details.appendChild(grid);
    details.appendChild(actions);

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

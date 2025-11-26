// Mock data for the prototype

const mockProfiles = [
    {
        id: 'profile-1',
        name: 'Gaming - Horizontal',
        status: 'active',
        destinations: [
            {
                id: 'dest-1-1',
                service: 'Twitch',
                status: 'active',
                resolution: '1920x1080',
                bitrate: 6000,
                currentBitrate: 5823,
                fps: 60,
                droppedFrames: 12,
                totalFrames: 54230,
                duration: 2723, // seconds
                error: null
            },
            {
                id: 'dest-1-2',
                service: 'YouTube',
                status: 'active',
                resolution: '1920x1080',
                bitrate: 8000,
                currentBitrate: 7645,
                fps: 60,
                droppedFrames: 3,
                totalFrames: 54230,
                duration: 2723,
                error: null
            },
            {
                id: 'dest-1-3',
                service: 'Facebook',
                status: 'error',
                resolution: '1280x720',
                bitrate: 3500,
                currentBitrate: 0,
                fps: 30,
                droppedFrames: 0,
                totalFrames: 0,
                duration: 0,
                error: 'Connection lost - Authentication failed'
            }
        ]
    },
    {
        id: 'profile-2',
        name: 'Gaming - Vertical',
        status: 'inactive',
        destinations: [
            {
                id: 'dest-2-1',
                service: 'TikTok',
                status: 'inactive',
                resolution: '1080x1920',
                bitrate: 4500,
                currentBitrate: 0,
                fps: 30,
                droppedFrames: 0,
                totalFrames: 0,
                duration: 0,
                error: null
            },
            {
                id: 'dest-2-2',
                service: 'Instagram',
                status: 'inactive',
                resolution: '1080x1920',
                bitrate: 4000,
                currentBitrate: 0,
                fps: 30,
                droppedFrames: 0,
                totalFrames: 0,
                duration: 0,
                error: null
            },
            {
                id: 'dest-2-3',
                service: 'YouTube Shorts',
                status: 'inactive',
                resolution: '1080x1920',
                bitrate: 5000,
                currentBitrate: 0,
                fps: 30,
                droppedFrames: 0,
                totalFrames: 0,
                duration: 0,
                error: null
            }
        ]
    },
    {
        id: 'profile-3',
        name: 'Podcast - Audio',
        status: 'starting',
        destinations: [
            {
                id: 'dest-3-1',
                service: 'YouTube',
                status: 'starting',
                resolution: '1280x720',
                bitrate: 2500,
                currentBitrate: 0,
                fps: 30,
                droppedFrames: 0,
                totalFrames: 0,
                duration: 0,
                error: null
            },
            {
                id: 'dest-3-2',
                service: 'Spotify Anchor',
                status: 'active',
                resolution: 'Audio',
                bitrate: 128,
                currentBitrate: 124,
                fps: 0,
                droppedFrames: 0,
                totalFrames: 0,
                duration: 135,
                error: null
            }
        ]
    }
];

const mockProcesses = [
    {
        id: 'proc-1',
        reference: 'Gaming Horizontal Stream',
        state: 'running',
        uptime: 2723,
        cpu: 24.5,
        memory: 512
    },
    {
        id: 'proc-2',
        reference: 'Podcast Stream',
        state: 'starting',
        uptime: 135,
        cpu: 8.2,
        memory: 256
    }
];

const mockSessions = [
    {
        id: 'sess-1',
        remoteAddr: '192.168.1.100:54321',
        bytesSent: 1024 * 1024 * 523, // 523 MB
        duration: 2723
    },
    {
        id: 'sess-2',
        remoteAddr: '192.168.1.101:54322',
        bytesSent: 1024 * 1024 * 48, // 48 MB
        duration: 135
    }
];

// Utility functions
function formatDuration(seconds) {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    return `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(secs).padStart(2, '0')}`;
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function formatBitrate(kbps) {
    if (kbps >= 1000) {
        return (kbps / 1000).toFixed(1) + ' Mbps';
    }
    return kbps + ' Kbps';
}

function getStatusIcon(status) {
    const icons = {
        'active': '🟢',
        'starting': '🟡',
        'error': '🔴',
        'inactive': '⚫',
        'paused': '🟣'
    };
    return icons[status] || '⚫';
}

function getStatusText(status) {
    const texts = {
        'active': 'Active',
        'starting': 'Starting',
        'error': 'Error',
        'inactive': 'Stopped',
        'paused': 'Paused'
    };
    return texts[status] || 'Unknown';
}

function calculateDroppedPercent(dropped, total) {
    if (total === 0) return 0;
    return ((dropped / total) * 100).toFixed(2);
}

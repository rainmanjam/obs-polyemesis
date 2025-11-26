# OBS Polyemesis UI Prototype

An interactive HTML/CSS/JavaScript prototype of the streamlined OBS Polyemesis interface design.

## 🎯 Features

### ✅ Fully Interactive
- ✓ Profile management (create, edit, delete, duplicate)
- ✓ Start/stop individual destinations or entire profiles
- ✓ Real-time status updates and statistics
- ✓ Right-click context menus throughout the interface
- ✓ Modal dialogs for settings and monitoring
- ✓ Expandable/collapsible profiles
- ✓ Live metric updates (CPU, memory, bitrate, dropped frames)

### ✅ OBS-Authentic Styling
- ✓ Matches OBS Studio's Dark theme (Yami/Dark variants)
- ✓ Native OBS color palette and typography
- ✓ Proper spacing, borders, and shadows
- ✓ Smooth animations and transitions
- ✓ Responsive hover and active states

### ✅ Production-Ready UX
- ✓ Per-destination status indicators (🟢🟡🔴⚫)
- ✓ Context menus (right-click) for all major elements
- ✓ Keyboard shortcuts (Ctrl+S, Ctrl+Q, Ctrl+N, Ctrl+M, Esc)
- ✓ Empty states with helpful messaging
- ✓ Loading states and transitions
- ✓ Inline editing and quick actions

## 📂 File Structure

```
ui-prototype/
├── index.html           # Main HTML structure
├── css/
│   ├── main.css        # Core styles, variables, layout
│   ├── profiles.css    # Profile/destination widgets
│   ├── context-menu.css # Context menu styling
│   └── modals.css      # Modal dialogs and tables
├── js/
│   ├── data.js         # Mock data and utility functions
│   ├── context-menu.js # Context menu logic
│   ├── modals.js       # Modal management
│   ├── profiles.js     # Profile rendering and actions
│   ├── monitoring.js   # Monitoring data updates
│   └── main.js         # App initialization
├── assets/             # (Future: images, icons)
└── README.md           # This file
```

## 🚀 How to Use

### Option 1: Open Directly
1. Open `index.html` in a modern web browser (Chrome, Firefox, Safari, Edge)
2. The prototype will load with sample data

### Option 2: Local Server (Recommended)
```bash
# Navigate to the prototype folder
cd ui-prototype

# Python 3
python3 -m http.server 8000

# Python 2
python -m SimpleHTTPServer 8000

# Or use any other local server
# Then open: http://localhost:8000
```

### Option 3: VS Code Live Server
1. Install "Live Server" extension in VS Code
2. Right-click `index.html` → "Open with Live Server"

## 🎮 Interactions

### Primary Actions
- **Click profile header** → Expand/collapse destinations
- **Click Start/Stop buttons** → Control profiles or individual destinations
- **Right-click anywhere** → Open context menu with actions
- **Double-click destination** → Show detailed stats
- **Hover over elements** → See tooltips and actions

### Right-Click Context Menus

**Profile Header:**
- Start/Stop/Restart Profile
- Edit/Duplicate/Delete Profile
- View Statistics
- Export Configuration
- Profile Settings

**Individual Destination:**
- Start/Stop/Restart/Pause Stream
- Edit Destination
- Copy Stream URL/Key
- View Stream Stats/Logs
- Test Stream Health
- Disable/Remove Destination

**Connection Status:**
- Test Connection
- Reconnect/Disconnect
- Edit Connection Settings
- View Server Stats/Logs
- Probe Server

### Keyboard Shortcuts
- `Ctrl/Cmd + S` → Start all profiles
- `Ctrl/Cmd + Q` → Stop all profiles
- `Ctrl/Cmd + N` → Create new profile
- `Ctrl/Cmd + M` → Open monitoring
- `Esc` → Close modals and menus

## 📊 Features Demonstrated

### Main Window
1. **Connection Section**
   - Status indicator with real-time updates
   - Quick test and settings buttons
   - Right-click for advanced actions

2. **Streaming Profiles**
   - Profile list with aggregate status
   - Per-destination status indicators
   - Inline start/stop/edit actions
   - Expandable to show all destinations
   - Live bitrate, dropped frames, duration

3. **Quick Actions**
   - Monitoring modal (CPU, Memory, Bitrate, Dropped Frames)
   - Advanced settings modal
   - Server settings (placeholder)

### Modals

**Monitoring Modal:**
- Real-time metrics with progress bars
- Processes table (ID, State, Uptime, CPU, Memory)
- Sessions table (ID, Remote Address, Bytes Sent, Duration)
- Auto-updating every second

**Connection Settings Modal:**
- Host, Port, HTTPS toggle
- Username/Password fields
- Save & Test button

**Profile Edit Modal:**
- Profile name input
- Destinations list with edit/remove
- Add destination button

**Destination Edit Modal:**
- Service selection dropdown
- Stream key (with show/hide toggle)
- Resolution, Bitrate, FPS inputs

**Advanced Modal:**
- Orientation settings
- FFmpeg capabilities
- Protocol monitoring

## 🎨 Design Details

### Color Scheme (OBS Dark Theme)
- **Background Primary:** `#1e1e1e`
- **Background Secondary:** `#2d2d30`
- **Background Tertiary:** `#3e3e42`
- **Background Hover:** `#4e4e52`
- **Text Primary:** `#cccccc`
- **Text Secondary:** `#969696`
- **Border Primary:** `#3e3e42`
- **Focus/Active:** `#007acc`

### Status Colors
- **Active (🟢):** `#4ec9b0` (Green)
- **Starting (🟡):** `#dcdcaa` (Yellow)
- **Error (🔴):** `#f48771` (Red)
- **Inactive (⚫):** `#6e6e6e` (Gray)
- **Paused (🟣):** `#c586c0` (Purple)

### Typography
- **Font Family:** `-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial`
- **Base Size:** `13px`
- **Small:** `11px`
- **Medium:** `14px`
- **Large:** `16px`

### Spacing Scale
- **XS:** `4px`
- **SM:** `8px`
- **MD:** `12px`
- **LG:** `16px`
- **XL:** `24px`

## 🔄 Real-Time Simulation

The prototype includes realistic simulation of:

1. **Stream Statistics** (updated every second):
   - Duration counters
   - Bitrate fluctuation (±10%)
   - Dropped frames (random, ~0.1% rate)
   - Total frame count

2. **Process Metrics**:
   - CPU usage (20-50% range)
   - Memory usage (128MB-1GB range)
   - Uptime counters

3. **Session Data**:
   - Bytes sent (increasing ~100-300 KB/s)
   - Connection duration

4. **State Transitions**:
   - Starting → Active (1.5s delay)
   - Active → Stopped (immediate)
   - Proper status propagation

## 📝 Mock Data

The prototype includes 3 sample profiles:

1. **Gaming - Horizontal** (Active)
   - Twitch: 🟢 Active (1920x1080, 6 Mbps)
   - YouTube: 🟢 Active (1920x1080, 8 Mbps)
   - Facebook: 🔴 Error (Connection lost)

2. **Gaming - Vertical** (Inactive)
   - TikTok: ⚫ Stopped (1080x1920, 4.5 Mbps)
   - Instagram: ⚫ Stopped (1080x1920, 4 Mbps)
   - YouTube Shorts: ⚫ Stopped (1080x1920, 5 Mbps)

3. **Podcast - Audio** (Starting)
   - YouTube: 🟡 Starting (1280x720, 2.5 Mbps)
   - Spotify Anchor: 🟢 Active (Audio, 128 Kbps)

## 🚧 Future Enhancements

Potential additions for the prototype:

- [ ] Drag-and-drop profile reordering
- [ ] Profile import/export functionality
- [ ] Stream health graphs (bitrate over time)
- [ ] Multi-select for bulk operations
- [ ] Profile templates library
- [ ] Search/filter for profiles
- [ ] Compact vs. Detailed view toggle
- [ ] Dark/Light theme switcher
- [ ] Accessibility improvements (ARIA labels)
- [ ] Touch-friendly mobile view

## 💡 Implementation Notes

### For Qt/C++ Integration

This prototype demonstrates the UX but will need translation to Qt:

1. **HTML → QWidget hierarchy**
   - `<div class="profile-widget">` → `ProfileWidget` class
   - `<div class="destination-row">` → `DestinationWidget` class
   - Context menus → `QMenu` with `QAction`s

2. **CSS → Qt Stylesheets + QPalette**
   - CSS variables → Qt stylesheet variables
   - Colors → `obs_theme_get_*_color()` functions
   - Layouts → `QVBoxLayout`, `QHBoxLayout`, `QGridLayout`

3. **JavaScript → C++ Qt**
   - Event listeners → Qt signals/slots
   - DOM manipulation → Qt widget updates
   - State management → C structures + `std::vector`
   - Timers → `QTimer`

4. **Key Classes to Implement**
   ```cpp
   class ProfileWidget : public QWidget {
       Q_OBJECT
   public:
       ProfileWidget(output_profile_t *profile);
       void setExpanded(bool expanded);
       void updateStatus();
   protected:
       void contextMenuEvent(QContextMenuEvent *event) override;
   signals:
       void startRequested();
       void stopRequested();
       void editRequested();
   };

   class DestinationWidget : public QWidget {
       Q_OBJECT
   public:
       DestinationWidget(const DestinationConfig &config);
       void setStatus(StreamStatus status);
       void setBitrate(float current, float max);
   protected:
       void contextMenuEvent(QContextMenuEvent *event) override;
   signals:
       void startRequested();
       void stopRequested();
   };
   ```

## 📞 Support

For questions or feedback about this prototype:
1. Open the browser console (F12) for debug logs
2. Check the console for interaction confirmations
3. All actions are logged for debugging

## ✨ Credits

Designed to match OBS Studio's native look and feel while providing an improved, streamlined user experience for the OBS Polyemesis plugin.

---

**Enjoy exploring the prototype!** 🎉

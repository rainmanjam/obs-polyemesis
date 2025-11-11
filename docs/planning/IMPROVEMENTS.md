# OBS Polyemesis Improvement Plan

Based on comprehensive analysis of:
- obs-aitum-multistream
- obs-vertical-canvas
- shadcn/ui design principles

---

## Executive Summary

### Key Findings from Aitum Plugins

**obs-aitum-multistream strengths:**
- Dynamic property UI generation from encoder introspection
- Profile-aware configuration with automatic switching
- Safe file writing with tmp/bak pattern
- Thread-safe UI updates via QMetaObject::invokeMethod
- Platform icon auto-detection from server URLs
- Sidebar navigation with stacked widgets
- Encoder index flexibility for resource optimization

**obs-vertical-canvas strengths:**
- RAII pattern with OBSSourceAutoRelease wrappers
- Atomic scene updates to prevent signal cascades
- Custom styled checkbox widgets
- Comprehensive accessibility support
- WebSocket API vendor system
- Multi-canvas architecture
- Comprehensive internationalization (10 languages)
- Property-based Qt theming

---

## Priority 1: Critical Robustness Improvements

### 1.1 Memory Management & Resource Cleanup

**Issue:** Inconsistent use of OBS auto-release wrappers
**Impact:** Potential memory leaks

**Solution:**
```cpp
// Replace manual release:
obs_source_t *source = obs_get_source_by_name(name);
// ... use source
obs_source_release(source);

// With RAII wrappers:
OBSSourceAutoRelease source = obs_get_source_by_name(name);
// Automatically released when out of scope
```

**Files to update:**
- `src/restreamer-dock.cpp`
- `src/restreamer-multistream.cpp`

### 1.2 Safe Configuration File Writing

**Issue:** Current `obs_data_save_json()` can corrupt config on crash
**Impact:** User settings lost on power failure or crash

**Solution:**
```cpp
// Replace:
obs_data_save_json(settings, path);

// With atomic write:
obs_data_save_json_safe(settings, path, "tmp", "bak");
```

Process:
1. Write to `config.json.tmp`
2. Rename `config.json` to `config.json.bak`
3. Rename `config.json.tmp` to `config.json`

**Files to update:**
- `src/restreamer-dock.cpp` (saveSettings function)

### 1.3 Thread-Safe UI Updates

**Issue:** Signal handlers updating UI directly from OBS threads
**Impact:** Potential crashes and race conditions

**Solution:**
```cpp
// Replace direct UI updates:
void signal_handler(void *data, calldata_t *cd) {
    auto dock = static_cast<RestreamerDock*>(data);
    dock->button->setText("Active");  // UNSAFE!
}

// With queued invocation:
void signal_handler(void *data, calldata_t *cd) {
    auto dock = static_cast<RestreamerDock*>(data);
    QMetaObject::invokeMethod(dock, [dock]() {
        dock->button->setText("Active");  // Safe!
    }, Qt::QueuedConnection);
}
```

**Files to update:**
- All signal handler callbacks in `restreamer-dock.cpp`

### 1.4 Signal Handler Cleanup

**Issue:** Signal handlers not disconnected before object destruction
**Impact:** Callbacks on destroyed objects = crashes

**Solution:**
```cpp
// In destructor or cleanup:
signal_handler_t *sh = obs_output_get_signal_handler(output);
if (sh) {
    signal_handler_disconnect(sh, "start", output_start_handler, this);
    signal_handler_disconnect(sh, "stop", output_stop_handler, this);
}
obs_output_force_stop(output);
obs_output_release(output);
```

**Files to update:**
- `src/restreamer-dock.cpp` destructor
- `src/restreamer-output-profile.cpp` cleanup functions

---

## Priority 2: UI/UX Redesign (shadcn/ui inspired)

### 2.1 Design System Principles

**shadcn/ui characteristics to adapt:**
- Neutral color palette with subtle accents
- Generous spacing (4px, 8px, 12px, 16px, 24px, 32px grid)
- Subtle borders (1px, neutral-200/neutral-800)
- Soft shadows for depth
- Clear visual hierarchy
- State-driven styling (default, hover, active, disabled)
- Card-based layouts for grouping
- Smooth transitions (150ms ease)

**Qt Stylesheet Template:**
```css
/* shadcn/ui inspired color palette */
QDockWidget {
    background: #ffffff;  /* bg-background */
    color: #0a0a0a;       /* text-foreground */
}

/* Card style for group boxes */
QGroupBox {
    background: #ffffff;
    border: 1px solid #e5e5e5;  /* border-neutral-200 */
    border-radius: 8px;
    padding: 16px;
    margin-top: 8px;
    font-weight: 500;
}

/* Primary button */
QPushButton {
    background: #0a0a0a;      /* bg-primary */
    color: #fafafa;           /* text-primary-foreground */
    border: none;
    border-radius: 6px;
    padding: 8px 16px;
    font-weight: 500;
}

QPushButton:hover {
    background: #171717;      /* hover:bg-primary/90 */
}

QPushButton:pressed {
    background: #262626;
}

QPushButton:disabled {
    background: #f5f5f5;
    color: #a3a3a3;
}

/* Secondary button */
QPushButton[class="secondary"] {
    background: #f5f5f5;      /* bg-secondary */
    color: #0a0a0a;
    border: 1px solid #e5e5e5;
}

QPushButton[class="secondary"]:hover {
    background: #e5e5e5;
}

/* Destructive button */
QPushButton[class="destructive"] {
    background: #ef4444;      /* bg-destructive */
    color: #ffffff;
}

QPushButton[class="destructive"]:hover {
    background: #dc2626;
}

/* Input fields */
QLineEdit, QTextEdit, QPlainTextEdit {
    background: #ffffff;
    border: 1px solid #e5e5e5;
    border-radius: 6px;
    padding: 8px 12px;
    color: #0a0a0a;
}

QLineEdit:focus, QTextEdit:focus {
    border-color: #0a0a0a;
    outline: 2px solid rgba(10, 10, 10, 0.2);
    outline-offset: 2px;
}

/* List widgets */
QListWidget {
    background: #ffffff;
    border: 1px solid #e5e5e5;
    border-radius: 8px;
    padding: 4px;
}

QListWidget::item {
    border-radius: 6px;
    padding: 8px 12px;
    margin: 2px;
}

QListWidget::item:hover {
    background: #f5f5f5;
}

QListWidget::item:selected {
    background: #0a0a0a;
    color: #fafafa;
}

/* Table widgets */
QTableWidget {
    background: #ffffff;
    border: 1px solid #e5e5e5;
    border-radius: 8px;
    gridline-color: #f5f5f5;
}

QHeaderView::section {
    background: #fafafa;
    border: none;
    border-bottom: 1px solid #e5e5e5;
    padding: 8px 12px;
    font-weight: 500;
}

/* Tabs */
QTabWidget::pane {
    border: 1px solid #e5e5e5;
    border-radius: 8px;
    background: #ffffff;
}

QTabBar::tab {
    background: #fafafa;
    border: 1px solid #e5e5e5;
    border-bottom: none;
    padding: 8px 16px;
    margin-right: 2px;
}

QTabBar::tab:selected {
    background: #ffffff;
    border-bottom: 2px solid #0a0a0a;
}

QTabBar::tab:hover:!selected {
    background: #f5f5f5;
}
```

### 2.2 Sidebar Navigation Pattern

**Current Issue:** Tabs are cramped and don't scale well
**Solution:** Adopt Aitum's sidebar + stacked widget pattern

**New Layout:**
```
┌────────────────────────────────────────┐
│  Restreamer Control                    │
├────────────┬───────────────────────────┤
│            │                           │
│ Connection │  Connection Settings      │
│ Profiles   │  ┌─────────────────────┐ │
│ Processes  │  │ Host: [         ]   │ │
│ Settings   │  │ Port: [    ]        │ │
│ About      │  │ ☑ Use HTTPS         │ │
│            │  │ Username: [      ]  │ │
│            │  │ Password: [••••••]  │ │
│            │  │ [Test Connection]   │ │
│            │  └─────────────────────┘ │
│            │                           │
│            │  Status: ● Connected     │
└────────────┴───────────────────────────┘
```

**Implementation:**
```cpp
void RestreamerDock::setupUI() {
    QHBoxLayout *mainLayout = new QHBoxLayout();

    // Sidebar navigation
    QListWidget *navigation = new QListWidget();
    navigation->setMaximumWidth(180);
    navigation->setSpacing(2);
    navigation->addItem("Connection");
    navigation->addItem("Profiles");
    navigation->addItem("Processes");
    navigation->addItem("Advanced");
    navigation->addItem("About");

    // Content pages
    QStackedWidget *pages = new QStackedWidget();
    pages->addWidget(createConnectionPage());
    pages->addWidget(createProfilesPage());
    pages->addWidget(createProcessesPage());
    pages->addWidget(createAdvancedPage());
    pages->addWidget(createAboutPage());

    connect(navigation, &QListWidget::currentRowChanged,
            pages, &QStackedWidget::setCurrentIndex);

    mainLayout->addWidget(navigation);
    mainLayout->addWidget(pages, 1);  // Stretch factor 1

    QWidget *mainWidget = new QWidget();
    mainWidget->setLayout(mainLayout);
    setWidget(mainWidget);
}
```

### 2.3 Card-Based Profile List

**Replace simple list with card layout:**

```cpp
class ProfileCard : public QFrame {
public:
    ProfileCard(output_profile_t *profile, QWidget *parent = nullptr)
        : QFrame(parent), profile(profile) {
        setupUI();
    }

private:
    void setupUI() {
        setFrameShape(QFrame::StyledPanel);
        setProperty("class", "card");

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(12);

        // Header with status indicator
        QHBoxLayout *header = new QHBoxLayout();
        QLabel *statusDot = new QLabel();
        statusDot->setFixedSize(8, 8);
        statusDot->setProperty("status", getStatusClass());

        QLabel *name = new QLabel(profile->profile_name);
        QFont nameFont = name->font();
        nameFont.setWeight(QFont::Medium);
        nameFont.setPointSize(nameFont.pointSize() + 1);
        name->setFont(nameFont);

        header->addWidget(statusDot);
        header->addWidget(name);
        header->addStretch();

        // Metadata
        QLabel *destinations = new QLabel(
            QString("%1 destinations").arg(profile->destination_count));
        destinations->setProperty("class", "muted");

        // Actions
        QHBoxLayout *actions = new QHBoxLayout();
        QPushButton *startBtn = new QPushButton("Start");
        startBtn->setProperty("class", "secondary");
        startBtn->setEnabled(profile->status == PROFILE_STATUS_INACTIVE);

        QPushButton *configBtn = new QPushButton("Configure");
        configBtn->setProperty("class", "ghost");

        actions->addWidget(startBtn);
        actions->addWidget(configBtn);
        actions->addStretch();

        layout->addLayout(header);
        layout->addWidget(destinations);
        layout->addLayout(actions);
    }

    QString getStatusClass() const {
        switch (profile->status) {
            case PROFILE_STATUS_ACTIVE: return "active";
            case PROFILE_STATUS_STARTING: return "warning";
            case PROFILE_STATUS_ERROR: return "error";
            default: return "inactive";
        }
    }

    output_profile_t *profile;
};
```

---

## Priority 3: Architectural Improvements

### 3.1 Dynamic Property UI Generation

**Current Issue:** Hardcoded encoder settings
**Solution:** Introspect encoder properties and generate UI dynamically

```cpp
void ProfileConfigDialog::loadEncoderProperties(const char *encoder_id) {
    // Clear previous properties
    clearPropertyWidgets();

    // Get encoder properties
    obs_properties_t *props = obs_get_encoder_properties(encoder_id);
    if (!props) return;

    // Iterate through properties
    obs_property_t *prop = obs_properties_first(props);
    while (prop) {
        addPropertyWidget(prop, encoderSettings);
        obs_property_next(&prop);
    }

    obs_properties_destroy(props);
}

void ProfileConfigDialog::addPropertyWidget(obs_property_t *prop,
                                           obs_data_t *settings) {
    const char *name = obs_property_name(prop);
    const char *desc = obs_property_description(prop);
    obs_property_type type = obs_property_get_type(prop);

    // Skip if not visible
    if (!obs_property_visible(prop)) return;

    QWidget *widget = nullptr;

    switch (type) {
        case OBS_PROPERTY_BOOL: {
            auto checkbox = new QCheckBox();
            checkbox->setChecked(obs_data_get_bool(settings, name));
            connect(checkbox, &QCheckBox::toggled, [=](bool checked) {
                obs_data_set_bool(settings, name, checked);
                if (obs_property_modified(prop, settings)) {
                    refreshProperties();  // Handle dependencies
                }
            });
            widget = checkbox;
            break;
        }

        case OBS_PROPERTY_INT: {
            auto spinbox = new QSpinBox();
            spinbox->setMinimum(obs_property_int_min(prop));
            spinbox->setMaximum(obs_property_int_max(prop));
            spinbox->setSingleStep(obs_property_int_step(prop));
            spinbox->setValue(obs_data_get_int(settings, name));
            connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
                    [=](int value) {
                obs_data_set_int(settings, name, value);
                if (obs_property_modified(prop, settings)) {
                    refreshProperties();
                }
            });
            widget = spinbox;
            break;
        }

        case OBS_PROPERTY_LIST: {
            auto combo = new QComboBox();
            obs_combo_type combo_type = obs_property_list_type(prop);

            size_t count = obs_property_list_item_count(prop);
            for (size_t i = 0; i < count; i++) {
                const char *item_name = obs_property_list_item_name(prop, i);

                if (combo_type == OBS_COMBO_TYPE_INT) {
                    long long val = obs_property_list_item_int(prop, i);
                    combo->addItem(item_name, QVariant::fromValue(val));
                } else {
                    const char *val = obs_property_list_item_string(prop, i);
                    combo->addItem(item_name, val);
                }
            }

            // Set current value
            if (combo_type == OBS_COMBO_TYPE_INT) {
                int current = obs_data_get_int(settings, name);
                int index = combo->findData(current);
                combo->setCurrentIndex(index);
            } else {
                QString current = obs_data_get_string(settings, name);
                int index = combo->findData(current);
                combo->setCurrentIndex(index);
            }

            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [=](int index) {
                QVariant data = combo->itemData(index);
                if (combo_type == OBS_COMBO_TYPE_INT) {
                    obs_data_set_int(settings, name, data.toLongLong());
                } else {
                    obs_data_set_string(settings, name,
                                       data.toString().toUtf8().constData());
                }
                if (obs_property_modified(prop, settings)) {
                    refreshProperties();
                }
            });

            widget = combo;
            break;
        }

        // Handle other types...
    }

    if (widget) {
        propertyLayout->addRow(desc, widget);
        propertyWidgets[prop] = widget;
    }
}

void ProfileConfigDialog::refreshProperties() {
    obs_properties_t *props = obs_get_encoder_properties(currentEncoderId);
    obs_property_t *prop = obs_properties_first(props);

    while (prop) {
        if (propertyWidgets.contains(prop)) {
            QWidget *widget = propertyWidgets[prop];
            bool visible = obs_property_visible(prop);
            widget->setVisible(visible);

            // Also hide/show label
            QFormLayout *form = static_cast<QFormLayout*>(propertyLayout);
            int row = form->getWidgetPosition(widget, &row, nullptr);
            QLabel *label = static_cast<QLabel*>(form->itemAt(row,
                                                   QFormLayout::LabelRole)->widget());
            if (label) label->setVisible(visible);
        }

        obs_property_next(&prop);
    }

    obs_properties_destroy(props);
}
```

### 3.2 Profile-Aware Configuration

**Solution:** Automatically switch settings when OBS profile changes

```cpp
// In constructor
obs_frontend_add_event_callback(frontendEventHandler, this);

static void frontendEventHandler(enum obs_frontend_event event, void *data) {
    auto dock = static_cast<RestreamerDock*>(data);

    switch (event) {
        case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
            dock->loadProfileSettings();
            break;

        case OBS_FRONTEND_EVENT_STREAMING_STARTED:
            dock->onStreamingStateChanged(true);
            break;

        case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
            dock->onStreamingStateChanged(false);
            break;

        case OBS_FRONTEND_EVENT_EXIT:
            dock->exiting = true;
            dock->cleanup();
            break;
    }
}

void RestreamerDock::loadProfileSettings() {
    const char *profile_name = obs_frontend_get_current_profile();
    const char *config_path = obs_module_config_path("config.json");

    obs_data_t *config = obs_data_create_from_json_file_safe(config_path, "bak");
    if (!config) {
        config = obs_data_create();
    }

    // Get profiles array
    obs_data_array_t *profiles = obs_data_get_array(config, "profiles");

    // Find matching profile
    bool found = false;
    for (size_t i = 0; i < obs_data_array_count(profiles); i++) {
        obs_data_t *profile = obs_data_array_item(profiles, i);
        const char *name = obs_data_get_string(profile, "name");

        if (strcmp(name, profile_name) == 0) {
            loadProfileData(profile);
            found = true;
            obs_data_release(profile);
            break;
        }

        obs_data_release(profile);
    }

    if (!found) {
        // Create new profile entry
        blog(LOG_INFO, "Creating settings for new profile: %s", profile_name);
        setDefaults();
    }

    obs_data_array_release(profiles);
    obs_data_release(config);
}
```

### 3.3 WebSocket API for Remote Control

**Add vendor API for external tools:**

```cpp
obs_websocket_vendor vendor;

bool obs_module_load() {
    // ... existing code

    // Register WebSocket vendor
    vendor = obs_websocket_register_vendor("polyemesis");
    if (vendor) {
        registerWebSocketHandlers();
    }

    return true;
}

void registerWebSocketHandlers() {
    // Get profiles
    obs_websocket_vendor_register_request(vendor, "GetProfiles",
        [](obs_data_t *request, obs_data_t *response, void *priv) {
            auto dock = static_cast<RestreamerDock*>(priv);
            obs_data_array_t *profiles = obs_data_array_create();

            for (size_t i = 0; i < dock->profileManager->profile_count; i++) {
                output_profile_t *p = dock->profileManager->profiles[i];
                obs_data_t *profile_data = obs_data_create();

                obs_data_set_string(profile_data, "id", p->profile_id);
                obs_data_set_string(profile_data, "name", p->profile_name);
                obs_data_set_string(profile_data, "status",
                                   profileStatusToString(p->status));
                obs_data_set_int(profile_data, "destinations",
                                p->destination_count);

                obs_data_array_push_back(profiles, profile_data);
                obs_data_release(profile_data);
            }

            obs_data_set_array(response, "profiles", profiles);
            obs_data_array_release(profiles);
        }, restreamer_dock);

    // Start profile
    obs_websocket_vendor_register_request(vendor, "StartProfile",
        [](obs_data_t *request, obs_data_t *response, void *priv) {
            auto dock = static_cast<RestreamerDock*>(priv);
            const char *profile_id = obs_data_get_string(request, "profileId");

            bool success = output_profile_start(dock->profileManager, profile_id);
            obs_data_set_bool(response, "success", success);

            if (!success) {
                obs_data_set_string(response, "error", "Failed to start profile");
            }
        }, restreamer_dock);

    // Stop profile
    obs_websocket_vendor_register_request(vendor, "StopProfile",
        [](obs_data_t *request, obs_data_t *response, void *priv) {
            auto dock = static_cast<RestreamerDock*>(priv);
            const char *profile_id = obs_data_get_string(request, "profileId");

            bool success = output_profile_stop(dock->profileManager, profile_id);
            obs_data_set_bool(response, "success", success);
        }, restreamer_dock);

    // Get connection status
    obs_websocket_vendor_register_request(vendor, "GetConnectionStatus",
        [](obs_data_t *request, obs_data_t *response, void *priv) {
            auto dock = static_cast<RestreamerDock*>(priv);
            bool connected = dock->api && restreamer_api_is_connected(dock->api);

            obs_data_set_bool(response, "connected", connected);
            if (connected) {
                obs_data_set_string(response, "host",
                                   dock->hostEdit->text().toUtf8().constData());
            }
        }, restreamer_dock);
}

// Emit events when state changes
void RestreamerDock::onProfileStateChanged(output_profile_t *profile) {
    if (!vendor) return;

    obs_data_t *event_data = obs_data_create();
    obs_data_set_string(event_data, "profileId", profile->profile_id);
    obs_data_set_string(event_data, "profileName", profile->profile_name);
    obs_data_set_string(event_data, "status",
                       profileStatusToString(profile->status));

    obs_websocket_vendor_emit_event(vendor, "ProfileStateChanged", event_data);
    obs_data_release(event_data);
}
```

---

## Priority 4: Code Quality Enhancements

### 4.1 Add RAII Wrappers

**Create helper header:**

```cpp
// src/obs-helpers.hpp
#pragma once

#include <obs.h>
#include <obs-frontend-api.h>

// RAII wrapper for obs_source_t
class OBSSourceAutoRelease {
public:
    obs_source_t *source;

    OBSSourceAutoRelease(obs_source_t *src) : source(src) {}
    ~OBSSourceAutoRelease() {
        if (source) obs_source_release(source);
    }

    operator obs_source_t*() { return source; }
    operator bool() const { return source != nullptr; }
};

// RAII wrapper for obs_data_t
class OBSDataAutoRelease {
public:
    obs_data_t *data;

    OBSDataAutoRelease(obs_data_t *d) : data(d) {}
    ~OBSDataAutoRelease() {
        if (data) obs_data_release(data);
    }

    operator obs_data_t*() { return data; }
    operator bool() const { return data != nullptr; }
};

// RAII wrapper for obs_data_array_t
class OBSDataArrayAutoRelease {
public:
    obs_data_array_t *array;

    OBSDataArrayAutoRelease(obs_data_array_t *arr) : array(arr) {}
    ~OBSDataArrayAutoRelease() {
        if (array) obs_data_array_release(array);
    }

    operator obs_data_array_t*() { return array; }
    operator bool() const { return array != nullptr; }
};
```

### 4.2 Mutex Protection for Shared State

```cpp
// In restreamer-dock.h
class RestreamerDock : public QDockWidget {
    Q_OBJECT

private:
    std::recursive_mutex apiMutex;
    std::recursive_mutex profileMutex;

    // Thread-safe API access
    void withApiLock(std::function<void(restreamer_api_t*)> func) {
        std::lock_guard<std::recursive_mutex> lock(apiMutex);
        if (api) func(api);
    }
};

// Usage
dock->withApiLock([](restreamer_api_t *api) {
    restreamer_api_get_processes(api, &list);
});
```

---

## Implementation Roadmap

### Phase 1: Critical Fixes (Week 1)
- [ ] Add RAII wrappers
- [ ] Implement safe file writing
- [ ] Add thread-safe UI updates
- [ ] Fix signal handler cleanup
- [ ] Add mutex protection

### Phase 2: UI Redesign (Week 2-3)
- [ ] Create shadcn/ui inspired stylesheet
- [ ] Implement sidebar navigation
- [ ] Create card-based profile list
- [ ] Add status indicators and badges
- [ ] Improve spacing and typography

### Phase 3: Architecture (Week 4-5)
- [ ] Dynamic property UI generation
- [ ] Profile-aware configuration
- [ ] WebSocket API vendor
- [ ] Encoder index flexibility
- [ ] Platform icon auto-detection

### Phase 4: Polish (Week 6)
- [ ] Add internationalization
- [ ] Accessibility improvements
- [ ] Comprehensive error messages
- [ ] Loading states and progress indicators
- [ ] Unit tests for critical paths

---

## Testing Checklist

- [ ] Profile switching preserves settings
- [ ] No crashes on OBS shutdown
- [ ] No memory leaks (run with valgrind/ASan)
- [ ] Thread safety verified
- [ ] Config file corruption recovery
- [ ] Multiple simultaneous profiles
- [ ] Network failure handling
- [ ] Invalid input validation
- [ ] Cross-platform compatibility (macOS, Windows, Linux)

---

## References

- **obs-aitum-multistream**: https://github.com/Aitum/obs-aitum-multistream
- **obs-vertical-canvas**: https://github.com/Aitum/obs-vertical-canvas
- **shadcn/ui**: https://ui.shadcn.com
- **OBS Plugin Development**: https://obsproject.com/docs/plugins.html

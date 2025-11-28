/*
 * OBS Polyemesis Plugin - Connection Configuration Dialog Implementation
 */

#include "connection-config-dialog.h"
#include "obs-helpers.hpp"
#include "restreamer-config.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <obs-frontend-api.h>
#include <obs-module.h>

extern "C" {
#include <plugin-support.h>
}

ConnectionConfigDialog::ConnectionConfigDialog(QWidget *parent)
    : QDialog(parent) {
  setupUI();
  loadSettings();

  /* Auto-test connection if settings are already populated */
  if (!m_urlEdit->text().trimmed().isEmpty()) {
    /* Use QTimer to test after dialog is shown */
    QTimer::singleShot(100, this, [this]() { onTestConnection(); });
  }
}

ConnectionConfigDialog::~ConnectionConfigDialog() {
  /* Widgets are deleted automatically by Qt parent/child relationship */
}

void ConnectionConfigDialog::setupUI() {
  setWindowTitle("Connection Configuration");
  setModal(true);
  setMinimumWidth(500);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(16);
  mainLayout->setContentsMargins(20, 20, 20, 20);

  /* Connection Settings Group */
  QGroupBox *connectionGroup = new QGroupBox("Restreamer Connection");
  QFormLayout *formLayout = new QFormLayout(connectionGroup);
  formLayout->setSpacing(12);
  formLayout->setContentsMargins(16, 16, 16, 16);

  /* URL Input */
  m_urlEdit = new QLineEdit(this);
  m_urlEdit->setPlaceholderText("https://example.com or http://localhost:8080");
  m_urlEdit->setToolTip(
      "Enter the Restreamer URL. You can specify a custom port:\n"
      "Examples:\n"
      "  • https://rs.example.com (uses port 443)\n"
      "  • https://rs.example.com:8080 (custom port)\n"
      "  • http://localhost:8080 (local HTTP)\n"
      "  • example.com:9000 (auto-detects protocol)");

  QLabel *urlLabel = new QLabel("Restreamer URL:");
  formLayout->addRow(urlLabel, m_urlEdit);

  /* Help text for URL field */
  QLabel *urlHelpLabel =
      new QLabel("<small style='color: #888;'>Tip: Include port number if not "
                 "using standard ports (80/443)</small>");
  urlHelpLabel->setWordWrap(true);
  formLayout->addRow("", urlHelpLabel);

  /* Username Input */
  m_usernameEdit = new QLineEdit(this);
  m_usernameEdit->setPlaceholderText("admin");
  formLayout->addRow("Username:", m_usernameEdit);

  /* Password Input */
  m_passwordEdit = new QLineEdit(this);
  m_passwordEdit->setEchoMode(QLineEdit::Password);
  m_passwordEdit->setPlaceholderText("Enter password");
  formLayout->addRow("Password:", m_passwordEdit);

  /* Timeout Input */
  m_timeoutSpinBox = new QSpinBox(this);
  m_timeoutSpinBox->setRange(1, 60);
  m_timeoutSpinBox->setValue(10);
  m_timeoutSpinBox->setSuffix(" seconds");
  formLayout->addRow("Connection Timeout:", m_timeoutSpinBox);

  mainLayout->addWidget(connectionGroup);

  /* Test Connection Button */
  m_testButton = new QPushButton("Test Connection", this);
  m_testButton->setMinimumHeight(32);
  connect(m_testButton, &QPushButton::clicked, this,
          &ConnectionConfigDialog::onTestConnection);
  mainLayout->addWidget(m_testButton);

  /* Status Label */
  m_statusLabel = new QLabel(this);
  m_statusLabel->setWordWrap(true);
  m_statusLabel->setStyleSheet("padding: 8px; border-radius: 4px;");
  m_statusLabel->hide();
  mainLayout->addWidget(m_statusLabel);

  mainLayout->addStretch();

  /* Dialog Buttons */
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setSpacing(8);

  m_cancelButton = new QPushButton("Cancel", this);
  m_cancelButton->setMinimumHeight(32);
  connect(m_cancelButton, &QPushButton::clicked, this,
          &ConnectionConfigDialog::onCancel);

  m_saveButton = new QPushButton("Save", this);
  m_saveButton->setMinimumHeight(32);
  m_saveButton->setDefault(true);
  connect(m_saveButton, &QPushButton::clicked, this,
          &ConnectionConfigDialog::onSave);

  buttonLayout->addStretch();
  buttonLayout->addWidget(m_cancelButton);
  buttonLayout->addWidget(m_saveButton);

  mainLayout->addLayout(buttonLayout);

  setLayout(mainLayout);
}

void ConnectionConfigDialog::loadSettings() {
  /* Load settings from module config file */
  OBSDataAutoRelease settings(obs_data_create_from_json_file_safe(
      obs_module_config_path("config.json"), "bak"));

  if (!settings) {
    return;
  }

  /* Load settings with keys matching restreamer_config_load() */
  const char *host = obs_data_get_string(settings, "host");
  int port = (int)obs_data_get_int(settings, "port");
  bool use_https = obs_data_get_bool(settings, "use_https");
  const char *username = obs_data_get_string(settings, "username");
  const char *password = obs_data_get_string(settings, "password");

  /* Reconstruct URL from host, port, and use_https */
  if (host && strlen(host) > 0) {
    QString url;
    if (port > 0 && port != (use_https ? 443 : 80)) {
      /* Non-standard port, include it */
      url = QString("%1://%2:%3")
                .arg(use_https ? "https" : "http")
                .arg(host)
                .arg(port);
    } else {
      /* Standard port, omit it */
      url = QString("%1://%2").arg(use_https ? "https" : "http").arg(host);
    }
    m_urlEdit->setText(url);
    /* Security: Don't log URL as it may contain embedded credentials */
    obs_log(LOG_DEBUG, "Connection configuration loaded");
  }

  if (username && strlen(username) > 0) {
    m_usernameEdit->setText(username);
  }
  if (password && strlen(password) > 0) {
    m_passwordEdit->setText(password);
  }
}

void ConnectionConfigDialog::saveSettings() {
  /* Load existing settings */
  OBSDataAutoRelease settings(obs_data_create_from_json_file_safe(
      obs_module_config_path("config.json"), "bak"));

  if (!settings) {
    settings = OBSDataAutoRelease(obs_data_create());
  }

  /* Parse URL into host, port, and use_https */
  QString url = m_urlEdit->text().trimmed();
  QString host;
  int port = 0;
  bool use_https = false;
  parseUrl(url, host, port, use_https);

  /* Save connection settings with keys matching restreamer_config_load() */
  obs_data_set_string(settings, "host", host.toUtf8().constData());
  obs_data_set_int(settings, "port", port);
  obs_data_set_bool(settings, "use_https", use_https);
  obs_data_set_string(settings, "username",
                      m_usernameEdit->text().toUtf8().constData());
  obs_data_set_string(settings, "password",
                      m_passwordEdit->text().toUtf8().constData());

  /* Save to module config file */
  const char *config_path = obs_module_config_path("config.json");
  if (!obs_data_save_json_safe(settings, config_path, "tmp", "bak")) {
    obs_log(LOG_ERROR, "Failed to save connection settings to %s", config_path);
    return;
  }

  obs_log(LOG_INFO, "Connection settings saved: host=%s, port=%d, use_https=%d",
          host.toUtf8().constData(), port, use_https);

  /* Call restreamer_config_load() to update global connection */
  restreamer_config_load(settings);
}

QString ConnectionConfigDialog::getUrl() const { return m_urlEdit->text(); }

QString ConnectionConfigDialog::getUsername() const {
  return m_usernameEdit->text();
}

QString ConnectionConfigDialog::getPassword() const {
  return m_passwordEdit->text();
}

int ConnectionConfigDialog::getTimeout() const {
  return m_timeoutSpinBox->value();
}

void ConnectionConfigDialog::setUrl(const QString &url) {
  m_urlEdit->setText(url);
}

void ConnectionConfigDialog::setUsername(const QString &username) {
  m_usernameEdit->setText(username);
}

void ConnectionConfigDialog::setPassword(const QString &password) {
  m_passwordEdit->setText(password);
}

void ConnectionConfigDialog::setTimeout(int timeout) {
  m_timeoutSpinBox->setValue(timeout);
}

void ConnectionConfigDialog::parseUrl(const QString &url, QString &host,
                                      int &port, bool &use_https) const {
  /* Try parsing as full URL first */
  if (url.contains("://")) {
    QUrl parsedUrl(url);
    host = parsedUrl.host();
    port = parsedUrl.port(-1);
    use_https = (parsedUrl.scheme() == "https");
  } else {
    /* Parse host:port format */
    QStringList parts = url.split(":");
    host = parts[0];
    if (parts.size() > 1) {
      port = parts[1].toInt();
    }
    /* Check if it looks like a domain name (has dots) to guess https */
    if (host.contains(".") && !host.startsWith("localhost") &&
        !host.startsWith("127.")) {
      use_https = true; // Assume https for domain names
    }
  }

  /* Set default port based on protocol if not specified */
  if (port <= 0) {
    port = use_https ? 443 : 80;
  }
}

void ConnectionConfigDialog::onTestConnection() {
  QString url = m_urlEdit->text().trimmed();
  QString username = m_usernameEdit->text().trimmed();
  QString password = m_passwordEdit->text().trimmed();

  if (url.isEmpty()) {
    m_statusLabel->setText("⚠️ Please enter a Restreamer URL to test");
    m_statusLabel->setStyleSheet("background-color: #5a3a00; color: #ffcc00; "
                                 "padding: 8px; border-radius: 4px;");
    m_statusLabel->show();
    return;
  }

  m_testButton->setEnabled(false);

  /* Parse URL into host, port, and use_https */
  QString host;
  int port = 0;
  bool use_https = false;
  parseUrl(url, host, port, use_https);

  QString connectionUrl = QString("%1://%2:%3")
                              .arg(use_https ? "https" : "http")
                              .arg(host)
                              .arg(port);

  /* Security: Don't log credentials or URLs that may contain credentials */
  obs_log(LOG_INFO, "Testing connection to Restreamer at %s:%d",
          host.toUtf8().constData(), port);

  /* Show testing status with connection details */
  m_statusLabel->setText(
      QString("🔄 Testing connection to %1...").arg(connectionUrl));
  m_statusLabel->setStyleSheet("background-color: #1a3a5a; color: #6eb6ff; "
                               "padding: 8px; border-radius: 4px;");
  m_statusLabel->show();

  /* Create temporary API connection with parsed values */
  restreamer_connection_t conn = {0};
  conn.host = bstrdup(host.toUtf8().constData());
  conn.port = (uint16_t)port;
  conn.use_https = use_https;
  if (!username.isEmpty()) {
    conn.username = bstrdup(username.toUtf8().constData());
  }
  if (!password.isEmpty()) {
    conn.password = bstrdup(password.toUtf8().constData());
  }

  restreamer_api_t *test_api = restreamer_api_create(&conn);

  /* Test the connection */
  bool success = false;
  const char *error_msg = nullptr;

  if (test_api) {
    success = restreamer_api_test_connection(test_api);
    if (!success) {
      error_msg = restreamer_api_get_error(test_api);
    }
    restreamer_api_destroy(test_api);
  } else {
    error_msg = "Failed to create API client";
  }

  /* Clean up connection struct */
  bfree(conn.host);
  bfree(conn.username);
  bfree(conn.password);

  /* Update UI with result */
  if (success) {
    m_statusLabel->setText(
        "✅ Connection successful! Restreamer is reachable.");
    m_statusLabel->setStyleSheet("background-color: #1a3a2a; color: #6eff6e; "
                                 "padding: 8px; border-radius: 4px;");
    obs_log(LOG_INFO, "Connection test succeeded to %s",
            connectionUrl.toUtf8().constData());
  } else {
    /* Build detailed error message */
    QString errorText =
        QString("❌ Connection failed to %1\n").arg(connectionUrl);

    if (error_msg) {
      errorText += QString("Error: %1\n").arg(error_msg);

      /* Add hints based on error type */
      QString errorStr = QString(error_msg).toLower();
      if (errorStr.contains("401") || errorStr.contains("unauthorized") ||
          errorStr.contains("authentication")) {
        errorText += "\n💡 Hint: Check username/password";
      } else if (errorStr.contains("404") || errorStr.contains("not found")) {
        errorText += "\n💡 Hint: Check URL and port number";
      } else if (errorStr.contains("connection refused") ||
                 errorStr.contains("could not connect")) {
        errorText += "\n💡 Hint: Check if Restreamer is running and verify the "
                     "port number\n"
                     "   (Use port 443 for HTTPS with Let's Encrypt, or custom "
                     "port like 8080)";
      } else if (errorStr.contains("timeout")) {
        errorText +=
            "\n💡 Hint: Server may be slow or unreachable, verify URL and port";
      }
    } else {
      errorText += "Error: Unknown connection error";
    }

    m_statusLabel->setText(errorText);
    m_statusLabel->setStyleSheet("background-color: #3a1a1a; color: #ff6e6e; "
                                 "padding: 8px; border-radius: 4px;");
    obs_log(LOG_WARNING, "Connection test failed to %s: %s",
            connectionUrl.toUtf8().constData(),
            error_msg ? error_msg : "Unknown error");
  }

  m_testButton->setEnabled(true);
}

void ConnectionConfigDialog::onSave() {
  QString url = m_urlEdit->text().trimmed();

  if (url.isEmpty()) {
    QMessageBox::warning(this, "Invalid Configuration",
                         "Please enter a Restreamer URL before saving.");
    return;
  }

  saveSettings();

  emit settingsSaved(url, m_usernameEdit->text(), m_passwordEdit->text(),
                     m_timeoutSpinBox->value());

  accept();
}

void ConnectionConfigDialog::onCancel() { reject(); }

/*
 * OBS Polyemesis Plugin - Connection Configuration Dialog
 */

#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

class ConnectionConfigDialog : public QDialog {
  Q_OBJECT

public:
  explicit ConnectionConfigDialog(QWidget *parent = nullptr);
  ~ConnectionConfigDialog();

  /* Getters for connection settings */
  QString getUrl() const;
  QString getUsername() const;
  QString getPassword() const;
  int getTimeout() const;

  /* Setters for connection settings */
  void setUrl(const QString &url);
  void setUsername(const QString &username);
  void setPassword(const QString &password);
  void setTimeout(int timeout);

signals:
  void settingsSaved(const QString &url, const QString &username,
                     const QString &password, int timeout);

private slots:
  void onTestConnection();
  void onSave();
  void onCancel();

private:
  void setupUI();
  void loadSettings();
  void saveSettings();
  void parseUrl(const QString &url, QString &host, int &port,
                bool &use_https) const;

  /* UI Elements */
  QLineEdit *m_urlEdit;
  QLineEdit *m_usernameEdit;
  QLineEdit *m_passwordEdit;
  QSpinBox *m_timeoutSpinBox;
  QPushButton *m_testButton;
  QPushButton *m_saveButton;
  QPushButton *m_cancelButton;
  QLabel *m_statusLabel;
};

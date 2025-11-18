/*
 * OBS Polyemesis - UI Integration Tests
 * Tests for UI component integration and interaction
 */

#include <QtTest/QtTest>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QSignalSpy>
#include <QUrl>

// Mock UI component representing a destination entry widget
class MockDestinationWidget : public QWidget {
    Q_OBJECT

public:
    explicit MockDestinationWidget(QWidget *parent = nullptr)
        : QWidget(parent) {
        setupUI();
    }

    QString getName() const { return m_nameEdit->text(); }
    QString getUrl() const { return m_urlEdit->text(); }
    QString getStreamKey() const { return m_streamKeyEdit->text(); }
    bool isEnabled() const { return m_enabledCheckbox->isChecked(); }

    void setName(const QString &name) { m_nameEdit->setText(name); }
    void setUrl(const QString &url) { m_urlEdit->setText(url); }
    void setStreamKey(const QString &key) { m_streamKeyEdit->setText(key); }
    void setEnabled(bool enabled) { m_enabledCheckbox->setChecked(enabled); }

    bool validateInputs() {
        // Name validation
        QString name = getName().trimmed();
        if (name.isEmpty() || name.length() > 64) {
            m_lastError = "Invalid name: must be 1-64 characters";
            return false;
        }

        // URL validation
        QUrl url(getUrl());
        if (!url.isValid() || url.host().isEmpty()) {
            m_lastError = "Invalid URL: must have valid host";
            return false;
        }

        QString scheme = url.scheme().toLower();
        if (scheme != "rtmp" && scheme != "rtmps" && scheme != "srt") {
            m_lastError = "Invalid URL: must use rtmp/rtmps/srt protocol";
            return false;
        }

        // Stream key validation (optional but if present, must be valid)
        QString key = getStreamKey().trimmed();
        if (!key.isEmpty() && key.length() > 256) {
            m_lastError = "Invalid stream key: maximum 256 characters";
            return false;
        }

        return true;
    }

    QString lastError() const { return m_lastError; }

signals:
    void destinationChanged();
    void removeRequested();

public slots:
    void onRemoveClicked() {
        emit removeRequested();
    }

private:
    void setupUI() {
        auto *layout = new QVBoxLayout(this);

        m_nameEdit = new QLineEdit(this);
        m_nameEdit->setPlaceholderText("Destination Name");
        connect(m_nameEdit, &QLineEdit::textChanged, this, &MockDestinationWidget::destinationChanged);

        m_urlEdit = new QLineEdit(this);
        m_urlEdit->setPlaceholderText("rtmp://server.com/app");
        connect(m_urlEdit, &QLineEdit::textChanged, this, &MockDestinationWidget::destinationChanged);

        m_streamKeyEdit = new QLineEdit(this);
        m_streamKeyEdit->setPlaceholderText("Stream Key (optional)");
        m_streamKeyEdit->setEchoMode(QLineEdit::Password);
        connect(m_streamKeyEdit, &QLineEdit::textChanged, this, &MockDestinationWidget::destinationChanged);

        m_enabledCheckbox = new QCheckBox("Enabled", this);
        m_enabledCheckbox->setChecked(true);
        connect(m_enabledCheckbox, &QCheckBox::toggled, this, &MockDestinationWidget::destinationChanged);

        m_removeButton = new QPushButton("Remove", this);
        connect(m_removeButton, &QPushButton::clicked, this, &MockDestinationWidget::onRemoveClicked);

        layout->addWidget(new QLabel("Name:"));
        layout->addWidget(m_nameEdit);
        layout->addWidget(new QLabel("URL:"));
        layout->addWidget(m_urlEdit);
        layout->addWidget(new QLabel("Stream Key:"));
        layout->addWidget(m_streamKeyEdit);
        layout->addWidget(m_enabledCheckbox);
        layout->addWidget(m_removeButton);
    }

    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_urlEdit = nullptr;
    QLineEdit *m_streamKeyEdit = nullptr;
    QCheckBox *m_enabledCheckbox = nullptr;
    QPushButton *m_removeButton = nullptr;
    QString m_lastError;
};

class TestUIIntegration : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        qDebug() << "Starting UI Integration Tests";
    }

    void cleanupTestCase() {
        qDebug() << "UI Integration Tests Complete";
    }

    // Widget Creation Tests
    void testDestinationWidgetCreation() {
        MockDestinationWidget widget;

        QVERIFY(widget.getName().isEmpty());
        QVERIFY(widget.getUrl().isEmpty());
        QVERIFY(widget.getStreamKey().isEmpty());
        QVERIFY(widget.isEnabled());  // Should start enabled
    }

    // Input Validation Tests
    void testValidInputs() {
        MockDestinationWidget widget;
        widget.setName("Twitch");
        widget.setUrl("rtmp://live.twitch.tv/app");
        widget.setStreamKey("live_12345_abc");

        QVERIFY(widget.validateInputs());
        QVERIFY(widget.lastError().isEmpty());
    }

    void testEmptyNameValidation() {
        MockDestinationWidget widget;
        widget.setName("");
        widget.setUrl("rtmp://live.twitch.tv/app");

        QVERIFY(!widget.validateInputs());
        QVERIFY(widget.lastError().contains("name"));
    }

    void testLongNameValidation() {
        MockDestinationWidget widget;
        widget.setName(QString(100, 'x'));  // 100 characters (too long)
        widget.setUrl("rtmp://live.twitch.tv/app");

        QVERIFY(!widget.validateInputs());
        QVERIFY(widget.lastError().contains("name"));
    }

    void testInvalidUrlValidation() {
        MockDestinationWidget widget;
        widget.setName("Test");
        widget.setUrl("not-a-url");

        QVERIFY(!widget.validateInputs());
        QVERIFY(widget.lastError().contains("URL"));
    }

    void testInvalidProtocolValidation() {
        MockDestinationWidget widget;
        widget.setName("Test");
        widget.setUrl("http://example.com");

        QVERIFY(!widget.validateInputs());
        QVERIFY(widget.lastError().contains("protocol"));
    }

    void testEmptyHostValidation() {
        MockDestinationWidget widget;
        widget.setName("Test");
        widget.setUrl("rtmp://");

        QVERIFY(!widget.validateInputs());
        QVERIFY(widget.lastError().contains("host"));
    }

    void testLongStreamKeyValidation() {
        MockDestinationWidget widget;
        widget.setName("Test");
        widget.setUrl("rtmp://example.com/app");
        widget.setStreamKey(QString(300, 'x'));  // 300 characters (too long)

        QVERIFY(!widget.validateInputs());
        QVERIFY(widget.lastError().contains("stream key"));
    }

    // Signal Tests
    void testDestinationChangedSignal() {
        MockDestinationWidget widget;
        QSignalSpy changedSpy(&widget, &MockDestinationWidget::destinationChanged);

        widget.setName("Test");
        QCOMPARE(changedSpy.count(), 1);

        widget.setUrl("rtmp://example.com");
        QCOMPARE(changedSpy.count(), 2);

        widget.setStreamKey("key123");
        QCOMPARE(changedSpy.count(), 3);

        widget.setEnabled(false);
        QCOMPARE(changedSpy.count(), 4);
    }

    void testRemoveRequestedSignal() {
        MockDestinationWidget widget;
        QSignalSpy removeSpy(&widget, &MockDestinationWidget::removeRequested);

        // Simulate remove button click
        widget.onRemoveClicked();

        QCOMPARE(removeSpy.count(), 1);
    }

    // Data Integrity Tests
    void testSetAndGetName() {
        MockDestinationWidget widget;

        widget.setName("YouTube");
        QCOMPARE(widget.getName(), QString("YouTube"));

        widget.setName("Twitch");
        QCOMPARE(widget.getName(), QString("Twitch"));
    }

    void testSetAndGetUrl() {
        MockDestinationWidget widget;

        QString url1 = "rtmp://a.rtmp.youtube.com/live2";
        widget.setUrl(url1);
        QCOMPARE(widget.getUrl(), url1);

        QString url2 = "rtmp://live.twitch.tv/app";
        widget.setUrl(url2);
        QCOMPARE(widget.getUrl(), url2);
    }

    void testSetAndGetStreamKey() {
        MockDestinationWidget widget;

        widget.setStreamKey("key123");
        QCOMPARE(widget.getStreamKey(), QString("key123"));

        widget.setStreamKey("different_key");
        QCOMPARE(widget.getStreamKey(), QString("different_key"));
    }

    void testSetAndGetEnabled() {
        MockDestinationWidget widget;

        QVERIFY(widget.isEnabled());  // Default is enabled

        widget.setEnabled(false);
        QVERIFY(!widget.isEnabled());

        widget.setEnabled(true);
        QVERIFY(widget.isEnabled());
    }

    // Edge Case Tests
    void testWhitespaceHandling() {
        MockDestinationWidget widget;
        widget.setName("  Test  ");  // Whitespace should be trimmed in validation
        widget.setUrl("rtmp://example.com/app");

        QVERIFY(widget.validateInputs());  // Should pass after trimming
    }

    void testSpecialCharactersInName() {
        MockDestinationWidget widget;
        widget.setName("Test!@#$%^&*()");
        widget.setUrl("rtmp://example.com/app");

        // Name can contain special characters
        QVERIFY(widget.validateInputs());
    }

    void testUnicodeInInputs() {
        MockDestinationWidget widget;
        widget.setName("测试");  // Chinese characters
        widget.setUrl("rtmp://example.com/app");

        QVERIFY(widget.validateInputs());
    }

    void testRtmpsProtocol() {
        MockDestinationWidget widget;
        widget.setName("Secure Stream");
        widget.setUrl("rtmps://secure.example.com:443/app");

        QVERIFY(widget.validateInputs());
    }

    void testSrtProtocol() {
        MockDestinationWidget widget;
        widget.setName("SRT Stream");
        widget.setUrl("srt://example.com:9000");

        QVERIFY(widget.validateInputs());
    }

    void testUrlWithPort() {
        MockDestinationWidget widget;
        widget.setName("Custom Port");
        widget.setUrl("rtmp://example.com:1935/app");

        QVERIFY(widget.validateInputs());
    }

    void testUrlWithCredentials() {
        MockDestinationWidget widget;
        widget.setName("With Auth");
        widget.setUrl("rtmp://user:pass@example.com/app");

        QVERIFY(widget.validateInputs());
    }

    void testEmptyStreamKeyIsValid() {
        MockDestinationWidget widget;
        widget.setName("Test");
        widget.setUrl("rtmp://example.com/app");
        widget.setStreamKey("");  // Empty stream key should be allowed

        QVERIFY(widget.validateInputs());
    }
};

QTEST_MAIN(TestUIIntegration)
#include "test_ui_integration.moc"

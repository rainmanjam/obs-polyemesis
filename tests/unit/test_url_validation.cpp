/*
 * OBS Polyemesis - URL Validation Tests
 * Tests for RTMP/SRT URL validation and stream key handling
 */

#include <QtTest/QtTest>
#include <QString>
#include <QUrl>
#include <QRegularExpression>

class TestUrlValidation : public QObject {
    Q_OBJECT

private:
    // Helper: Validate RTMP URL format
    bool isValidRtmpUrl(const QString &url) {
        if (url.isEmpty()) {
            return false;
        }

        QUrl parsedUrl(url);
        if (!parsedUrl.isValid() || parsedUrl.host().isEmpty()) {
            return false;
        }

        QString scheme = parsedUrl.scheme().toLower();
        return (scheme == "rtmp" || scheme == "rtmps");
    }

    // Helper: Validate SRT URL format
    bool isValidSrtUrl(const QString &url) {
        if (url.isEmpty()) {
            return false;
        }

        QUrl parsedUrl(url);
        if (!parsedUrl.isValid() || parsedUrl.host().isEmpty()) {
            return false;
        }

        return parsedUrl.scheme().toLower() == "srt";
    }

    // Helper: Extract stream key from RTMP URL
    QString extractStreamKey(const QString &url) {
        QUrl parsedUrl(url);
        QString path = parsedUrl.path();

        // Remove trailing slash if present
        if (path.endsWith('/')) {
            path.chop(1);
        }

        // Stream key is typically after the last '/' (e.g., /app/streamkey)
        // We need at least 2 segments for a stream key to exist
        qsizetype lastSlash = path.lastIndexOf('/');
        qsizetype firstSlash = path.indexOf('/');

        // If there's only one slash (at the start), no stream key
        if (lastSlash == firstSlash || lastSlash < 0) {
            return QString();
        }

        // Extract everything after the last slash
        QString streamKey = path.mid(lastSlash + 1);
        return streamKey.isEmpty() ? QString() : streamKey;
    }

private slots:
    void initTestCase() {
        qDebug() << "Starting URL Validation Tests";
    }

    void cleanupTestCase() {
        qDebug() << "URL Validation Tests Complete";
    }

    void testValidRtmpUrls() {
        QVERIFY(isValidRtmpUrl("rtmp://live.twitch.tv/app"));
        QVERIFY(isValidRtmpUrl("rtmp://a.rtmp.youtube.com/live2"));
        QVERIFY(isValidRtmpUrl("rtmps://live-api-s.facebook.com:443/rtmp/"));
        QVERIFY(isValidRtmpUrl("rtmp://ingest.global.contribute.live-video.net/app"));
    }

    void testInvalidRtmpUrls() {
        QVERIFY(!isValidRtmpUrl(""));
        QVERIFY(!isValidRtmpUrl("http://example.com"));
        QVERIFY(!isValidRtmpUrl("https://example.com"));
        QVERIFY(!isValidRtmpUrl("ftp://example.com"));
        QVERIFY(!isValidRtmpUrl("not-a-url"));
        QVERIFY(!isValidRtmpUrl("rtmp://"));
    }

    void testValidSrtUrls() {
        QVERIFY(isValidSrtUrl("srt://example.com:1935"));
        QVERIFY(isValidSrtUrl("srt://192.168.1.100:9000"));
        QVERIFY(isValidSrtUrl("srt://localhost:8888"));
    }

    void testInvalidSrtUrls() {
        QVERIFY(!isValidSrtUrl(""));
        QVERIFY(!isValidSrtUrl("http://example.com"));
        QVERIFY(!isValidSrtUrl("rtmp://example.com"));
        QVERIFY(!isValidSrtUrl("srt://"));
    }

    void testStreamKeyExtraction() {
        QCOMPARE(extractStreamKey("rtmp://live.twitch.tv/app/live_12345_abc"),
                 QString("live_12345_abc"));

        QCOMPARE(extractStreamKey("rtmp://a.rtmp.youtube.com/live2/streamkey123"),
                 QString("streamkey123"));

        QCOMPARE(extractStreamKey("rtmp://server.com/app/very_long_stream_key_with-dashes"),
                 QString("very_long_stream_key_with-dashes"));

        // No stream key cases
        QVERIFY(extractStreamKey("rtmp://server.com/app").isEmpty());
        QVERIFY(extractStreamKey("rtmp://server.com/app/").isEmpty());
    }

    void testUrlWithPort() {
        QVERIFY(isValidRtmpUrl("rtmp://example.com:1935/app"));
        QVERIFY(isValidRtmpUrl("rtmps://example.com:443/app"));
        QVERIFY(isValidSrtUrl("srt://example.com:9000"));
    }

    void testUrlWithCredentials() {
        // URLs with credentials should still be valid
        QVERIFY(isValidRtmpUrl("rtmp://user:pass@example.com/app"));
        QVERIFY(isValidSrtUrl("srt://user:pass@example.com:9000"));
    }

    void testUrlEdgeCases() {
        // Very long URLs
        QString longUrl = "rtmp://very-long-server-name.example.com:1935/application/";
        longUrl += QString(100, 'x');
        QVERIFY(isValidRtmpUrl(longUrl));

        // IPv4 addresses
        QVERIFY(isValidRtmpUrl("rtmp://192.168.1.100/app"));
        QVERIFY(isValidSrtUrl("srt://10.0.0.1:9000"));

        // IPv6 addresses (should work with Qt's QUrl)
        QVERIFY(isValidRtmpUrl("rtmp://[::1]/app"));
        QVERIFY(isValidSrtUrl("srt://[2001:db8::1]:9000"));
    }
};

QTEST_MAIN(TestUrlValidation)
#include "test_url_validation.moc"

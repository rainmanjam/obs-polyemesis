/*
 * OBS Polyemesis - Destination Management Tests
 * Tests for adding, removing, and validating stream destinations
 */

#include <QtTest/QtTest>
#include <QString>
#include <QList>
#include <QMap>

// Mock destination structure
struct StreamDestination {
    QString name;
    QString url;
    QString streamKey;
    bool enabled;

    bool operator==(const StreamDestination &other) const {
        return name == other.name &&
               url == other.url &&
               streamKey == other.streamKey &&
               enabled == other.enabled;
    }
};

class DestinationManager {
public:
    bool addDestination(const StreamDestination &dest) {
        // Check if destination with same name already exists
        for (const auto &existing : m_destinations) {
            if (existing.name == dest.name) {
                return false;  // Duplicate name
            }
        }

        // Validate destination
        if (dest.name.isEmpty() || dest.url.isEmpty()) {
            return false;
        }

        m_destinations.append(dest);
        return true;
    }

    bool removeDestination(const QString &name) {
        for (int i = 0; i < m_destinations.size(); ++i) {
            if (m_destinations[i].name == name) {
                m_destinations.removeAt(i);
                return true;
            }
        }
        return false;
    }

    StreamDestination *getDestination(const QString &name) {
        for (auto &dest : m_destinations) {
            if (dest.name == name) {
                return &dest;
            }
        }
        return nullptr;
    }

    qsizetype count() const {
        return m_destinations.size();
    }

    int enabledCount() const {
        int count = 0;
        for (const auto &dest : m_destinations) {
            if (dest.enabled) {
                ++count;
            }
        }
        return count;
    }

    void clear() {
        m_destinations.clear();
    }

    QList<StreamDestination> getAll() const {
        return m_destinations;
    }

private:
    QList<StreamDestination> m_destinations;
};

class TestDestinationManagement : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        qDebug() << "Starting Destination Management Tests";
    }

    void cleanupTestCase() {
        qDebug() << "Destination Management Tests Complete";
    }

    void init() {
        // Reset before each test
        manager.clear();
    }

    void testAddDestination() {
        StreamDestination dest{"Twitch", "rtmp://live.twitch.tv/app", "stream_key", true};

        QVERIFY(manager.addDestination(dest));
        QCOMPARE(manager.count(), 1);
    }

    void testAddMultipleDestinations() {
        StreamDestination twitch{"Twitch", "rtmp://live.twitch.tv/app", "key1", true};
        StreamDestination youtube{"YouTube", "rtmp://a.rtmp.youtube.com/live2", "key2", true};
        StreamDestination facebook{"Facebook", "rtmps://live-api-s.facebook.com:443/rtmp/", "key3", false};

        QVERIFY(manager.addDestination(twitch));
        QVERIFY(manager.addDestination(youtube));
        QVERIFY(manager.addDestination(facebook));

        QCOMPARE(manager.count(), 3);
        QCOMPARE(manager.enabledCount(), 2);
    }

    void testAddDuplicateName() {
        StreamDestination dest1{"Twitch", "rtmp://live.twitch.tv/app", "key1", true};
        StreamDestination dest2{"Twitch", "rtmp://different.url/app", "key2", true};

        QVERIFY(manager.addDestination(dest1));
        QVERIFY(!manager.addDestination(dest2));  // Should fail due to duplicate name

        QCOMPARE(manager.count(), 1);
    }

    void testAddInvalidDestination() {
        StreamDestination emptyName{"", "rtmp://valid.url/app", "key", true};
        StreamDestination emptyUrl{"Valid Name", "", "key", true};

        QVERIFY(!manager.addDestination(emptyName));
        QVERIFY(!manager.addDestination(emptyUrl));

        QCOMPARE(manager.count(), 0);
    }

    void testRemoveDestination() {
        StreamDestination dest{"Twitch", "rtmp://live.twitch.tv/app", "key", true};

        manager.addDestination(dest);
        QCOMPARE(manager.count(), 1);

        QVERIFY(manager.removeDestination("Twitch"));
        QCOMPARE(manager.count(), 0);
    }

    void testRemoveNonexistent() {
        QVERIFY(!manager.removeDestination("NonExistent"));

        StreamDestination dest{"Twitch", "rtmp://live.twitch.tv/app", "key", true};
        manager.addDestination(dest);

        QVERIFY(!manager.removeDestination("YouTube"));
        QCOMPARE(manager.count(), 1);
    }

    void testGetDestination() {
        StreamDestination dest{"Twitch", "rtmp://live.twitch.tv/app", "key", true};

        manager.addDestination(dest);

        StreamDestination *retrieved = manager.getDestination("Twitch");
        QVERIFY(retrieved != nullptr);
        QCOMPARE(retrieved->name, QString("Twitch"));
        QCOMPARE(retrieved->url, QString("rtmp://live.twitch.tv/app"));
        QCOMPARE(retrieved->streamKey, QString("key"));
        QVERIFY(retrieved->enabled);
    }

    void testGetNonexistentDestination() {
        StreamDestination *retrieved = manager.getDestination("NonExistent");
        QVERIFY(retrieved == nullptr);
    }

    void testModifyDestination() {
        StreamDestination dest{"Twitch", "rtmp://live.twitch.tv/app", "key", true};

        manager.addDestination(dest);

        StreamDestination *retrieved = manager.getDestination("Twitch");
        QVERIFY(retrieved != nullptr);

        // Modify
        retrieved->enabled = false;
        retrieved->streamKey = "new_key";

        // Verify changes persisted
        StreamDestination *updated = manager.getDestination("Twitch");
        QVERIFY(!updated->enabled);
        QCOMPARE(updated->streamKey, QString("new_key"));
    }

    void testEnabledCount() {
        manager.addDestination({"Twitch", "rtmp://url1", "key1", true});
        manager.addDestination({"YouTube", "rtmp://url2", "key2", true});
        manager.addDestination({"Facebook", "rtmp://url3", "key3", false});
        manager.addDestination({"Kick", "rtmp://url4", "key4", true});
        manager.addDestination({"TikTok", "rtmp://url5", "key5", false});

        QCOMPARE(manager.count(), 5);
        QCOMPARE(manager.enabledCount(), 3);
    }

    void testClear() {
        manager.addDestination({"Dest1", "rtmp://url1", "key1", true});
        manager.addDestination({"Dest2", "rtmp://url2", "key2", true});

        QCOMPARE(manager.count(), 2);

        manager.clear();

        QCOMPARE(manager.count(), 0);
        QCOMPARE(manager.enabledCount(), 0);
    }

    void testGetAll() {
        StreamDestination dest1{"Twitch", "rtmp://url1", "key1", true};
        StreamDestination dest2{"YouTube", "rtmp://url2", "key2", true};

        manager.addDestination(dest1);
        manager.addDestination(dest2);

        QList<StreamDestination> all = manager.getAll();

        QCOMPARE(all.size(), 2);
        QVERIFY(all.contains(dest1));
        QVERIFY(all.contains(dest2));
    }

private:
    DestinationManager manager;
};

QTEST_MAIN(TestDestinationManagement)
#include "test_destination_management.moc"

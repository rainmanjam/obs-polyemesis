/*
 * OBS Polyemesis - Process ID Generation Tests
 * Tests for generating valid Restreamer process IDs
 */

#include <QtTest/QtTest>
#include <QString>
#include <QRegularExpression>
#include <QSet>

class TestProcessIdGeneration : public QObject {
    Q_OBJECT

private:
    // Helper: Generate process ID from profile name
    QString generateProcessId(const QString &profileName) {
        // Process IDs must be:
        // - Lowercase
        // - Alphanumeric + hyphens only
        // - No spaces or special characters
        // - Unique

        QString processId = profileName.toLower();

        // Replace spaces and underscores with hyphens
        processId.replace(QRegularExpression("[\\s_]+"), "-");

        // Remove any non-alphanumeric characters except hyphens
        processId.replace(QRegularExpression("[^a-z0-9-]"), "");

        // Remove leading/trailing hyphens
        processId = processId.trimmed();
        while (processId.startsWith('-')) {
            processId = processId.mid(1);
        }
        while (processId.endsWith('-')) {
            processId.chop(1);
        }

        // Collapse multiple consecutive hyphens
        processId.replace(QRegularExpression("-+"), "-");

        return processId;
    }

    // Helper: Validate process ID format
    bool isValidProcessId(const QString &processId) {
        if (processId.isEmpty()) {
            return false;
        }

        // Must be lowercase alphanumeric + hyphens only
        QRegularExpression validPattern("^[a-z0-9-]+$");
        if (!validPattern.match(processId).hasMatch()) {
            return false;
        }

        // Cannot start or end with hyphen
        if (processId.startsWith('-') || processId.endsWith('-')) {
            return false;
        }

        // Cannot have consecutive hyphens
        if (processId.contains("--")) {
            return false;
        }

        return true;
    }

private slots:
    void initTestCase() {
        qDebug() << "Starting Process ID Generation Tests";
    }

    void cleanupTestCase() {
        qDebug() << "Process ID Generation Tests Complete";
    }

    void testBasicGeneration() {
        QCOMPARE(generateProcessId("My Profile"), QString("my-profile"));
        QCOMPARE(generateProcessId("Test_Profile"), QString("test-profile"));
        QCOMPARE(generateProcessId("Profile123"), QString("profile123"));
        QCOMPARE(generateProcessId("UPPERCASE"), QString("uppercase"));
    }

    void testSpecialCharacterRemoval() {
        QCOMPARE(generateProcessId("Profile@#$%"), QString("profile"));
        QCOMPARE(generateProcessId("Test!Profile"), QString("testprofile"));
        QCOMPARE(generateProcessId("Profile (2024)"), QString("profile-2024"));
        QCOMPARE(generateProcessId("Name & Name"), QString("name-name"));
    }

    void testMultipleSpaces() {
        QCOMPARE(generateProcessId("Multiple   Spaces"), QString("multiple-spaces"));
        QCOMPARE(generateProcessId("Many    Spaces   Here"), QString("many-spaces-here"));
    }

    void testLeadingTrailingChars() {
        QCOMPARE(generateProcessId("  Leading Spaces"), QString("leading-spaces"));
        QCOMPARE(generateProcessId("Trailing Spaces  "), QString("trailing-spaces"));
        QCOMPARE(generateProcessId("---Hyphens---"), QString("hyphens"));
    }

    void testValidationPositive() {
        QVERIFY(isValidProcessId("valid-process-id"));
        QVERIFY(isValidProcessId("test123"));
        QVERIFY(isValidProcessId("profile-2024"));
        QVERIFY(isValidProcessId("a"));
        QVERIFY(isValidProcessId("1"));
    }

    void testValidationNegative() {
        QVERIFY(!isValidProcessId(""));
        QVERIFY(!isValidProcessId("-leading"));
        QVERIFY(!isValidProcessId("trailing-"));
        QVERIFY(!isValidProcessId("double--hyphen"));
        QVERIFY(!isValidProcessId("UPPERCASE"));
        QVERIFY(!isValidProcessId("has spaces"));
        QVERIFY(!isValidProcessId("has_underscore"));
        QVERIFY(!isValidProcessId("special@chars"));
    }

    void testUniqueness() {
        // Generate IDs from multiple profiles
        QSet<QString> generatedIds;

        QStringList profileNames = {
            "Profile 1",
            "Profile 2",
            "Test Profile",
            "Another Test",
            "Final Profile"
        };

        for (const QString &name : profileNames) {
            QString id = generateProcessId(name);
            QVERIFY(!generatedIds.contains(id));  // Should be unique
            generatedIds.insert(id);
        }

        QCOMPARE(generatedIds.size(), profileNames.size());
    }

    void testEdgeCases() {
        // Single character
        QCOMPARE(generateProcessId("A"), QString("a"));

        // Numbers only
        QCOMPARE(generateProcessId("12345"), QString("12345"));

        // Very long name
        QString longName = QString(200, 'a');
        QString longId = generateProcessId(longName);
        QCOMPARE(longId.length(), 200);
        QVERIFY(isValidProcessId(longId));

        // Only special characters (should result in empty string)
        QString onlySpecial = generateProcessId("@#$%^&*()");
        QVERIFY(onlySpecial.isEmpty() || isValidProcessId(onlySpecial));
    }

    void testRealWorldNames() {
        QCOMPARE(generateProcessId("Twitch Stream"), QString("twitch-stream"));
        QCOMPARE(generateProcessId("YouTube Live"), QString("youtube-live"));
        QCOMPARE(generateProcessId("Multi-Platform (Main)"), QString("multi-platform-main"));
        QCOMPARE(generateProcessId("Test Stream #1"), QString("test-stream-1"));
        QCOMPARE(generateProcessId("Gaming @ 1080p60"), QString("gaming-1080p60"));
    }
};

QTEST_MAIN(TestProcessIdGeneration)
#include "test_process_id_generation.moc"
